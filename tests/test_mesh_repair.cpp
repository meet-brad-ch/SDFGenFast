// SDFGen - Signed Distance Field Generator
// Copyright (c) 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Unit tests for mesh watertightness analysis and repair API
#include "mesh_repair.h"
#include <iostream>
#include <string>
#include <cmath>

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) std::cout << "  Testing: " << name << "... ";
#define PASS() { std::cout << "PASSED\n"; tests_passed++; }
#define FAIL(msg) { std::cout << "FAILED: " << msg << "\n"; tests_failed++; }

// Create a watertight cube mesh (8 vertices, 12 triangles)
void create_watertight_cube(std::vector<Vec3f>& vertices, std::vector<Vec3ui>& faces) {
    vertices.clear();
    faces.clear();

    // 8 vertices of unit cube
    vertices.push_back(Vec3f(0, 0, 0));  // 0
    vertices.push_back(Vec3f(1, 0, 0));  // 1
    vertices.push_back(Vec3f(1, 1, 0));  // 2
    vertices.push_back(Vec3f(0, 1, 0));  // 3
    vertices.push_back(Vec3f(0, 0, 1));  // 4
    vertices.push_back(Vec3f(1, 0, 1));  // 5
    vertices.push_back(Vec3f(1, 1, 1));  // 6
    vertices.push_back(Vec3f(0, 1, 1));  // 7

    // 12 triangles (2 per face, 6 faces)
    // Bottom (z=0)
    faces.push_back(Vec3ui(0, 2, 1));
    faces.push_back(Vec3ui(0, 3, 2));
    // Top (z=1)
    faces.push_back(Vec3ui(4, 5, 6));
    faces.push_back(Vec3ui(4, 6, 7));
    // Front (y=0)
    faces.push_back(Vec3ui(0, 1, 5));
    faces.push_back(Vec3ui(0, 5, 4));
    // Back (y=1)
    faces.push_back(Vec3ui(2, 3, 7));
    faces.push_back(Vec3ui(2, 7, 6));
    // Left (x=0)
    faces.push_back(Vec3ui(0, 4, 7));
    faces.push_back(Vec3ui(0, 7, 3));
    // Right (x=1)
    faces.push_back(Vec3ui(1, 2, 6));
    faces.push_back(Vec3ui(1, 6, 5));
}

// Create a cube with one face missing (hole)
void create_cube_with_hole(std::vector<Vec3f>& vertices, std::vector<Vec3ui>& faces) {
    create_watertight_cube(vertices, faces);
    // Remove last 2 triangles (right face) to create a hole
    faces.pop_back();
    faces.pop_back();
}

// Create mesh with duplicate vertices (like STL format)
void create_cube_stl_style(std::vector<Vec3f>& vertices, std::vector<Vec3ui>& faces) {
    vertices.clear();
    faces.clear();

    // STL-style: each triangle has its own 3 vertices (36 vertices for 12 triangles)
    // Bottom face
    vertices.push_back(Vec3f(0, 0, 0)); vertices.push_back(Vec3f(1, 1, 0)); vertices.push_back(Vec3f(1, 0, 0));
    faces.push_back(Vec3ui(0, 1, 2));
    vertices.push_back(Vec3f(0, 0, 0)); vertices.push_back(Vec3f(0, 1, 0)); vertices.push_back(Vec3f(1, 1, 0));
    faces.push_back(Vec3ui(3, 4, 5));

    // Top face
    vertices.push_back(Vec3f(0, 0, 1)); vertices.push_back(Vec3f(1, 0, 1)); vertices.push_back(Vec3f(1, 1, 1));
    faces.push_back(Vec3ui(6, 7, 8));
    vertices.push_back(Vec3f(0, 0, 1)); vertices.push_back(Vec3f(1, 1, 1)); vertices.push_back(Vec3f(0, 1, 1));
    faces.push_back(Vec3ui(9, 10, 11));

    // Front face
    vertices.push_back(Vec3f(0, 0, 0)); vertices.push_back(Vec3f(1, 0, 0)); vertices.push_back(Vec3f(1, 0, 1));
    faces.push_back(Vec3ui(12, 13, 14));
    vertices.push_back(Vec3f(0, 0, 0)); vertices.push_back(Vec3f(1, 0, 1)); vertices.push_back(Vec3f(0, 0, 1));
    faces.push_back(Vec3ui(15, 16, 17));

    // Back face
    vertices.push_back(Vec3f(1, 1, 0)); vertices.push_back(Vec3f(0, 1, 0)); vertices.push_back(Vec3f(0, 1, 1));
    faces.push_back(Vec3ui(18, 19, 20));
    vertices.push_back(Vec3f(1, 1, 0)); vertices.push_back(Vec3f(0, 1, 1)); vertices.push_back(Vec3f(1, 1, 1));
    faces.push_back(Vec3ui(21, 22, 23));

    // Left face
    vertices.push_back(Vec3f(0, 0, 0)); vertices.push_back(Vec3f(0, 0, 1)); vertices.push_back(Vec3f(0, 1, 1));
    faces.push_back(Vec3ui(24, 25, 26));
    vertices.push_back(Vec3f(0, 0, 0)); vertices.push_back(Vec3f(0, 1, 1)); vertices.push_back(Vec3f(0, 1, 0));
    faces.push_back(Vec3ui(27, 28, 29));

    // Right face
    vertices.push_back(Vec3f(1, 0, 0)); vertices.push_back(Vec3f(1, 1, 0)); vertices.push_back(Vec3f(1, 1, 1));
    faces.push_back(Vec3ui(30, 31, 32));
    vertices.push_back(Vec3f(1, 0, 0)); vertices.push_back(Vec3f(1, 1, 1)); vertices.push_back(Vec3f(1, 0, 1));
    faces.push_back(Vec3ui(33, 34, 35));
}

int main() {
    std::cout << "========================================\n";
    std::cout << "SDFGen Mesh Repair API Test\n";
    std::cout << "========================================\n\n";

    std::vector<Vec3f> vertices;
    std::vector<Vec3ui> faces;

    // =========================================================================
    // Test 1: Watertight mesh analysis
    // =========================================================================
    std::cout << "Test Group 1: Watertight Mesh Analysis\n";

    TEST("watertight cube detection")
    create_watertight_cube(vertices, faces);
    meshio::MeshAnalysis analysis = meshio::analyze_mesh(vertices, faces);
    if (analysis.is_watertight && analysis.is_manifold &&
        analysis.boundary_edges == 0 && analysis.num_holes == 0) {
        PASS()
    } else {
        FAIL("watertight cube not detected as watertight")
    }

    TEST("watertight cube edge count")
    // A cube has 12 edges, each shared by 2 triangles = 12 unique edges
    // But we count directed edges (each triangle has 3 edges) = 36 / 2 = 18 unique
    // Actually for a cube: 12 triangles * 3 edges / 2 = 18 edges
    if (analysis.total_edges == 18) {
        PASS()
    } else {
        FAIL("expected 18 edges, got " + std::to_string(analysis.total_edges))
    }

    // =========================================================================
    // Test 2: Non-watertight mesh analysis
    // =========================================================================
    std::cout << "\nTest Group 2: Non-Watertight Mesh Analysis\n";

    TEST("mesh with hole detection")
    create_cube_with_hole(vertices, faces);
    analysis = meshio::analyze_mesh(vertices, faces);
    if (!analysis.is_watertight && analysis.boundary_edges > 0 && analysis.num_holes > 0) {
        PASS()
    } else {
        FAIL("mesh with hole not detected correctly")
    }

    TEST("hole count for missing face")
    // Removing one quad face (2 triangles) creates 1 rectangular hole with 4 edges
    if (analysis.num_holes == 1) {
        PASS()
    } else {
        FAIL("expected 1 hole, got " + std::to_string(analysis.num_holes))
    }

    TEST("boundary edge count for missing face")
    // A missing quad face exposes 4 boundary edges
    if (analysis.boundary_edges == 4) {
        PASS()
    } else {
        FAIL("expected 4 boundary edges, got " + std::to_string(analysis.boundary_edges))
    }

    // =========================================================================
    // Test 3: Vertex welding
    // =========================================================================
    std::cout << "\nTest Group 3: Vertex Welding\n";

    TEST("STL-style mesh has duplicate vertices")
    create_cube_stl_style(vertices, faces);
    size_t original_count = vertices.size();
    if (original_count == 36) {  // 12 triangles * 3 vertices
        PASS()
    } else {
        FAIL("expected 36 vertices, got " + std::to_string(original_count))
    }

    TEST("welding reduces vertex count")
    int welded = meshio::weld_vertices(vertices, faces, 1e-5f);
    if (vertices.size() == 8) {  // Cube has 8 unique vertices
        PASS()
    } else {
        FAIL("expected 8 vertices after welding, got " + std::to_string(vertices.size()))
    }

    TEST("welding reports correct removal count")
    if (welded == 28) {  // 36 - 8 = 28 duplicates removed
        PASS()
    } else {
        FAIL("expected 28 removed, got " + std::to_string(welded))
    }

    TEST("welded mesh is still watertight")
    analysis = meshio::analyze_mesh(vertices, faces);
    if (analysis.is_watertight) {
        PASS()
    } else {
        FAIL("welded mesh should be watertight")
    }

    // =========================================================================
    // Test 4: Mesh repair
    // =========================================================================
    std::cout << "\nTest Group 4: Mesh Repair\n";

    TEST("repair fills holes")
    create_cube_with_hole(vertices, faces);
    size_t faces_before = faces.size();
    int holes_filled = meshio::repair_mesh(vertices, faces, 0.0f);  // No welding needed
    if (holes_filled == 1) {
        PASS()
    } else {
        FAIL("expected 1 hole filled, got " + std::to_string(holes_filled))
    }

    TEST("repair adds triangles")
    // A 4-vertex hole needs 2 triangles to fill
    if (faces.size() == faces_before + 2) {
        PASS()
    } else {
        FAIL("expected " + std::to_string(faces_before + 2) + " faces, got " + std::to_string(faces.size()))
    }

    TEST("repaired mesh is watertight")
    analysis = meshio::analyze_mesh(vertices, faces);
    if (analysis.is_watertight) {
        PASS()
    } else {
        FAIL("repaired mesh should be watertight")
    }

    // =========================================================================
    // Test 5: Edge cases
    // =========================================================================
    std::cout << "\nTest Group 5: Edge Cases\n";

    TEST("empty mesh analysis")
    vertices.clear();
    faces.clear();
    analysis = meshio::analyze_mesh(vertices, faces);
    // Empty mesh is technically watertight (no holes)
    if (analysis.total_edges == 0 && analysis.boundary_edges == 0) {
        PASS()
    } else {
        FAIL("empty mesh should have no edges")
    }

    TEST("single triangle analysis")
    vertices.push_back(Vec3f(0, 0, 0));
    vertices.push_back(Vec3f(1, 0, 0));
    vertices.push_back(Vec3f(0, 1, 0));
    faces.push_back(Vec3ui(0, 1, 2));
    analysis = meshio::analyze_mesh(vertices, faces);
    if (!analysis.is_watertight && analysis.boundary_edges == 3 && analysis.num_holes == 1) {
        PASS()
    } else {
        FAIL("single triangle should have 3 boundary edges and 1 hole")
    }

    // =========================================================================
    // Summary
    // =========================================================================
    std::cout << "\n========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "  Passed: " << tests_passed << "\n";
    std::cout << "  Failed: " << tests_failed << "\n";

    if (tests_failed == 0) {
        std::cout << "\n[OK] ALL TESTS PASSED\n";
        return 0;
    } else {
        std::cout << "\n[FAIL] SOME TESTS FAILED\n";
        return 1;
    }
}
