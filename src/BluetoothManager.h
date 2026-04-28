#pragma once

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QVector>

// Nordic UART Service (NUS) UUIDs
static const QBluetoothUuid NUS_SERVICE_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static const QBluetoothUuid NUS_TX_UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");     // PC -> ESP32 (write)
static const QBluetoothUuid NUS_RX_UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");     // ESP32 -> PC (notify)

class BluetoothManager : public QObject {
    Q_OBJECT

    QBluetoothDeviceDiscoveryAgent *discovery_;
    QLowEnergyController *controller_ = nullptr;
    QLowEnergyService *uartService_ = nullptr;
    QLowEnergyCharacteristic txChar_;   // write to ESP32
    QLowEnergyCharacteristic rxChar_;   // notify from ESP32
    QBluetoothDeviceInfo targetDevice_;
    QVector<QBluetoothDeviceInfo> devices_;
    QByteArray buffer_;

public:
    explicit BluetoothManager(QObject *parent = nullptr);
    ~BluetoothManager() override;

    void startScan();
    void stopScan();
    void connectToDevice(int deviceIndex);
    void disconnect();

    const QVector<QBluetoothDeviceInfo> &discoveredDevices() const { return devices_; }
    bool isConnected() const;

signals:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void scanFinished();
    void connected();
    void disconnected();
    void newLineParsed(const QString &line);
    void errorOccurred(const QString &message);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);

    // BLE connection flow
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid &serviceUuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &c, const QByteArray &value);
};
