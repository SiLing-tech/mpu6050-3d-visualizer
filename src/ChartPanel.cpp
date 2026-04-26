#include "ChartPanel.h"
#include <QDateTime>
#include <QOpenGLWidget>
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
    accelView_->setViewport(new QOpenGLWidget);

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
    gyroView_->setViewport(new QOpenGLWidget);

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

    // Throttle chart redraws to 10 Hz — data arrives at 20 Hz
    pointCount_++;
    if (pointCount_ % 2 != 0)
        return;

    const int n = bufAccelX_.size();

    // Build point lists from rolling buffers in one pass
    QList<QPointF> ptsAccelX, ptsAccelY, ptsAccelZ;
    QList<QPointF> ptsGyroX, ptsGyroY, ptsGyroZ;
    ptsAccelX.reserve(n);
    ptsAccelY.reserve(n);
    ptsAccelZ.reserve(n);
    ptsGyroX.reserve(n);
    ptsGyroY.reserve(n);
    ptsGyroZ.reserve(n);

    for (int i = 0; i < n; ++i) {
        qreal t = bufTime_[i];
        ptsAccelX.append(QPointF(t, bufAccelX_[i]));
        ptsAccelY.append(QPointF(t, bufAccelY_[i]));
        ptsAccelZ.append(QPointF(t, bufAccelZ_[i]));
        ptsGyroX.append(QPointF(t, bufGyroX_[i]));
        ptsGyroY.append(QPointF(t, bufGyroY_[i]));
        ptsGyroZ.append(QPointF(t, bufGyroZ_[i]));
    }

    // Replace all points at once — single signal emission per series
    accelX_->replace(ptsAccelX);
    accelY_->replace(ptsAccelY);
    accelZ_->replace(ptsAccelZ);
    gyroX_->replace(ptsGyroX);
    gyroY_->replace(ptsGyroY);
    gyroZ_->replace(ptsGyroZ);

    QDateTime start = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(bufTime_[0]));
    QDateTime end = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestamp));
    accelAxisX_->setRange(start, end);
    gyroAxisX_->setRange(start, end);
}
