#include "ChartPanel.h"
#include <QDateTime>
#include <QtCharts/QChartGlobal>

ChartPanel::ChartPanel(QWidget *parent)
    : QWidget(parent)
{
    // --- Accelerometer chart ---
    accelX_ = new QLineSeries;
    accelX_->setName("Accel X");
    accelY_ = new QLineSeries;
    accelY_->setName("Accel Y");
    accelZ_ = new QLineSeries;
    accelZ_->setName("Accel Z");

    accelChart_ = new QChart;
    accelChart_->addSeries(accelX_);
    accelChart_->addSeries(accelY_);
    accelChart_->addSeries(accelZ_);
    accelChart_->setTitle("Acceleration (m/s²)");
    accelChart_->legend()->setVisible(true);

    accelAxisX_ = new QDateTimeAxis;
    accelAxisX_->setFormat("mm:ss");
    accelAxisX_->setTitleText("Time");
    accelChart_->addAxis(accelAxisX_, Qt::AlignBottom);

    accelAxisY_ = new QValueAxis;
    accelAxisY_->setRange(-20, 20);
    accelAxisY_->setTitleText("m/s²");
    accelChart_->addAxis(accelAxisY_, Qt::AlignLeft);

    accelX_->attachAxis(accelAxisX_);
    accelX_->attachAxis(accelAxisY_);
    accelY_->attachAxis(accelAxisX_);
    accelY_->attachAxis(accelAxisY_);
    accelZ_->attachAxis(accelAxisX_);
    accelZ_->attachAxis(accelAxisY_);

    accelView_ = new QChartView(accelChart_);
    accelView_->setRenderHint(QPainter::Antialiasing);

    // --- Gyroscope chart ---
    gyroX_ = new QLineSeries;
    gyroX_->setName("Gyro X");
    gyroY_ = new QLineSeries;
    gyroY_->setName("Gyro Y");
    gyroZ_ = new QLineSeries;
    gyroZ_->setName("Gyro Z");

    gyroChart_ = new QChart;
    gyroChart_->addSeries(gyroX_);
    gyroChart_->addSeries(gyroY_);
    gyroChart_->addSeries(gyroZ_);
    gyroChart_->setTitle("Gyroscope (rad/s)");
    gyroChart_->legend()->setVisible(true);

    gyroAxisX_ = new QDateTimeAxis;
    gyroAxisX_->setFormat("mm:ss");
    gyroAxisX_->setTitleText("Time");
    gyroChart_->addAxis(gyroAxisX_, Qt::AlignBottom);

    gyroAxisY_ = new QValueAxis;
    gyroAxisY_->setRange(-10, 10);
    gyroAxisY_->setTitleText("rad/s");
    gyroChart_->addAxis(gyroAxisY_, Qt::AlignLeft);

    gyroX_->attachAxis(gyroAxisX_);
    gyroX_->attachAxis(gyroAxisY_);
    gyroY_->attachAxis(gyroAxisX_);
    gyroY_->attachAxis(gyroAxisY_);
    gyroZ_->attachAxis(gyroAxisX_);
    gyroZ_->attachAxis(gyroAxisY_);

    gyroView_ = new QChartView(gyroChart_);
    gyroView_->setRenderHint(QPainter::Antialiasing);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(accelView_);
    layout->addWidget(gyroView_);
}

void ChartPanel::addDataPoint(const SensorData &data)
{
    qreal timestamp = QDateTime::currentMSecsSinceEpoch();

    bufAccelX_.push(data.accelX);
    bufAccelY_.push(data.accelY);
    bufAccelZ_.push(data.accelZ);
    bufGyroX_.push(data.gyroX);
    bufGyroY_.push(data.gyroY);
    bufGyroZ_.push(data.gyroZ);
    bufTime_.push(timestamp);

    const int n = bufAccelX_.size();

    accelX_->clear();
    accelY_->clear();
    accelZ_->clear();
    gyroX_->clear();
    gyroY_->clear();
    gyroZ_->clear();

    for (int i = 0; i < n; ++i) {
        qreal t = bufTime_[i];
        accelX_->append(t, bufAccelX_[i]);
        accelY_->append(t, bufAccelY_[i]);
        accelZ_->append(t, bufAccelZ_[i]);
        gyroX_->append(t, bufGyroX_[i]);
        gyroY_->append(t, bufGyroY_[i]);
        gyroZ_->append(t, bufGyroZ_[i]);
    }

    QDateTime start = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(bufTime_[0]));
    QDateTime end = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestamp));
    accelAxisX_->setRange(start, end);
    gyroAxisX_->setRange(start, end);

    pointCount_++;
}
