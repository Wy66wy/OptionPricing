// MeshGenerator.hpp
//
// Free functions that build arrays and matrices of option parameters.
//
// These are free functions rather than members of a class because they
// belong to no object: a mesh of doubles has nothing to do with options,
// and the matrix builders need only read an OptionData, not own one.
// Making them members would tie general-purpose code to a particular class
// for no gain.

#ifndef MESHGENERATOR_HPP
#define MESHGENERATOR_HPP

#include "OptionData.hpp"
#include "OptionException.hpp"

#include <vector>

// Which parameter a matrix of option data varies along. An enumeration
// rather than a string or an int: the compiler then rejects a typo, and
// the call site reads as what it means.
enum class Parameter
{
    S,                 // Underlying price
    K,                 // Strike
    T,                 // Expiry
    Sigma,             // Volatility
    R,                 // Interest rate
    B                  // Cost of carry
};

// A mesh of n points from start to end inclusive, separated by h.
//
// Every range in the test program is built with this function: the
// ranges of S for European prices, for the greeks, and for the perpetual
// American options.
//
// The last point is assigned rather than accumulated, because adding h
// n times accumulates rounding error and would leave the endpoint slightly
// off.
std::vector<double> CreateMesh(std::size_t n, double start, double end);

// The same mesh described by its step size instead of its point count:
// 10, 11, 12, ..., 50 with h = 1.
std::vector<double> CreateMeshByStep(double start, double end, double h);

// A matrix of option parameters: one row per mesh value, identical to the
// base option except in the chosen parameter.
//
// Returning a vector of OptionData rather than of raw numbers is what
// makes the pricer general. The pricer then needs to know nothing about
// which parameter varied, or indeed that anything varied at all - it is
// handed complete parameter sets and prices each one.
std::vector<OptionData> CreateMatrix(const OptionData& base,
    const std::vector<double>& values,
    Parameter parameter);

// The same, with the mesh built in one step.
std::vector<OptionData> CreateMatrix(const OptionData& base,
    double start, double end, double h,
    Parameter parameter);

#endif