#include "EcuMonitor.hpp"
#include <random>
#include <algorithm>
#include <mutex>

EcuMonitor& EcuMonitor::getInstance() {
    static EcuMonitor instance;
    return instance;
}

EcuMonitor::EcuMonitor() {
    resetAllEcus();
}

void EcuMonitor::resetAllEcus() {
    std::unique_lock<std::shared_mutex> lock(ecuMutex_);
    ecus_.clear();
    
    ecus_["ECM"]  = { "ECM",  18.5, 4096, 99.9, EcuState::ONLINE, 0 };
    ecus_["BCM"]  = { "BCM",  8.2,  2048, 99.5, EcuState::ONLINE, 0 };
    ecus_["BMS"]  = { "BMS",  5.4,  1024, 99.8, EcuState::ONLINE, 0 };
    ecus_["TPMS"] = { "TPMS", 3.1,  512,  99.7, EcuState::ONLINE, 0 };
}

void EcuMonitor::updateTelemetry(uint32_t currentTick) {
    std::unique_lock<std::shared_mutex> lock(ecuMutex_);
    
    // Seed locally or use simple pseudo-random fluctuation based on tick
    for (auto& [name, health] : ecus_) {
        if (health.state == EcuState::ONLINE) {
            // CPU fluctuates around a baseline based on name
            double baseCpu = 5.0;
            if (name == "ECM") baseCpu = 22.0;
            else if (name == "BCM") baseCpu = 12.0;
            else if (name == "BMS") baseCpu = 7.0;
            
            // Fluctuates between -3% and +3%
            double cycle = std::sin(currentTick * 0.15 + (name[0] * 0.5));
            health.cpuLoad = std::max(0.1, baseCpu + (cycle * 4.5));
            
            // Memory fluctuates very slightly (simulating heap variance)
            int memFluct = static_cast<int>(cycle * 12.0);
            uint32_t baselineMem = 1024;
            if (name == "ECM") baselineMem = 4096;
            else if (name == "BCM") baselineMem = 2048;
            else if (name == "TPMS") baselineMem = 512;
            health.memoryAllocated = static_cast<uint32_t>(std::max(128LL, static_cast<long long>(baselineMem) + memFluct));

            // Packet success rate stays high but fluctuates slightly
            double noise = std::cos(currentTick * 0.23) * 0.15;
            health.rxPacketSuccessRate = std::min(100.0, std::max(95.0, 99.8 + noise));
            health.lastMessageTick = currentTick;
        } else if (health.state == EcuState::BOOTING) {
            health.cpuLoad = 85.0; // High CPU during boot loader check
            health.rxPacketSuccessRate = 0.0;
            // Boot finishes in 3 cycles
            if (currentTick - health.lastMessageTick >= 3) {
                health.state = EcuState::ONLINE;
                health.lastMessageTick = currentTick;
            }
        } else if (health.state == EcuState::UPDATING) {
            health.cpuLoad = 45.0;
            health.rxPacketSuccessRate = 0.0;
        } else { // OFFLINE or STANDBY
            health.cpuLoad = 0.0;
            health.rxPacketSuccessRate = 0.0;
        }
    }
}

void EcuMonitor::setEcuState(const std::string& ecuName, EcuState state) {
    std::unique_lock<std::shared_mutex> lock(ecuMutex_);
    auto it = ecus_.find(ecuName);
    if (it != ecus_.end()) {
        it->second.state = state;
        // Seed timestamp of state change
        it->second.lastMessageTick = 0; // Will be synchronized on update cycle
    }
}

EcuState EcuMonitor::getEcuState(const std::string& ecuName) const {
    std::shared_lock<std::shared_mutex> lock(ecuMutex_);
    auto it = ecus_.find(ecuName);
    if (it != ecus_.end()) {
        return it->second.state;
    }
    return EcuState::OFFLINE;
}

std::map<std::string, EcuHealth> EcuMonitor::getHealthReports() const {
    std::shared_lock<std::shared_mutex> lock(ecuMutex_);
    return ecus_;
}

std::string EcuMonitor::stateToString(EcuState state) {
    switch (state) {
        case EcuState::ONLINE:   return "ONLINE";
        case EcuState::STANDBY:  return "STANDBY";
        case EcuState::OFFLINE:  return "OFFLINE (FAULT)";
        case EcuState::BOOTING:  return "BOOTING...";
        case EcuState::UPDATING: return "UPDATING...";
        default:                 return "UNKNOWN";
    }
}
