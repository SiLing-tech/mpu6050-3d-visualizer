# Bluetooth Module Restoration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore Bluetooth SPP connectivity with dual-mode WiFi/BT toolbar switching, and merge ESP32 firmware into one unified file with serial command mode control.

**Architecture:** Restore existing BluetoothManager code (unchanged), add mode toggle to MainWindow toolbar, and rewrite ESP32 firmware to run BT+WiFi simultaneously with serial command switching.

**Tech Stack:** Qt6 C++ (Widgets, Bluetooth, Network, Charts, OpenGLWidgets), CMake, ESP32 Arduino (BluetoothSerial, WiFi, Adafruit MPU6050)

---

### Task 1: Update CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt:9-44`

- [ ] **Step 1: Add Qt6::Bluetooth and BluetoothManager sources to build**

Add `Bluetooth` to `find_package` (line 9):
```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Charts OpenGLWidgets Bluetooth)
```

Add `src/BluetoothManager.cpp` to SOURCES (line 18):
```cmake
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/TcpManager.cpp
    src/BluetoothManager.cpp
    src/SensorDataParser.cpp
    src/ChartPanel.cpp
    src/AttitudeEstimator.cpp
    src/Model3DWidget.cpp
    src/ObjLoader.cpp
)
```

Add `src/BluetoothManager.h` to HEADERS (line 25):
```cmake
set(HEADERS
    src/MainWindow.h
    src/TcpManager.h
    src/BluetoothManager.h
    src/SensorData.h
    src/SensorDataParser.h
    src/ChartPanel.h
    src/RollingBuffer.h
    src/AttitudeEstimator.h
    src/Model3DWidget.h
    src/ObjLoader.h
)
```

Add `Qt6::Bluetooth` to `target_link_libraries` (line 42):
```cmake
target_link_libraries(mpu6050_visualizer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Bluetooth
    Qt6::Charts
    Qt6::OpenGLWidgets
)
```

- [ ] **Step 2: Verify CMake configures successfully**

Run: `cmake -S . -B build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug`
Expected: Configuration complete with no errors, Qt6::Bluetooth found

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: add Qt6::Bluetooth and BluetoothManager to build

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 2: Update MainWindow.h — Add Bluetooth members and slots

**Files:**
- Modify: `src/MainWindow.h:1-52`

- [ ] **Step 1: Replace MainWindow.h with BT-enabled version**

```cpp
#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QToolBar>

#include "TcpManager.h"
#include "BluetoothManager.h"
#include "ChartPanel.h"
#include "Model3DWidget.h"
#include "AttitudeEstimator.h"
#include "SensorData.h"
#include "SensorDataParser.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    TcpManager *tcp_;
    BluetoothManager *bt_;

    // Mode toggle
    QPushButton *wifiModeBtn_;
    QPushButton *btModeBtn_;

    // WiFi toolbar widgets
    QWidget *wifiGroup_;
    QLineEdit *wifiIp_;
    QLineEdit *wifiPort_;
    QPushButton *wifiConnectBtn_;

    // BT toolbar widgets
    QWidget *btGroup_;
    QPushButton *scanBtn_;
    QComboBox *deviceCombo_;
    QPushButton *btConnectBtn_;

    QLabel *statusLabel_;

    AttitudeEstimator estimator_{0.1f};

    // Content
    Model3DWidget *model3D_;
    ChartPanel *chartPanel_;
    QPlainTextEdit *rawLog_;

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // Mode switching
    void onSwitchToWifi();
    void onSwitchToBt();

    // WiFi
    void onWifiConnectClicked();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &msg);

    // BT
    void onScanClicked();
    void onBtConnectClicked();
    void onBtConnected();
    void onBtDisconnected();
    void onBtError(const QString &msg);
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanFinished();

    // Data
    void onNewLine(const QString &line);

private:
    void setupUi();
    void setupToolbar();
    void disconnectAll();
    void updateWifiButtonState();
    void updateBtButtonState();
};
```

- [ ] **Step 2: Commit**

```bash
git add src/MainWindow.h
git commit -m "feat: add Bluetooth members and slots to MainWindow header

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 3: Update MainWindow.cpp — Wire Bluetooth and mode switching

**Files:**
- Modify: `src/MainWindow.cpp:1-146`

- [ ] **Step 1: Replace constructor (lines 9-23) with BT-enabled version**

```cpp
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
```

- [ ] **Step 2: Replace setupToolbar (lines 53-82) with dual-mode version**

```cpp
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
```

- [ ] **Step 3: Add disconnectAll helper**

```cpp
void MainWindow::disconnectAll()
{
    if (tcp_->isConnected())
        tcp_->disconnect();
    if (bt_->isConnected())
        bt_->disconnect();
}
```

- [ ] **Step 4: Add mode switching slots**

```cpp
void MainWindow::onSwitchToWifi()
{
    disconnectAll();
    wifiGroup_->setVisible(true);
    btGroup_->setVisible(false);
    wifiModeBtn_->setStyleSheet("background:#2196F3; color:white; font-weight:bold;");
    btModeBtn_->setStyleSheet("");
    updateWifiButtonState();
    statusBar()->showMessage("Switched to WiFi mode");
}

void MainWindow::onSwitchToBt()
{
    disconnectAll();
    wifiGroup_->setVisible(false);
    btGroup_->setVisible(true);
    btModeBtn_->setStyleSheet("background:#2196F3; color:white; font-weight:bold;");
    wifiModeBtn_->setStyleSheet("");
    updateBtButtonState();
    statusBar()->showMessage("Switched to Bluetooth mode");
}
```

- [ ] **Step 5: Add Bluetooth slot implementations**

```cpp
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
    int idx = deviceCombo_->currentIndex();
    if (idx < 0) {
        QMessageBox::warning(this, "BT Connect", "No device selected.");
        return;
    }
    btConnectBtn_->setEnabled(false);
    statusLabel_->setText(" BT connecting...");
    bt_->connectToDevice(idx);
}

void MainWindow::onBtConnected()
{
    btConnectBtn_->setText("Disconnect");
    btConnectBtn_->setEnabled(true);
    statusLabel_->setText(" BT connected");
    statusBar()->showMessage("Bluetooth SPP connected");
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
    scanBtn_->setEnabled(true);
    statusLabel_->setText(" BT error");
    statusBar()->showMessage("BT Error: " + msg);
    QMessageBox::critical(this, "BT Error", msg);
}

void MainWindow::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    deviceCombo_->addItem(info.name() + " [" + info.address().toString() + "]");
}

void MainWindow::onScanFinished()
{
    scanBtn_->setEnabled(true);
    statusLabel_->setText(" BT idle");
    if (deviceCombo_->count() == 1 && deviceCombo_->itemText(0) == "Scanning...")
        deviceCombo_->clear();
    if (deviceCombo_->count() == 0)
        deviceCombo_->addItem("No devices found");
}
```

- [ ] **Step 6: Add button state update helpers (for mode switching context)**

Add `updateWifiButtonState()` and `updateBtButtonState()` as private helper methods (declare in header or keep as free functions). These sync button text with current connection state after mode switch. Include these before the mode switch slots:

```cpp
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
```

Also declare these in MainWindow.h among the private methods:
```cpp
void updateWifiButtonState();
void updateBtButtonState();
```

And update `onWifiConnectClicked` (line 86-104) to use the helper:
```cpp
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
```

Update `onTcpConnected` (line 106-112) and `onTcpDisconnected` (line 114-120) and `onTcpError` (line 122-128) to use the helper:
```cpp
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
```

- [ ] **Step 7: Commit**

```bash
git add src/MainWindow.cpp src/MainWindow.h
git commit -m "feat: add dual-mode WiFi/BT switching in MainWindow toolbar

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 4: Build verification (PC side)

- [ ] **Step 1: Run CMake configure**

```bash
cmake -S . -B build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug
```
Expected: Configures successfully, no errors.

- [ ] **Step 2: Build the project**

```bash
cmake --build build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug
```
Expected: Compiles with 0 errors. Warnings only if pre-existing.

- [ ] **Step 3: Commit (if any build fixes were needed)**

If build succeeds with no changes, skip this commit. Otherwise:
```bash
git add -u
git commit -m "fix: resolve build issues from Bluetooth restoration"
```

---

### Task 5: Rewrite ESP32 merged firmware

**Files:**
- Create: `ESP32_mpu6050.c` (overwrite existing)

- [ ] **Step 1: Write the merged firmware**

```cpp
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BluetoothSerial.h>
#include <WiFi.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Run `make menuconfig` to enable it
#endif

// --- Mode flags ---
bool btEnabled = true;
bool wifiEnabled = true;

// --- Hardware ---
Adafruit_MPU6050 mpu;
BluetoothSerial SerialBT;
WiFiServer server(8888);
WiFiClient client;

// --- I2C pins ---
#define I2C_SDA 21
#define I2C_SCL 22

// --- Timing ---
const unsigned long sendInterval = 50;  // 20 Hz
unsigned long previousMillis = 0;

// --- WiFi config ---
const char* wifiSSID = "ESP32_MPU6050";
const char* wifiPass = "12345678";
const char* btName = "ESP32_MPU6050";

// --- Mode switching ---
void setBtMode() {
    if (btEnabled) return;
    SerialBT.begin(btName);
    btEnabled = true;
    Serial.println("BT restarted");
}

void setWifiMode() {
    if (wifiEnabled) return;
    WiFi.softAP(wifiSSID, wifiPass);
    server.begin();
    wifiEnabled = true;
    Serial.println("WiFi AP restarted");
}

void setBothMode() {
    if (!btEnabled) setBtMode();
    if (!wifiEnabled) setWifiMode();
}

void setBtOnly() {
    if (wifiEnabled) {
        if (client && client.connected()) client.stop();
        server.end();
        WiFi.softAPdisconnect(true);
        wifiEnabled = false;
        Serial.println("WiFi disabled");
    }
    if (!btEnabled) setBtMode();
}

void setWifiOnly() {
    if (btEnabled) {
        SerialBT.end();
        btEnabled = false;
        Serial.println("BT disabled");
    }
    if (!wifiEnabled) setWifiMode();
}

void printStatus() {
    Serial.println("=== STATUS ===");
    Serial.print("BT: ");
    Serial.println(btEnabled ? (SerialBT.hasClient() ? "connected" : "waiting") : "disabled");
    Serial.print("WiFi: ");
    Serial.println(wifiEnabled ? (client && client.connected() ? "client connected" : "AP running") : "disabled");
    Serial.println("==============");
}

void handleSerialCommand() {
    if (!Serial.available()) return;
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "bt_only") {
        setBtOnly();
        Serial.println("Mode: BT only");
    } else if (cmd == "wifi_only") {
        setWifiOnly();
        Serial.println("Mode: WiFi only");
    } else if (cmd == "both") {
        setBothMode();
        Serial.println("Mode: both");
    } else if (cmd == "status") {
        printStatus();
    } else if (cmd.length() > 0) {
        Serial.print("Unknown: ");
        Serial.println(cmd);
        Serial.println("Commands: bt_only, wifi_only, both, status");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 MPU6050 — Dual BT+WiFi");

    Wire.begin(I2C_SDA, I2C_SCL);

    if (!mpu.begin()) {
        Serial.println("MPU6050 init failed!");
        while (1) delay(10);
    }
    Serial.println("MPU6050 OK");

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    SerialBT.begin(btName);
    Serial.println("BT started: ESP32_MPU6050");

    WiFi.softAP(wifiSSID, wifiPass);
    server.begin();
    Serial.print("WiFi AP: ");
    Serial.print(wifiSSID);
    Serial.print(" @ ");
    Serial.println(WiFi.softAPIP());
    Serial.println("TCP server on port 8888");

    Serial.println("Ready. Commands: bt_only, wifi_only, both, status");
}

void loop() {
    // WiFi client handling
    if (!client || !client.connected()) {
        client = server.available();
        if (client) Serial.println("WiFi client connected");
    }

    // Serial commands
    handleSerialCommand();

    // Sensor sampling at 20 Hz
    unsigned long now = millis();
    if (now - previousMillis < sendInterval) return;
    previousMillis = now;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    String data;
    data += "ACC_X:"; data += String(a.acceleration.x, 2);
    data += ",ACC_Y:"; data += String(a.acceleration.y, 2);
    data += ",ACC_Z:"; data += String(a.acceleration.z, 2);
    data += ",GYRO_X:"; data += String(g.gyro.x, 2);
    data += ",GYRO_Y:"; data += String(g.gyro.y, 2);
    data += ",GYRO_Z:"; data += String(g.gyro.z, 2);
    data += ",TEMP:"; data += String(temp.temperature, 2);
    data += "\n";

    if (btEnabled) SerialBT.print(data);
    if (wifiEnabled && client && client.connected()) client.print(data);
    Serial.print(data);  // debug output always
}
```

- [ ] **Step 2: Commit**

```bash
git add ESP32_mpu6050.c
git commit -m "feat: merge ESP32 firmware — BT+WiFi dual-channel with serial commands

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 6: Delete old WiFi-only firmware

- [ ] **Step 1: Remove ESP32_mpu6050_wifi.c**

```bash
git rm ESP32_mpu6050_wifi.c
```

- [ ] **Step 2: Commit**

```bash
git commit -m "feat: remove obsolete WiFi-only ESP32 firmware

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 7: Final verification

- [ ] **Step 1: Rebuild from clean to verify everything compiles**

```bash
cmake --build build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug --clean-first
```
Expected: 0 errors.

- [ ] **Step 2: Verify all files are tracked**

```bash
git status
```
Expected: clean working tree, all changes committed.

- [ ] **Step 3: Review commit log**

```bash
git log --oneline -6
```
Expected: 5-6 new commits on top of `37eb667`.
