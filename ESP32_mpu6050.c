#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>

// --- Nordic UART Service (NUS) UUIDs ---
#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // PC -> ESP32 (write)
#define NUS_RX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 -> PC (notify)

// --- Mode flags ---
bool btEnabled = true;
bool wifiEnabled = true;
bool bleConnected = false;

// --- Hardware ---
Adafruit_MPU6050 mpu;
WiFiServer server(8888);
WiFiClient client;

// --- BLE ---
BLEServer *bleServer = nullptr;
BLEService *nusService = nullptr;
BLECharacteristic *rxChar = nullptr;   // ESP32 -> PC (notify)
BLECharacteristic *txChar = nullptr;   // PC -> ESP32 (write)

// --- BLE Server Callbacks ---
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *s) {
        bleConnected = true;
        Serial.println("BLE client connected");
    }
    void onDisconnect(BLEServer *s) {
        bleConnected = false;
        Serial.println("BLE client disconnected");
        // Restart advertising so PC can reconnect
        BLEDevice::startAdvertising();
    }
};

// --- BLE TX Characteristic Callbacks (data from PC) ---
class TxCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *c) {
        // We currently don't process incoming BLE data, but this is ready if needed
        Serial.print("BLE RX: ");
        Serial.println(c->getValue().c_str());
    }
};

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

// --- BLE setup ---
void setupBLE() {
    BLEDevice::init(btName);
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new ServerCallbacks());

    nusService = bleServer->createService(NUS_SERVICE_UUID);

    // RX: ESP32 -> PC (notify)
    rxChar = nusService->createCharacteristic(NUS_RX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    rxChar->addDescriptor(new BLE2902());

    // TX: PC -> ESP32 (write)
    txChar = nusService->createCharacteristic(NUS_TX_UUID, BLECharacteristic::PROPERTY_WRITE);
    txChar->setCallbacks(new TxCallbacks());

    nusService->start();

    // Start advertising
    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(NUS_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x06);
    adv->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    btEnabled = true;
    bleConnected = false;
    Serial.println("BLE NUS started: ESP32_MPU6050");
}

void stopBLE() {
    BLEDevice::stopAdvertising();
    if (bleServer) {
        bleServer->getAdvertising()->stop();
    }
    btEnabled = false;
    bleConnected = false;
    Serial.println("BLE disabled");
}

// --- Mode switching ---
void setBtMode() {
    if (btEnabled) return;
    setupBLE();
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
        stopBLE();
    }
    if (!wifiEnabled) setWifiMode();
}

void printStatus() {
    Serial.println("=== STATUS ===");
    Serial.print("BT (BLE): ");
    Serial.println(btEnabled ? (bleConnected ? "connected" : "advertising") : "disabled");
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
    Serial.println("ESP32 MPU6050 — Dual BLE+WiFi");

    Wire.begin(I2C_SDA, I2C_SCL);

    if (!mpu.begin()) {
        Serial.println("MPU6050 init failed!");
        while (1) delay(10);
    }
    Serial.println("MPU6050 OK");

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    setupBLE();

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

    char buf[128];
    snprintf(buf, sizeof(buf),
        "ACC_X:%.2f,ACC_Y:%.2f,ACC_Z:%.2f,GYRO_X:%.2f,GYRO_Y:%.2f,GYRO_Z:%.2f,TEMP:%.2f\n",
        a.acceleration.x, a.acceleration.y, a.acceleration.z,
        g.gyro.x, g.gyro.y, g.gyro.z,
        temp.temperature);

    if (btEnabled && bleConnected) {
        rxChar->setValue(buf);
        rxChar->notify();
    }
    if (wifiEnabled && client && client.connected()) client.print(buf);
    Serial.print(buf);
}
