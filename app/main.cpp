// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#include "config.h"
#include "sdfgen_unified.h"  // Unified API with CPU/GPU backend selection
#include "sdf_io.h"          // Shared SDF file I/O functions
#include "mesh_io.h"         // Mesh file loading (OBJ, STL)
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

int main(int argc, char* argv[]) {

  // Detect mode based on argument count and file extension
  bool mode_precise = false;  // Mode 2: precise grid dimensions (STL with Nx, Ny, Nz)
  bool is_stl = false;
  std::string filename;

  if(argc >= 2) {
    filename = std::string(argv[1]);
    if(filename.size() >= 4 && filename.substr(filename.size()-4) == std::string(".stl")) {
      is_stl = true;
      if(argc >= 3) {
        mode_precise = true;  // STL with Nx [padding] OR Nx Ny Nz [padding]
      }
    }
  }

  // Print usage if arguments don't match any mode
  if((!mode_precise && argc < 4) || (mode_precise && argc < 3)) {
    std::cout << "SDFGen - A utility for converting closed oriented triangle meshes into grid-based signed distance fields.\n\n";

    std::cout << "=== Mode 1: Legacy OBJ with dx spacing ===\n";
    std::cout << "Usage: SDFGen <file.obj> <dx> <padding> [threads]\n\n";
    std::cout << "Where:\n";
    std::cout << "  <file.obj>  Wavefront OBJ file (text format, triangles only)\n";
    std::cout << "  <dx>        Grid cell size (determines resolution automatically)\n";
    std::cout << "  <padding>   Number of padding cells around mesh (minimum 1)\n";
    std::cout << "  [threads]   Optional: Number of CPU threads (0=auto, default: 0)\n\n";

    std::cout << "=== Mode 2a: STL with proportional dimensions (recommended) ===\n";
    std::cout << "Usage: SDFGen <file.stl> <Nx> [padding] [threads]\n\n";
    std::cout << "Where:\n";
    std::cout << "  <file.stl>  Binary STL file\n";
    std::cout << "  <Nx>        Grid size in X dimension (Ny, Nz calculated proportionally)\n";
    std::cout << "  [padding]   Optional padding cells (default: 1)\n";
    std::cout << "  [threads]   Optional: Number of CPU threads (0=auto, default: 0)\n\n";

    std::cout << "=== Mode 2b: STL with manual dimensions ===\n";
    std::cout << "Usage: SDFGen <file.stl> <Nx> <Ny> <Nz> [padding] [threads]\n\n";
    std::cout << "Where:\n";
    std::cout << "  <file.stl>  Binary STL file\n";
    std::cout << "  <Nx>        Exact grid size in X dimension\n";
    std::cout << "  <Ny>        Exact grid size in Y dimension\n";
    std::cout << "  <Nz>        Exact grid size in Z dimension\n";
    std::cout << "  [padding]   Optional padding cells (default: 1)\n";
    std::cout << "  [threads]   Optional: Number of CPU threads (0=auto, default: 0)\n\n";

    std::cout << "Output: Binary SDF file with 36-byte header + float32 grid data\n";
    std::cout << "Header: 3 ints (Nx,Ny,Nz) + 6 floats (bounds_min, bounds_max)\n\n";

    std::cout << "=== Hardware Acceleration ===\n";
    std::cout << "GPU acceleration (CUDA) is used automatically if available.\n";
    std::cout << "The program will detect and report which backend is being used.\n";
    std::cout << "No special flags needed - it just works!\n\n";

    exit(-1);
  }

  // Common variables
  std::vector<Vec3f> vertList;
  std::vector<Vec3ui> faceList;
  Vec3f min_box, max_box;
  int padding = 1;
  int target_nx = 0, target_ny = 0, target_nz = 0;
  float dx = 0.0f;
  int num_threads = 0;  // 0 = auto-detect

  std::cout << "========================================\n";
  std::cout << "SDFGen - SDF Generation Tool\n";
  std::cout << "========================================\n\n";

  if(mode_precise) {
    // === MODE 2: STL with precise dimensions ===
    std::cout << "Mode: Precise grid dimensions (STL)\n";
    std::cout << "Input: " << filename << "\n\n";

    // Load STL file first to get mesh dimensions
    if(!meshio::load_stl(argv[1], vertList, faceList, min_box, max_box)) {
      std::cerr << "Failed to load STL file.\n";
      exit(-1);
    }

    Vec3f mesh_size = max_box - min_box;

    // Parse grid dimensions - support both 1-parameter and 3-parameter modes
    // Handle argc=5 ambiguity: could be Mode 2a (Nx, padding, threads) or Mode 2b (Nx, Ny, Nz)
    // Heuristic: if 2nd value < 20, treat as Mode 2a (padding is usually small)
    bool is_mode2a = (argc == 3 || argc == 4 || (argc == 5 && atoi(argv[3]) < 20));

    if(is_mode2a) {
      // Single parameter mode: calculate Ny and Nz proportionally from Nx
      target_nx = atoi(argv[2]);
      if(argc >= 4) {
        padding = atoi(argv[3]);
      }
      if(argc == 5) {
        num_threads = atoi(argv[4]);
      }

      if(target_nx <= 0) {
        std::cerr << "Error: Grid dimension must be a positive integer.\n";
        exit(-1);
      }
      if(padding < 1) padding = 1;

      // Calculate dx based on X dimension
      dx = mesh_size[0] / (target_nx - 2 * padding);

      // Calculate Ny and Nz proportionally to maintain aspect ratio
      target_ny = (int)((mesh_size[1] / dx) + 0.5f) + 2 * padding;
      target_nz = (int)((mesh_size[2] / dx) + 0.5f) + 2 * padding;

      std::cout << "Mode: Proportional dimensions (single parameter)\n";
      std::cout << "Input Nx: " << target_nx << "\n";
      std::cout << "Calculated grid: " << target_nx << " x " << target_ny << " x " << target_nz << "\n";
      std::cout << "Padding: " << padding << " cells\n";
      if(argc == 5) {
        std::cout << "CPU threads: " << (num_threads == 0 ? "auto-detect" : std::to_string(num_threads)) << "\n";
      }
      std::cout << "\n";

      std::cout << "Grid spacing calculation:\n";
      std::cout << "  Mesh size: " << mesh_size[0] << " x " << mesh_size[1] << " x " << mesh_size[2] << " m\n";
      std::cout << "  dx = " << dx << " m (based on X dimension)\n";
      std::cout << "  Aspect ratios preserved: Y=" << (mesh_size[1]/mesh_size[0]) << ", Z=" << (mesh_size[2]/mesh_size[0]) << "\n\n";

    } else {
      // Three parameter mode: manual specification
      target_nx = atoi(argv[2]);
      target_ny = atoi(argv[3]);
      target_nz = atoi(argv[4]);
      if(argc >= 6) {
        padding = atoi(argv[5]);
      }
      if(argc == 7) {
        num_threads = atoi(argv[6]);
      }

      if(target_nx <= 0 || target_ny <= 0 || target_nz <= 0) {
        std::cerr << "Error: Grid dimensions must be positive integers.\n";
        exit(-1);
      }
      if(padding < 1) padding = 1;

      std::cout << "Mode: Manual dimensions (three parameters)\n";
      std::cout << "Target grid: " << target_nx << " x " << target_ny << " x " << target_nz << "\n";
      std::cout << "Padding: " << padding << " cells\n";
      if(argc == 7) {
        std::cout << "CPU threads: " << (num_threads == 0 ? "auto-detect" : std::to_string(num_threads)) << "\n";
      }
      std::cout << "\n";

      // Calculate dx to fit the mesh into target grid
      float dx_x = mesh_size[0] / (target_nx - 2 * padding);
      float dx_y = mesh_size[1] / (target_ny - 2 * padding);
      float dx_z = mesh_size[2] / (target_nz - 2 * padding);

      // Use the maximum dx to ensure all dimensions fit
      dx = std::max(dx_x, std::max(dx_y, dx_z));

      std::cout << "Grid spacing calculation:\n";
      std::cout << "  Mesh size: " << mesh_size[0] << " x " << mesh_size[1] << " x " << mesh_size[2] << " m\n";
      std::cout << "  dx_x = " << dx_x << ", dx_y = " << dx_y << ", dx_z = " << dx_z << "\n";
      std::cout << "  Using dx = " << dx << " m (maximum to fit all dimensions)\n\n";
    }

  } else {
    // === MODE 1: OBJ with dx spacing (legacy) ===
    std::cout << "Mode: Legacy dx spacing (OBJ)\n";
    std::cout << "Input: " << filename << "\n\n";

    if(filename.size() < 5 || filename.substr(filename.size()-4) != std::string(".obj")) {
      std::cerr << "Error: Mode 1 requires OBJ file (.obj extension).\n";
      exit(-1);
    }

    // Parse dx and padding
    std::stringstream arg2(argv[2]);
    arg2 >> dx;

    std::stringstream arg3(argv[3]);
    arg3 >> padding;

    if(padding < 1) padding = 1;

    // Parse optional threads parameter
    if(argc >= 5) {
      std::stringstream arg4(argv[4]);
      arg4 >> num_threads;
    }

    std::cout << "Grid spacing (dx): " << dx << "\n";
    std::cout << "Padding: " << padding << " cells\n";
    if(argc >= 5) {
      std::cout << "CPU threads: " << (num_threads == 0 ? "auto-detect" : std::to_string(num_threads)) << "\n";
    }
    std::cout << "\n";

    // Load OBJ file
    if(!meshio::load_obj(argv[1], vertList, faceList, min_box, max_box)) {
      std::cerr << "Failed to load OBJ file. Terminating.\n";
      exit(-1);
    }
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
  sdfgen::HardwareBackend backend = sdfgen::HardwareBackend::Auto;

  // Report which backend will be/was used
  std::cout << "  Hardware: ";
  if(sdfgen::is_gpu_available()) {
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
