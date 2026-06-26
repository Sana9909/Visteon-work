#pragma once

// ============================================================================
// Alert.hpp — Alert Management Component
// ============================================================================
// Demonstrates:
//   • Copy/Move Semantics (Rule of Five)
//   • Operator Overloading (<, ==, <<)
//   • Static Members (alertCount_)
//   • STL Containers (std::deque, std::vector)
//   • Lambda-based filtering via std::function
//   • Smart Pointer interop (accepts std::unique_ptr<Sensor> vectors)
//   • Chrono for timestamping
// ============================================================================

#include <string>
#include <chrono>
#include <vector>
#include <deque>
#include <functional>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <mutex>
#include "DriverProfile.hpp"

// Forward declarations
enum class SensorType : int;
class Sensor;

// ===== Alert Severity Enum =====
// Ordered from least to most severe for intuitive comparisons.
enum class AlertSeverity {
    INFO,       // Informational — no action required
    LOW,        // Low priority alert
    MEDIUM,     // Medium priority alert
    HIGH,       // High priority alert
    WARNING,    // Degraded condition — driver should be aware
    CRITICAL    // Immediate danger — requires urgent attention
};

// Free function: convert severity enum to a human-readable string.
[[nodiscard]] std::string severityToString(AlertSeverity severity);

// ===== Alert Class =====
// Represents a single alert event with severity, message, source sensor name,
// and a high-resolution timestamp.  Fully supports copy and move semantics
// (Rule of Five) so it can be stored efficiently in STL containers.
class Alert {
private:
    AlertSeverity severity_;
    std::string message_;
    std::string source_;                                   // Originating sensor name
    std::chrono::system_clock::time_point timestamp_;      // When the alert fired
    double priorityScore_ = 0.0;                           // Dynamic priority score
    static int alertCount_;                                // Tracks total live Alert objects

public:
    // ── Construction ────────────────────────────────────────────────────────
    Alert(AlertSeverity severity, const std::string& message, const std::string& source);

    // ── Rule of Five — Copy/Move Semantics ──────────────────────────────────
    Alert(const Alert& other);                     // Copy constructor
    Alert(Alert&& other) noexcept;                 // Move constructor
    Alert& operator=(const Alert& other);          // Copy assignment
    Alert& operator=(Alert&& other) noexcept;      // Move assignment
    ~Alert();                                      // Destructor

    // ── Getters ─────────────────────────────────────────────────────────────
    [[nodiscard]] AlertSeverity getSeverity() const;
    [[nodiscard]] const std::string& getMessage() const;
    [[nodiscard]] const std::string& getSource() const;
    [[nodiscard]] std::string getTimestampString() const;
    [[nodiscard]] std::chrono::system_clock::time_point getTimestamp() const;
    [[nodiscard]] double getPriorityScore() const;
    void setPriorityScore(double score);
    [[nodiscard]] static int getAlertCount();

    // ── Operator Overloading ────────────────────────────────────────────────
    // operator< : establishes a total ordering with CRITICAL as highest
    //             priority (sorts first).
    [[nodiscard]] bool operator<(const Alert& other) const;

    // operator== : two alerts are "equal" if they share the same message
    //              and source — used for deduplication.
    [[nodiscard]] bool operator==(const Alert& other) const;

    // operator<< : pretty-prints the alert for console / log output.
    friend std::ostream& operator<<(std::ostream& os, const Alert& alert);

    // ── Utility ─────────────────────────────────────────────────────────────
    [[nodiscard]] std::string toString() const;
};

// Forward declaration of AlertManager for AlertRule
class AlertManager;

// ===== OOP Strategy Pattern: Alert Rule Interface =====
class AlertRule {
public:
    virtual ~AlertRule() = default;
    virtual void evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) = 0;
};

class EngineTempRule : public AlertRule {
public:
    void evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) override;
};

class BatteryVoltageRule : public AlertRule {
public:
    void evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) override;
};

class SpeedRule : public AlertRule {
public:
    void evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) override;
};

class TirePressureRule : public AlertRule {
public:
    void evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) override;
};

class DoorRule : public AlertRule {
public:
    void evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) override;
};

class SeatbeltRule : public AlertRule {
public:
    void evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) override;
};

// ===== AlertManager Class =====
// Owns a bounded deque of active alerts and an unbounded history vector.
// Provides sensor-evaluation logic with configurable thresholds and
// lambda-based filtering for flexible queries.
class AlertManager {
private:
    std::deque<Alert> activeAlerts_;       // Rolling window of recent alerts
    std::vector<Alert> alertHistory_;      // Full historical record
    size_t maxActiveAlerts_;               // Cap for the active deque
    std::vector<std::unique_ptr<AlertRule>> rules_; // Extensible dynamic rule strategies

    // C++17 pre-indexed fast array mapping for O(1) checks during evaluation loop
    bool activeAlertsByType_[6] = {false};

    // Dynamic Active Driver Profile
    DriverProfile activeProfile_;

    // ── Threshold constants ─────────────────────────────────────────────────
    // constexpr so they are embedded at compile time — zero runtime cost.
    static constexpr double ENGINE_TEMP_THRESHOLD      = 110.0;  // °C
    static constexpr double BATTERY_LOW_THRESHOLD      = 10.0;   // Volts
    static constexpr double TIRE_PRESSURE_LOW_THRESHOLD = 25.0;  // PSI
    static constexpr double SPEED_HIGH_THRESHOLD       = 120.0;  // km/h
    static constexpr double SPEED_DOOR_THRESHOLD       = 10.0;   // km/h — door-open check

public:
    // Dynamic profile accessors
    void setActiveProfile(const DriverProfile& profile);
    [[nodiscard]] const DriverProfile& getActiveProfile() const;

    // Determine the overall vehicle status ("NORMAL", "WARNING", "CRITICAL")
    [[nodiscard]] std::string getSystemStatus() const;

    // maxActive: how many alerts to keep in the active deque before eviction.
    explicit AlertManager(size_t maxActive = 50);

    // Evaluate every sensor in the vector and generate alerts as needed.
    void evaluateSensors(const std::vector<std::unique_ptr<Sensor>>& sensors);

    // Dynamic context-based prioritization
    void prioritizeAlerts(double speedKmh);

    // ── Alert management ────────────────────────────────────────────────────
    void addAlert(Alert alert);                          // Takes by value → moved in
    void removeAlert(const std::string& source);         // Remove an active alert by source name
    void clearExpiredAlerts(size_t keepCount = 20);       // Trim active deque
    
    // Fast O(1) alert checker for Strategy rules
    [[nodiscard]] bool isAlertActiveByType(SensorType type) const;
    void setAlertActiveByType(SensorType type, bool active);

    // ── Accessors ───────────────────────────────────────────────────────────
    [[nodiscard]] const std::deque<Alert>& getActiveAlerts() const;
    [[nodiscard]] const std::vector<Alert>& getAlertHistory() const;

    // ── Lambda-based filtering ──────────────────────────────────────────────
    [[nodiscard]] std::vector<Alert> filterAlerts(
        std::function<bool(const Alert&)> predicate) const;
    [[nodiscard]] std::vector<Alert> filterByseverity(AlertSeverity severity) const;
    [[nodiscard]] std::vector<Alert> filterBySource(const std::string& source) const;

    // ── Statistics ──────────────────────────────────────────────────────────
    [[nodiscard]] size_t getActiveAlertCount() const;
    [[nodiscard]] size_t getTotalAlertCount() const;
};
