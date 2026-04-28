#include "BluetoothManager.h"
#include <QDebug>

BluetoothManager::BluetoothManager(QObject *parent)
    : QObject(parent)
    , discovery_(new QBluetoothDeviceDiscoveryAgent(this))
    , socket_(new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this))
    , localDevice_(new QBluetoothLocalDevice(this))
{
    // Scan errors
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BluetoothManager::onScanError);

    // Device discovery
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothManager::onDeviceDiscovered);
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothManager::scanFinished);

    // Socket events
    connect(socket_, &QBluetoothSocket::connected,
            this, &BluetoothManager::onSocketConnected);
    connect(socket_, &QBluetoothSocket::disconnected,
            this, &BluetoothManager::onSocketDisconnected);
    connect(socket_, &QBluetoothSocket::readyRead,
            this, &BluetoothManager::onReadyRead);
    connect(socket_, &QBluetoothSocket::errorOccurred,
            this, &BluetoothManager::onSocketErrorOccurred);

    // Pairing
    connect(localDevice_, &QBluetoothLocalDevice::pairingFinished,
            this, &BluetoothManager::onPairingFinished);
}

BluetoothManager::~BluetoothManager()
{
    disconnect();
}

void BluetoothManager::startScan()
{
    devices_.clear();
    discovery_->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
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

    // Ensure device is paired (Windows requires pairing for SPP)
    QBluetoothLocalDevice::Pairing pairing = localDevice_->pairingStatus(info.address());
    if (pairing != QBluetoothLocalDevice::Paired
        && pairing != QBluetoothLocalDevice::AuthorizedPaired) {
        qDebug() << "Requesting pairing with" << info.name();
        localDevice_->requestPairing(info.address(), QBluetoothLocalDevice::Paired);
        return;  // will retry in onPairingFinished
    }

    // Connect using standard SPP UUID
    static const QBluetoothUuid sppUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
    qDebug() << "Connecting to" << info.name() << "SPP UUID:" << sppUuid.toString();
    socket_->connectToService(info.address(), sppUuid);
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
    qDebug() << "BT device found:" << info.name()
             << info.address().toString()
             << "rssi:" << info.rssi();

    if (info.name().isEmpty())
        return;

    devices_.append(info);
    emit deviceDiscovered(info);
}

void BluetoothManager::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "BT scan error:" << error;
    emit errorOccurred(discovery_->errorString());
}

void BluetoothManager::onSocketConnected()
{
    qDebug() << "BT socket connected";
    buffer_.clear();
    emit connected();
}

void BluetoothManager::onSocketDisconnected()
{
    qDebug() << "BT socket disconnected";
    emit disconnected();
}

void BluetoothManager::onReadyRead()
{
    buffer_.append(socket_->readAll());

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
    qDebug() << "BT socket error:" << socket_->errorString();
    emit errorOccurred(socket_->errorString());
}

void BluetoothManager::onPairingFinished(QBluetoothAddress address, QBluetoothLocalDevice::Pairing pairing)
{
    qDebug() << "Pairing finished:" << address.toString()
             << "status:" << pairing;

    if (pairing == QBluetoothLocalDevice::Paired
        || pairing == QBluetoothLocalDevice::AuthorizedPaired) {
        // Retry connection now that device is paired
        for (int i = 0; i < devices_.size(); ++i) {
            if (devices_[i].address() == address) {
                static const QBluetoothUuid sppUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
                socket_->connectToService(address, sppUuid);
                return;
            }
        }
    } else {
        emit errorOccurred("Pairing failed: " + address.toString());
    }
}
