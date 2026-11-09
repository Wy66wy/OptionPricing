// FiniteDifferencePricer.cpp

#include "FiniteDifferencePricer.hpp"

#include <cmath>
#include <algorithm>
#include <string>

FiniteDifferencePricer::FiniteDifferencePricer(long spaceSteps,
                                               long timeSteps,
                                               double smaxFactor)
    : m_spaceSteps(spaceSteps), m_timeSteps(timeSteps),
      m_smaxFactor(smaxFactor)
{
    if (spaceSteps < 2)
    {
        throw InvalidParameterException("spaceSteps",
            static_cast<double>(spaceSteps), "need at least two space steps");
    }

    if (timeSteps < 1)
    {
        throw InvalidParameterException("timeSteps",
            static_cast<double>(timeSteps), "need at least one time step");
    }

    if (smaxFactor <= 1.0)
    {
        throw InvalidParameterException("smaxFactor", smaxFactor,
            "Smax must lie above the strike");
    }
}

long FiniteDifferencePricer::MinimumStableSteps(const OptionData& d) const
{
    const double Smax = m_smaxFactor * d.K;
    const double h = Smax / static_cast<double>(m_spaceSteps);

    // N >= T sig^2 Smax^2 / h^2, rounded up
    double n = d.T * d.sig * d.sig * Smax * Smax / (h * h);

    return static_cast<long>(std::ceil(n));
}

void FiniteDifferencePricer::Solve(const OptionData& d,
                                   std::vector<double>& S,
                                   std::vector<double>& V) const
{
    long minN = MinimumStableSteps(d);

    if (m_timeSteps < minN)
    {
        throw InvalidParameterException("timeSteps",
            static_cast<double>(m_timeSteps),
            "unstable: explicit Euler needs at least "
            + std::to_string(minN) + " time steps for this grid");
    }

    const long J = m_spaceSteps;
    const double Smax = m_smaxFactor * d.K;
    const double h = Smax / static_cast<double>(J);
    const double k = d.T / static_cast<double>(m_timeSteps);
    const bool isCall = (d.type == OptionType::Call);

    S.assign(J + 1, 0.0);
    for (long j = 0; j <= J; ++j)
    {
        S[j] = static_cast<double>(j) * h;
    }
    S[J] = Smax;    // exact, not accumulated

    // Initial condition: the payoff at expiry.
    V.assign(J + 1, 0.0);
    for (long j = 0; j <= J; ++j)
    {
        V[j] = isCall ? std::max(S[j] - d.K, 0.0) : std::max(d.K - S[j], 0.0);
    }

    // The coefficients depend only on j, not on time, so they are built
    // once. Writing the update as V_new = a V(j-1) + c V(j) + e V(j+1)
    // makes the stability condition visible: c is the coefficient that
    // turns negative when k is too large.
    std::vector<double> a(J + 1), c(J + 1), e(J + 1);
    for (long j = 1; j < J; ++j)
    {
        double diff = 0.5 * d.sig * d.sig * S[j] * S[j] / (h * h);
        double conv = d.b * S[j] / (2.0 * h);

        a[j] = k * (diff - conv);
        c[j] = 1.0 - k * (2.0 * diff + d.r);
        e[j] = k * (diff + conv);
    }

    std::vector<double> next(J + 1);

    for (long n = 1; n <= m_timeSteps; ++n)
    {
        const double tau = static_cast<double>(n) * k;

        for (long j = 1; j < J; ++j)
        {
            next[j] = a[j] * V[j - 1] + c[j] * V[j] + e[j] * V[j + 1];
        }

        // Boundary conditions at the new time level.
        if (isCall)
        {
            next[0] = 0.0;
            next[J] = Smax * std::exp((d.b - d.r) * tau)
                    - d.K * std::exp(-d.r * tau);
        }
        else
        {
            next[0] = d.K * std::exp(-d.r * tau);
            next[J] = 0.0;
        }

        V.swap(next);
    }
}

double FiniteDifferencePricer::Price(const OptionData& d) const
{
    std::vector<double> S, V;
    Solve(d, S, V);

    const double Smax = S.back();

    if (d.S < 0.0 || d.S > Smax)
    {
        throw InvalidParameterException("S", d.S,
            "outside the grid [0, Smax]; increase smaxFactor");
    }

    // Linear interpolation between the two grid points around S.
    const double h = S[1] - S[0];
    long j = static_cast<long>(d.S / h);

    if (j >= static_cast<long>(S.size()) - 1)
    {
        return V.back();
    }

    double w = (d.S - S[j]) / h;

    return (1.0 - w) * V[j] + w * V[j + 1];
}
