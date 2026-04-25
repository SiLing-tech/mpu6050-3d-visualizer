#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "BluetoothSerial.h"

// 检查是否支持蓝牙
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif

// 创建MPU6050对象
Adafruit_MPU6050 mpu;

// 创建蓝牙对象
BluetoothSerial SerialBT;

// 蓝牙设备名称
const char* deviceName = "ESP32_MPU6050";

// I2C引脚定义（根据您的ESP32型号调整）
#define I2C_SDA 21    // SDA引脚
#define I2C_SCL 22    // SCL引脚

// 数据发送间隔（毫秒）
const unsigned long sendInterval = 50; // 20Hz采样率
unsigned long previousMillis = 0;

void setup() {
  // 初始化串口（用于调试）
  Serial.begin(115200);
  Serial.println("ESP32 MPU6050 蓝牙数据传输开始");
  
  // 初始化I2C总线
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // 初始化MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 初始化失败!");
    Serial.println("请检查连接或尝试重置设备");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 初始化成功");

  // 设置MPU6050参数
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("加速度计量程设置为: ");
  switch (mpu.getAccelerometerRange()) {
    case MPU6050_RANGE_2_G:  Serial.println("+-2G"); break;
    case MPU6050_RANGE_4_G:  Serial.println("+-4G"); break;
    case MPU6050_RANGE_8_G:  Serial.println("+-8G"); break;
    case MPU6050_RANGE_16_G: Serial.println("+-16G"); break;
  }
  
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("陀螺仪量程设置为: ");
  switch (mpu.getGyroRange()) {
    case MPU6050_RANGE_250_DEG:  Serial.println("+-250 deg/s"); break;
    case MPU6050_RANGE_500_DEG:  Serial.println("+-500 deg/s"); break;
    case MPU6050_RANGE_1000_DEG: Serial.println("+-1000 deg/s"); break;
    case MPU6050_RANGE_2000_DEG: Serial.println("+-2000 deg/s"); break;
  }
  
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("数字低通滤波器设置为: ");
  switch (mpu.getFilterBandwidth()) {
    case MPU6050_BAND_260_HZ: Serial.println("260 Hz"); break;
    case MPU6050_BAND_184_HZ: Serial.println("184 Hz"); break;
    case MPU6050_BAND_94_HZ:  Serial.println("94 Hz"); break;
    case MPU6050_BAND_44_HZ:  Serial.println("44 Hz"); break;
    case MPU6050_BAND_21_HZ:  Serial.println("21 Hz"); break;
    case MPU6050_BAND_10_HZ:  Serial.println("10 Hz"); break;
    case MPU6050_BAND_5_HZ:   Serial.println("5 Hz"); break;
  }

  // 初始化蓝牙
  SerialBT.begin(deviceName); // 启动蓝牙串口，设备名为"ESP32_MPU6050"
  Serial.println("蓝牙已启动");
  Serial.print("设备名称: ");
  Serial.println(deviceName);
  Serial.println("请在电脑端搜索并配对此设备");
  
  delay(2000); // 等待蓝牙稳定
}

void loop() {
  unsigned long currentMillis = millis();
  
  // 按固定间隔发送数据
  if (currentMillis - previousMillis >= sendInterval) {
    previousMillis = currentMillis;
    
    // 读取传感器事件
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    // 格式化数据字符串
    String dataString = "";
    
    // 加速度数据 (m/s²)
    dataString += "ACC_X:";
    dataString += String(a.acceleration.x, 2);
    dataString += ",ACC_Y:";
    dataString += String(a.acceleration.y, 2);
    dataString += ",ACC_Z:";
    dataString += String(a.acceleration.z, 2);
    
    // 陀螺仪数据 (rad/s)
    dataString += ",GYRO_X:";
    dataString += String(g.gyro.x, 2);
    dataString += ",GYRO_Y:";
    dataString += String(g.gyro.y, 2);
    dataString += ",GYRO_Z:";
    dataString += String(g.gyro.z, 2);
    
    // 温度数据 (°C)
    dataString += ",TEMP:";
    dataString += String(temp.temperature, 2);
    dataString += "\n"; // 结尾换行符
    
    // 通过蓝牙发送数据
    SerialBT.print(dataString);
    
    // 同时通过串口发送（用于调试）
    Serial.print(dataString);
  }
}