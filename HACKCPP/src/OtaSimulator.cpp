#include "OtaSimulator.hpp"
#include "EcuMonitor.hpp"

OtaSimulator& OtaSimulator::getInstance() {
    static OtaSimulator instance;
    return instance;
}

OtaSimulator::OtaSimulator() = default;

bool OtaSimulator::startUpdate(double currentSpeed, size_t activeDtcCount, std::string& outErrorMessage) {
    if (state_ != OtaState::IDLE && state_ != OtaState::COMPLETED && state_ != OtaState::FAILED) {
        outErrorMessage = "OTA update sequence is already in progress.";
        return false;
    }

    // Safety Interlocks
    if (currentSpeed > 0.0) {
        state_ = OtaState::FAILED;
        outErrorMessage = "SAFETY LOCK: Vehicle speed > 0 km/h. Flashing blocked while driving!";
        return false;
    }

    if (activeDtcCount > 0) {
        state_ = OtaState::FAILED;
        outErrorMessage = "SAFETY LOCK: Active OBD-II Diagnostic Trouble Codes present. Clear faults first!";
        return false;
    }

    // Pass interlocks
    state_ = OtaState::DOWNLOADING;
    progress_ = 0;
    
    // Set ECUs to Standby/Updating state to represent flash locking
    EcuMonitor::getInstance().setEcuState("ECM", EcuState::UPDATING);
    EcuMonitor::getInstance().setEcuState("BCM", EcuState::UPDATING);
    EcuMonitor::getInstance().setEcuState("BMS", EcuState::UPDATING);
    EcuMonitor::getInstance().setEcuState("TPMS", EcuState::UPDATING);

    return true;
}

void OtaSimulator::tick() {
    OtaState s = state_.load();
    if (s == OtaState::IDLE || s == OtaState::COMPLETED || s == OtaState::FAILED) {
        return;
    }

    int p = progress_.load();
    switch (s) {
        case OtaState::DOWNLOADING: {
            p += 15;
            if (p >= 100) {
                p = 0;
                state_ = OtaState::VERIFYING;
            }
            break;
        }
        case OtaState::VERIFYING: {
            p += 25; // Simulate SHA256 cryptographic signature check
            if (p >= 100) {
                p = 0;
                state_ = OtaState::FLASHING;
            }
            break;
        }
        case OtaState::FLASHING: {
            p += 20; // Flashing raw sectors
            if (p >= 100) {
                p = 0;
                state_ = OtaState::REBOOTING;
                
                // Trigger boot loader check on virtual ECUs
                EcuMonitor::getInstance().setEcuState("ECM", EcuState::BOOTING);
                EcuMonitor::getInstance().setEcuState("BCM", EcuState::BOOTING);
                EcuMonitor::getInstance().setEcuState("BMS", EcuState::BOOTING);
                EcuMonitor::getInstance().setEcuState("TPMS", EcuState::BOOTING);
            }
            break;
        }
        case OtaState::REBOOTING: {
            p += 34; // Simulating ECU hardware warm boot
            if (p >= 100) {
                p = 100;
                state_ = OtaState::COMPLETED;
                currentVersion_ = targetVersion_;
            }
            break;
        }
        default:
            break;
    }
    progress_ = p;
}

OtaState OtaSimulator::getState() const {
    return state_.load();
}

std::string OtaSimulator::getStateString() const {
    switch (state_.load()) {
        case OtaState::IDLE:        return "System Idle (Nominal)";
        case OtaState::DOWNLOADING: return "Downloading firmware over 5G...";
        case OtaState::VERIFYING:    return "Verifying secure SHA256 signature...";
        case OtaState::FLASHING:     return "Flashing non-volatile program memory...";
        case OtaState::REBOOTING:    return "Warm rebooting all ECUs...";
        case OtaState::COMPLETED:   return "Firmware Update Successful!";
        case OtaState::FAILED:      return "Update Terminated - SAFETY INTERLOCK BREACH";
        default:                    return "Unknown";
    }
}

int OtaSimulator::getProgress() const {
    return progress_.load();
}

std::string OtaSimulator::getVersion() const {
    return currentVersion_;
}

void OtaSimulator::reset() {
    state_ = OtaState::IDLE;
    progress_ = 0;
    
    EcuMonitor::getInstance().setEcuState("ECM", EcuState::ONLINE);
    EcuMonitor::getInstance().setEcuState("BCM", EcuState::ONLINE);
    EcuMonitor::getInstance().setEcuState("BMS", EcuState::ONLINE);
    EcuMonitor::getInstance().setEcuState("TPMS", EcuState::ONLINE);
}
