#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QToolBar>

#include "TcpManager.h"
#include "ChartPanel.h"
#include "Model3DWidget.h"
#include "AttitudeEstimator.h"
#include "SensorData.h"
#include "SensorDataParser.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    TcpManager *tcp_;

    // WiFi toolbar widgets
    QLineEdit *wifiIp_;
    QLineEdit *wifiPort_;
    QPushButton *wifiConnectBtn_;

    QLabel *statusLabel_;

    AttitudeEstimator estimator_{0.1f};

    // Content
    Model3DWidget *model3D_;
    ChartPanel *chartPanel_;
    QPlainTextEdit *rawLog_;

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // WiFi
    void onWifiConnectClicked();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &msg);

    // Data
    void onNewLine(const QString &line);

private:
    void setupUi();
    void setupToolbar();
};
