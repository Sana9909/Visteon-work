#pragma once

#include <string>
#include <vector>
#include <map>
#include <shared_mutex>

enum class DtcStatus {
    PENDING,
    CONFIRMED,
    CLEARED
};

// Snapshot of telemetry parameters at the exact moment a DTC was registered
struct FreezeFrame {
    std::string timestamp;
    double engineTempC = 0.0;
    double batteryVoltageV = 0.0;
    double speedKmh = 0.0;
    double tirePressurePsi = 0.0;
    bool isDoorOpen = false;
    bool isSeatbeltUnlocked = false;
};

// Information regarding a Diagnostic Trouble Code
struct DiagnosticTroubleCode {
    std::string code;
    std::string description;
    DtcStatus status;
    int occurrences = 0;
    FreezeFrame freezeFrame;
};

// OBD-II Standard Compliant Diagnostic Trouble Code Manager
class DtcManager {
public:
    static DtcManager& getInstance();

    // Trigger or update a specific DTC
    void triggerDtc(const std::string& code, const std::string& description, 
                    double temp, double volt, double speed, double pressure, bool door, bool seatbelt);

    // Downgrade or clear DTCs
    void clearDtcs();

    // Get lists of codes
    std::vector<DiagnosticTroubleCode> getActiveDtcs() const;
    std::vector<DiagnosticTroubleCode> getPendingDtcs() const;
    std::vector<DiagnosticTroubleCode> getConfirmedDtcs() const;

    // Retrieve Captured Freeze Frame
    bool getFreezeFrame(const std::string& code, FreezeFrame& outFrame) const;

    // OBD-II Service Emulations
    std::string executeObdService(uint8_t serviceId, const std::string& parameter = "");

    std::string getVin() const;
    std::string getEcuInfo() const;

private:
    DtcManager();
    ~DtcManager() = default;
    DtcManager(const DtcManager&) = delete;
    DtcManager& operator=(const DtcManager&) = delete;

    mutable std::shared_mutex dtcMutex_;
    std::map<std::string, DiagnosticTroubleCode> dtcRegistry_;
    std::string vin_ = "1FA6P8CF0H987251A";
    std::string ecuId_ = "Bosch_MED17.5.2";
};
