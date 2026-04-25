#pragma once

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QBluetoothDeviceInfo>
#include <QVector>

class BluetoothManager : public QObject {
    Q_OBJECT

    QBluetoothDeviceDiscoveryAgent *discovery_;
    QBluetoothSocket *socket_;
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
    void onSocketConnected();
    void onSocketDisconnected();
    void onReadyRead();
    void onSocketErrorOccurred(QBluetoothSocket::SocketError error);
};
