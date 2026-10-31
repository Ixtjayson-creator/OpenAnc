#pragma once
#include <vector>
#include <algorithm>

class Limiter {
public:
    explicit Limiter(float maxLevel = 0.9f) : maxLevel_(maxLevel) {}
    void setMaxLevel(float level) { maxLevel_ = std::clamp(level, 0.0f, 1.0f); }
    void process(std::vector<float>& data) {
        for (float &v : data) {
            if (v > maxLevel_) v = maxLevel_;
            else if (v < -maxLevel_) v = -maxLevel_;
        }
    }
private:
    float maxLevel_;
};

