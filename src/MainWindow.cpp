#include "MainWindow.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QMessageBox>
#include <QIntValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , tcp_(new TcpManager(this))
    , bt_(new BluetoothManager(this))
{
    setupUi();
    setupToolbar();

    // WiFi/TCP signals
    connect(tcp_, &TcpManager::connected, this, &MainWindow::onTcpConnected);
    connect(tcp_, &TcpManager::disconnected, this, &MainWindow::onTcpDisconnected);
    connect(tcp_, &TcpManager::errorOccurred, this, &MainWindow::onTcpError);
    connect(tcp_, &TcpManager::newLineParsed, this, &MainWindow::onNewLine);

    // BT signals
    connect(bt_, &BluetoothManager::connected, this, &MainWindow::onBtConnected);
    connect(bt_, &BluetoothManager::disconnected, this, &MainWindow::onBtDisconnected);
    connect(bt_, &BluetoothManager::errorOccurred, this, &MainWindow::onBtError);
    connect(bt_, &BluetoothManager::newLineParsed, this, &MainWindow::onNewLine);
    connect(bt_, &BluetoothManager::deviceDiscovered, this, &MainWindow::onDeviceDiscovered);
    connect(bt_, &BluetoothManager::scanFinished, this, &MainWindow::onScanFinished);

    // Default: WiFi mode visible, BT hidden
    onSwitchToWifi();
    statusBar()->showMessage("Ready — WiFi + Bluetooth");
}

void MainWindow::setupUi()
{
    model3D_ = new Model3DWidget(this);
    chartPanel_ = new ChartPanel(this);

    rawLog_ = new QPlainTextEdit(this);
    rawLog_->setReadOnly(true);
    rawLog_->setFont(QFont("Consolas", 9));
    rawLog_->setPlaceholderText("Raw data log...");
    rawLog_->setMaximumBlockCount(200);

    // Right panel: charts on top, raw log on bottom
    auto *rightSplitter = new QSplitter(Qt::Vertical);
    rightSplitter->addWidget(chartPanel_);
    rightSplitter->addWidget(rawLog_);
    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 1);

    // Main layout: 3D view on left, panels on right
    auto *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(model3D_);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    setCentralWidget(mainSplitter);
}

void MainWindow::setupToolbar()
{
    auto *toolbar = addToolBar("Connection");

    // --- Mode toggle buttons ---
    wifiModeBtn_ = new QPushButton("WiFi", this);
    wifiModeBtn_->setCheckable(false);
    toolbar->addWidget(wifiModeBtn_);

    btModeBtn_ = new QPushButton("BT", this);
    btModeBtn_->setCheckable(false);
    toolbar->addWidget(btModeBtn_);

    toolbar->addSeparator();

    // --- WiFi group ---
    wifiGroup_ = new QWidget(this);
    auto *wifiLayout = new QHBoxLayout();
    wifiLayout->setContentsMargins(0, 0, 0, 0);
    wifiLayout->addWidget(new QLabel("IP:"));
    wifiIp_ = new QLineEdit("192.168.4.1");
    wifiIp_->setMaximumWidth(120);
    wifiLayout->addWidget(wifiIp_);
    wifiLayout->addWidget(new QLabel(":"));
    wifiPort_ = new QLineEdit("8888");
    wifiPort_->setMaximumWidth(55);
    wifiPort_->setValidator(new QIntValidator(1, 65535, this));
    wifiLayout->addWidget(wifiPort_);
    wifiConnectBtn_ = new QPushButton("Connect");
    wifiLayout->addWidget(wifiConnectBtn_);
    wifiGroup_->setLayout(wifiLayout);
    toolbar->addWidget(wifiGroup_);

    // --- BT group ---
    btGroup_ = new QWidget(this);
    auto *btLayout = new QHBoxLayout();
    btLayout->setContentsMargins(0, 0, 0, 0);
    scanBtn_ = new QPushButton("Scan");
    btLayout->addWidget(scanBtn_);
    deviceCombo_ = new QComboBox();
    deviceCombo_->setMinimumWidth(160);
    btLayout->addWidget(deviceCombo_);
    btConnectBtn_ = new QPushButton("Connect");
    btLayout->addWidget(btConnectBtn_);
    btGroup_->setLayout(btLayout);
    toolbar->addWidget(btGroup_);

    toolbar->addSeparator();

    statusLabel_ = new QLabel(" Idle");
    toolbar->addWidget(statusLabel_);

    // Mode switch connections
    connect(wifiModeBtn_, &QPushButton::clicked, this, &MainWindow::onSwitchToWifi);
    connect(btModeBtn_, &QPushButton::clicked, this, &MainWindow::onSwitchToBt);

    // WiFi connections
    connect(wifiConnectBtn_, &QPushButton::clicked, this, &MainWindow::onWifiConnectClicked);

    // BT connections
    connect(scanBtn_, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(btConnectBtn_, &QPushButton::clicked, this, &MainWindow::onBtConnectClicked);
}

// --- Helpers ---

void MainWindow::disconnectAll()
{
    if (tcp_->isConnected())
        tcp_->disconnect();
    wifiConnectBtn_->setEnabled(true);
    bt_->stopScan();
    if (bt_->isConnected())
        bt_->disconnect();
    btConnectBtn_->setEnabled(true);
}

void MainWindow::updateWifiButtonState()
{
    if (tcp_->isConnected()) {
        wifiConnectBtn_->setText("Disconnect");
        statusLabel_->setText(" WiFi connected");
    } else {
        wifiConnectBtn_->setText("Connect");
        statusLabel_->setText(" WiFi idle");
    }
}

void MainWindow::updateBtButtonState()
{
    if (bt_->isConnected()) {
        btConnectBtn_->setText("Disconnect");
        statusLabel_->setText(" BT connected");
    } else {
        btConnectBtn_->setText("Connect");
        scanBtn_->setEnabled(true);
        statusLabel_->setText(" BT idle");
    }
}

// --- Mode switching ---

void MainWindow::onSwitchToWifi()
{
    if (wifiModeActive_) return;  // already in WiFi mode
    disconnectAll();
    wifiGroup_->setVisible(true);
    btGroup_->setVisible(false);
    wifiModeBtn_->setStyleSheet("background:#2196F3; color:white; font-weight:bold;");
    btModeBtn_->setStyleSheet("");
    wifiModeActive_ = true;
    updateWifiButtonState();
    statusBar()->showMessage("Switched to WiFi mode");
}

void MainWindow::onSwitchToBt()
{
    if (!wifiModeActive_) return;    // already in BT mode
    disconnectAll();
    wifiGroup_->setVisible(false);
    btGroup_->setVisible(true);
    btModeBtn_->setStyleSheet("background:#2196F3; color:white; font-weight:bold;");
    wifiModeBtn_->setStyleSheet("");
    wifiModeActive_ = false;
    updateBtButtonState();
    statusBar()->showMessage("Switched to Bluetooth mode");
}

// --- WiFi/TCP slots ---

void MainWindow::onWifiConnectClicked()
{
    if (tcp_->isConnected()) {
        tcp_->disconnect();
        return;
    }

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

// --- BT slots ---

void MainWindow::onScanClicked()
{
    deviceCombo_->clear();
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
    QString address = deviceCombo_->currentData().toString();
    if (address.isEmpty()) {
        QMessageBox::warning(this, "BT Connect", "No device selected.");
        return;
    }
    btConnectBtn_->setEnabled(false);
    scanBtn_->setEnabled(false);
    statusLabel_->setText(" BT connecting...");
    bt_->connectToDevice(address);
}

void MainWindow::onBtConnected()
{
    btConnectBtn_->setText("Disconnect");
    btConnectBtn_->setEnabled(true);
    scanBtn_->setEnabled(true);
    statusLabel_->setText(" BT connected");
    statusBar()->showMessage("Bluetooth BLE connected");
}

void MainWindow::onBtDisconnected()
{
    btConnectBtn_->setText("Connect");
    btConnectBtn_->setEnabled(true);
    scanBtn_->setEnabled(true);
    statusLabel_->setText(" BT disconnected");
    statusBar()->showMessage("Bluetooth disconnected");
}

void MainWindow::onBtError(const QString &msg)
{
    btConnectBtn_->setEnabled(true);
    scanBtn_->setEnabled(true);
    statusLabel_->setText(" BT error");
    statusBar()->showMessage("BT Error: " + msg);
    QMessageBox::critical(this, "BT Error", msg);
}

void MainWindow::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    deviceCombo_->addItem(info.name() + " [" + info.address().toString() + "]",
                          info.address().toString());
    btConnectBtn_->setEnabled(true);
}

void MainWindow::onScanFinished()
{
    if (wifiModeActive_) return;
    // If connect is disabled, we're mid-connection — don't stomp state
    if (!btConnectBtn_->isEnabled()) return;
    scanBtn_->setEnabled(true);
    statusLabel_->setText(" BT idle");
    if (deviceCombo_->count() == 0)
        deviceCombo_->addItem("No devices found");
}

// --- Data ---

void MainWindow::onNewLine(const QString &line)
{
    rawLog_->appendPlainText(line);

    SensorData data = SensorDataParser::parse(line);
    if (!data.valid)
        return;

    estimator_.update(data, 0.05f);
    EulerAngles angles = estimator_.eulerAngles();

    model3D_->setRotation(angles.roll, angles.pitch, angles.yaw);
    chartPanel_->addDataPoint(data);
}
