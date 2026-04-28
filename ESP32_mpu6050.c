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
