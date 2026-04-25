#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>

// WiFi AP 配置
const char* ssid = "ESP32_MPU6050";
const char* password = "12345678";  // 至少 8 位

// TCP 服务器
WiFiServer server(8888);
WiFiClient client;

// MPU6050 对象
Adafruit_MPU6050 mpu;

// I2C 引脚
#define I2C_SDA 21
#define I2C_SCL 22

// 发送间隔 (ms)
const unsigned long sendInterval = 50;
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 MPU6050 — WiFi TCP Server");

  // 初始化 I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // 初始化 MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 init failed!");
    while (1) delay(10);
  }
  Serial.println("MPU6050 OK");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // 启动 WiFi AP
  WiFi.softAP(ssid, password);
  Serial.print("WiFi AP: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // 启动 TCP 服务器
  server.begin();
  Serial.println("TCP Server on port 8888");
}

void loop() {
  // 处理客户端连接
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      Serial.println("Client connected");
    }
  }

  // 按固定间隔发送数据
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= sendInterval) {
    previousMillis = currentMillis;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    String dataString = "";
    dataString += "ACC_X:";
    dataString += String(a.acceleration.x, 2);
    dataString += ",ACC_Y:";
    dataString += String(a.acceleration.y, 2);
    dataString += ",ACC_Z:";
    dataString += String(a.acceleration.z, 2);
    dataString += ",GYRO_X:";
    dataString += String(g.gyro.x, 2);
    dataString += ",GYRO_Y:";
    dataString += String(g.gyro.y, 2);
    dataString += ",GYRO_Z:";
    dataString += String(g.gyro.z, 2);
    dataString += ",TEMP:";
    dataString += String(temp.temperature, 2);
    dataString += "\n";

    // 通过串口打印 (调试)
    Serial.print(dataString);

    // 通过 TCP 发送
    if (client && client.connected()) {
      client.print(dataString);
    }
  }
}
