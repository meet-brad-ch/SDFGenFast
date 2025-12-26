// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#include "config.h"
#include "sdfgen_unified.h"  // Unified API with CPU/GPU backend selection
#include "sdf_io.h"          // Shared SDF file I/O functions
#include "mesh_io.h"         // Mesh file loading (OBJ, STL)
#include "mesh_repair.h"     // Mesh watertightness check and repair
#include <CLI/CLI.hpp>
#include <cmath>

#ifdef HAVE_VTK
  #include <vtkImageData.h>
  #include <vtkFloatArray.h>
  #include <vtkXMLImageDataWriter.h>
  #include <vtkPointData.h>
  #include <vtkSmartPointer.h>
#endif


#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

int main(int argc, char* argv[]) {

  CLI::App app{"SDFGen - Generate signed distance fields from triangle meshes"};

  // Positional arguments
  std::string filename;
  std::vector<float> dimensions;  // Can be: [dx, padding] for OBJ, or [Nx], [Nx,pad], [Nx,Ny,Nz], [Nx,Ny,Nz,pad] for STL

  app.add_option("input", filename, "Input mesh file (.obj or .stl)")
      ->required()
      ->check(CLI::ExistingFile);
  app.add_option("dimensions", dimensions, "Grid dimensions:\n"
      "  OBJ: <dx> <padding>           - cell size and padding\n"
      "  STL: <Nx> [Ny Nz] [padding]   - grid size (proportional or manual)")
      ->expected(1, 4);

  // Optional flags
  bool force_cpu = false;
  bool fix_mesh = false;
  int num_threads = 0;
  int padding = 1;

  app.add_flag("--cpu", force_cpu, "Force CPU backend (skip GPU)");
  app.add_flag("--fix", fix_mesh, "Repair non-watertight meshes (fill holes)");
  app.add_option("-t,--threads", num_threads, "CPU thread count (0=auto)")
      ->default_val(0);
  app.add_option("-p,--padding", padding, "Padding cells around mesh")
      ->default_val(1);

  // Show full help on error (e.g., missing required arguments)
  app.failure_message(CLI::FailureMessage::help);

  CLI11_PARSE(app, argc, argv);

  // Detect file type
  std::string ext = filename.substr(filename.find_last_of(".") + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  bool is_stl = (ext == "stl");
  bool mode_precise = is_stl;  // STL uses grid dimensions, OBJ uses dx

  // Validate dimensions
  if (dimensions.empty()) {
    std::cerr << "Error: Grid dimensions required.\n";
    std::cerr << "  OBJ: SDFGen mesh.obj <dx> <padding>\n";
    std::cerr << "  STL: SDFGen mesh.stl <Nx> [Ny Nz] [padding]\n";
    return 1;
  }

  // Common variables
  std::vector<Vec3f> vertList;
  std::vector<Vec3ui> faceList;
  Vec3f min_box, max_box;
  int target_nx = 0, target_ny = 0, target_nz = 0;
  float dx = 0.0f;

  std::cout << "========================================\n";
  std::cout << "SDFGen - SDF Generation Tool\n";
  std::cout << "========================================\n\n";

  if(mode_precise) {
    // === MODE 2: STL with grid dimensions ===
    std::cout << "Mode: Grid dimensions (STL)\n";
    std::cout << "Input: " << filename << "\n\n";

    // Load STL file first to get mesh dimensions
    if(!meshio::load_stl(filename.c_str(), vertList, faceList, min_box, max_box)) {
      std::cerr << "Failed to load STL file.\n";
      return 1;
    }

    Vec3f mesh_size = max_box - min_box;

    // Parse dimensions: [Nx] or [Nx, Ny, Nz]
    if (dimensions.size() == 1 || dimensions.size() == 2) {
      // Proportional mode: Nx only (optional padding in dimensions[1] for backwards compat)
      target_nx = (int)dimensions[0];
      if (dimensions.size() == 2 && dimensions[1] < 20) {
        padding = (int)dimensions[1];  // Backwards compat: SDFGen mesh.stl 256 2
      }

      if(target_nx <= 0) {
        std::cerr << "Error: Grid dimension must be a positive integer.\n";
        return 1;
      }
      if(padding < 1) padding = 1;

      // Calculate dx based on X dimension
      dx = mesh_size[0] / (target_nx - 2 * padding);

      // Calculate Ny and Nz proportionally to maintain aspect ratio
      target_ny = (int)((mesh_size[1] / dx) + 0.5f) + 2 * padding;
      target_nz = (int)((mesh_size[2] / dx) + 0.5f) + 2 * padding;

      std::cout << "Mode: Proportional dimensions\n";
      std::cout << "Input Nx: " << target_nx << "\n";
      std::cout << "Calculated grid: " << target_nx << " x " << target_ny << " x " << target_nz << "\n";

    } else if (dimensions.size() >= 3) {
      // Manual mode: Nx, Ny, Nz
      target_nx = (int)dimensions[0];
      target_ny = (int)dimensions[1];
      target_nz = (int)dimensions[2];

      if(target_nx <= 0 || target_ny <= 0 || target_nz <= 0) {
        std::cerr << "Error: Grid dimensions must be positive integers.\n";
        return 1;
      }
      if(padding < 1) padding = 1;

      // Calculate dx to fit the mesh into target grid
      float dx_x = mesh_size[0] / (target_nx - 2 * padding);
      float dx_y = mesh_size[1] / (target_ny - 2 * padding);
      float dx_z = mesh_size[2] / (target_nz - 2 * padding);
      dx = std::max(dx_x, std::max(dx_y, dx_z));

      std::cout << "Mode: Manual dimensions\n";
      std::cout << "Target grid: " << target_nx << " x " << target_ny << " x " << target_nz << "\n";
    }

    std::cout << "Padding: " << padding << " cells\n";
    std::cout << "Threads: " << (num_threads == 0 ? "auto" : std::to_string(num_threads)) << "\n\n";
    std::cout << "Mesh size: " << mesh_size[0] << " x " << mesh_size[1] << " x " << mesh_size[2] << " m\n";
    std::cout << "Cell size (dx): " << dx << " m\n\n";

  } else {
    // === MODE 1: OBJ with dx spacing ===
    std::cout << "Mode: Cell size spacing (OBJ)\n";
    std::cout << "Input: " << filename << "\n\n";

    // OBJ requires: dx [padding]
    if (dimensions.empty()) {
      std::cerr << "Error: OBJ mode requires cell size (dx).\n";
      std::cerr << "Usage: SDFGen mesh.obj <dx> [-p padding]\n";
      return 1;
    }

    dx = dimensions[0];
    if (dimensions.size() >= 2) {
      padding = (int)dimensions[1];  // Backwards compat
    }
    if(padding < 1) padding = 1;

    std::cout << "Cell size (dx): " << dx << "\n";
    std::cout << "Padding: " << padding << " cells\n";
    std::cout << "Threads: " << (num_threads == 0 ? "auto" : std::to_string(num_threads)) << "\n\n";

    // Load OBJ file
    if(!meshio::load_obj(filename.c_str(), vertList, faceList, min_box, max_box)) {
      std::cerr << "Failed to load OBJ file.\n";
      return 1;
    }
  }

  // Weld duplicate vertices (STL files have separate vertices per triangle)
  int welded = meshio::weld_vertices(vertList, faceList, 1e-5f);
  if (welded > 0) {
    std::cout << "Welded " << welded << " duplicate vertices\n";
    std::cout << "Mesh now has " << vertList.size() << " vertices, " << faceList.size() << " triangles\n";
  }

  // Analyze mesh watertightness (always)
  meshio::MeshAnalysis mesh_analysis = meshio::analyze_mesh(vertList, faceList);
  meshio::print_mesh_analysis(mesh_analysis, false);

  // Optionally repair mesh if --fix flag was provided
  if (fix_mesh && !mesh_analysis.is_watertight) {
    std::cout << "\nAttempting mesh repair (--fix)...\n";
    int holes_filled = meshio::repair_mesh(vertList, faceList, 0.0f);  // No re-welding needed
    if (holes_filled > 0) {
      // Recalculate bounding box after repair
      min_box = Vec3f(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
      max_box = Vec3f(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
      for (const auto& v : vertList) {
        meshio::update_minmax(v, min_box, max_box);
      }
    }
    std::cout << "\n";
  }

  // Add padding around the box and compute final grid dimensions
  Vec3ui sizes;
  if(mode_precise) {
    // Use exact target dimensions
    sizes = Vec3ui(target_nx, target_ny, target_nz);

    // Recalculate bounds to exactly fit the target grid with calculated dx
    // Center the mesh in the grid with padding on all sides
    Vec3f mesh_size = max_box - min_box;
    Vec3f grid_size = Vec3f(sizes[0] * dx, sizes[1] * dx, sizes[2] * dx);
    Vec3f mesh_center = (min_box + max_box) * 0.5f;

    min_box = mesh_center - grid_size * 0.5f;
    max_box = mesh_center + grid_size * 0.5f;
  } else {
    // Legacy mode: add padding, then calculate sizes
    Vec3f unit(1,1,1);
    min_box -= padding*dx*unit;
    max_box += padding*dx*unit;
    sizes = Vec3ui((max_box - min_box)/dx);
  }

  std::cout << "Computing signed distance field...\n";
  std::cout << "  Padded bounds: (" << min_box << ") to (" << max_box << ")\n";
  std::cout << "  Grid dimensions: " << sizes[0] << " x " << sizes[1] << " x " << sizes[2] << "\n";
  std::cout << "  Total cells: " << (sizes[0] * sizes[1] * sizes[2]) << "\n";

  // Runtime dispatch between CPU and GPU implementations using unified API
  Array3f phi_grid;
  sdfgen::HardwareBackend backend = force_cpu ? sdfgen::HardwareBackend::CPU : sdfgen::HardwareBackend::Auto;

  // Report which backend will be/was used
  std::cout << "  Hardware: ";
  if(force_cpu) {
    std::cout << "CPU mode forced (--cpu flag)\n";
    std::cout << "  Implementation: CPU (multi-threaded)\n\n";
  } else if(sdfgen::is_gpu_available()) {
    std::cout << "GPU acceleration available\n";
    std::cout << "  Implementation: GPU (CUDA)\n\n";
  } else {
    std::cout << "No CUDA GPU detected\n";
    std::cout << "  Implementation: CPU (multi-threaded)\n\n";
  }

  sdfgen::make_level_set3(faceList, vertList, min_box, dx, sizes[0], sizes[1], sizes[2], phi_grid, 1, backend, num_threads);

  std::cout << "SDF computation complete.\n\n";

  // Generate output filename
  std::string outname;
  std::string base_filename = filename.substr(0, filename.find_last_of("."));

  #ifdef HAVE_VTK
    // VTK output mode
    if(mode_precise) {
      char dims[64];
      sprintf(dims, "_sdf_%dx%dx%d", phi_grid.ni, phi_grid.nj, phi_grid.nk);
      outname = base_filename + std::string(dims) + ".vti";
    } else {
      outname = base_filename + ".vti";
    }
    std::cout << "Writing VTK output to: " << outname << "\n";
    vtkSmartPointer<vtkImageData> output_volume = vtkSmartPointer<vtkImageData>::New();

    output_volume->SetDimensions(phi_grid.ni ,phi_grid.nj ,phi_grid.nk);
    output_volume->SetOrigin( phi_grid.ni*dx/2, phi_grid.nj*dx/2,phi_grid.nk*dx/2);
    output_volume->SetSpacing(dx,dx,dx);

    vtkSmartPointer<vtkFloatArray> distance = vtkSmartPointer<vtkFloatArray>::New();
    
    distance->SetNumberOfTuples(phi_grid.a.size());
    
    output_volume->GetPointData()->AddArray(distance);
    distance->SetName("Distance");

    for(unsigned int i = 0; i < phi_grid.a.size(); ++i) {
      distance->SetValue(i, phi_grid.a[i]);
    }

    vtkSmartPointer<vtkXMLImageDataWriter> writer =
    vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetFileName(outname.c_str());

    #if VTK_MAJOR_VERSION <= 5
      writer->SetInput(output_volume);
    #else
      writer->SetInputData(output_volume);
    #endif
    writer->Write();

  #else
    // Binary SDF output (no VTK)
    if(mode_precise) {
      char dims[128];
      // Proportional or manual mode: hill_sdf_615x615x113.sdf
      sprintf(dims, "_sdf_%dx%dx%d", phi_grid.ni, phi_grid.nj, phi_grid.nk);
      outname = base_filename + std::string(dims) + ".sdf";
    } else {
      outname = base_filename + ".sdf";
    }

    std::cout << "Writing binary SDF to: " << outname << "\n";

    // Use shared SDF file writing function
    int inside_count = 0;
    int total_count = phi_grid.ni * phi_grid.nj * phi_grid.nk;

    if (!write_sdf_binary(outname, phi_grid, min_box, dx, &inside_count)) {
      std::cerr << "ERROR: Failed to write SDF file.\n";
      exit(-1);
    }

    // Print validation statistics
    std::cout << "\n========================================\n";
    std::cout << "Output Summary\n";
    std::cout << "========================================\n";
    std::cout << "File: " << outname << "\n";
    std::cout << "Dimensions: " << phi_grid.ni << " x " << phi_grid.nj << " x " << phi_grid.nk << "\n";

    if(mode_precise) {
      bool exact_match = (phi_grid.ni == target_nx && phi_grid.nj == target_ny && phi_grid.nk == target_nz);
      std::cout << "Target dimensions: " << target_nx << " x " << target_ny << " x " << target_nz << "\n";
      std::cout << "Match: " << (exact_match ? "OK" : "FAIL") << "\n";
    }

    std::cout << "Grid spacing (dx): " << dx << "\n";
    std::cout << "Bounds: (" << min_box << ") to (" << max_box << ")\n";
    std::cout << "Inside cells: " << inside_count << " / " << total_count;
    std::cout << " (" << (100.0f * inside_count / total_count) << "%)\n";

    long long file_size_bytes = 36 + (long long)total_count * sizeof(float);
    float file_size_mb = file_size_bytes / (1024.0f * 1024.0f);
    std::cout << "File size: " << file_size_mb << " MB\n";
    std::cout << "========================================\n";
  #endif

  std::cout << "Processing complete.\n";

return 0;
}
