#include "BluetoothManager.h"
#include <QDebug>

BluetoothManager::BluetoothManager(QObject *parent)
    : QObject(parent)
    , discovery_(new QBluetoothDeviceDiscoveryAgent(this))
{
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BluetoothManager::onScanError);
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothManager::onDeviceDiscovered);
    connect(discovery_, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothManager::scanFinished);
}

BluetoothManager::~BluetoothManager()
{
    QObject::disconnect();  // break all signal/slot connections before cleanup
    delete uartService_;
    uartService_ = nullptr;
    delete controller_;
    controller_ = nullptr;
}

void BluetoothManager::startScan()
{
    devices_.clear();
    discovery_->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
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

    // Clean up previous connection if any
    if (controller_) {
        controller_->disconnectFromDevice();
        delete controller_;
        controller_ = nullptr;
    }
    uartService_ = nullptr;
    txChar_ = QLowEnergyCharacteristic();
    rxChar_ = QLowEnergyCharacteristic();

    targetDevice_ = devices_[deviceIndex];
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

    controller_->connectToDevice();
}

void BluetoothManager::disconnect()
{
    if (uartService_) {
        delete uartService_;
        uartService_ = nullptr;
    }
    if (controller_) {
        controller_->disconnectFromDevice();
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

    // BLE devices may be discovered multiple times; deduplicate by address
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
    emit errorOccurred(discovery_->errorString());
}

// --- BLE Connection ---

void BluetoothManager::onControllerConnected()
{
    qDebug() << "BLE controller connected, discovering services...";
    controller_->discoverServices();
}

void BluetoothManager::onControllerDisconnected()
{
    qDebug() << "BLE controller disconnected";
    if (uartService_) {
        delete uartService_;
        uartService_ = nullptr;
    }
    emit disconnected();
}

void BluetoothManager::onControllerError(QLowEnergyController::Error error)
{
    qDebug() << "BLE controller error:" << error << controller_->errorString();
    emit errorOccurred(controller_->errorString());
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

    // Find TX and RX characteristics
    const QList<QLowEnergyCharacteristic> chars = uartService_->characteristics();
    for (const auto &c : chars) {
        qDebug() << "  char:" << c.uuid().toString();
        if (c.uuid() == NUS_RX_UUID) {
            rxChar_ = c;
            // Enable notifications on RX characteristic
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
    // Write acknowledged; nothing needed
}
