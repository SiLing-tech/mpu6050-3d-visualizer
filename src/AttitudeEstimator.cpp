#include "AttitudeEstimator.h"
#include <cmath>

AttitudeEstimator::AttitudeEstimator(float beta)
    : beta_(beta)
{
}

void AttitudeEstimator::update(const SensorData &data, float dt)
{
    if (!data.valid || dt <= 0.0f)
        return;

    float ax = data.accelX, ay = data.accelY, az = data.accelZ;
    float gx = data.gyroX, gy = data.gyroY, gz = data.gyroZ;

    // Normalize accelerometer
    float norm = std::sqrt(ax * ax + ay * ay + az * az);
    if (norm < 1e-6f) return;
    ax /= norm; ay /= norm; az /= norm;

    float q0 = q0_, q1 = q1_, q2 = q2_, q3 = q3_;

    // Objective function f = estimated gravity - measured acceleration
    //   f1 = 2*(q1*q3 - q0*q2) - ax
    //   f2 = 2*(q0*q1 + q2*q3) - ay
    //   f3 = 2*(0.5 - q1^2 - q2^2) - az
    float f1 = 2.0f * (q1 * q3 - q0 * q2) - ax;
    float f2 = 2.0f * (q0 * q1 + q2 * q3) - ay;
    float f3 = 2.0f * (0.5f - q1 * q1 - q2 * q2) - az;

    // Jacobian of f with respect to q
    float J11 = -2.0f * q2, J12 = 2.0f * q3,  J13 = -2.0f * q0, J14 = 2.0f * q1;
    float J21 = 2.0f * q1,  J22 = 2.0f * q0,  J23 = 2.0f * q3,   J24 = 2.0f * q2;
    float J31 = 0.0f,      J32 = -4.0f * q1, J33 = -4.0f * q2,  J34 = 0.0f;

    // gradient = J^T * f
    float grad0 = J11 * f1 + J21 * f2 + J31 * f3;
    float grad1 = J12 * f1 + J22 * f2 + J32 * f3;
    float grad2 = J13 * f1 + J23 * f2 + J33 * f3;
    float grad3 = J14 * f1 + J24 * f2 + J34 * f3;

    float gradNorm = std::sqrt(grad0 * grad0 + grad1 * grad1 + grad2 * grad2 + grad3 * grad3);
    if (gradNorm > 1e-6f) {
        grad0 /= gradNorm; grad1 /= gradNorm; grad2 /= gradNorm; grad3 /= gradNorm;
    }

    // Quaternion derivative: qDot = 0.5 * q * omega - beta * grad
    float qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz) - beta_ * grad0;
    float qDot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy) - beta_ * grad1;
    float qDot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx) - beta_ * grad2;
    float qDot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx) - beta_ * grad3;

    // Integrate
    q0 += qDot0 * dt;
    q1 += qDot1 * dt;
    q2 += qDot2 * dt;
    q3 += qDot3 * dt;

    // Normalize quaternion
    norm = std::sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (norm > 1e-6f) {
        q0 /= norm; q1 /= norm; q2 /= norm; q3 /= norm;
    }

    q0_ = q0; q1_ = q1; q2_ = q2; q3_ = q3;
}

EulerAngles AttitudeEstimator::eulerAngles() const
{
    EulerAngles ea;
    float q0 = q0_, q1 = q1_, q2 = q2_, q3 = q3_;

    float sinr_cosp = 2.0f * (q0 * q1 + q2 * q3);
    float cosr_cosp = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
    ea.roll = std::atan2(sinr_cosp, cosr_cosp) * 57.29578f;

    float sinp = 2.0f * (q0 * q2 - q3 * q1);
    if (std::abs(sinp) >= 1.0f)
        ea.pitch = std::copysign(1.5707963f, sinp) * 57.29578f;
    else
        ea.pitch = std::asin(sinp) * 57.29578f;

    float siny_cosp = 2.0f * (q0 * q3 + q1 * q2);
    float cosy_cosp = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
    ea.yaw = std::atan2(siny_cosp, cosy_cosp) * 57.29578f;

    return ea;
}

void AttitudeEstimator::reset()
{
    q0_ = 1.0f;
    q1_ = q2_ = q3_ = 0.0f;
}
