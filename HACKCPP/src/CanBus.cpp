#include "CanBus.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <mutex>

CanBus& CanBus::getInstance() {
    static CanBus instance;
    return instance;
}

CanBus::CanBus() = default;

CanFrame CanBus::encodeSignal(uint32_t id, double physicalValue, double scale, double offset) {
    CanFrame frame{};
    frame.id = id;
    frame.dlc = 8;
    
    // Scale and convert physical value to uint64_t raw representation
    double scaled = (physicalValue - offset) / scale;
    uint64_t raw = static_cast<uint64_t>(std::max(0.0, std::round(scaled)));

    // Store raw value as Little Endian bytes in data payload
    for (int i = 0; i < 8; ++i) {
        frame.data[i] = static_cast<uint8_t>((raw >> (i * 8)) & 0xFF);
    }

    return frame;
}

double CanBus::decodeSignal(const CanFrame& frame, double scale, double offset) const {
    uint64_t raw = 0;
    for (int i = 0; i < 8; ++i) {
        raw |= (static_cast<uint64_t>(frame.data[i]) << (i * 8));
    }
    return (static_cast<double>(raw) * scale) + offset;
}

void CanBus::transmitFrame(const CanFrame& frame) {
    std::unique_lock<std::shared_mutex> lock(statsMutex_);
    txCount_++;
    rxCount_++; // Every transmitter frame is monitored on the shared CAN bus
    
    // Dynamically fluctuate simulated bus load based on count
    simulatedBusLoad_ = 25.0 + (txCount_ % 100) * 0.15;
}

double CanBus::getBusLoad() const {
    std::shared_lock<std::shared_mutex> lock(statsMutex_);
    return simulatedBusLoad_;
}

uint32_t CanBus::getTxCount() const {
    std::shared_lock<std::shared_mutex> lock(statsMutex_);
    return txCount_;
}

uint32_t CanBus::getRxCount() const {
    std::shared_lock<std::shared_mutex> lock(statsMutex_);
    return rxCount_;
}

uint32_t CanBus::getErrorFrames() const {
    std::shared_lock<std::shared_mutex> lock(statsMutex_);
    return errorFrames_;
}

void CanBus::injectBusError() {
    std::unique_lock<std::shared_mutex> lock(statsMutex_);
    errorFrames_++;
    simulatedBusLoad_ += 5.5; // Collision retransmissions increase bus load
}

void CanBus::resetStats() {
    std::unique_lock<std::shared_mutex> lock(statsMutex_);
    txCount_ = 0;
    rxCount_ = 0;
    errorFrames_ = 0;
    simulatedBusLoad_ = 28.5;
}

std::string CanBus::frameToString(const CanFrame& frame) {
    std::ostringstream oss;
    oss << "CAN ID: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(3) << frame.id
        << " [DLC=" << std::dec << static_cast<int>(frame.dlc) << "] Data: ";
    for (int i = 0; i < frame.dlc; ++i) {
        oss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(frame.data[i]) << " ";
    }
    return oss.str();
}
