// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include <cassert>
#include <cmath>
#include <iostream>
#include "util.h"

/**
 * @brief Fixed-size vector template for N-dimensional vectors
 *
 * Thin wrapper around fixed-size C-style arrays providing vector arithmetic operations.
 * For example, Vec<3,float> represents a 3D float vector equivalent to float[3].
 * Entries are accessed via operator[] (zero-indexed). Arithmetic operators (+, -, *, /)
 * are overloaded for element-wise operations. Additional functions provide dot products,
 * norms, cross products, and other vector operations. Common types have convenient
 * typedefs: Vec3f (Vec<3,float>), Vec2i (Vec<2,int>), Vec3ui (Vec<3,unsigned int>), etc.
 *
 * @tparam N Dimension of the vector (number of components)
 * @tparam T Component type (float, int, unsigned int, etc.)
 */
template<unsigned int N, class T>
struct Vec
{
   T v[N]; /**< Array storing vector components */

   Vec<N,T>(void)
   {}

   explicit Vec<N,T>(T value_for_all)
   { for(unsigned int i=0; i<N; ++i) v[i]=value_for_all; }

   template<class S>
   explicit Vec<N,T>(const S *source)
   { for(unsigned int i=0; i<N; ++i) v[i]=(T)source[i]; }

   template <class S>
   explicit Vec<N,T>(const Vec<N,S>& source)
   { for(unsigned int i=0; i<N; ++i) v[i]=(T)source[i]; }

   Vec<N,T>(T v0, T v1)
   {
      assert(N==2);
      v[0]=v0; v[1]=v1;
   }

   Vec<N,T>(T v0, T v1, T v2)
   {
      assert(N==3);
      v[0]=v0; v[1]=v1; v[2]=v2;
   }

   Vec<N,T>(T v0, T v1, T v2, T v3)
   {
      assert(N==4);
      v[0]=v0; v[1]=v1; v[2]=v2; v[3]=v3;
   }

   Vec<N,T>(T v0, T v1, T v2, T v3, T v4)
   {
      assert(N==5);
      v[0]=v0; v[1]=v1; v[2]=v2; v[3]=v3; v[4]=v4;
   }

     Vec<N,T>(T v0, T v1, T v2, T v3, T v4, T v5)
   {
      assert(N==6);
      v[0]=v0; v[1]=v1; v[2]=v2; v[3]=v3; v[4]=v4; v[5]=v5;
   }

   T &operator[](int index)
   {
      assert(0<=index && (unsigned int)index<N);
      return v[index];
   }

   const T &operator[](int index) const
   {
      assert(0<=index && (unsigned int)index<N);
      return v[index];
   }

   bool nonzero(void) const
   {
      for(unsigned int i=0; i<N; ++i) if(v[i]) return true;
      return false;
   }

   Vec<N,T> operator+=(const Vec<N,T> &w)
   {
      for(unsigned int i=0; i<N; ++i) v[i]+=w[i];
      return *this;
   }

   Vec<N,T> operator+(const Vec<N,T> &w) const
   {
      Vec<N,T> sum(*this);
      sum+=w;
      return sum;
   }

   Vec<N,T> operator-=(const Vec<N,T> &w)
   {
      for(unsigned int i=0; i<N; ++i) v[i]-=w[i];
      return *this;
   }

   Vec<N,T> operator-(void) const // unary minus
   {
      Vec<N,T> negative;
      for(unsigned int i=0; i<N; ++i) negative.v[i]=-v[i];
      return negative;
   }

   Vec<N,T> operator-(const Vec<N,T> &w) const // (binary) subtraction
   {
      Vec<N,T> diff(*this);
      diff-=w;
      return diff;
   }

   Vec<N,T> operator*=(T a)
   {
      for(unsigned int i=0; i<N; ++i) v[i]*=a;
      return *this;
   }

   Vec<N,T> operator*(T a) const
   {
      Vec<N,T> w(*this);
      w*=a;
      return w;
   }

   Vec<N,T> operator*=(const Vec<N,T> &w)
   {
      for(unsigned int i=0; i<N; ++i) v[i]*=w.v[i];
      return *this;
   }

   Vec<N,T> operator*(const Vec<N,T> &w) const
   {
      Vec<N,T> componentwise_product;
      for(unsigned int i=0; i<N; ++i) componentwise_product[i]=v[i]*w.v[i];
      return componentwise_product;
   }

   Vec<N,T> operator/=(T a)
   {
      for(unsigned int i=0; i<N; ++i) v[i]/=a;
      return *this;
   }

   Vec<N,T> operator/(T a) const
   {
      Vec<N,T> w(*this);
      w/=a;
      return w;
   }
};

/** @name Common Vec Type Aliases
 * Convenient typedefs for commonly used vector types
 * @{
 */
typedef Vec<2,double>         Vec2d;  /**< 2D double-precision vector */
typedef Vec<2,float>          Vec2f;  /**< 2D single-precision vector */
typedef Vec<2,int>            Vec2i;  /**< 2D integer vector */
typedef Vec<2,unsigned int>   Vec2ui; /**< 2D unsigned integer vector */
typedef Vec<2,short>          Vec2s;  /**< 2D short integer vector */
typedef Vec<2,unsigned short> Vec2us; /**< 2D unsigned short vector */
typedef Vec<2,char>           Vec2c;  /**< 2D char vector */
typedef Vec<2,unsigned char>  Vec2uc; /**< 2D unsigned char vector */

typedef Vec<3,double>         Vec3d;  /**< 3D double-precision vector */
typedef Vec<3,float>          Vec3f;  /**< 3D single-precision vector */
typedef Vec<3,int>            Vec3i;  /**< 3D integer vector */
typedef Vec<3,unsigned int>   Vec3ui; /**< 3D unsigned integer vector */
typedef Vec<3,short>          Vec3s;  /**< 3D short integer vector */
typedef Vec<3,unsigned short> Vec3us; /**< 3D unsigned short vector */
typedef Vec<3,char>           Vec3c;  /**< 3D char vector */
typedef Vec<3,unsigned char>  Vec3uc; /**< 3D unsigned char vector */

typedef Vec<4,double>         Vec4d;  /**< 4D double-precision vector */
typedef Vec<4,float>          Vec4f;  /**< 4D single-precision vector */
typedef Vec<4,int>            Vec4i;  /**< 4D integer vector */
typedef Vec<4,unsigned int>   Vec4ui; /**< 4D unsigned integer vector */
typedef Vec<4,short>          Vec4s;  /**< 4D short integer vector */
typedef Vec<4,unsigned short> Vec4us; /**< 4D unsigned short vector */
typedef Vec<4,char>           Vec4c;  /**< 4D char vector */
typedef Vec<4,unsigned char>  Vec4uc; /**< 4D unsigned char vector */

typedef Vec<6,double>         Vec6d;  /**< 6D double-precision vector */
typedef Vec<6,float>          Vec6f;  /**< 6D single-precision vector */
typedef Vec<6,unsigned int>   Vec6ui; /**< 6D unsigned integer vector */
typedef Vec<6,int>            Vec6i;  /**< 6D integer vector */
typedef Vec<6,short>          Vec6s;  /**< 6D short integer vector */
typedef Vec<6,unsigned short> Vec6us; /**< 6D unsigned short vector */
typedef Vec<6,char>           Vec6c;  /**< 6D char vector */
typedef Vec<6,unsigned char>  Vec6uc; /**< 6D unsigned char vector */
/** @} */

/** @name Vector Operations
 * Mathematical operations on vectors
 * @{
 */

/**
 * @brief Compute squared magnitude (length) of vector
 * @param a Input vector
 * @return Sum of squares of all components
 */
template<unsigned int N, class T>
T mag2(const Vec<N,T> &a)
{
   T l=sqr(a.v[0]);
   for(unsigned int i=1; i<N; ++i) l+=sqr(a.v[i]);
   return l;
}

/**
 * @brief Compute magnitude (length) of vector
 * @param a Input vector
 * @return Square root of sum of squares of components
 */
template<unsigned int N, class T>
T mag(const Vec<N,T> &a)
{ return sqrt(mag2(a)); }

/**
 * @brief Compute squared Euclidean distance between two vectors
 * @param a First vector
 * @param b Second vector
 * @return Squared distance between a and b
 */
template<unsigned int N, class T>
inline T dist2(const Vec<N,T> &a, const Vec<N,T> &b)
{
   T d=sqr(a.v[0]-b.v[0]);
   for(unsigned int i=1; i<N; ++i) d+=sqr(a.v[i]-b.v[i]);
   return d;
}

/**
 * @brief Compute Euclidean distance between two vectors
 * @param a First vector
 * @param b Second vector
 * @return Distance between a and b
 */
template<unsigned int N, class T>
inline T dist(const Vec<N,T> &a, const Vec<N,T> &b)
{ return std::sqrt(dist2(a,b)); }

/**
 * @brief Normalize vector in-place to unit length
 * @param a Vector to normalize (modified in-place)
 */
template<unsigned int N, class T>
inline void normalize(Vec<N,T> &a)
{ a/=mag(a); }

/**
 * @brief Return normalized copy of vector
 * @param a Input vector
 * @return Unit vector in same direction as a
 */
template<unsigned int N, class T>
inline Vec<N,T> normalized(const Vec<N,T> &a)
{ return a/mag(a); }

template<unsigned int N, class T> 
inline T infnorm(const Vec<N,T> &a)
{
   T d=std::fabs(a.v[0]);
   for(unsigned int i=1; i<N; ++i) d=max(std::fabs(a.v[i]),d);
   return d;
}

template<unsigned int N, class T>
void zero(Vec<N,T> &a)
{ 
   for(unsigned int i=0; i<N; ++i)
      a.v[i] = 0;
}

template<unsigned int N, class T>
std::ostream &operator<<(std::ostream &out, const Vec<N,T> &v)
{
   out<<v.v[0];
   for(unsigned int i=1; i<N; ++i)
      out<<' '<<v.v[i];
   return out;
}

template<unsigned int N, class T>
std::istream &operator>>(std::istream &in, Vec<N,T> &v)
{
   in>>v.v[0];
   for(unsigned int i=1; i<N; ++i)
      in>>v.v[i];
   return in;
}

template<unsigned int N, class T> 
inline bool operator==(const Vec<N,T> &a, const Vec<N,T> &b)
{ 
   bool t = (a.v[0] == b.v[0]);
   unsigned int i=1;
   while(i<N && t) {
      t = t && (a.v[i]==b.v[i]); 
      ++i;
   }
   return t;
}

template<unsigned int N, class T> 
inline bool operator!=(const Vec<N,T> &a, const Vec<N,T> &b)
{ 
   bool t = (a.v[0] != b.v[0]);
   unsigned int i=1;
   while(i<N && !t) {
      t = t || (a.v[i]!=b.v[i]); 
      ++i;
   }
   return t;
}

template<unsigned int N, class T>
inline Vec<N,T> operator*(T a, const Vec<N,T> &v)
{
   Vec<N,T> w(v);
   w*=a;
   return w;
}

template<unsigned int N, class T>
inline T min(const Vec<N,T> &a)
{
   T m=a.v[0];
   for(unsigned int i=1; i<N; ++i) if(a.v[i]<m) m=a.v[i];
   return m;
}

template<unsigned int N, class T>
inline Vec<N,T> min_union(const Vec<N,T> &a, const Vec<N,T> &b)
{
   Vec<N,T> m;
   for(unsigned int i=0; i<N; ++i) (a.v[i] < b.v[i]) ? m.v[i]=a.v[i] : m.v[i]=b.v[i];
   return m;
}

template<unsigned int N, class T>
inline Vec<N,T> max_union(const Vec<N,T> &a, const Vec<N,T> &b)
{
   Vec<N,T> m;
   for(unsigned int i=0; i<N; ++i) (a.v[i] > b.v[i]) ? m.v[i]=a.v[i] : m.v[i]=b.v[i];
   return m;
}

template<unsigned int N, class T>
inline T max(const Vec<N,T> &a)
{
   T m=a.v[0];
   for(unsigned int i=1; i<N; ++i) if(a.v[i]>m) m=a.v[i];
   return m;
}

/**
 * @brief Compute dot product of two vectors
 * @param a First vector
 * @param b Second vector
 * @return Dot product (sum of component-wise products)
 */
template<unsigned int N, class T>
inline T dot(const Vec<N,T> &a, const Vec<N,T> &b)
{
   T d=a.v[0]*b.v[0];
   for(unsigned int i=1; i<N; ++i) d+=a.v[i]*b.v[i];
   return d;
}

/**
 * @brief Rotate 2D vector by angle (counter-clockwise)
 * @param a Vector to rotate
 * @param angle Rotation angle in radians
 * @return Rotated vector
 */
template<class T>
inline Vec<2,T> rotate(const Vec<2,T>& a, float angle)
{
   T c = cos(angle);
   T s = sin(angle);
   return Vec<2,T>(c*a[0] - s*a[1],s*a[0] + c*a[1]);
}

/**
 * @brief Compute perpendicular 2D vector (90 degree counter-clockwise rotation)
 * @param a Input vector
 * @return Perpendicular vector (-y, x)
 */
template<class T>
inline Vec<2,T> perp(const Vec<2,T> &a)
{ return Vec<2,T>(-a.v[1], a.v[0]); }

/**
 * @brief Compute 2D cross product (z-component of 3D cross product)
 * @param a First vector
 * @param b Second vector
 * @return Scalar cross product a.x*b.y - a.y*b.x
 */
template<class T>
inline T cross(const Vec<2,T> &a, const Vec<2,T> &b)
{ return a.v[0]*b.v[1]-a.v[1]*b.v[0]; }

/**
 * @brief Compute 3D cross product
 * @param a First vector
 * @param b Second vector
 * @return Vector perpendicular to both a and b following right-hand rule
 */
template<class T>
inline Vec<3,T> cross(const Vec<3,T> &a, const Vec<3,T> &b)
{ return Vec<3,T>(a.v[1]*b.v[2]-a.v[2]*b.v[1], a.v[2]*b.v[0]-a.v[0]*b.v[2], a.v[0]*b.v[1]-a.v[1]*b.v[0]); }

template<class T>
inline T triple(const Vec<3,T> &a, const Vec<3,T> &b, const Vec<3,T> &c)
{ return a.v[0]*(b.v[1]*c.v[2]-b.v[2]*c.v[1])
        +a.v[1]*(b.v[2]*c.v[0]-b.v[0]*c.v[2])
        +a.v[2]*(b.v[0]*c.v[1]-b.v[1]*c.v[0]); }

template<unsigned int N, class T>
inline unsigned int hash(const Vec<N,T> &a)
{
   unsigned int h=a.v[0];
   for(unsigned int i=1; i<N; ++i)
      h=hash(h ^ a.v[i]);
   return h;
}

template<unsigned int N, class T>
inline void assign(const Vec<N,T> &a, T &a0, T &a1)
{ 
   assert(N==2);
   a0=a.v[0]; a1=a.v[1];
}

template<unsigned int N, class T>
inline void assign(const Vec<N,T> &a, T &a0, T &a1, T &a2)
{ 
   assert(N==3);
   a0=a.v[0]; a1=a.v[1]; a2=a.v[2];
}

template<unsigned int N, class T>
inline void assign(const Vec<N,T> &a, T &a0, T &a1, T &a2, T &a3)
{ 
   assert(N==4);
   a0=a.v[0]; a1=a.v[1]; a2=a.v[2]; a3=a.v[3];
}

template<unsigned int N, class T>
inline void assign(const Vec<N,T> &a, T &a0, T &a1, T &a2, T &a3, T &a4, T &a5)
{ 
   assert(N==6);
   a0=a.v[0]; a1=a.v[1]; a2=a.v[2]; a3=a.v[3]; a4=a.v[4]; a5=a.v[5];
}

template<unsigned int N, class T>
inline Vec<N,int> round(const Vec<N,T> &a)
{ 
   Vec<N,int> rounded;
   for(unsigned int i=0; i<N; ++i)
      rounded.v[i]=lround(a.v[i]);
   return rounded; 
}

template<unsigned int N, class T>
inline Vec<N,int> floor(const Vec<N,T> &a)
{ 
   Vec<N,int> rounded;
   for(unsigned int i=0; i<N; ++i)
      rounded.v[i]=(int)floor(a.v[i]);
   return rounded; 
}

template<unsigned int N, class T>
inline Vec<N,int> ceil(const Vec<N,T> &a)
{ 
   Vec<N,int> rounded;
   for(unsigned int i=0; i<N; ++i)
      rounded.v[i]=(int)ceil(a.v[i]);
   return rounded; 
}

template<unsigned int N, class T>
inline Vec<N,T> fabs(const Vec<N,T> &a)
{ 
   Vec<N,T> result;
   for(unsigned int i=0; i<N; ++i)
      result.v[i]=fabs(a.v[i]);
   return result; 
}

template<unsigned int N, class T>
inline void minmax(const Vec<N,T> &x0, const Vec<N,T> &x1, Vec<N,T> &xmin, Vec<N,T> &xmax)
{
   for(unsigned int i=0; i<N; ++i)
      minmax(x0.v[i], x1.v[i], xmin.v[i], xmax.v[i]);
}

template<unsigned int N, class T>
inline void minmax(const Vec<N,T> &x0, const Vec<N,T> &x1, const Vec<N,T> &x2, Vec<N,T> &xmin, Vec<N,T> &xmax)
{
   for(unsigned int i=0; i<N; ++i)
      minmax(x0.v[i], x1.v[i], x2.v[i], xmin.v[i], xmax.v[i]);
}

template<unsigned int N, class T>
inline void minmax(const Vec<N,T> &x0, const Vec<N,T> &x1, const Vec<N,T> &x2, const Vec<N,T> &x3,
                   Vec<N,T> &xmin, Vec<N,T> &xmax)
{
   for(unsigned int i=0; i<N; ++i)
      minmax(x0.v[i], x1.v[i], x2.v[i], x3.v[i], xmin.v[i], xmax.v[i]);
}

template<unsigned int N, class T>
inline void minmax(const Vec<N,T> &x0, const Vec<N,T> &x1, const Vec<N,T> &x2, const Vec<N,T> &x3, const Vec<N,T> &x4,
                   Vec<N,T> &xmin, Vec<N,T> &xmax)
{
   for(unsigned int i=0; i<N; ++i)
      minmax(x0.v[i], x1.v[i], x2.v[i], x3.v[i], x4.v[i], xmin.v[i], xmax.v[i]);
}

template<unsigned int N, class T>
inline void minmax(const Vec<N,T> &x0, const Vec<N,T> &x1, const Vec<N,T> &x2, const Vec<N,T> &x3, const Vec<N,T> &x4,
                   const Vec<N,T> &x5, Vec<N,T> &xmin, Vec<N,T> &xmax)
{
   for(unsigned int i=0; i<N; ++i)
      minmax(x0.v[i], x1.v[i], x2.v[i], x3.v[i], x4.v[i], x5.v[i], xmin.v[i], xmax.v[i]);
}

template<unsigned int N, class T>
/**
 * @brief Update bounding box to include vector
 * @param x Vector to include
 * @param xmin Minimum corner of bounding box (updated in-place)
 * @param xmax Maximum corner of bounding box (updated in-place)
 */
inline void update_minmax(const Vec<N,T> &x, Vec<N,T> &xmin, Vec<N,T> &xmax)
{
   for(unsigned int i=0; i<N; ++i) update_minmax(x[i], xmin[i], xmax[i]);
}

/** @} */ // end of Vector Operations group

