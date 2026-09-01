// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include <algorithm>

#ifndef M_PI
const double M_PI = 3.1415926535897932384626433832795;
#endif

#ifdef WIN32
#undef min
#undef max
#endif

using std::min;
using std::max;
using std::swap;

/** @name Basic Math Utilities
 * Simple mathematical helper functions
 * @{
 */

/**
 * @brief Compute square of a value
 * @tparam T Numeric type
 * @param x Value to square
 * @return x multiplied by itself (x*x)
 */
template<class T>
inline T sqr(const T& x)
{ return x*x; }

/**
 * @brief Find minimum of three values
 * @tparam T Comparable type
 * @param a1 First value
 * @param a2 Second value
 * @param a3 Third value
 * @return Smallest of the three values
 */
template<class T>
inline T min(T a1, T a2, T a3)
{ return min(a1, min(a2, a3)); }

/**
 * @brief Find maximum of three values
 * @tparam T Comparable type
 * @param a1 First value
 * @param a2 Second value
 * @param a3 Third value
 * @return Largest of the three values
 */
template<class T>
inline T max(T a1, T a2, T a3)
{ return max(a1, max(a2, a3)); }

/**
 * @brief Update min/max range to include new value
 *
 * Expands the range [amin, amax] to include value a1 if necessary.
 *
 * @tparam T Comparable type
 * @param a1 Value to include in range
 * @param amin Minimum of range (updated if a1 < amin)
 * @param amax Maximum of range (updated if a1 > amax)
 */
template<class T>
inline void update_minmax(T a1, T& amin, T& amax)
{
   if(a1<amin) amin=a1;
   else if(a1>amax) amax=a1;
}

/**
 * @brief Clamp value to specified range
 *
 * Returns value constrained to [lower, upper] range.
 *
 * @tparam T Comparable type
 * @param a Value to clamp
 * @param lower Lower bound of range
 * @param upper Upper bound of range
 * @return a if lower <= a <= upper, lower if a < lower, upper if a > upper
 */
template<class T>
inline T clamp(T a, T lower, T upper)
{
   if(a<lower) return lower;
   else if(a>upper) return upper;
   else return a;
}

/** @} */
