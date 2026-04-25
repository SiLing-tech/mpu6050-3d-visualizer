# MPU6050 3D Visualizer

Qt6 C++ 上位机应用，通过蓝牙 SPP 或 WiFi TCP 接收 ESP32+MPU6050 六轴传感器数据，实时显示数值、绘制曲线并驱动 3D 模型姿态。

## 功能

- **双连接模式** — 经典蓝牙 SPP + WiFi TCP 双通道，一键切换
- **实时数据面板** — 加速度 (m/s²)、角速度 (rad/s)、温度 (°C)、姿态角 Roll/Pitch/Yaw
- **双通道曲线图** — 加速度 3 通道 + 角速度 3 通道，25 秒滚动窗口
- **3D 姿态可视化** — OpenGL 渲染方块模型，姿态受 Madgwick 滤波结果驱动
- **OBJ 模型替换** — 支持加载自定义 OBJ 模型替换默认方块

## 硬件要求

- ESP32 开发板
- MPU6050 六轴传感器模块
- 经典蓝牙或 WiFi 连接

## 构建环境

- **Windows 10/11**
- **Qt 6.11** (MinGW 或 MSVC)
- **CMake 3.22+**
- **GCC** (MinGW) 或 MSVC 2022

## 编译

```bash
# 配置（MinGW 示例，根据实际 Qt 路径调整）
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Qt/6.11.0/mingw_64

# 编译
cmake --build build

# 部署 Qt DLL
windeployqt build/mpu6050_visualizer.exe
```

## 运行

```bash
# 双击 mpu6050_visualizer.exe 或
./mpu6050_visualizer.exe
```

## ESP32 固件

| 文件 | 通信方式 | 说明 |
|------|----------|------|
| `ESP32_mpu6050.c` | 经典蓝牙 SPP | 设备名 `ESP32_MPU6050`，50ms 发送间隔 |
| `ESP32_mpu6050_wifi.c` | WiFi AP + TCP Server | SSID `ESP32_MPU6050`，密码 `12345678`，端口 `8888` |

### 依赖库

安装 Arduino IDE 库管理器：
- **Adafruit MPU6050** by Adafruit
- **Adafruit Unified Sensor** by Adafruit

### 数据格式

两种固件输出格式相同：

```
ACC_X:0.12,ACC_Y:-0.05,ACC_Z:9.81,GYRO_X:0.01,GYRO_Y:0.02,GYRO_Z:-0.01,TEMP:25.36
```

## 项目结构

```
├── CMakeLists.txt
├── .gitignore
├── ESP32_mpu6050.c          # ESP32 蓝牙固件
├── ESP32_mpu6050_wifi.c     # ESP32 WiFi 固件
├── assets/                  # OBJ 模型文件
└── src/
    ├── main.cpp
    ├── MainWindow.h/cpp     # 主窗口，三分区布局
    ├── BluetoothManager.h/cpp  # 蓝牙 SPP 管理
    ├── TcpManager.h/cpp     # WiFi TCP 管理
    ├── SensorData.h         # 数据结构
    ├── SensorDataParser.h/cpp  # 文本行解析
    ├── AttitudeEstimator.h/cpp # Madgwick AHRS 姿态解算
    ├── Model3DWidget.h/cpp  # OpenGL 3D 渲染
    ├── ObjLoader.h/cpp      # OBJ 模型加载
    ├── DataPanel.h/cpp      # 实时数值面板
    ├── ChartPanel.h/cpp     # 曲线图面板
    └── RollingBuffer.h      # 循环缓冲区
```

## 窗口布局

```
┌──────────────────────┬──────────────┐
│                      │  实时数据     │
│                      │  ACC_X: ...  │
│    3D 模型视图       │  GYRO_X: ... │
│    (QOpenGLWidget)   │  TEMP:  ...  │
│                      │  Roll:  ...  │
│                      ├──────────────┤
│                      │  曲线图       │
│                      │  [加速度 3线] │
│                      │  [角速度 3线] │
└──────────────────────┴──────────────┘
```

## 使用流程

### 蓝牙模式

1. ESP32 烧录 `ESP32_mpu6050.c`
2. PC 蓝牙配对 `ESP32_MPU6050`
3. 上位机点 **Scan** → 选择设备 → **Connect**
4. 晃动 MPU6050，观察数据和 3D 方块实时变化

### WiFi 模式

1. ESP32 烧录 `ESP32_mpu6050_wifi.c`
2. PC 连接 WiFi 热点 `ESP32_MPU6050`（密码 `12345678`）
3. 上位机 WiFi 栏填入 `192.168.4.1:8888` → **Connect**
4. 晃动 MPU6050，观察数据

## 自定义 3D 模型

支持 OBJ 格式模型替换：

1. 用 Blender / 3ds Max 导出 `.obj` 文件放入 `assets/`
2. 代码中调用 `model3D_->loadObjModel("assets/your_model.obj")`

## License

MIT
