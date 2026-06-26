cmake_minimum_required(VERSION 3.14)
project(SmartVehicleMonitor VERSION 1.0 LANGUAGES CXX)

# Enforce C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files for target
set(COMMON_SOURCES
    src/Sensor.cpp
    src/Alert.cpp
    src/Logger.cpp
    src/Dashboard.cpp
    src/ThreadManager.cpp
    src/ServiceBus.cpp
    src/CanBus.cpp
    src/DtcManager.cpp
    src/EcuMonitor.cpp
    src/OtaSimulator.cpp
    src/EdrRecorder.cpp
)

# Main Interactive Application Target (Consolidated Build)
add_executable(${PROJECT_NAME} src/main.cpp src/tests.cpp ${COMMON_SOURCES})
target_compile_definitions(${PROJECT_NAME} PRIVATE CONSOLIDATED_BUILD)
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Find and link threads library
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
