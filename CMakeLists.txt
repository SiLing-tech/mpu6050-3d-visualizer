cmake_minimum_required(VERSION 3.22)

project(mpu6050_visualizer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Charts OpenGLWidgets Bluetooth)

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

qt_add_executable(mpu6050_visualizer ${SOURCES} ${HEADERS})

target_include_directories(mpu6050_visualizer PRIVATE src)

target_link_libraries(mpu6050_visualizer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Bluetooth
    Qt6::Charts
    Qt6::OpenGLWidgets
)
