#pragma once

#include <string>
#include <atomic>

enum class OtaState {
    IDLE,
    DOWNLOADING,
    VERIFYING,
    FLASHING,
    REBOOTING,
    COMPLETED,
    FAILED
};

// Simulated Over-The-Air (OTA) Flash Update orchestration
class OtaSimulator {
public:
    static OtaSimulator& getInstance();

    // Trigger update (Checks safety interlocks: Speed == 0, activeDtcCount == 0)
    bool startUpdate(double currentSpeed, size_t activeDtcCount, std::string& outErrorMessage);

    // Call periodically to drive the update process state machine
    void tick();

    // Getters
    OtaState getState() const;
    std::string getStateString() const;
    int getProgress() const;
    std::string getVersion() const;

    // Reset OTA sequence
    void reset();

private:
    OtaSimulator();
    ~OtaSimulator() = default;
    OtaSimulator(const OtaSimulator&) = delete;
    OtaSimulator& operator=(const OtaSimulator&) = delete;

    std::atomic<OtaState> state_{OtaState::IDLE};
    std::atomic<int> progress_{0};
    std::string currentVersion_ = "v1.0.0";
    std::string targetVersion_ = "v2.0.0";
};
