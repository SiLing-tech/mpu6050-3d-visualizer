#pragma once

#include "SensorData.h"

class AttitudeEstimator {
    float q0_ = 1.0f, q1_ = 0.0f, q2_ = 0.0f, q3_ = 0.0f;
    float beta_ = 0.1f;

public:
    explicit AttitudeEstimator(float beta = 0.1f);
    void update(const SensorData &data, float dt);
    EulerAngles eulerAngles() const;
    void reset();
};
