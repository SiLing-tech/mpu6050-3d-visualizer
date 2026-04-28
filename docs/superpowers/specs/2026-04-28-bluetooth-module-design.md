# Bluetooth Module Restoration Design

**Date:** 2026-04-28
**Status:** approved

## Overview

Restore Bluetooth SPP connectivity to the MPU6050 3D Visualizer (removed in commit `37eb667`), add dual-mode WiFi/BT switching in the toolbar, and merge the two separate ESP32 firmware files into one unified firmware with serial command mode control.

## PC Side: Qt6 Application

### CMakeLists.txt

- Add `Qt6::Bluetooth` to `find_package` and `target_link_libraries`
- Add `src/BluetoothManager.cpp` to SOURCES
- Add `src/BluetoothManager.h` to HEADERS

### MainWindow.h

- Include `BluetoothManager.h`
- Add `BluetoothManager *bt_` member
- Add BT toolbar widgets: `scanBtn_`, `deviceCombo_`, `btConnectBtn_`
- Add mode toggle buttons: `wifiModeBtn_`, `btModeBtn_`
- Add `QWidget *wifiGroup_` and `QWidget *btGroup_` for widget visibility groups
- Add slots: `onScanClicked`, `onBtConnectClicked`, `onBtConnected`, `onBtDisconnected`, `onBtError`, `onDeviceDiscovered`, `onScanFinished`, `onSwitchToWifi`, `onSwitchToBt`
- Add `disconnectAll()` helper

### MainWindow.cpp — Toolbar Layout

```
[WiFi btn | BT btn]  |  WiFi group: [IP] [Port] [Connect]  |  BT group: [Scan] [DeviceCombo] [Connect]  |  [Status]
```

- Mode toggle buttons: active mode gets blue highlight, inactive is grey
- Switching mode calls `disconnectAll()` first
- `wifiGroup_` / `btGroup_` show/hide via `setVisible()`
- BT `newLineParsed` signal connects to `MainWindow::onNewLine` (same pipeline as WiFi)
- Default mode on startup: WiFi (matches current behavior)

### BluetoothManager

No code changes. Existing implementation is complete and correct.

## ESP32 Side: Merged Firmware

### File: `ESP32_mpu6050.c` (replaces both old files)

Delete: `ESP32_mpu6050_wifi.c`

### Architecture

Both Bluetooth SPP and WiFi AP+Server run simultaneously by default. A serial command parser in the main loop allows runtime mode switching.

### Serial Commands (115200 baud)

| Command | Action |
|---------|--------|
| `bt_only` | Disable WiFi radio, send only via Bluetooth |
| `wifi_only` | Disable Bluetooth, send only via WiFi TCP |
| `both` | Enable both channels (default) |
| `status` | Print current mode, BT client state, WiFi client count |

### Flow

```
setup():
  Serial.begin(115200)
  Wire.begin(SDA=21, SCL=22)
  MPU6050 init (8G, 500dps, 21Hz filter)
  SerialBT.begin("ESP32_MPU6050")
  WiFi.softAP("ESP32_MPU6050", "12345678")
  server.begin(8888)

loop():
  handle WiFi client connect/disconnect
  read serial → parse command → update mode flags
  if 50ms elapsed:
    read MPU6050
    format: "ACC_X:...,ACC_Y:...,ACC_Z:...,GYRO_X:...,GYRO_Y:...,GYRO_Z:...,TEMP:...\n"
    if BT enabled: SerialBT.print(data)
    if WiFi enabled && client: client.print(data)
    Serial.print(data)  // always debug output
```

### Mode Switching

- `bt_only`: calls `WiFi.softAPdisconnect(true)` to fully shut down WiFi radio
- `wifi_only`: calls `SerialBT.end()` to shut down Bluetooth
- `both`: restarts disabled radio if needed (reinitialize WiFi AP or BT)

## Data Format (unchanged)

```
ACC_X:0.12,ACC_Y:-0.05,ACC_Z:9.81,GYRO_X:0.01,GYRO_Y:0.02,GYRO_Z:-0.01,TEMP:25.36\n
```

## Files Changed

| File | Action |
|------|--------|
| `CMakeLists.txt` | Add BluetoothManager sources + Qt6::Bluetooth |
| `src/MainWindow.h` | Add BT members, slots, toolbar widgets |
| `src/MainWindow.cpp` | Add BT setup, mode switching, signal wiring |
| `ESP32_mpu6050.c` | Rewrite as merged firmware |
| `ESP32_mpu6050_wifi.c` | Delete |

## Files Unchanged

| File |
|------|
| `src/BluetoothManager.h` |
| `src/BluetoothManager.cpp` |
| `src/TcpManager.h` / `.cpp` |
| `src/ChartPanel.h` / `.cpp` |
| `src/Model3DWidget.h` / `.cpp` |
| `src/AttitudeEstimator.h` / `.cpp` |
| `src/SensorDataParser.h` / `.cpp` |
| `src/SensorData.h` |
| `src/RollingBuffer.h` |
