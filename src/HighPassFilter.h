#pragma once

#include <vector>
#include <cmath>

class HighPassFilter {
public:
    explicit HighPassFilter(float sampleRate = 48000.0f, float cutoffHz = 200.0f);
    void setCutoff(float cutoffHz);
    void setSampleRate(float sampleRate);
    // Process a block of samples in‑place
    void process(std::vector<float>& data);
private:
    float a0_{0.0f}, a1_{0.0f}, b1_{0.0f};
    float z1_{0.0f}; // previous output
    void updateCoeffs();
    float sampleRate_{48000.0f};
    float cutoffHz_{200.0f};
};

