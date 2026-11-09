// MeshGenerator.cpp
//
// Implementation of the mesh and matrix generators.

#include "MeshGenerator.hpp"

#include <cmath>

std::vector<double> CreateMesh(std::size_t n, double start, double end)
{
    if (n == 0)
    {
        throw InvalidParameterException("n", 0.0,
            "a mesh needs at least one point");
    }

    std::vector<double> mesh(n);

    if (n == 1)
    {
        mesh[0] = start;
        return mesh;
    }

    double h = (end - start) / static_cast<double>(n - 1);

    for (std::size_t j = 0; j < n; ++j)
    {
        mesh[j] = start + static_cast<double>(j) * h;
    }

    // Set the endpoint outright. Computed from j it would be within
    // rounding of end rather than equal to it, and a mesh whose last point
    // is 49.999999999 is awkward to check against by eye.
    mesh[n - 1] = end;

    return mesh;
}

std::vector<double> CreateMeshByStep(double start, double end, double h)
{
    if (h <= 0.0)
    {
        throw InvalidParameterException("h", h, "mesh size must be positive");
    }

    if (end < start)
    {
        throw InvalidParameterException("end", end,
            "end must not be less than start");
    }

    // +1 because both endpoints are included: 10 to 50 with h = 1 is 41
    // points, not 40.
    std::size_t n = static_cast<std::size_t>((end - start) / h) + 1;

    return CreateMesh(n, start, start + static_cast<double>(n - 1) * h);
}

std::vector<OptionData> CreateMatrix(const OptionData& base,
    const std::vector<double>& values,
    Parameter parameter)
{
    std::vector<OptionData> matrix(values.size());

    for (std::size_t j = 0; j < values.size(); ++j)
    {
        // Start from a copy, so every parameter the caller set is carried
        // over and only the chosen one moves.
        OptionData row = base;

        switch (parameter)
        {
        case Parameter::S:     row.S = values[j];   break;
        case Parameter::K:     row.K = values[j];   break;
        case Parameter::T:     row.T = values[j];   break;
        case Parameter::Sigma: row.sig = values[j]; break;
        case Parameter::R:
            // r and b move together for a stock option, which is the
            // case b = r. Leaving b behind would silently turn the
            // option into a different model.
            row.r = values[j];
            row.b = values[j];
            break;
        case Parameter::B:     row.b = values[j];   break;
        }

        matrix[j] = row;
    }

    return matrix;
}

std::vector<OptionData> CreateMatrix(const OptionData& base,
    double start, double end, double h,
    Parameter parameter)
{
    return CreateMatrix(base, CreateMeshByStep(start, end, h), parameter);
}