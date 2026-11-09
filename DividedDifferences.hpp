// DividedDifferences.hpp
//
// Approximating the greeks by divided differences.
//
// An exact formula for a sensitivity is not always available, and where
// it is not, the derivative can be approximated from prices alone:
//
//     Delta  =  ( V(S+h) - V(S-h) ) / 2h
//     Gamma  =  ( V(S+h) - 2V(S) + V(S-h) ) / h^2
//
// Both are three-point second-order approximations, so halving h should
// quarter the error - until rounding takes over. The numerators subtract
// nearly equal numbers, and the smaller h is the more of their leading
// digits agree and cancel. Dividing what little remains by h or by h^2
// then magnifies whatever noise is left, so the error falls with h, turns
// and rises again. There is a best h, and it is not the smallest one.
//
// This is written against Option rather than against any particular
// option class, so it works for anything with a Price(): the two classes
// here, and any added later with or without exact greeks.

#ifndef DIVIDEDDIFFERENCES_HPP
#define DIVIDEDDIFFERENCES_HPP

#include "Option.hpp"
#include "OptionData.hpp"
#include "OptionException.hpp"

#include <vector>

class DividedDifferences
{
private:
    double m_h;            // The step in S

public:
    // The default is a compromise: large enough that the subtraction
    // keeps its significant digits, small enough that the approximation
    // is close. 0.1 is around the best value for options priced in the
    // tens, as the test cases here are.
    DividedDifferences(double h = 0.1);

    double H() const;
    void H(double newH);

    // Delta and Gamma of a given option, by the formulae above.
    //
    // The argument is a const Option& - a reference to the base class, so
    // the call is resolved at run time to whichever concrete option was
    // passed. Neither function knows or cares which it was given.
    double Delta(const Option& option) const;
    double Gamma(const Option& option) const;

    // The same for a whole matrix of parameters, so that approximated and
    // exact greeks can be compared over a range. OptionClass says which
    // option to build from each row.
    template <typename OptionClass>
    std::vector<double> Delta(const std::vector<OptionData>& matrix) const
    {
        std::vector<double> results(matrix.size());

        for (std::size_t j = 0; j < matrix.size(); ++j)
        {
            results[j] = Delta(OptionClass(matrix[j]));
        }

        return results;
    }

    template <typename OptionClass>
    std::vector<double> Gamma(const std::vector<OptionData>& matrix) const
    {
        std::vector<double> results(matrix.size());

        for (std::size_t j = 0; j < matrix.size(); ++j)
        {
            results[j] = Gamma(OptionClass(matrix[j]));
        }

        return results;
    }
};

#endif