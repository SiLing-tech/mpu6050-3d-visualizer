#include "MainWindow.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMessageBox>
#include <QIntValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , tcp_(new TcpManager(this))
{
    setupUi();
    setupToolbar();

    // WiFi/TCP signals
    connect(tcp_, &TcpManager::connected, this, &MainWindow::onTcpConnected);
    connect(tcp_, &TcpManager::disconnected, this, &MainWindow::onTcpDisconnected);
    connect(tcp_, &TcpManager::errorOccurred, this, &MainWindow::onTcpError);
    connect(tcp_, &TcpManager::newLineParsed, this, &MainWindow::onNewLine);

    statusBar()->showMessage("Ready — WiFi TCP + Charts");
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

    connect(wifiConnectBtn_, &QPushButton::clicked, this, &MainWindow::onWifiConnectClicked);
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
