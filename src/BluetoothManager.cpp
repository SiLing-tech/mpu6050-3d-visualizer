#include "BluetoothManager.h"
#include <QDebug>

BluetoothManager::BluetoothManager(QObject *parent)
    : QObject(parent)
    , discovery_(new QBluetoothDeviceDiscoveryAgent(this))
    , socket_(new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this))
{
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothManager::onDeviceDiscovered);
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothManager::scanFinished);

    connect(socket_, &QBluetoothSocket::connected,
            this, &BluetoothManager::onSocketConnected);
    connect(socket_, &QBluetoothSocket::disconnected,
            this, &BluetoothManager::onSocketDisconnected);
    connect(socket_, &QBluetoothSocket::readyRead,
            this, &BluetoothManager::onReadyRead);
    connect(socket_, &QBluetoothSocket::errorOccurred,
            this, &BluetoothManager::onSocketErrorOccurred);
}

BluetoothManager::~BluetoothManager()
{
    disconnect();
}

void BluetoothManager::startScan()
{
    devices_.clear();
    discovery_->start();
}

void BluetoothManager::stopScan()
{
    discovery_->stop();
}

void BluetoothManager::connectToDevice(int deviceIndex)
{
    if (deviceIndex < 0 || deviceIndex >= devices_.size()) {
        emit errorOccurred("Invalid device index");
        return;
    }

    const QBluetoothDeviceInfo &info = devices_[deviceIndex];
    socket_->connectToService(info.address(), QBluetoothServiceInfo::RfcommProtocol);
}

void BluetoothManager::disconnect()
{
    if (socket_->state() == QBluetoothSocket::SocketState::ConnectedState)
        socket_->disconnectFromService();
}

bool BluetoothManager::isConnected() const
{
    return socket_->state() == QBluetoothSocket::SocketState::ConnectedState;
}

void BluetoothManager::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (info.name().isEmpty())
        return;
    devices_.append(info);
    emit deviceDiscovered(info);
}

void BluetoothManager::onSocketConnected()
{
    buffer_.clear();
    emit connected();
}

void BluetoothManager::onSocketDisconnected()
{
    emit disconnected();
}

void BluetoothManager::onReadyRead()
{
    buffer_.append(socket_->readAll());

    // Extract complete lines (newline-delimited)
    while (true) {
        int idx = buffer_.indexOf('\n');
        if (idx < 0)
            break;

        QByteArray line = buffer_.left(idx).trimmed();
        buffer_.remove(0, idx + 1);

        if (!line.isEmpty())
            emit newLineParsed(QString::fromLatin1(line));
    }
}

void BluetoothManager::onSocketErrorOccurred(QBluetoothSocket::SocketError error)
{
    Q_UNUSED(error)
    emit errorOccurred(socket_->errorString());
}
