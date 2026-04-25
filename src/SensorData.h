#pragma once

struct SensorData {
    float accelX = 0.0f;
    float accelY = 0.0f;
    float accelZ = 0.0f;
    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;
    float temp = 0.0f;
    bool valid = false;
};

struct EulerAngles {
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
};
