#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <shared_mutex>

// Standard CAN frame representation
struct CanFrame {
    uint32_t id;         // 11-bit standard ID
    uint8_t dlc;         // Data length code (0-8)
    uint8_t data[8];     // Payload bytes
};

// Simulated CAN Bus interface and stats tracker
class CanBus {
public:
    static CanBus& getInstance();

    // Serialize a physical value into a CAN frame
    CanFrame encodeSignal(uint32_t id, double physicalValue, double scale, double offset);

    // Deserialize a physical value from a CAN frame
    double decodeSignal(const CanFrame& frame, double scale, double offset) const;

    // Simulate sending a CAN frame and update stats
    void transmitFrame(const CanFrame& frame);

    // Dynamic stats queries
    double getBusLoad() const;
    uint32_t getTxCount() const;
    uint32_t getRxCount() const;
    uint32_t getErrorFrames() const;

    // Inject simulated CAN bus error/collision
    void injectBusError();
    void resetStats();

    // Pretty-print a CAN frame for logging/display
    static std::string frameToString(const CanFrame& frame);

private:
    CanBus();
    ~CanBus() = default;
    CanBus(const CanBus&) = delete;
    CanBus& operator=(const CanBus&) = delete;

    mutable std::shared_mutex statsMutex_;
    uint32_t txCount_ = 0;
    uint32_t rxCount_ = 0;
    uint32_t errorFrames_ = 0;
    double simulatedBusLoad_ = 28.5; // Nominal base bus load percentage
};
