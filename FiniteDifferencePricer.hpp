// FiniteDifferencePricer.hpp
//
// Prices a European option by solving the Black-Scholes PDE on a grid.
//
// In time to maturity tau the PDE is
//
//     V_tau = 1/2 sig^2 S^2 V_SS + b S V_S - r V
//
// solved on S in [0, Smax] from the payoff at tau = 0 up to tau = T. The
// scheme is explicit Euler: centred differences in S, a forward step in
// tau. Each step is a direct update, with no linear system to solve.
//
// The price of that simplicity is conditional stability. The step in time
// must satisfy
//
//     k <= h^2 / (sig^2 Smax^2)
//
// or errors are amplified at every step and the solution diverges. This
// class checks the condition and throws rather than returning a number
// that is meaningless - a diverged explicit scheme otherwise produces
// values like 1e+25 with no warning at all. MinimumStableSteps() gives the
// smallest N that satisfies it. The bound is sufficient rather than
// necessary, so it is slightly conservative.
//
// The value at an arbitrary S is read by linear interpolation between the
// two neighbouring grid points, so S need not land exactly on the mesh.
//
// Smax must be far enough out that the boundary condition there is
// accurate. Five times the strike is enough for short maturities; long
// ones need more, because over a long life the underlying has a real
// chance of reaching a truncated boundary.

#ifndef FINITEDIFFERENCEPRICER_HPP
#define FINITEDIFFERENCEPRICER_HPP

#include "OptionData.hpp"
#include "OptionException.hpp"

#include <vector>

class FiniteDifferencePricer
{
private:
    long m_spaceSteps;     // J
    long m_timeSteps;      // N
    double m_smaxFactor;   // Smax = factor * K

public:
    FiniteDifferencePricer(long spaceSteps, long timeSteps,
                           double smaxFactor = 5.0);

    // The smallest N that keeps the scheme stable for this option and
    // this J. Choose N at least this large.
    long MinimumStableSteps(const OptionData& data) const;

    // The price at data.S, interpolated from the grid.
    double Price(const OptionData& data) const;

    // The whole solution curve, for plotting or inspection.
    void Solve(const OptionData& data,
               std::vector<double>& S,
               std::vector<double>& V) const;
};

#endif
