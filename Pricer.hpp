// Pricer.hpp
//
// Computing a vector of results from a vector of option parameters.
//
// The flexibility here comes from not fixing what is computed:
// Compute() takes a function and applies it to each row, so the same
// function serves prices, deltas, gammas and perpetual American values
// without a line changing. Writing a PriceMatrix, a DeltaMatrix and a
// GammaMatrix instead would be the same loop three times over, and a
// fourth would be needed for every quantity added later.
//
// Everything is a template, so it lives in the header.

#ifndef PRICER_HPP
#define PRICER_HPP

#include "OptionData.hpp"
#include "OptionException.hpp"

#include <vector>

namespace Pricer
{
    // Apply f to every row of a matrix of option parameters.
    //
    // F is deduced, so f may be a function, a function pointer, a lambda
    // or a function object - anything callable with an OptionData that
    // returns a double.
    template <typename F>
    std::vector<double> Compute(const std::vector<OptionData>& matrix, F f)
    {
        std::vector<double> results(matrix.size());

        for (std::size_t j = 0; j < matrix.size(); ++j)
        {
            results[j] = f(matrix[j]);
        }

        return results;
    }

    // The same for a function returning something other than a double -
    // a bool from a parity check, say. R is given explicitly because it
    // cannot be deduced from the arguments.
    template <typename R, typename F>
    std::vector<R> ComputeAs(const std::vector<OptionData>& matrix, F f)
    {
        std::vector<R> results(matrix.size());

        for (std::size_t j = 0; j < matrix.size(); ++j)
        {
            results[j] = f(matrix[j]);
        }

        return results;
    }

    // Price a matrix with a given option class. The template parameter is
    // the class to build from each row, so Price<EuropeanOption>(m)
    // and Price<PerpetualAmericanOption>(m) both work and neither
    // needed any code of its own.
    template <typename OptionClass>
    std::vector<double> Price(const std::vector<OptionData>& matrix)
    {
        return Compute(matrix,
            [](const OptionData& d)
            {
                return OptionClass(d).Price();
            });
    }

    // Element-by-element difference of two result vectors, for comparing
    // one method against another - an exact greek against a divided
    // difference, say.
    inline std::vector<double> Difference(const std::vector<double>& left,
        const std::vector<double>& right)
    {
        if (left.size() != right.size())
        {
            throw SizeMismatchException(left.size(), right.size());
        }

        std::vector<double> result(left.size());

        for (std::size_t j = 0; j < left.size(); ++j)
        {
            result[j] = left[j] - right[j];
        }

        return result;
    }

    // Largest absolute difference between two result vectors. A single
    // number for how far apart two methods are over a whole range, which
    // is what the accuracy comparisons in the test program need.
    inline double MaxAbsoluteDifference(const std::vector<double>& left,
        const std::vector<double>& right)
    {
        if (left.size() != right.size())
        {
            throw SizeMismatchException(left.size(), right.size());
        }

        double worst = 0.0;

        for (std::size_t j = 0; j < left.size(); ++j)
        {
            double gap = left[j] - right[j];

            if (gap < 0.0)
            {
                gap = -gap;
            }

            if (gap > worst)
            {
                worst = gap;
            }
        }

        return worst;
    }
}

#endif