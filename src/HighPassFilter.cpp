#include "HighPassFilter.h"
#include <algorithm>

HighPassFilter::HighPassFilter(float sampleRate, float cutoffHz)
    : sampleRate_(sampleRate), cutoffHz_(cutoffHz) {
    updateCoeffs();
}

void HighPassFilter::setCutoff(float cutoffHz) {
    cutoffHz_ = cutoffHz;
    updateCoeffs();
}

void HighPassFilter::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateCoeffs();
}

void HighPassFilter::updateCoeffs() {
    // Simple first-order high‑pass (bilinear transform)
    const float pi = 3.14159265358979323846f;
    float RC = 1.0f / (2.0f * pi * cutoffHz_);
    float dt = 1.0f / sampleRate_;
    float alpha = RC / (RC + dt);
    a0_ = alpha;
    a1_ = -alpha;
    b1_ = alpha;
}

void HighPassFilter::process(std::vector<float>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        float input = data[i];
        float output = a0_ * input + a1_ * (i ? data[i-1] : 0.0f) + b1_ * z1_;
        z1_ = output;
        data[i] = output;
    }
}

