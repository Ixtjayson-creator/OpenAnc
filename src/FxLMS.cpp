#include "FxLMS.h"
#include <cstring>
#include <algorithm>

FxLMS::FxLMS() {}
FxLMS::~FxLMS() {}

void FxLMS::Init(size_t filterLength, float stepSize, float leakage) {
    length_ = filterLength;
    mu_ = stepSize;
    leakage_ = leakage;
    weights_.assign(length_, 0.0f);
    buffer_.assign(length_, 0.0f);
    bufPos_ = 0;
    intensity_ = 1.0f;
}

void FxLMS::SetIntensity(float intensity) {
    intensity_ = std::clamp(intensity, 0.0f, 1.0f);
}

float FxLMS::Dot(const float* a, const float* b, size_t n) const {
    // Use AVX if available (assuming AVX2). Fallback scalar.
    size_t i = 0;
    __m256 sum = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 av = _mm256_loadu_ps(a + i);
        __m256 bv = _mm256_loadu_ps(b + i);
        sum = _mm256_fmadd_ps(av, bv, sum);
    }
    float result[8];
    _mm256_storeu_ps(result, sum);
    float dot = result[0] + result[1] + result[2] + result[3] +
                result[4] + result[5] + result[6] + result[7];
    // tail scalar
    for (; i < n; ++i) dot += a[i] * b[i];
    return dot;
}

std::vector<float> FxLMS::Process(const float* reference, size_t count) {
    std::vector<float> output(count);
    for (size_t n = 0; n < count; ++n) {
        // update circular buffer with newest reference sample
        buffer_[bufPos_] = reference[n];
        // compute filter output y = w^T * x (dot product of weights and buffer in reverse order)
        // Build temporary pointer to buffer data starting at current pos and wrapping.
        std::vector<float> x(length_);
        for (size_t i = 0; i < length_; ++i) {
            size_t idx = (bufPos_ + length_ - i) % length_;
            x[i] = buffer_[idx];
        }
        float y = Dot(weights_.data(), x.data(), length_);
        // anti‑noise = -y (destructive interference). Apply intensity scaling.
        float anti = -y * intensity_;
        output[n] = anti;
        // placeholder adaptation: treat reference as error (no feedback loop) – simple NLMS update
        // e = reference[n] - anti (desired - output)
        float e = reference[n] - anti;
        // update weights: w = w + mu * e * x  (with leakage)
        for (size_t i = 0; i < length_; ++i) {
            weights_[i] = (1.0f - leakage_) * weights_[i] + mu_ * e * x[i];
        }
        // advance circular buffer position
        bufPos_ = (bufPos_ + 1) % length_;
    }
    return output;
}

