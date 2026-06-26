#pragma once

#include <string>
#include <vector>
#include <map>
#include <shared_mutex>

enum class EcuState {
    ONLINE,
    STANDBY,
    OFFLINE,
    BOOTING,
    UPDATING
};

struct EcuHealth {
    std::string name;
    double cpuLoad = 0.0;          // % CPU Usage
    uint32_t memoryAllocated = 0;   // Memory in KB
    double rxPacketSuccessRate = 0.0; // % RX success rate
    EcuState state = EcuState::ONLINE;
    uint32_t lastMessageTick = 0;
};

// ECU Health Monitoring Subsystem
class EcuMonitor {
public:
    static EcuMonitor& getInstance();

    // Periodic simulation update to fluctuate telemetry
    void updateTelemetry(uint32_t currentTick);

    // Dynamic state management
    void setEcuState(const std::string& ecuName, EcuState state);
    EcuState getEcuState(const std::string& ecuName) const;

    // Get snapshot of all ECU health reports
    std::map<std::string, EcuHealth> getHealthReports() const;

    // Simulate system reset
    void resetAllEcus();

    static std::string stateToString(EcuState state);

private:
    EcuMonitor();
    ~EcuMonitor() = default;
    EcuMonitor(const EcuMonitor&) = delete;
    EcuMonitor& operator=(const EcuMonitor&) = delete;

    mutable std::shared_mutex ecuMutex_;
    std::map<std::string, EcuHealth> ecus_;
};
