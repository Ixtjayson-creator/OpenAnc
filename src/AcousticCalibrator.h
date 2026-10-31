#pragma once
#include <vector>
#include <cmath>
#include <cstddef>

class AcousticCalibrator {
public:
    static std::vector<float> BuildSweep(float sampleRate,
                                         float startFreq = 20.0f,
                                         float endFreq   = 20000.0f,
                                         float duration  = 3.0f)
    {
        size_t N = static_cast<size_t>(sampleRate * duration);
        std::vector<float> sweep(N);
        const double pi = 3.14159265358979323846;
        const double K = std::log(endFreq / startFreq) / duration;
        const double L = duration / std::log(endFreq / startFreq);

        for (size_t n = 0; n < N; ++n) {
            double t = static_cast<double>(n) / sampleRate;
            double phase = 2.0 * pi * startFreq * L * (std::exp(K * t) - 1.0);
            sweep[n] = static_cast<float>(std::sin(phase));
        }
        size_t fadeSamples = static_cast<size_t>(0.01 * sampleRate);
        for (size_t i = 0; i < fadeSamples && i < N; ++i) {
            float f = static_cast<float>(i) / fadeSamples;
            sweep[i] *= f;
            sweep[N - 1 - i] *= f;
        }
        return sweep;
    }

    static std::vector<float> CrossCorrelate(const std::vector<float>& a,
                                             const std::vector<float>& b)
    {
        size_t Na = a.size();
        size_t Nb = b.size();
        if (Na == 0 || Nb == 0) return {};
        size_t N = Na + Nb - 1;
        std::vector<float> corr(N, 0.0f);

        for (size_t i = 0; i < N; ++i) {
            float sum = 0.0f;
            size_t kmin = (i >= Nb - 1) ? i - (Nb - 1) : 0;
            size_t kmax = (i < Na - 1) ? i : Na - 1;
            for (size_t k = kmin; k <= kmax; ++k) {
                sum += a[k] * b[i - k];
            }
            corr[i] = sum;
        }
        return corr;
    }
};

