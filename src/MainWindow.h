#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QToolBar>

#include "TcpManager.h"
#include "BluetoothManager.h"
#include "ChartPanel.h"
#include "Model3DWidget.h"
#include "AttitudeEstimator.h"
#include "SensorData.h"
#include "SensorDataParser.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    TcpManager *tcp_;
    BluetoothManager *bt_;

    // Mode toggle
    QPushButton *wifiModeBtn_;
    QPushButton *btModeBtn_;

    // WiFi toolbar widgets
    QWidget *wifiGroup_;
    QLineEdit *wifiIp_;
    QLineEdit *wifiPort_;
    QPushButton *wifiConnectBtn_;

    // BT toolbar widgets
    QWidget *btGroup_;
    QPushButton *scanBtn_;
    QComboBox *deviceCombo_;
    QPushButton *btConnectBtn_;

    bool wifiModeActive_ = false;

    AttitudeEstimator estimator_{0.1f};

    // Content
    Model3DWidget *model3D_;
    ChartPanel *chartPanel_;
    QPlainTextEdit *rawLog_;

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // Mode switching
    void onSwitchToWifi();
    void onSwitchToBt();

    // WiFi
    void onWifiConnectClicked();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &msg);

    // BT
    void onScanClicked();
    void onBtConnectClicked();
    void onBtConnected();
    void onBtDisconnected();
    void onBtError(const QString &msg);
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanFinished();

    // Data
    void onNewLine(const QString &line);

private:
    void setupUi();
    void setupToolbar();
    void disconnectAll();
};
