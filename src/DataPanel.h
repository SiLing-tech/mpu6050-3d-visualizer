#pragma once

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include "SensorData.h"

class DataPanel : public QGroupBox {
    Q_OBJECT

    QLabel *accelX_, *accelY_, *accelZ_;
    QLabel *gyroX_, *gyroY_, *gyroZ_;
    QLabel *temp_;
    QLabel *roll_, *pitch_, *yaw_;

public:
    explicit DataPanel(QWidget *parent = nullptr);
    void updateData(const SensorData &data, const EulerAngles &angles);
};
