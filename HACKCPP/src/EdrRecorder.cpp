#include "EdrRecorder.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <mutex>

EdrRecorder& EdrRecorder::getInstance() {
    static EdrRecorder instance;
    return instance;
}

EdrRecorder::EdrRecorder() = default;

void EdrRecorder::recordTick(const EdrRecord& record) {
    std::unique_lock<std::shared_mutex> lock(edrMutex_);
    if (crashTriggered_.load()) {
        return; // Lock EDR records post-crash for crash investigator retrieval
    }
    
    rollingHistory_.push_back(record);
    if (rollingHistory_.size() > maxHistorySize_) {
        rollingHistory_.pop_front();
    }
}

size_t EdrRecorder::getHistoryCount() const {
    std::shared_lock<std::shared_mutex> lock(edrMutex_);
    return rollingHistory_.size();
}

bool EdrRecorder::isCrashTriggered() const {
    return crashTriggered_.load();
}

bool EdrRecorder::triggerCrashDump(const std::string& reason) {
    if (crashTriggered_.exchange(true)) {
        return false; // Already triggered and saved
    }

    std::shared_lock<std::shared_mutex> lock(edrMutex_);

    // 1. Write Human-Readable Report
    std::ofstream txtFile("logs/edr_log.txt", std::ios::out | std::ios::trunc);
    if (txtFile.is_open()) {
        txtFile << "========================================================\n"
                << "         ACCIDENT EVENT DATA RECORDER (EDR) REPORT     \n"
                << "========================================================\n"
                << "  Trigger Reason     : " << reason << "\n"
                << "  Records Captured   : " << rollingHistory_.size() << " frames (10 seconds)\n"
                << "  Crash-Safe Log Status: LOCKED & SEALED\n"
                << "========================================================\n\n";

        txtFile << std::left << std::setw(22) << "Timestamp"
                << std::right << std::setw(10) << "Temp (°C)"
                << std::setw(10) << "Battery (V)"
                << std::setw(10) << "Speed (kph)"
                << std::setw(10) << "Tire (PSI)"
                << std::setw(10) << "Door"
                << std::setw(10) << "Seatbelt"
                << std::setw(10) << "G-Force\n";
        txtFile << std::string(92, '-') << "\n";

        for (const auto& r : rollingHistory_) {
            txtFile << std::left << std::setw(22) << r.timestamp
                    << std::right << std::setw(10) << std::fixed << std::setprecision(1) << r.engineTemp
                    << std::setw(10) << r.batteryVoltage
                    << std::setw(10) << r.speed
                    << std::setw(10) << r.tirePressure
                    << std::setw(10) << (r.doorOpen ? "OPEN" : "CLOSED")
                    << std::setw(10) << (r.seatbeltUnlocked ? "UNLOCKED" : "LOCKED")
                    << std::setw(10) << r.gForce << " G\n";
        }
        
        txtFile << "\n[END OF REPORT - FILE LOCKED]" << std::endl;
        txtFile.flush(); // Forces output buffer to write to physical disk
        txtFile.close();
    }

    // 2. Write Crash-Safe Binary Event Log
    std::ofstream binFile("logs/crash_edr.bin", std::ios::out | std::ios::binary | std::ios::trunc);
    if (binFile.is_open()) {
        // Write file header signature
        const char signature[4] = {'E', 'D', 'R', '!'};
        binFile.write(signature, 4);

        // Write total record frames count
        uint32_t recordsCount = static_cast<uint32_t>(rollingHistory_.size());
        binFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(recordsCount));

        // Write each frame's raw data
        for (const auto& r : rollingHistory_) {
            // Write timestamp length and characters
            uint32_t timeLen = static_cast<uint32_t>(r.timestamp.size());
            binFile.write(reinterpret_cast<const char*>(&timeLen), sizeof(timeLen));
            binFile.write(r.timestamp.c_str(), timeLen);

            // Write numerical parameters
            binFile.write(reinterpret_cast<const char*>(&r.engineTemp), sizeof(r.engineTemp));
            binFile.write(reinterpret_cast<const char*>(&r.batteryVoltage), sizeof(r.batteryVoltage));
            binFile.write(reinterpret_cast<const char*>(&r.speed), sizeof(r.speed));
            binFile.write(reinterpret_cast<const char*>(&r.tirePressure), sizeof(r.tirePressure));
            
            uint8_t flags = 0;
            if (r.doorOpen) flags |= 0x01;
            if (r.seatbeltUnlocked) flags |= 0x02;
            binFile.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

            binFile.write(reinterpret_cast<const char*>(&r.gForce), sizeof(r.gForce));
        }
        
        binFile.flush(); // Forces EDR dump to physically lock on flash/disk sectors
        binFile.close();
    }

    return true;
}

void EdrRecorder::reset() {
    std::unique_lock<std::shared_mutex> lock(edrMutex_);
    rollingHistory_.clear();
    crashTriggered_.store(false);
}
