#include "DtcManager.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>

DtcManager& DtcManager::getInstance() {
    static DtcManager instance;
    return instance;
}

DtcManager::DtcManager() = default;

static std::string getTimestampNow() {
    auto now = std::chrono::system_clock::now();
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

void DtcManager::triggerDtc(const std::string& code, const std::string& description, 
                            double temp, double volt, double speed, double pressure, bool door, bool seatbelt) {
    std::unique_lock<std::shared_mutex> lock(dtcMutex_);
    
    auto& dtc = dtcRegistry_[code];
    dtc.code = code;
    dtc.description = description;
    dtc.occurrences++;

    if (dtc.occurrences >= 2) {
        dtc.status = DtcStatus::CONFIRMED;
    } else {
        dtc.status = DtcStatus::PENDING;
    }

    // Capture Freeze Frame
    dtc.freezeFrame.timestamp = getTimestampNow();
    dtc.freezeFrame.engineTempC = temp;
    dtc.freezeFrame.batteryVoltageV = volt;
    dtc.freezeFrame.speedKmh = speed;
    dtc.freezeFrame.tirePressurePsi = pressure;
    dtc.freezeFrame.isDoorOpen = door;
    dtc.freezeFrame.isSeatbeltUnlocked = seatbelt;
}

void DtcManager::clearDtcs() {
    std::unique_lock<std::shared_mutex> lock(dtcMutex_);
    dtcRegistry_.clear();
}

std::vector<DiagnosticTroubleCode> DtcManager::getActiveDtcs() const {
    std::shared_lock<std::shared_mutex> lock(dtcMutex_);
    std::vector<DiagnosticTroubleCode> result;
    for (const auto& [code, dtc] : dtcRegistry_) {
        result.push_back(dtc);
    }
    return result;
}

std::vector<DiagnosticTroubleCode> DtcManager::getPendingDtcs() const {
    std::shared_lock<std::shared_mutex> lock(dtcMutex_);
    std::vector<DiagnosticTroubleCode> result;
    for (const auto& [code, dtc] : dtcRegistry_) {
        if (dtc.status == DtcStatus::PENDING) {
            result.push_back(dtc);
        }
    }
    return result;
}

std::vector<DiagnosticTroubleCode> DtcManager::getConfirmedDtcs() const {
    std::shared_lock<std::shared_mutex> lock(dtcMutex_);
    std::vector<DiagnosticTroubleCode> result;
    for (const auto& [code, dtc] : dtcRegistry_) {
        if (dtc.status == DtcStatus::CONFIRMED) {
            result.push_back(dtc);
        }
    }
    return result;
}

bool DtcManager::getFreezeFrame(const std::string& code, FreezeFrame& outFrame) const {
    std::shared_lock<std::shared_mutex> lock(dtcMutex_);
    auto it = dtcRegistry_.find(code);
    if (it != dtcRegistry_.end()) {
        outFrame = it->second.freezeFrame;
        return true;
    }
    return false;
}

std::string DtcManager::getVin() const {
    return vin_;
}

std::string DtcManager::getEcuInfo() const {
    return ecuId_;
}

std::string DtcManager::executeObdService(uint8_t serviceId, const std::string& parameter) {
    std::ostringstream oss;
    
    switch (serviceId) {
        case 0x01: { // Service $01: Show Current Diagnostic Data
            oss << "\n  [OBD-II Service $01 - Live Parameter Telemetry]\n"
                << "    * Calibration Identification : " << getEcuInfo() << "\n"
                << "    * VIN Number                 : " << getVin() << "\n"
                << "    * Diagnostic Status          : " 
                << (getActiveDtcs().empty() ? "NO MALFUNCTION INDICATOR LIGHT (MIL) ACTIVE" : "MIL IS ACTIVE (RED)") << "\n";
            break;
        }
        case 0x03: { // Service $03: Request Emission-Related Diagnostic Trouble Codes (Confirmed)
            auto confirmed = getConfirmedDtcs();
            oss << "\n  [OBD-II Service $03 - Request Confirmed DTCs]\n";
            if (confirmed.empty()) {
                oss << "    * No confirmed diagnostic trouble codes stored.\n";
            } else {
                for (const auto& code : confirmed) {
                    oss << "    * " << code.code << ": " << code.description << " [CONFIRMED]\n";
                }
            }
            break;
        }
        case 0x04: { // Service $04: Clear/Reset Emission-Related Diagnostic Information
            clearDtcs();
            oss << "\n  [OBD-II Service $04 - Clear Diagnostic Codes]\n"
                << "    * All active and stored DTCs have been cleared.\n"
                << "    * OBD-II freeze frame memory reset successfully.\n";
            break;
        }
        case 0x07: { // Service $07: Request Test Results for On-Board Monitoring (Pending DTCs)
            auto pending = getPendingDtcs();
            oss << "\n  [OBD-II Service $07 - Request Pending DTCs]\n";
            if (pending.empty()) {
                oss << "    * No pending diagnostic trouble codes detected.\n";
            } else {
                for (const auto& code : pending) {
                    oss << "    * " << code.code << ": " << code.description << " [PENDING (Occurred: " << code.occurrences << ")]\n";
                }
            }
            break;
        }
        case 0x09: { // Service $09: Request Vehicle Information
            oss << "\n  [OBD-II Service $09 - Request Vehicle Info]\n"
                << "    * VIN Number                 : " << getVin() << "\n"
                << "    * ECU Module Identifier      : " << getEcuInfo() << "\n"
                << "    * OBD-II Standards Compliance: OBD-II, EOBD, CAN-Bus enabled\n";
            break;
        }
        default: {
            oss << "  [OBD-II Error] Invalid Service ID $0" << std::hex << static_cast<int>(serviceId) << "\n";
            break;
        }
    }
    
    return oss.str();
}
