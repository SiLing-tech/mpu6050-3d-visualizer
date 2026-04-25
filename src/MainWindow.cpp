#include "MainWindow.h"
#include "SensorDataParser.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QMessageBox>
#include <QIntValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , bt_(new BluetoothManager(this))
    , tcp_(new TcpManager(this))
    , estimator_(0.1f)
{
    setupUi();
    setupToolbar();

    // Bluetooth signals
    connect(bt_, &BluetoothManager::deviceDiscovered, this, &MainWindow::onDeviceDiscovered);
    connect(bt_, &BluetoothManager::scanFinished, this, &MainWindow::onScanFinished);
    connect(bt_, &BluetoothManager::connected, this, &MainWindow::onBtConnected);
    connect(bt_, &BluetoothManager::disconnected, this, &MainWindow::onBtDisconnected);
    connect(bt_, &BluetoothManager::errorOccurred, this, &MainWindow::onBtError);
    connect(bt_, &BluetoothManager::newLineParsed, this, &MainWindow::onNewLine);

    // WiFi/TCP signals
    connect(tcp_, &TcpManager::connected, this, &MainWindow::onTcpConnected);
    connect(tcp_, &TcpManager::disconnected, this, &MainWindow::onTcpDisconnected);
    connect(tcp_, &TcpManager::errorOccurred, this, &MainWindow::onTcpError);
    connect(tcp_, &TcpManager::newLineParsed, this, &MainWindow::onNewLine);

    statusBar()->showMessage("Ready — Bluetooth or WiFi");
}

void MainWindow::setupUi()
{
    model3D_ = new Model3DWidget(this);
    dataPanel_ = new DataPanel(this);
    chartPanel_ = new ChartPanel(this);

    auto *rightPanel = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(dataPanel_, 2);
    rightLayout->addWidget(chartPanel_, 3);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(model3D_);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(splitter);
}

void MainWindow::setupToolbar()
{
    auto *toolbar = addToolBar("Connection");

    // --- Bluetooth group ---
    QLabel *btLabel = new QLabel(" BT:");
    toolbar->addWidget(btLabel);

    scanBtn_ = new QPushButton("Scan", this);
    toolbar->addWidget(scanBtn_);

    deviceCombo_ = new QComboBox(this);
    deviceCombo_->setMinimumWidth(200);
    toolbar->addWidget(deviceCombo_);

    btConnectBtn_ = new QPushButton("Connect", this);
    toolbar->addWidget(btConnectBtn_);

    toolbar->addSeparator();

    // --- WiFi group ---
    QLabel *wifiLabel = new QLabel(" WiFi:");
    toolbar->addWidget(wifiLabel);

    wifiIp_ = new QLineEdit("192.168.4.1", this);
    wifiIp_->setMaximumWidth(120);
    toolbar->addWidget(wifiIp_);

    QLabel *portLabel = new QLabel(":");
    toolbar->addWidget(portLabel);

    wifiPort_ = new QLineEdit("8888", this);
    wifiPort_->setMaximumWidth(55);
    wifiPort_->setValidator(new QIntValidator(1, 65535, this));
    toolbar->addWidget(wifiPort_);

    wifiConnectBtn_ = new QPushButton("Connect", this);
    toolbar->addWidget(wifiConnectBtn_);

    toolbar->addSeparator();

    statusLabel_ = new QLabel(" Idle");
    toolbar->addWidget(statusLabel_);

    connect(scanBtn_, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(btConnectBtn_, &QPushButton::clicked, this, &MainWindow::onBtConnectClicked);
    connect(wifiConnectBtn_, &QPushButton::clicked, this, &MainWindow::onWifiConnectClicked);
}

// --- Bluetooth slots ---

void MainWindow::onScanClicked()
{
    deviceCombo_->clear();
    deviceCombo_->addItem("Scanning...");
    scanBtn_->setEnabled(false);
    statusLabel_->setText(" BT scanning...");
    bt_->startScan();
}

void MainWindow::onBtConnectClicked()
{
    if (bt_->isConnected()) {
        bt_->disconnect();
        return;
    }

    disconnectAll();

    int idx = deviceCombo_->currentIndex();
    if (idx < 0) {
        QMessageBox::warning(this, "Connect", "No device selected.");
        return;
    }
    btConnectBtn_->setEnabled(false);
    statusLabel_->setText(" BT connecting...");
    bt_->connectToDevice(idx);
}

void MainWindow::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (deviceCombo_->count() == 1 && deviceCombo_->itemText(0) == "Scanning...")
        deviceCombo_->clear();

    deviceCombo_->addItem(info.name() + " [" + info.address().toString() + "]");
}

void MainWindow::onScanFinished()
{
    scanBtn_->setEnabled(true);
    if (deviceCombo_->count() == 0)
        deviceCombo_->addItem("No devices found");
    statusLabel_->setText(" BT scan finished");
}

void MainWindow::onBtConnected()
{
    btConnectBtn_->setText("Disconnect");
    btConnectBtn_->setEnabled(true);
    statusLabel_->setText(" BT connected");
    statusBar()->showMessage("Bluetooth connected");
}

void MainWindow::onBtDisconnected()
{
    btConnectBtn_->setText("Connect");
    btConnectBtn_->setEnabled(true);
    statusLabel_->setText(" BT disconnected");
    statusBar()->showMessage("Bluetooth disconnected");
}

void MainWindow::onBtError(const QString &msg)
{
    btConnectBtn_->setEnabled(true);
    statusLabel_->setText(" BT error");
    statusBar()->showMessage("BT Error: " + msg);
    QMessageBox::critical(this, "Bluetooth Error", msg);
}

// --- WiFi/TCP slots ---

void MainWindow::onWifiConnectClicked()
{
    if (tcp_->isConnected()) {
        tcp_->disconnect();
        return;
    }

    disconnectAll();

    QString ip = wifiIp_->text().trimmed();
    quint16 port = static_cast<quint16>(wifiPort_->text().toUInt());

    if (ip.isEmpty() || port == 0) {
        QMessageBox::warning(this, "WiFi Connect", "Enter valid IP and port.");
        return;
    }

    wifiConnectBtn_->setEnabled(false);
    statusLabel_->setText(" WiFi connecting...");
    tcp_->connectToHost(ip, port);
}

void MainWindow::onTcpConnected()
{
    wifiConnectBtn_->setText("Disconnect");
    wifiConnectBtn_->setEnabled(true);
    statusLabel_->setText(" WiFi connected");
    statusBar()->showMessage("WiFi/TCP connected");
}

void MainWindow::onTcpDisconnected()
{
    wifiConnectBtn_->setText("Connect");
    wifiConnectBtn_->setEnabled(true);
    statusLabel_->setText(" WiFi disconnected");
    statusBar()->showMessage("WiFi/TCP disconnected");
}

void MainWindow::onTcpError(const QString &msg)
{
    wifiConnectBtn_->setEnabled(true);
    statusLabel_->setText(" WiFi error");
    statusBar()->showMessage("WiFi Error: " + msg);
    QMessageBox::critical(this, "WiFi Error", msg);
}

// --- Data ---

void MainWindow::onNewLine(const QString &line)
{
    SensorData data = SensorDataParser::parse(line);
    if (!data.valid)
        return;

    estimator_.update(data, 0.05f);
    EulerAngles angles = estimator_.eulerAngles();

    model3D_->setRotation(angles.roll, angles.pitch, angles.yaw);
    dataPanel_->updateData(data, angles);
    chartPanel_->addDataPoint(data);
}

// --- Helpers ---

void MainWindow::disconnectAll()
{
    if (bt_->isConnected())
        bt_->disconnect();
    if (tcp_->isConnected())
        tcp_->disconnect();
}
