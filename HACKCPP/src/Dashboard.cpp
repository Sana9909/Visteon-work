// Dashboard.cpp — Complete implementation of SensorStats, VehicleStatistics,
// and the Dashboard display renderer.
//
// Design notes:
//   - All output goes through an std::ostream& parameter so unit tests can
//     capture output via std::ostringstream instead of printing to the console.
//   - displayCount_ is mutable so the logically-const display() can track
//     how many times the dashboard has been refreshed.
//   - We use box-drawing characters (═, ║, ╔, ╗, ╚, ╝) for visual appeal
//     in terminals that support UTF-8.  Falls back gracefully on others.
//   - SensorStats uses numeric_limits to initialise min/max so that the
//     first real reading always replaces the sentinel values.
//   - ANSI colour codes are used for colourful terminal output.  The ANSI
//     sequences are also written into .txt log files, so any ANSI-aware
//     viewer (cat, less -R, VS Code terminal) will show the colours there too.

#include "Dashboard.hpp"
#include "Sensor.hpp"
#include "Alert.hpp"
#include "CanBus.hpp"
#include "DtcManager.hpp"
#include "EcuMonitor.hpp"
#include "OtaSimulator.hpp"

#include <limits>
#include <chrono>
#include <ctime>

// ═══════════════════════════════════════════════════════════════════════════
// ANSI colour helpers
// ═══════════════════════════════════════════════════════════════════════════

namespace Color {
    // Reset
    constexpr const char* RESET   = "\033[0m";

    // Regular text colours
    constexpr const char* BLACK   = "\033[30m";
    constexpr const char* RED     = "\033[31m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* BLUE    = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* WHITE   = "\033[37m";

    // Bright / bold variants
    constexpr const char* BRIGHT_RED     = "\033[1;31m";
    constexpr const char* BRIGHT_GREEN   = "\033[1;32m";
    constexpr const char* BRIGHT_YELLOW  = "\033[1;33m";
    constexpr const char* BRIGHT_BLUE    = "\033[1;34m";
    constexpr const char* BRIGHT_MAGENTA = "\033[1;35m";
    constexpr const char* BRIGHT_CYAN    = "\033[1;36m";
    constexpr const char* BRIGHT_WHITE   = "\033[1;37m";

    // Background colours
    constexpr const char* BG_BLUE    = "\033[44m";
    constexpr const char* BG_CYAN    = "\033[46m";
    constexpr const char* BG_GREEN   = "\033[42m";
    constexpr const char* BG_RED     = "\033[41m";
    constexpr const char* BG_MAGENTA = "\033[45m";
    constexpr const char* BG_YELLOW  = "\033[43m";
    constexpr const char* BG_BLACK   = "\033[40m";

    // Bold
    constexpr const char* BOLD   = "\033[1m";
    constexpr const char* DIM    = "\033[2m";
    constexpr const char* BLINK  = "\033[5m";
}

// ═══════════════════════════════════════════════════════════════════════════
// SensorStats implementation
// ═══════════════════════════════════════════════════════════════════════════

SensorStats::SensorStats()
    : min(std::numeric_limits<double>::max()),
      max(std::numeric_limits<double>::lowest()),
      sum(0.0),
      count(0) {
}

double SensorStats::average() const {
    if (count == 0) return 0.0;
    return sum / static_cast<double>(count);
}

void SensorStats::update(double value) {
    if (value < min) min = value;
    if (value > max) max = value;
    sum += value;
    ++count;
}

void SensorStats::reset() {
    min = std::numeric_limits<double>::max();
    max = std::numeric_limits<double>::lowest();
    sum = 0.0;
    count = 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// VehicleStatistics implementation
// ═══════════════════════════════════════════════════════════════════════════

void VehicleStatistics::update(const std::string& sensorName, double value) {
    stats_[sensorName].update(value);
}

SensorStats VehicleStatistics::getStats(const std::string& sensorName) const {
    auto it = stats_.find(sensorName);
    if (it != stats_.end()) return it->second;
    return SensorStats{};
}

const std::map<std::string, SensorStats>& VehicleStatistics::getAllStats() const {
    return stats_;
}

void VehicleStatistics::reset() {
    stats_.clear();
}

// Colour-coded statistics table
std::ostream& operator<<(std::ostream& os, const VehicleStatistics& stats) {
    const auto& allStats = stats.getAllStats();

    if (allStats.empty()) {
        os << Color::DIM << "  No statistics available yet." << Color::RESET << std::endl;
        return os;
    }

    // Table header — cyan + bold
    os << Color::BRIGHT_CYAN
       << "  " << std::left  << std::setw(22) << "Sensor"
       << std::right << std::setw(10) << "Min"
       << std::setw(10) << "Max"
       << std::setw(10) << "Avg"
       << std::setw(8)  << "Count"
       << Color::RESET << std::endl;

    os << Color::CYAN << "  " << std::string(60, '-') << Color::RESET << std::endl;

    // Alternate row colours: white / bright white for readability
    bool alternateRow = false;
    for (const auto& [name, s] : allStats) {
        const char* rowColor = alternateRow ? Color::BRIGHT_WHITE : Color::WHITE;
        os << rowColor
           << "  " << std::left  << std::setw(22) << name
           << std::right << std::fixed << std::setprecision(1)
           << Color::BRIGHT_GREEN  << std::setw(10) << (s.count > 0 ? s.min : 0.0)
           << Color::BRIGHT_RED    << std::setw(10) << (s.count > 0 ? s.max : 0.0)
           << Color::BRIGHT_YELLOW << std::setw(10) << s.average()
           << Color::CYAN          << std::setw(8)  << s.count
           << Color::RESET << std::endl;
        alternateRow = !alternateRow;
    }

    return os;
}

// ═══════════════════════════════════════════════════════════════════════════
// Dashboard implementation
// ═══════════════════════════════════════════════════════════════════════════

Dashboard::Dashboard() : displayCount_(0) {}

// ---------------------------------------------------------------------------
// Formatting helpers
// ---------------------------------------------------------------------------

std::string Dashboard::horizontalLine(int width) {
    return std::string(static_cast<size_t>(width), '=');
}

std::string Dashboard::centeredText(const std::string& text, int width) {
    int textLen = static_cast<int>(text.length());
    if (textLen >= width) return text;
    int leftPad  = (width - textLen) / 2;
    int rightPad = width - textLen - leftPad;
    return std::string(static_cast<size_t>(leftPad),  ' ')
         + text
         + std::string(static_cast<size_t>(rightPad), ' ');
}

// ---------------------------------------------------------------------------
// Helper: get current timestamp
// ---------------------------------------------------------------------------
static std::string getCurrentTimestamp() {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ---------------------------------------------------------------------------
// Main display
// ---------------------------------------------------------------------------
void Dashboard::display(const std::vector<std::unique_ptr<Sensor>>& sensors,
                        const AlertManager& alertManager,
                        const VehicleStatistics& stats,
                        const std::vector<double>& speedHistory,
                        std::ostream& os) const {
    for (int i = 0; i < 50; ++i) os << std::endl;
    ++displayCount_;

    displayHeader(&alertManager, os);
    displaySensors(sensors, os);
    displaySparklineGraph(speedHistory, os);
    displayAlerts(alertManager, os);
    displayDtcCodes(os);
    displayEcuStatus(os);
    displayOtaStatus(os);
    displayStatistics(stats, os);
    displayAlertHistory(alertManager, os);
    displayFooter(os);
}

// ---------------------------------------------------------------------------
// Header — blue background, bold white title, cyan timestamp
// ---------------------------------------------------------------------------
void Dashboard::displayHeader(const AlertManager* alertManager, std::ostream& os) const {
    std::string line = horizontalLine(60);

    // Top border — bright blue
    os << Color::BRIGHT_BLUE << line << Color::RESET << std::endl;

    // Title — white text on blue background, bold
    os << Color::BG_BLUE << Color::BOLD << Color::BRIGHT_WHITE
       << centeredText("SMART VEHICLE DASHBOARD", 60)
       << Color::RESET << std::endl;

    // Subtitle — cyan
    os << Color::CYAN
       << centeredText("Adaptive Smart Cabin & Vehicle Health Monitor", 60)
       << Color::RESET << std::endl;

    // Separator
    os << Color::BRIGHT_BLUE << line << Color::RESET << std::endl;

    // Timestamp — yellow
    os << Color::BRIGHT_YELLOW
       << centeredText("Timestamp: " + getCurrentTimestamp(), 60)
       << Color::RESET << std::endl;

    // Bottom border of header
    os << Color::BRIGHT_BLUE << line << Color::RESET << std::endl;

    // Driver Profile Comfort Panel — green text/background
    if (alertManager) {
        const auto& profile = alertManager->getActiveProfile();
        os << Color::BG_GREEN << Color::BOLD << Color::BLACK
           << " [ ACTIVE DRIVER COMFORT PROFILE ] "
           << Color::RESET << std::endl;
        os << Color::BRIGHT_GREEN << "  Driver Name   : " << Color::BRIGHT_WHITE << profile.name << std::endl;
        os << Color::BRIGHT_GREEN << "  Seat Position : " << Color::BRIGHT_WHITE << std::fixed << std::setprecision(1) << profile.seatHeightCm << " cm"
           << Color::BRIGHT_GREEN << "  |  Mirror Angle: " << Color::BRIGHT_WHITE << profile.mirrorAngleDeg << "°" << std::endl;
        os << Color::BRIGHT_GREEN << "  HVAC Target   : " << Color::BRIGHT_WHITE << profile.temperaturePreferenceC << " °C" << std::endl;
        os << Color::BRIGHT_GREEN << "  Safety Limits : " 
           << Color::BRIGHT_YELLOW << "Speed > " << static_cast<int>(profile.maxSpeedThresholdKmh) << " km/h | "
           << "Battery < " << profile.lowBatteryThresholdV << " V | "
           << "Engine > " << static_cast<int>(profile.engineTempThresholdC) << " °C" << std::endl;
        os << Color::BRIGHT_BLUE << line << Color::RESET << std::endl;
    }

    os << std::endl;
}

// ---------------------------------------------------------------------------
// Sensors — section label in bright cyan; values in green
// ---------------------------------------------------------------------------
void Dashboard::displaySensors(const std::vector<std::unique_ptr<Sensor>>& sensors,
                               std::ostream& os) const {
    // Section heading
    os << Color::BG_CYAN << Color::BOLD << Color::BLACK
       << " [ SENSOR READINGS ] "
       << Color::RESET << std::endl;
    os << Color::CYAN << " " << std::string(58, '-') << Color::RESET << std::endl;

    if (sensors.empty()) {
        os << Color::DIM << "  No sensors registered." << Color::RESET << std::endl;
        os << std::endl;
        return;
    }

    // Column headers — bold white
    os << Color::BOLD << Color::WHITE
       << "  " << std::left  << std::setw(24) << "Sensor Name"
       << std::right << std::setw(15) << "Value"
       << std::setw(15) << "Unit"
       << Color::RESET << std::endl;
    os << Color::CYAN << "  " << std::string(54, '-') << Color::RESET << std::endl;

    // Rows
    for (const auto& sensor : sensors) {
        if (sensor) {
            // Colour the value green; unit in magenta
            os << Color::BRIGHT_WHITE << "  " << std::left << std::setw(24) << sensor->getName()
               << Color::BRIGHT_GREEN  << std::right << std::setw(15) << sensor->getValueString()
               << Color::BRIGHT_MAGENTA << std::setw(15) << sensor->getUnit()
               << Color::RESET << std::endl;
        }
    }

    os << std::endl;
}

// ---------------------------------------------------------------------------
// Alerts — colour-coded by severity
// ---------------------------------------------------------------------------
void Dashboard::displayAlerts(const AlertManager& alertManager,
                              std::ostream& os) const {
    // Section heading
    os << Color::BG_YELLOW << Color::BOLD << Color::BLACK
       << " [ ACTIVE ALERTS ]  (Count: "
       << alertManager.getActiveAlertCount() << ") "
       << Color::RESET << std::endl;
    os << Color::YELLOW << " " << std::string(58, '-') << Color::RESET << std::endl;

    const auto& activeAlerts = alertManager.getActiveAlerts();

    if (activeAlerts.empty()) {
        os << Color::BRIGHT_GREEN << "  No active alerts. All systems nominal."
           << Color::RESET << std::endl;
        os << std::endl;
        return;
    }

    for (const auto& alert : activeAlerts) {
        std::string marker;
        const char* severityColor;

        switch (alert.getSeverity()) {
            case AlertSeverity::CRITICAL:
                marker        = "[!!!]";
                severityColor = Color::BRIGHT_RED;
                break;
            case AlertSeverity::HIGH:
                marker        = "[!! ]";
                severityColor = Color::BRIGHT_RED;
                break;
            case AlertSeverity::WARNING:
                marker        = "[ ! ]";
                severityColor = Color::BRIGHT_YELLOW;
                break;
            case AlertSeverity::MEDIUM:
                marker        = "[ ~ ]";
                severityColor = Color::BRIGHT_YELLOW;
                break;
            case AlertSeverity::LOW:
                marker        = "[ . ]";
                severityColor = Color::BRIGHT_CYAN;
                break;
            case AlertSeverity::INFO:
            default:
                marker        = "[ i ]";
                severityColor = Color::BRIGHT_CYAN;
                break;
        }

        os << severityColor << Color::BOLD
           << "  " << marker << " "
           << std::left << std::setw(10) << severityToString(alert.getSeverity())
           << Color::RESET
           << Color::WHITE
           << " | " << std::setw(18) << alert.getSource()
           << " | " << alert.getMessage()
           << Color::RESET << std::endl;
    }

    os << std::endl;
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------
void Dashboard::displayStatistics(const VehicleStatistics& stats,
                                  std::ostream& os) const {
    os << Color::BG_BLUE << Color::BOLD << Color::BRIGHT_WHITE
       << " [ SENSOR STATISTICS ] "
       << Color::RESET << std::endl;
    os << Color::BLUE << " " << std::string(58, '-') << Color::RESET << std::endl;
    os << stats;
    os << std::endl;
}

// ---------------------------------------------------------------------------
// Alert History — dim for older entries, coloured severity badges
// ---------------------------------------------------------------------------
void Dashboard::displayAlertHistory(const AlertManager& alertManager,
                                    std::ostream& os) const {
    os << Color::BG_MAGENTA << Color::BOLD << Color::WHITE
       << " [ ALERT HISTORY ]  (Total: "
       << alertManager.getTotalAlertCount() << ") "
       << Color::RESET << std::endl;
    os << Color::MAGENTA << " " << std::string(58, '-') << Color::RESET << std::endl;

    const auto& history = alertManager.getAlertHistory();

    if (history.empty()) {
        os << Color::DIM << "  No alert history." << Color::RESET << std::endl;
        os << std::endl;
        return;
    }

    const size_t maxDisplay = 10;
    size_t startIdx = history.size() > maxDisplay ? history.size() - maxDisplay : 0;

    for (size_t i = startIdx; i < history.size(); ++i) {
        const auto& alert = history[i];

        const char* severityColor;
        switch (alert.getSeverity()) {
            case AlertSeverity::CRITICAL: severityColor = Color::BRIGHT_RED;     break;
            case AlertSeverity::HIGH:     severityColor = Color::BRIGHT_RED;     break;
            case AlertSeverity::WARNING:  severityColor = Color::BRIGHT_YELLOW;  break;
            case AlertSeverity::MEDIUM:   severityColor = Color::BRIGHT_YELLOW;  break;
            default:                      severityColor = Color::BRIGHT_CYAN;    break;
        }

        os << Color::DIM << Color::WHITE << "  [" << alert.getTimestampString() << "] "
           << Color::RESET << severityColor << Color::BOLD
           << std::left << std::setw(10) << severityToString(alert.getSeverity())
           << Color::RESET
           << Color::WHITE << " " << alert.getSource()
           << ": " << alert.getMessage()
           << Color::RESET << std::endl;
    }

    os << std::endl;
}

// ---------------------------------------------------------------------------
// Footer — bright blue border, cyan refresh counter
// ---------------------------------------------------------------------------
void Dashboard::displayFooter(std::ostream& os) const {
    std::string line = horizontalLine(60);
    os << Color::BRIGHT_BLUE << line << Color::RESET << std::endl;
    os << Color::BRIGHT_CYAN << "  Display Refresh #" << displayCount_ << Color::RESET << std::endl;
    os << Color::BRIGHT_BLUE << line << Color::RESET << std::endl;
}

// Stream insertion for Dashboard — prints a human-readable one-liner
std::ostream& operator<<(std::ostream& os, const Dashboard& dashboard) {
    os << Color::CYAN << "[Dashboard] Refreshes: " << dashboard.displayCount_ << Color::RESET;
    return os;
}

// ---------------------------------------------------------------------------
// ECU Health & CAN Bus Panel
// ---------------------------------------------------------------------------
void Dashboard::displayEcuStatus(std::ostream& os) const {
    os << Color::BG_BLUE << Color::BOLD << Color::BRIGHT_WHITE
       << " [ ECU SYSTEM HEALTH & CAN-BUS TELEMETRY ] "
       << Color::RESET << std::endl;
    os << Color::BLUE << " " << std::string(58, '-') << Color::RESET << std::endl;

    auto reports = EcuMonitor::getInstance().getHealthReports();
    
    // Column Headers
    os << Color::BOLD << Color::WHITE
       << "  " << std::left << std::setw(8) << "ECU"
       << std::right << std::setw(12) << "State"
       << std::setw(12) << "CPU Load"
       << std::setw(12) << "Memory"
       << std::setw(12) << "Net Rate"
       << Color::RESET << std::endl;
    os << Color::BLUE << "  " << std::string(54, '-') << Color::RESET << std::endl;

    for (const auto& [name, r] : reports) {
        const char* stateColor = Color::BRIGHT_GREEN;
        if (r.state == EcuState::OFFLINE) stateColor = Color::BRIGHT_RED;
        else if (r.state == EcuState::UPDATING || r.state == EcuState::BOOTING) stateColor = Color::BRIGHT_YELLOW;

        os << Color::BRIGHT_WHITE << "  " << std::left << std::setw(8) << name
           << stateColor << std::right << std::setw(12) << EcuMonitor::stateToString(r.state)
           << Color::BRIGHT_YELLOW << std::setw(10) << std::fixed << std::setprecision(1) << r.cpuLoad << " %"
           << Color::BRIGHT_CYAN << std::setw(10) << r.memoryAllocated << " KB"
           << Color::BRIGHT_MAGENTA << std::setw(10) << r.rxPacketSuccessRate << " %"
           << Color::RESET << std::endl;
    }

    // Print CAN Bus status metrics
    auto& can = CanBus::getInstance();
    os << Color::BLUE << "  " << std::string(54, '-') << Color::RESET << std::endl;
    os << Color::BRIGHT_CYAN << "  CAN Bus Load : " << Color::BRIGHT_WHITE << std::fixed << std::setprecision(1) << can.getBusLoad() << " %"
       << Color::BRIGHT_CYAN << " | Tx/Rx Frames: " << Color::BRIGHT_WHITE << can.getTxCount() << "/" << can.getRxCount()
       << Color::BRIGHT_CYAN << " | Error Frames: " 
       << (can.getErrorFrames() > 0 ? Color::BRIGHT_RED : Color::BRIGHT_GREEN) << can.getErrorFrames()
       << Color::RESET << std::endl;

    os << std::endl;
}

// ---------------------------------------------------------------------------
// OBD-II Active DTC Codes Panel
// ---------------------------------------------------------------------------
void Dashboard::displayDtcCodes(std::ostream& os) const {
    os << Color::BG_RED << Color::BOLD << Color::BRIGHT_WHITE
       << " [ OBD-II ON-BOARD DIAGNOSTICS - ACTIVE DTC CODES ] "
       << Color::RESET << std::endl;
    os << Color::RED << " " << std::string(58, '-') << Color::RESET << std::endl;

    auto dtcs = DtcManager::getInstance().getActiveDtcs();
    if (dtcs.empty()) {
        os << Color::BRIGHT_GREEN << "  No active Diagnostic Trouble Codes detected (MIL OFF)." << Color::RESET << std::endl;
    } else {
        os << Color::BOLD << Color::WHITE
           << "  " << std::left << std::setw(8) << "Code"
           << std::setw(14) << "Status"
           << "Description & Captured Freeze Frame"
           << Color::RESET << std::endl;
        os << Color::RED << "  " << std::string(54, '-') << Color::RESET << std::endl;

        for (const auto& d : dtcs) {
            const char* statusColor = (d.status == DtcStatus::CONFIRMED) ? Color::BRIGHT_RED : Color::BRIGHT_YELLOW;
            std::string statusStr = (d.status == DtcStatus::CONFIRMED) ? "CONFIRMED" : "PENDING";
            
            os << Color::BRIGHT_RED << "  " << std::left << std::setw(8) << d.code
               << statusColor << std::setw(14) << statusStr
               << Color::BRIGHT_WHITE << d.description << "\n"
               << Color::DIM << Color::WHITE 
               << "    Freeze Frame -> [" << d.freezeFrame.timestamp << "] Speed: " << d.freezeFrame.speedKmh << " km/h"
               << " | Temp: " << d.freezeFrame.engineTempC << " °C | Volt: " << d.freezeFrame.batteryVoltageV << " V"
               << Color::RESET << std::endl;
        }
    }
    os << std::endl;
}

// ---------------------------------------------------------------------------
// Sparkline/Graph Panel
// ---------------------------------------------------------------------------
void Dashboard::displaySparklineGraph(const std::vector<double>& speedHistory, std::ostream& os) const {
    os << Color::BG_GREEN << Color::BOLD << Color::BLACK
       << " [ ACCELERATION & VEHICLE SPEED HISTORICAL GRAPH ] "
       << Color::RESET << std::endl;
    os << Color::GREEN << " " << std::string(58, '-') << Color::RESET << std::endl;

    if (speedHistory.empty()) {
        os << Color::DIM << "  No historical speed metrics captured yet." << Color::RESET << std::endl;
    } else {
        // Draw last 8 records as horizontal bars
        size_t start = speedHistory.size() > 8 ? speedHistory.size() - 8 : 0;
        for (size_t i = start; i < speedHistory.size(); ++i) {
            double s = speedHistory[i];
            int numBlocks = static_cast<int>(s / 5.0); // 1 block per 5 km/h
            if (numBlocks < 1) numBlocks = 1;
            if (numBlocks > 30) numBlocks = 30;

            std::string bar = std::string(static_cast<size_t>(numBlocks), '#');
            std::string dots = std::string(static_cast<size_t>(30 - numBlocks), ' ');

            os << Color::BRIGHT_WHITE << "  t-" << std::setw(2) << (speedHistory.size() - 1 - i) << " : "
               << Color::BRIGHT_GREEN << "[" << bar << Color::DIM << Color::WHITE << dots << Color::RESET << Color::BRIGHT_GREEN << "]"
               << Color::BRIGHT_WHITE << " " << std::fixed << std::setprecision(1) << std::setw(5) << s << " km/h"
               << Color::RESET << std::endl;
        }
    }
    os << std::endl;
}

// ---------------------------------------------------------------------------
// OTA Update Progress Panel
// ---------------------------------------------------------------------------
void Dashboard::displayOtaStatus(std::ostream& os) const {
    auto& ota = OtaSimulator::getInstance();
    auto state = ota.getState();
    
    if (state == OtaState::IDLE) {
        return;
    }

    os << Color::BG_MAGENTA << Color::BOLD << Color::BRIGHT_WHITE
       << " [ OTA FIRMWARE DEPLOYMENT PIPELINE ] "
       << Color::RESET << std::endl;
    os << Color::MAGENTA << " " << std::string(58, '-') << Color::RESET << std::endl;

    int progress = ota.getProgress();
    int filledBlocks = progress / 5; // 20 blocks total
    if (filledBlocks < 0) filledBlocks = 0;
    if (filledBlocks > 20) filledBlocks = 20;

    std::string bar = std::string(static_cast<size_t>(filledBlocks), '=');
    std::string spaces = std::string(static_cast<size_t>(20 - filledBlocks), ' ');

    const char* statusColor = Color::BRIGHT_WHITE;
    if (state == OtaState::COMPLETED) statusColor = Color::BRIGHT_GREEN;
    else if (state == OtaState::FAILED) statusColor = Color::BRIGHT_RED;
    else statusColor = Color::BRIGHT_YELLOW;

    os << Color::BRIGHT_WHITE << "  Firmware Status : " << statusColor << ota.getStateString() << "\n"
       << Color::BRIGHT_WHITE << "  Current Version : " << Color::BRIGHT_CYAN << ota.getVersion() << "\n"
       << Color::BRIGHT_WHITE << "  Flashes Progress: "
       << Color::BRIGHT_MAGENTA << "[" << bar << Color::WHITE << spaces << "]"
       << Color::BRIGHT_WHITE << " " << progress << " %\n"
       << Color::RESET << std::endl;
}

void Dashboard::renderSystem(const std::vector<std::unique_ptr<Sensor>>& sensors,
                             const AlertManager& alertManager,
                             const VehicleStatistics& stats,
                             const std::vector<double>& speedHistory,
                             std::ostream& os) const
{
    // Clear the console for a fresh frame
    os << "\033[2J\033[H" << std::flush;

    // Render the full dashboard to the stream
    display(sensors, alertManager, stats, speedHistory, os);

    // Reminder for the user
    os << "\n  [P] Switch Profile | [O] Sensor Override | [R] Reset Stats | [Q] Quit\n";
    os << "  Press ENTER to open Interactive Command Menu..." << std::flush;
}
