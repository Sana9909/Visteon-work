#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

// Forward declarations — we depend on Sensor and AlertManager but only
// need pointers/references in this header, so forward-declaring avoids
// pulling in their full definitions and keeps compile times down.
class Sensor;
class AlertManager;
enum class AlertSeverity;

// ===== Sensor Statistics =====
// Tracks running min/max/sum/count for a single sensor's readings.
// Designed to be lightweight and embeddable inside an STL map.
struct SensorStats {
    double min;
    double max;
    double sum;
    int count;

    SensorStats();
    double average() const;
    void update(double value);
    void reset();
};

// ===== Vehicle Statistics =====
// Demonstrates: STL map, operator<<
//
// Aggregates SensorStats for every named sensor in the vehicle.
// The map key is the sensor name, enabling O(log n) lookup and
// automatic alphabetical ordering when iterating.
class VehicleStatistics {
private:
    std::map<std::string, SensorStats> stats_;

public:
    void update(const std::string& sensorName, double value);
    SensorStats getStats(const std::string& sensorName) const;
    const std::map<std::string, SensorStats>& getAllStats() const;
    void reset();

    // Stream insertion prints a formatted table of all sensor statistics
    friend std::ostream& operator<<(std::ostream& os, const VehicleStatistics& stats);
};

// ===== Dashboard =====
// Demonstrates: Stream operators, Formatted output, STL usage
//
// The Dashboard is a stateless renderer (aside from a display-refresh
// counter). It reads from Sensors, AlertManager, and VehicleStatistics
// and writes formatted ASCII output to any std::ostream (defaults to
// std::cout, but can target a file or string stream for testing).
class Dashboard {
public:
    Dashboard();

    // Main display method — clears the screen and renders all sections
    void display(const std::vector<std::unique_ptr<Sensor>>& sensors,
                 const AlertManager& alertManager,
                 const VehicleStatistics& stats,
                 const std::vector<double>& speedHistory,
                 std::ostream& os = std::cout) const;

    // Clears the screen, renders all sections, and prints the menu options
    void renderSystem(const std::vector<std::unique_ptr<Sensor>>& sensors,
                      const AlertManager& alertManager,
                      const VehicleStatistics& stats,
                      const std::vector<double>& speedHistory,
                      std::ostream& os = std::cout) const;

    // Individual display sections (public so callers can compose)
    void displayHeader(const AlertManager* alertManager = nullptr, std::ostream& os = std::cout) const;
    void displaySensors(const std::vector<std::unique_ptr<Sensor>>& sensors,
                        std::ostream& os = std::cout) const;
    void displayAlerts(const AlertManager& alertManager,
                       std::ostream& os = std::cout) const;
    void displayStatistics(const VehicleStatistics& stats,
                           std::ostream& os = std::cout) const;
    void displayAlertHistory(const AlertManager& alertManager,
                             std::ostream& os = std::cout) const;
    void displayEcuStatus(std::ostream& os = std::cout) const;
    void displayDtcCodes(std::ostream& os = std::cout) const;
    void displaySparklineGraph(const std::vector<double>& speedHistory, std::ostream& os = std::cout) const;
    void displayOtaStatus(std::ostream& os = std::cout) const;
    void displayFooter(std::ostream& os = std::cout) const;

    // Stream insertion for the dashboard itself (prints a brief summary)
    friend std::ostream& operator<<(std::ostream& os, const Dashboard& dashboard);

private:
    // Formatting helpers
    static std::string horizontalLine(int width = 60);
    static std::string centeredText(const std::string& text, int width = 60);

    // Mutable because display() is const but needs to bump the counter
    mutable int displayCount_;
};
