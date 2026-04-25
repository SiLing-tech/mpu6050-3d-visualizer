#include "DataPanel.h"

DataPanel::DataPanel(QWidget *parent)
    : QGroupBox("Sensor Data", parent)
{
    auto *grid = new QGridLayout(this);

    int row = 0;
    grid->addWidget(new QLabel("Accel X:"), row, 0);
    accelX_ = new QLabel("--");
    grid->addWidget(accelX_, row, 1);
    grid->addWidget(new QLabel("m/s²"), row, 2);

    grid->addWidget(new QLabel("Accel Y:"), ++row, 0);
    accelY_ = new QLabel("--");
    grid->addWidget(accelY_, row, 1);
    grid->addWidget(new QLabel("m/s²"), row, 2);

    grid->addWidget(new QLabel("Accel Z:"), ++row, 0);
    accelZ_ = new QLabel("--");
    grid->addWidget(accelZ_, row, 1);
    grid->addWidget(new QLabel("m/s²"), row, 2);

    ++row;
    grid->addWidget(new QLabel("Gyro X:"), ++row, 0);
    gyroX_ = new QLabel("--");
    grid->addWidget(gyroX_, row, 1);
    grid->addWidget(new QLabel("rad/s"), row, 2);

    grid->addWidget(new QLabel("Gyro Y:"), ++row, 0);
    gyroY_ = new QLabel("--");
    grid->addWidget(gyroY_, row, 1);
    grid->addWidget(new QLabel("rad/s"), row, 2);

    grid->addWidget(new QLabel("Gyro Z:"), ++row, 0);
    gyroZ_ = new QLabel("--");
    grid->addWidget(gyroZ_, row, 1);
    grid->addWidget(new QLabel("rad/s"), row, 2);

    ++row;
    grid->addWidget(new QLabel("Temp:"), ++row, 0);
    temp_ = new QLabel("--");
    grid->addWidget(temp_, row, 1);
    grid->addWidget(new QLabel("°C"), row, 2);

    ++row;
    grid->addWidget(new QLabel("Roll:"), ++row, 0);
    roll_ = new QLabel("--");
    grid->addWidget(roll_, row, 1);
    grid->addWidget(new QLabel("°"), row, 2);

    grid->addWidget(new QLabel("Pitch:"), ++row, 0);
    pitch_ = new QLabel("--");
    grid->addWidget(pitch_, row, 1);
    grid->addWidget(new QLabel("°"), row, 2);

    grid->addWidget(new QLabel("Yaw:"), ++row, 0);
    yaw_ = new QLabel("--");
    grid->addWidget(yaw_, row, 1);
    grid->addWidget(new QLabel("°"), row, 2);
}

void DataPanel::updateData(const SensorData &data, const EulerAngles &angles)
{
    accelX_->setText(QString::number(data.accelX, 'f', 2));
    accelY_->setText(QString::number(data.accelY, 'f', 2));
    accelZ_->setText(QString::number(data.accelZ, 'f', 2));
    gyroX_->setText(QString::number(data.gyroX, 'f', 2));
    gyroY_->setText(QString::number(data.gyroY, 'f', 2));
    gyroZ_->setText(QString::number(data.gyroZ, 'f', 2));
    temp_->setText(QString::number(data.temp, 'f', 2));
    roll_->setText(QString::number(angles.roll, 'f', 2));
    pitch_->setText(QString::number(angles.pitch, 'f', 2));
    yaw_->setText(QString::number(angles.yaw, 'f', 2));
}
