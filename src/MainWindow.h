#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QToolBar>

#include "BluetoothManager.h"
#include "TcpManager.h"
#include "Model3DWidget.h"
#include "DataPanel.h"
#include "ChartPanel.h"
#include "AttitudeEstimator.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    BluetoothManager *bt_;
    TcpManager *tcp_;
    AttitudeEstimator estimator_;

    // Bluetooth toolbar widgets
    QPushButton *scanBtn_;
    QComboBox *deviceCombo_;
    QPushButton *btConnectBtn_;

    // WiFi toolbar widgets
    QLineEdit *wifiIp_;
    QLineEdit *wifiPort_;
    QPushButton *wifiConnectBtn_;

    QLabel *statusLabel_;

    // Content panels
    Model3DWidget *model3D_;
    DataPanel *dataPanel_;
    ChartPanel *chartPanel_;

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // Bluetooth
    void onScanClicked();
    void onBtConnectClicked();
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanFinished();
    void onBtConnected();
    void onBtDisconnected();
    void onBtError(const QString &msg);

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
    void disconnectAll();
};
