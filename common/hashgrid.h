// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "hashtable.h"
#include "vec.h"

//========================================================= first do 2D ============================

/**
 * @brief 2D spatial hash grid for efficient spatial queries
 *
 * Uniform grid subdivision of 2D space using hash table for storage. Each cell can contain
 * multiple data items. Useful for broad-phase collision detection and nearest neighbor queries.
 *
 * @tparam DataType Type of data stored in grid cells
 */
template<class DataType>
struct HashGrid2
{
   double dx, overdx; /**< Grid cell size and its reciprocal */
   HashTable<Vec2i,DataType> grid; /**< Hash table mapping cell coordinates to data */

   explicit HashGrid2(double dx_=1, int expected_size=512)
      : dx(dx_), overdx(1/dx_), grid(expected_size)
   {}

   // only do this with an empty grid
   void set_grid_size(double dx_)
   { assert(size()==0); dx=dx_; overdx=1/dx; }

   void add_point(const Vec2d &x, const DataType &datum)
   { grid.add(round(x*overdx), datum); }

   void delete_point(const Vec2d &x, const DataType &datum)
   { grid.delete_entry(round(x*overdx), datum); }

   void add_box(const Vec2d &xmin, const Vec2d &xmax, const DataType &datum)
   {
      Vec2i imin=round(xmin*overdx), imax=round(xmax*overdx);
      for(int j=imin[1]; j<=imax[1]; ++j) for(int i=imin[0]; i<=imax[0]; ++i)
         grid.add(Vec2i(i,j), datum);
   }

   void delete_box(const Vec2d &xmin, const Vec2d &xmax, const DataType &datum)
   {
      Vec2i imin=round(xmin*overdx), imax=round(xmax*overdx);
      for(int j=imin[1]; j<=imax[1]; ++j) for(int i=imin[0]; i<=imax[0]; ++i)
         grid.delete_entry(Vec2i(i,j), datum);
   }

   unsigned int size(void) const
   { return grid.size(); }

   void clear(void)
   { grid.clear(); }

   void reserve(unsigned int expected_size)
   { grid.reserve(expected_size); }

   bool find_first_point(const Vec2d &x, DataType &datum) const
   { return grid.get_entry(round(x*overdx), datum); }

   bool find_point(const Vec2d &x, std::vector<DataType> &data_list) const
   {
      data_list.resize(0);
      grid.append_all_entries(round(x*overdx), data_list);
      return data_list.size()>0;
   }

   bool find_box(const Vec2d &xmin, const Vec2d &xmax, std::vector<DataType> &data_list) const
   {
      data_list.resize(0);
      Vec2i imin=round(xmin*overdx), imax=round(xmax*overdx);
      for(int j=imin[1]; j<=imax[1]; ++j) for(int i=imin[0]; i<=imax[0]; ++i)
         grid.append_all_entries(Vec2i(i,j), data_list);
      return data_list.size()>0;
   }
};

//==================================== and now in 3D =================================================

/**
 * @brief 3D spatial hash grid for efficient spatial queries
 *
 * Uniform grid subdivision of 3D space using hash table for storage. Each cell can contain
 * multiple data items. Used during SDF computation for fast triangle-to-grid-cell queries.
 *
 * @tparam DataType Type of data stored in grid cells (typically triangle indices)
 */
template<class DataType>
struct HashGrid3
{
   double dx, overdx; /**< Grid cell size and its reciprocal */
   HashTable<Vec3i,DataType> grid; /**< Hash table mapping cell coordinates to data */

   explicit HashGrid3(double dx_=1, int expected_size=512)
      : dx(dx_), overdx(1/dx_), grid(expected_size)
   {}

   // only do this with an empty grid
   void set_grid_size(double dx_)
   { assert(size()==0); dx=dx_; overdx=1/dx; }

   void add_point(const Vec3d &x, const DataType &datum)
   { grid.add(round(x*overdx), datum); }

   void delete_point(const Vec3d &x, const DataType &datum)
   { grid.delete_entry(round(x*overdx), datum); }

   void add_box(const Vec3d &xmin, const Vec3d &xmax, const DataType &datum)
   {
      Vec3i imin=round(xmin*overdx), imax=round(xmax*overdx);
      for(int k=imin[2]; k<=imax[2]; ++k) for(int j=imin[1]; j<=imax[1]; ++j) for(int i=imin[0]; i<=imax[0]; ++i)
         grid.add(Vec3i(i,j,k), datum);
   }

   void delete_box(const Vec3d &xmin, const Vec3d &xmax, const DataType &datum)
   {
      Vec3i imin=round(xmin*overdx), imax=round(xmax*overdx);
      for(int k=imin[2]; k<=imax[2]; ++k) for(int j=imin[1]; j<=imax[1]; ++j) for(int i=imin[0]; i<=imax[0]; ++i)
         grid.delete_entry(Vec3i(i,j,k), datum);
   }
   
   unsigned int size(void) const
   { return grid.size(); }

   void clear(void)
   { grid.clear(); }

   void reserve(unsigned int expected_size)
   { grid.reserve(expected_size); }

   bool find_first_point(const Vec3d &x, DataType &index) const
   { return grid.get_entry(round(x*overdx), index); }

   bool find_point(const Vec3d &x, std::vector<DataType> &data_list) const
   {
      data_list.resize(0);
      grid.append_all_entries(round(x*overdx), data_list);
      return data_list.size()>0;
   }

   bool find_box(const Vec3d &xmin, const Vec3d &xmax, std::vector<DataType> &data_list) const
   {
      data_list.resize(0);
      Vec3i imin=round(xmin*overdx), imax=round(xmax*overdx);
      for(int k=imin[2]; k<=imax[2]; ++k) for(int j=imin[1]; j<=imax[1]; ++j) for(int i=imin[0]; i<=imax[0]; ++i)
         grid.append_all_entries(Vec3i(i, j, k), data_list);
      return data_list.size()>0;
   }
};

