#pragma once

#include <string>

/// @brief Represents a driver comfort profile and dynamic thresholds.
struct DriverProfile {
    std::string name = "Alice (Default)";
    double seatHeightCm = 95.0;
    double mirrorAngleDeg = 12.5;
    double temperaturePreferenceC = 22.0;
    double maxSpeedThresholdKmh = 120.0;
    double lowBatteryThresholdV = 10.0;
    double engineTempThresholdC = 110.0;
};
