#pragma once
#include <vector>
#include <cstddef>
#include <immintrin.h>

class FxLMS {
public:
    FxLMS();
    ~FxLMS();
    // Initialize filter length (power of two recommended) and step size (mu)
    void Init(size_t filterLength, float stepSize = 0.01f, float leakage = 0.0f);
    // Process a block of samples. "reference" is the measured environmental noise,
    // "error" is the microphone signal after anti‑noise playback (for true adaptation).
    // For now we use reference as input and ignore error – placeholder adaptation.
    // Returns anti‑noise buffer (same length as reference).
    std::vector<float> Process(const float* reference, size_t count);
    // Simple intensity scaling (0.0‑1.0) applied to output.
    void SetIntensity(float intensity);
private:
    std::vector<float> weights_;   // adaptive filter coefficients
    std::vector<float> buffer_;    // circular buffer of past reference samples
    size_t bufPos_ = 0;
    size_t length_ = 0;
    float mu_ = 0.01f; // step size
    float leakage_ = 0.0f;
    float intensity_ = 1.0f;
    // Helper: dot product using SIMD if possible
    float Dot(const float* a, const float* b, size_t n) const;
};

