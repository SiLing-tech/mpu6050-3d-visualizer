#include "BluetoothManager.h"
#include <QDebug>

BluetoothManager::BluetoothManager(QObject *parent)
    : QObject(parent)
    , discovery_(new QBluetoothDeviceDiscoveryAgent(this))
    , scanTimer_(new QTimer(this))
    , connectTimer_(new QTimer(this))
{
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BluetoothManager::onScanError);
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothManager::onDeviceDiscovered);
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothManager::scanFinished);

    scanTimer_->setSingleShot(true);
    connect(scanTimer_, &QTimer::timeout, this, &BluetoothManager::onScanTimeout);

    connectTimer_->setSingleShot(true);
    connect(connectTimer_, &QTimer::timeout, this, &BluetoothManager::onConnectTimeout);
}

BluetoothManager::~BluetoothManager()
{
    QObject::disconnect();
    delete uartService_;
    uartService_ = nullptr;
    delete controller_;
    controller_ = nullptr;
}

void BluetoothManager::startScan()
{
    devices_.clear();
    scanTimer_->start(12000);
    discovery_->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    qDebug() << "BLE scan started";
}

void BluetoothManager::stopScan()
{
    scanTimer_->stop();
    discovery_->stop();
}

void BluetoothManager::connectToDevice(const QString &address)
{
    // Find device by address
    int idx = -1;
    for (int i = 0; i < devices_.size(); ++i) {
        if (devices_[i].address().toString() == address) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        emit errorOccurred("Device not found in scan results");
        return;
    }

    // Stop any ongoing scan
    stopScan();

    cleanupConnection();

    targetDevice_ = devices_[idx];
    qDebug() << "BLE connecting to" << targetDevice_.name() << targetDevice_.address().toString();

    controller_ = QLowEnergyController::createCentral(targetDevice_, this);
    connect(controller_, &QLowEnergyController::connected,
            this, &BluetoothManager::onControllerConnected);
    connect(controller_, &QLowEnergyController::disconnected,
            this, &BluetoothManager::onControllerDisconnected);
    connect(controller_, &QLowEnergyController::errorOccurred,
            this, &BluetoothManager::onControllerError);
    connect(controller_, &QLowEnergyController::serviceDiscovered,
            this, &BluetoothManager::onServiceDiscovered);

    connectTimer_->start(15000);
    controller_->connectToDevice();
}

void BluetoothManager::cleanupConnection()
{
    connectTimer_->stop();
    if (uartService_) {
        delete uartService_;
        uartService_ = nullptr;
    }
    if (controller_) {
        controller_->disconnect();       // break all signals — no callbacks during cleanup
        controller_->disconnectFromDevice();
        delete controller_;
        controller_ = nullptr;
    }
    txChar_ = QLowEnergyCharacteristic();
    rxChar_ = QLowEnergyCharacteristic();
}

void BluetoothManager::disconnect()
{
    connectTimer_->stop();
    if (uartService_) {
        delete uartService_;
        uartService_ = nullptr;
    }
    if (controller_) {
        controller_->disconnectFromDevice();
        // onControllerDisconnected() handles delete and emits disconnected()
    }
}

bool BluetoothManager::isConnected() const
{
    return controller_ && controller_->state() == QLowEnergyController::ConnectedState;
}

// --- Scan ---

void BluetoothManager::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (info.name().isEmpty())
        return;

    qDebug() << "BLE device found:" << info.name() << info.address().toString();

    for (const auto &d : devices_) {
        if (d.address() == info.address())
            return;
    }
    devices_.append(info);
    emit deviceDiscovered(info);
}

void BluetoothManager::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "BLE scan error:" << error << discovery_->errorString();
    scanTimer_->stop();
    emit scanFinished();
    emit errorOccurred(discovery_->errorString());
}

void BluetoothManager::onScanTimeout()
{
    qDebug() << "BLE scan timeout — stopping";
    discovery_->stop();
}

// --- BLE Connection ---

void BluetoothManager::onControllerConnected()
{
    qDebug() << "BLE controller connected, discovering services...";
    connectTimer_->stop();
    controller_->discoverServices();
}

void BluetoothManager::onControllerDisconnected()
{
    qDebug() << "BLE controller disconnected";
    connectTimer_->stop();
    delete uartService_;
    uartService_ = nullptr;
    delete controller_;
    controller_ = nullptr;
    emit disconnected();
}

void BluetoothManager::onControllerError(QLowEnergyController::Error error)
{
    qDebug() << "BLE controller error:" << error << controller_->errorString();
    connectTimer_->stop();
    emit errorOccurred(controller_->errorString());
}

void BluetoothManager::onConnectTimeout()
{
    qDebug() << "BLE connection timeout";
    cleanupConnection();
    emit errorOccurred("Connection timeout (15s)");
}

void BluetoothManager::onServiceDiscovered(const QBluetoothUuid &serviceUuid)
{
    qDebug() << "BLE service discovered:" << serviceUuid.toString();
    if (serviceUuid == NUS_SERVICE_UUID) {
        qDebug() << "Found NUS service";
        uartService_ = controller_->createServiceObject(serviceUuid, this);
        if (uartService_) {
            connect(uartService_, &QLowEnergyService::stateChanged,
                    this, &BluetoothManager::onServiceStateChanged);
            connect(uartService_, &QLowEnergyService::characteristicChanged,
                    this, &BluetoothManager::onCharacteristicChanged);
            connect(uartService_, &QLowEnergyService::characteristicWritten,
                    this, &BluetoothManager::onCharacteristicWritten);
            uartService_->discoverDetails();
        }
    }
}

void BluetoothManager::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    qDebug() << "NUS service state:" << state;
    if (state != QLowEnergyService::RemoteServiceDiscovered)
        return;

    const QList<QLowEnergyCharacteristic> chars = uartService_->characteristics();
    for (const auto &c : chars) {
        qDebug() << "  char:" << c.uuid().toString();
        if (c.uuid() == NUS_RX_UUID) {
            rxChar_ = c;
            QLowEnergyDescriptor cccd = c.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (cccd.isValid()) {
                uartService_->writeDescriptor(cccd, QByteArray::fromHex("0100"));
                qDebug() << "  RX notify enabled";
            }
        } else if (c.uuid() == NUS_TX_UUID) {
            txChar_ = c;
            qDebug() << "  TX char found";
        }
    }

    if (rxChar_.isValid()) {
        buffer_.clear();
        emit connected();
    } else {
        qDebug() << "  NUS RX characteristic not found!";
        emit errorOccurred("NUS RX characteristic not found");
    }
}

void BluetoothManager::onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    Q_UNUSED(c)
    buffer_.append(value);

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

void BluetoothManager::onCharacteristicWritten(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    Q_UNUSED(c)
    Q_UNUSED(value)
}
