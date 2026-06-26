// ============================================================================
// Alert.cpp — Full implementation of Alert and AlertManager
// ============================================================================
// Design notes:
//   • Alert implements the Rule of Five so it is safely copyable and movable
//     inside STL containers (deque, vector).  The static alertCount_ tracks
//     the number of live Alert objects for diagnostic purposes.
//   • AlertManager::evaluateSensors performs both single-sensor threshold
//     checks AND cross-sensor logic (door-open-while-moving, seatbelt check).
//     It first locates the speed sensor with std::find_if so the O(n) scan
//     happens only once per evaluation cycle.
//   • Lambda-based filtering (filterAlerts) lets callers compose arbitrary
//     predicates without AlertManager knowing the query semantics.
// ============================================================================

#include "Alert.hpp"
#include "Sensor.hpp"   // Full Sensor definition needed for getValue(), getName(), getType()

#include <utility>      // std::move, std::exchange
#include <ctime>        // std::localtime, std::put_time

// ============================================================================
// Free function: severityToString
// ============================================================================
std::string severityToString(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::INFO:     return "INFO";
        case AlertSeverity::LOW:      return "LOW";
        case AlertSeverity::MEDIUM:   return "MEDIUM";
        case AlertSeverity::HIGH:     return "HIGH";
        case AlertSeverity::WARNING:  return "WARNING";
        case AlertSeverity::CRITICAL: return "CRITICAL";
        default:                      return "UNKNOWN";
    }
}

// ============================================================================
// Static member initialization
// ============================================================================
int Alert::alertCount_ = 0;

// ============================================================================
// Alert — Constructor
// ============================================================================
Alert::Alert(AlertSeverity severity, const std::string& message, const std::string& source)
    : severity_{severity}
    , message_{message}
    , source_{source}
    , timestamp_{std::chrono::system_clock::now()}   // Capture creation time
{
    switch (severity) {
        case AlertSeverity::INFO:     priorityScore_ = 10.0; break;
        case AlertSeverity::LOW:      priorityScore_ = 30.0; break;
        case AlertSeverity::MEDIUM:   priorityScore_ = 50.0; break;
        case AlertSeverity::HIGH:     priorityScore_ = 70.0; break;
        case AlertSeverity::WARNING:  priorityScore_ = 80.0; break;
        case AlertSeverity::CRITICAL: priorityScore_ = 95.0; break;
        default:                      priorityScore_ = 0.0;  break;
    }
    ++alertCount_;   // Track live object count
}

// ============================================================================
// Alert — Copy Constructor
// ============================================================================
// Deep-copies every member.  Strings are value-copied so the new Alert is
// fully independent of the original.
Alert::Alert(const Alert& other)
    : severity_{other.severity_}
    , message_{other.message_}
    , source_{other.source_}
    , timestamp_{other.timestamp_}
    , priorityScore_{other.priorityScore_}
{
    ++alertCount_;
}

// ============================================================================
// Alert — Move Constructor
// ============================================================================
// Steals the guts of `other`'s strings, leaving them in a valid-but-empty
// state.  Severity and timestamp are trivially copyable so we just copy them.
Alert::Alert(Alert&& other) noexcept
    : severity_{other.severity_}
    , message_{std::move(other.message_)}
    , source_{std::move(other.source_)}
    , timestamp_{other.timestamp_}
    , priorityScore_{other.priorityScore_}
{
    ++alertCount_;
    // `other` is now in a moved-from state — its strings are empty but the
    // object remains destructible and assignable.
}

// ============================================================================
// Alert — Copy Assignment
// ============================================================================
Alert& Alert::operator=(const Alert& other) {
    // Self-assignment guard — prevents needless work and potential issues
    // if the class ever manages raw resources.
    if (this != &other) {
        severity_  = other.severity_;
        message_   = other.message_;
        source_    = other.source_;
        timestamp_ = other.timestamp_;
        priorityScore_ = other.priorityScore_;
    }
    // Note: alertCount_ is NOT modified because no new object is created or
    // destroyed — we are merely overwriting an existing one.
    return *this;
}

// ============================================================================
// Alert — Move Assignment
// ============================================================================
Alert& Alert::operator=(Alert&& other) noexcept {
    if (this != &other) {
        severity_  = other.severity_;
        message_   = std::move(other.message_);
        source_    = std::move(other.source_);
        timestamp_ = other.timestamp_;
        priorityScore_ = other.priorityScore_;
    }
    return *this;
}

// ============================================================================
// Alert — Destructor
// ============================================================================
Alert::~Alert() {
    --alertCount_;   // One fewer live Alert in the process
}

// ============================================================================
// Alert — Getters
// ============================================================================
AlertSeverity Alert::getSeverity() const {
    return severity_;
}

const std::string& Alert::getMessage() const {
    return message_;
}

const std::string& Alert::getSource() const {
    return source_;
}

std::chrono::system_clock::time_point Alert::getTimestamp() const {
    return timestamp_;
}

double Alert::getPriorityScore() const {
    return priorityScore_;
}

void Alert::setPriorityScore(double score) {
    priorityScore_ = score;
}

int Alert::getAlertCount() {
    return alertCount_;
}

// ============================================================================
// Alert — getTimestampString
// ============================================================================
// Formats the internal time_point as "YYYY-MM-DD HH:MM:SS" using the local
// timezone.  Thread-safe note: std::localtime is NOT thread-safe on all
// platforms (it returns a pointer to a static buffer).  For production code
// consider localtime_r / localtime_s.  Acceptable here for a demonstration.
std::string Alert::getTimestampString() const {
    auto timeT = std::chrono::system_clock::to_time_t(timestamp_);
    std::tm tmBuf{};

    // Use the safer platform-specific variant when available.
#if defined(_MSC_VER) || defined(_WIN32)
    localtime_s(&tmBuf, &timeT);       // Windows: parameters are reversed
#else
    localtime_r(&timeT, &tmBuf);       // POSIX
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ============================================================================
// Alert — operator<  (ordering by severity, CRITICAL first)
// ============================================================================
// We want CRITICAL to sort *before* WARNING *before* INFO.  Since the enum
// values are INFO=0, WARNING=1, CRITICAL=2, reversing the comparison achieves
// descending severity order.
bool Alert::operator<(const Alert& other) const {
    // Sort by dynamic priorityScore descending. If equal, fall back to severity.
    if (std::abs(priorityScore_ - other.priorityScore_) > 0.001) {
        return priorityScore_ > other.priorityScore_;
    }
    return static_cast<int>(severity_) > static_cast<int>(other.severity_);
}

// ============================================================================
// Alert — operator==  (equality by message + source)
// ============================================================================
// Two alerts are considered duplicates if they carry the same message from the
// same source, regardless of timestamp or severity.
bool Alert::operator==(const Alert& other) const {
    return (message_ == other.message_) && (source_ == other.source_);
}

// ============================================================================
// Alert — operator<<  (formatted stream output)
// ============================================================================
// Format: [INDICATOR] [SEVERITY] [TIMESTAMP] SOURCE: MESSAGE
//   [!!!] = CRITICAL    [!!] = WARNING    [i] = INFO
std::ostream& operator<<(std::ostream& os, const Alert& alert) {
    // Choose a visual severity indicator for quick scanning
    std::string indicator;
    switch (alert.severity_) {
        case AlertSeverity::CRITICAL: indicator = "[!!!]"; break;
        case AlertSeverity::HIGH:     indicator = "[!!]";  break;
        case AlertSeverity::WARNING:  indicator = "[!]";   break;
        case AlertSeverity::MEDIUM:   indicator = "[i]";   break;
        case AlertSeverity::LOW:      indicator = "[.]";   break;
        case AlertSeverity::INFO:     indicator = "[i]";   break;
        default:                      indicator = "[?]";   break;
    }

    os << indicator << " ["
       << severityToString(alert.severity_) << "] ["
       << alert.getTimestampString() << "] "
       << alert.source_ << ": "
       << alert.message_;

    return os;
}

// ============================================================================
// Alert — toString (convenience wrapper around operator<<)
// ============================================================================
std::string Alert::toString() const {
    std::ostringstream oss;
    oss << *this;        // Delegate to operator<<
    return oss.str();
}

// ============================================================================
// OOP Strategy Pattern concrete AlertRules implementation
// ============================================================================

void EngineTempRule::evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) {
    const Sensor* s = sensorMap[static_cast<size_t>(SensorType::ENGINE_TEMP)];
    if (s) {
        const double value = s->getValue();
        const auto& name = s->getName();
        const double engineTempThreshold = manager.getActiveProfile().engineTempThresholdC;
        if (value > engineTempThreshold) {
            if (!manager.isAlertActiveByType(SensorType::ENGINE_TEMP)) {
                manager.addAlert(Alert{
                    AlertSeverity::CRITICAL,
                    "ENGINE OVERHEAT - temperature at " + std::to_string(static_cast<int>(value)) + "°C",
                    name
                });
                manager.setAlertActiveByType(SensorType::ENGINE_TEMP, true);
            }
        } else {
            if (manager.isAlertActiveByType(SensorType::ENGINE_TEMP)) {
                manager.removeAlert(name);
                manager.setAlertActiveByType(SensorType::ENGINE_TEMP, false);
            }
        }
    }
}

void BatteryVoltageRule::evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) {
    const Sensor* s = sensorMap[static_cast<size_t>(SensorType::BATTERY_VOLTAGE)];
    if (s) {
        const double value = s->getValue();
        const auto& name = s->getName();
        const double batteryLowThreshold = manager.getActiveProfile().lowBatteryThresholdV;
        if (value < batteryLowThreshold) {
            if (!manager.isAlertActiveByType(SensorType::BATTERY_VOLTAGE)) {
                manager.addAlert(Alert{
                    AlertSeverity::HIGH,
                    "LOW BATTERY - voltage at " + std::to_string(static_cast<int>(value)) + "V",
                    name
                });
                manager.setAlertActiveByType(SensorType::BATTERY_VOLTAGE, true);
            }
        } else {
            if (manager.isAlertActiveByType(SensorType::BATTERY_VOLTAGE)) {
                manager.removeAlert(name);
                manager.setAlertActiveByType(SensorType::BATTERY_VOLTAGE, false);
            }
        }
    }
}

void SpeedRule::evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) {
    const Sensor* s = sensorMap[static_cast<size_t>(SensorType::SPEED)];
    if (s) {
        const double value = s->getValue();
        const auto& name = s->getName();
        const double speedHighThreshold = manager.getActiveProfile().maxSpeedThresholdKmh;
        if (value > speedHighThreshold) {
            if (!manager.isAlertActiveByType(SensorType::SPEED)) {
                manager.addAlert(Alert{
                    AlertSeverity::HIGH,
                    "OVERSPEED WARNING - current speed " + std::to_string(static_cast<int>(value)) + " km/h",
                    name
                });
                manager.setAlertActiveByType(SensorType::SPEED, true);
            }
        } else {
            if (manager.isAlertActiveByType(SensorType::SPEED)) {
                manager.removeAlert(name);
                manager.setAlertActiveByType(SensorType::SPEED, false);
            }
        }
    }
}

void TirePressureRule::evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) {
    const Sensor* s = sensorMap[static_cast<size_t>(SensorType::TIRE_PRESSURE)];
    if (s) {
        const double value = s->getValue();
        const auto& name = s->getName();
        constexpr double TIRE_PRESSURE_LOW_THRESHOLD = 25.0;
        if (value < TIRE_PRESSURE_LOW_THRESHOLD) {
            if (!manager.isAlertActiveByType(SensorType::TIRE_PRESSURE)) {
                manager.addAlert(Alert{
                    AlertSeverity::LOW,
                    "LOW TIRE PRESSURE - pressure at " + std::to_string(static_cast<int>(value)) + " PSI",
                    name
                });
                manager.setAlertActiveByType(SensorType::TIRE_PRESSURE, true);
            }
        } else {
            if (manager.isAlertActiveByType(SensorType::TIRE_PRESSURE)) {
                manager.removeAlert(name);
                manager.setAlertActiveByType(SensorType::TIRE_PRESSURE, false);
            }
        }
    }
}

void DoorRule::evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) {
    double speedValue = 0.0;
    const Sensor* speedS = sensorMap[static_cast<size_t>(SensorType::SPEED)];
    if (speedS) {
        speedValue = speedS->getValue();
    }

    const Sensor* doorS = sensorMap[static_cast<size_t>(SensorType::DOOR)];
    if (doorS) {
        const double value = doorS->getValue();
        const auto& name = doorS->getName();
        constexpr double SPEED_DOOR_THRESHOLD = 10.0;
        if (value == 1.0 && speedValue > SPEED_DOOR_THRESHOLD) {
            if (!manager.isAlertActiveByType(SensorType::DOOR)) {
                manager.addAlert(Alert{
                    AlertSeverity::CRITICAL,
                    "DOOR OPEN WHILE MOVING - speed is " + std::to_string(static_cast<int>(speedValue)) + " km/h",
                    name
                });
                manager.setAlertActiveByType(SensorType::DOOR, true);
            }
        } else {
            if (manager.isAlertActiveByType(SensorType::DOOR)) {
                manager.removeAlert(name);
                manager.setAlertActiveByType(SensorType::DOOR, false);
            }
        }
    }
}

void SeatbeltRule::evaluate(const std::array<const Sensor*, 6>& sensorMap, AlertManager& manager) {
    double speedValue = 0.0;
    const Sensor* speedS = sensorMap[static_cast<size_t>(SensorType::SPEED)];
    if (speedS) {
        speedValue = speedS->getValue();
    }

    const Sensor* seatbeltS = sensorMap[static_cast<size_t>(SensorType::SEATBELT)];
    if (seatbeltS) {
        const double value = seatbeltS->getValue();
        const auto& name = seatbeltS->getName();
        if (value == 0.0 && speedValue > 0.0) {
            if (!manager.isAlertActiveByType(SensorType::SEATBELT)) {
                manager.addAlert(Alert{
                    AlertSeverity::MEDIUM,
                    "SEATBELT UNLOCKED - vehicle is in motion",
                    name
                });
                manager.setAlertActiveByType(SensorType::SEATBELT, true);
            }
        } else {
            if (manager.isAlertActiveByType(SensorType::SEATBELT)) {
                manager.removeAlert(name);
                manager.setAlertActiveByType(SensorType::SEATBELT, false);
            }
        }
    }
}

// ============================================================================
// AlertManager — Constructor
// ============================================================================
AlertManager::AlertManager(size_t maxActive)
    : maxActiveAlerts_{maxActive}
{
    // Reserve a reasonable chunk of history to avoid early reallocations.
    alertHistory_.reserve(100);

    // Initialize extensibility rules (OOP Strategy Pattern)
    rules_.push_back(std::make_unique<EngineTempRule>());
    rules_.push_back(std::make_unique<BatteryVoltageRule>());
    rules_.push_back(std::make_unique<SpeedRule>());
    rules_.push_back(std::make_unique<TirePressureRule>());
    rules_.push_back(std::make_unique<DoorRule>());
    rules_.push_back(std::make_unique<SeatbeltRule>());
}

void AlertManager::setActiveProfile(const DriverProfile& profile) {
    activeProfile_ = profile;
}

const DriverProfile& AlertManager::getActiveProfile() const {
    return activeProfile_;
}

// ============================================================================
// AlertManager — getSystemStatus
// ============================================================================
std::string AlertManager::getSystemStatus() const {
    bool hasCritical = false;
    bool hasWarningOrHigh = false;

    for (const auto& a : activeAlerts_) {
        if (a.getSeverity() == AlertSeverity::CRITICAL) {
            hasCritical = true;
            break;
        } else if (a.getSeverity() == AlertSeverity::HIGH || a.getSeverity() == AlertSeverity::WARNING) {
            hasWarningOrHigh = true;
        }
    }
    if (hasCritical) return "CRITICAL";
    if (hasWarningOrHigh) return "WARNING";
    return "NORMAL";
}

// ============================================================================
// AlertManager — evaluateSensors
// ============================================================================
void AlertManager::evaluateSensors(const std::vector<std::unique_ptr<Sensor>>& sensors) {
    std::array<const Sensor*, 6> sensorMap{nullptr};
    for (const auto& s : sensors) {
        if (s) sensorMap[static_cast<size_t>(s->getType())] = s.get();
    }

    // Dynamic Strategy evaluation (OOP)
    for (const auto& rule : rules_) {
        rule->evaluate(sensorMap, *this);
    }
}

// ============================================================================
// AlertManager — removeAlert
// ============================================================================
void AlertManager::removeAlert(const std::string& source) {
    auto it = std::remove_if(activeAlerts_.begin(), activeAlerts_.end(),
        [&](const Alert& a) {
            return a.getSource() == source;
        });
    if (it != activeAlerts_.end()) {
        activeAlerts_.erase(it, activeAlerts_.end());
    }
}

// ============================================================================
// AlertManager — Fast O(1) alert checker for Strategy rules
// ============================================================================
bool AlertManager::isAlertActiveByType(SensorType type) const {
    return activeAlertsByType_[static_cast<size_t>(type)];
}

void AlertManager::setAlertActiveByType(SensorType type, bool active) {
    activeAlertsByType_[static_cast<size_t>(type)] = active;
}

// ============================================================================
// AlertManager — addAlert
// ============================================================================
// The parameter is taken by value so the caller can either copy or move into
// it.  Inside we move into the active deque (which takes ownership) and then
// copy into the history vector (which needs its own independent copy).
void AlertManager::addAlert(Alert alert) {
    // Deduplication check: if the same alert is already active, do not add it again
    auto it = std::find(activeAlerts_.begin(), activeAlerts_.end(), alert);
    if (it != activeAlerts_.end()) {
        return;
    }

    // Keep a copy for the permanent history BEFORE we move the alert.
    alertHistory_.push_back(alert);       // Copy into history

    // Move into the active (rolling) deque.
    activeAlerts_.push_back(std::move(alert));

    // Evict oldest alerts if the deque exceeds its capacity.
    while (activeAlerts_.size() > maxActiveAlerts_) {
        activeAlerts_.pop_front();        // FIFO eviction
    }
}

// ============================================================================
// AlertManager — clearExpiredAlerts
// ============================================================================
// Trims the active deque down to the last `keepCount` entries.  Older alerts
// remain in alertHistory_ for auditing.
void AlertManager::clearExpiredAlerts(size_t keepCount) {
    while (activeAlerts_.size() > keepCount) {
        activeAlerts_.pop_front();
    }
}

// ============================================================================
// AlertManager — Accessors
// ============================================================================
const std::deque<Alert>& AlertManager::getActiveAlerts() const {
    return activeAlerts_;
}

const std::vector<Alert>& AlertManager::getAlertHistory() const {
    return alertHistory_;
}

size_t AlertManager::getActiveAlertCount() const {
    return activeAlerts_.size();
}

size_t AlertManager::getTotalAlertCount() const {
    return alertHistory_.size();
}

// ============================================================================
// AlertManager — filterAlerts (generic predicate)
// ============================================================================
// Returns a new vector containing only alerts that satisfy the predicate.
// Uses std::copy_if for clarity and STL idiom compliance.
std::vector<Alert> AlertManager::filterAlerts(
        std::function<bool(const Alert&)> predicate) const {

    std::vector<Alert> result;
    // Reserve a reasonable amount — worst case is all alerts match.
    result.reserve(activeAlerts_.size());

    std::copy_if(activeAlerts_.begin(), activeAlerts_.end(),
                 std::back_inserter(result),
                 predicate);

    return result;
}

// ============================================================================
// AlertManager — filterByseverity
// ============================================================================
// Convenience wrapper: captures the desired severity in a lambda and
// delegates to filterAlerts.
std::vector<Alert> AlertManager::filterByseverity(AlertSeverity severity) const {
    return filterAlerts([severity](const Alert& a) {
        return a.getSeverity() == severity;
    });
}

// ============================================================================
// AlertManager — filterBySource
// ============================================================================
// Convenience wrapper: captures the source string by value and delegates.
std::vector<Alert> AlertManager::filterBySource(const std::string& source) const {
    return filterAlerts([source](const Alert& a) {
        return a.getSource() == source;
    });
}

// ============================================================================
// AlertManager — prioritizeAlerts (Dynamic Context Escalation)
// ============================================================================
void AlertManager::prioritizeAlerts(double speedKmh) {
    bool hasEngineOverheat = false;
    for (const auto& a : activeAlerts_) {
        if (a.getSource() == "Engine Temperature") {
            hasEngineOverheat = true;
            break;
        }
    }

    for (auto& a : activeAlerts_) {
        double score = 0.0;
        switch (a.getSeverity()) {
            case AlertSeverity::INFO:     score = 10.0; break;
            case AlertSeverity::LOW:      score = 30.0; break;
            case AlertSeverity::MEDIUM:   score = 50.0; break;
            case AlertSeverity::HIGH:     score = 70.0; break;
            case AlertSeverity::WARNING:  score = 80.0; break;
            case AlertSeverity::CRITICAL: score = 95.0; break;
            default:                      score = 0.0;  break;
        }

        // Context Escalation 1: Speed-related hazard elevations
        if (speedKmh > 100.0) {
            if (a.getSource() == "Seatbelt") {
                score += 40.0;
            } else if (a.getSource() == "Door") {
                score += 50.0;
            }
        }
        
        // Context Escalation 2: Engine temperature overheat at highway speeds
        if (hasEngineOverheat && speedKmh > 80.0 && a.getSource() == "Engine Temperature") {
            score += 25.0;
        }

        // Context Escalation 3: Multi-hazard synergistic warning elevation
        if (activeAlerts_.size() > 2) {
            score += 10.0;
        }

        if (score > 100.0) score = 100.0;
        a.setPriorityScore(score);
    }

    // Sort descending by priorityScore using our operator< overload
    std::sort(activeAlerts_.begin(), activeAlerts_.end());
}
