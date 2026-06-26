#pragma once

#include <string>
#include <vector>
#include <deque>
#include <shared_mutex>
#include <atomic>

struct EdrRecord {
    std::string timestamp;
    double engineTemp = 0.0;
    double batteryVoltage = 0.0;
    double speed = 0.0;
    double tirePressure = 0.0;
    bool doorOpen = false;
    bool seatbeltUnlocked = false;
    double gForce = 0.0; // Simulated longitudinal deceleration G-Force
};

class EdrRecorder {
public:
    static EdrRecorder& getInstance();

    // Log the current vehicle state into EDR circular buffer
    void recordTick(const EdrRecord& record);

    // Get rolling history count
    size_t getHistoryCount() const;

    // Trigger emergency EDR dump (simulates crash detection)
    bool triggerCrashDump(const std::string& reason);

    bool isCrashTriggered() const;

    // Clear state
    void reset();

private:
    EdrRecorder();
    ~EdrRecorder() = default;
    EdrRecorder(const EdrRecorder&) = delete;
    EdrRecorder& operator=(const EdrRecorder&) = delete;

    mutable std::shared_mutex edrMutex_;
    std::deque<EdrRecord> rollingHistory_; // Max size 20 (representing 10 seconds at 500ms ticks)
    std::atomic<bool> crashTriggered_{false};
    const size_t maxHistorySize_ = 20;
};
