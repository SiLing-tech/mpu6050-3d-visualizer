#pragma once

#include <QWidget>
#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QVBoxLayout>
#include <QtCharts/QChartGlobal>
#include "SensorData.h"
#include "RollingBuffer.h"

class ChartPanel : public QWidget {
    Q_OBJECT

    QChart *accelChart_;
    QChart *gyroChart_;
    QChartView *accelView_;
    QChartView *gyroView_;

    QLineSeries *accelX_;
    QLineSeries *accelY_;
    QLineSeries *accelZ_;
    QLineSeries *gyroX_;
    QLineSeries *gyroY_;
    QLineSeries *gyroZ_;

    QValueAxis *accelAxisY_;
    QValueAxis *gyroAxisY_;
    QDateTimeAxis *accelAxisX_;
    QDateTimeAxis *gyroAxisX_;

    static constexpr int BufferSize = 500;
    RollingBuffer<float, BufferSize> bufAccelX_;
    RollingBuffer<float, BufferSize> bufAccelY_;
    RollingBuffer<float, BufferSize> bufAccelZ_;
    RollingBuffer<float, BufferSize> bufGyroX_;
    RollingBuffer<float, BufferSize> bufGyroY_;
    RollingBuffer<float, BufferSize> bufGyroZ_;
    RollingBuffer<qreal, BufferSize> bufTime_;

    int pointCount_ = 0;

public:
    explicit ChartPanel(QWidget *parent = nullptr);
    void addDataPoint(const SensorData &data);
};
