// ============================================================================
// tests.cpp — Data-Driven Automated Test Suite with Execution Logging
// ============================================================================
// Parses data/test_cases.json using the custom JsonParser, executes all 10
// test cases dynamically, and utilizes the template EventLogger to generate
// a complete execution log file in logs/test_suite_log.txt.
// ============================================================================

#include "Sensor.hpp"
#include "Alert.hpp"
#include "Logger.hpp"
#include "Dashboard.hpp"
#include "ThreadManager.hpp"
#include "JsonParser.hpp"

#include <iostream>
#include <memory>
#include <cassert>
#include <vector>
#include <string>
#include <filesystem>
#include <thread>
#include <future>
#include <iomanip>

// Console text coloring helper
struct TermColor {
    static const char* RESET;
    static const char* RED;
    static const char* GREEN;
    static const char* YELLOW;
    static const char* CYAN;
    static const char* BOLD;
};

#ifdef _WIN32
// Windows console colors
const char* TermColor::RESET   = "";
const char* TermColor::RED     = "[FAIL] ";
const char* TermColor::GREEN   = "[PASS] ";
const char* TermColor::YELLOW  = "[WARN] ";
const char* TermColor::CYAN    = "[INFO] ";
const char* TermColor::BOLD    = "";
#else
// POSIX terminal ANSI escape codes
const char* TermColor::RESET   = "\033[0m";
const char* TermColor::RED     = "\033[1;31m";
const char* TermColor::GREEN   = "\033[1;32m";
const char* TermColor::YELLOW  = "\033[1;33m";
const char* TermColor::CYAN    = "\033[1;36m";
const char* TermColor::BOLD    = "\033[1m";
#endif

// Print a test header and log it to both suite logger and per-test logger
void printAndLogHeader(int num, const std::string& title,
                       EventLogger<std::string>& suiteLogger,
                       EventLogger<std::string>& testLogger) {
    suiteLogger.log("============================================================", "TEST_SUITE");
    suiteLogger.log(" RUNNING TEST CASE " + std::to_string(num) + ": " + title, "TEST_SUITE");
    suiteLogger.log("============================================================", "TEST_SUITE");
    testLogger.log("============================================================", "TEST_START");
    testLogger.log(" TEST CASE " + std::to_string(num) + ": " + title, "TEST_START");
    testLogger.log("============================================================", "TEST_START");
}

#include <fstream>
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

struct ProfileEntry {
    std::string testCase;
    std::string moduleName;
    unsigned long long cycleMin;
    unsigned long long cycleMax;
    unsigned long long cycleAvg;
    size_t codeSize;
    size_t dataSize;
};

class PerformanceProfiler {
private:
    std::vector<ProfileEntry> entries_;

public:
    void addEntry(const std::string& testCase, const std::string& moduleName,
                  unsigned long long min, unsigned long long max, unsigned long long avg,
                  size_t codeSize, size_t dataSize) {
        entries_.push_back({testCase, moduleName, min, max, avg, codeSize, dataSize});
    }

    void saveToCSV(const std::string& filePath) {
        std::ofstream csvFile(filePath);
        if (!csvFile.is_open()) {
            return;
        }
        csvFile << "Test Case,Module Name,CPU Cycles,Code size,Data size\n";
        for (const auto& entry : entries_) {
            csvFile << "\"" << entry.testCase << "\","
                    << "\"" << entry.moduleName << "\","
                    << entry.cycleMin << ","
                    << entry.codeSize << ","
                    << entry.dataSize << "\n";
        }
        csvFile.close();
    }
};

PerformanceProfiler g_profiler;

template<typename Func>
void profileBlock(const std::string& testCase, const std::string& moduleName,
                  size_t codeSize, size_t dataSize, Func&& func, int iterations = 100) {
    std::vector<unsigned long long> times;
    times.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        unsigned long long start = __rdtsc();
        func();
        unsigned long long end = __rdtsc();
        unsigned long long diff = (end > start) ? (end - start) : 0;
        times.push_back(diff);
    }

    unsigned long long minVal = *std::min_element(times.begin(), times.end());
    unsigned long long maxVal = *std::max_element(times.begin(), times.end());
    unsigned long long sumVal = std::accumulate(times.begin(), times.end(), 0ULL);
    unsigned long long avgVal = sumVal / iterations;

    g_profiler.addEntry(testCase, moduleName, minVal, maxVal, avgVal, codeSize, dataSize);
}

extern "C" void writePerformanceReport() {
    g_profiler.saveToCSV("logs/performance_metrics_new.csv");
}

struct TestReportEntry {
    std::string topic;
    std::string inputsStr;
    std::string expectedResultStr;
    std::string statusStr;
};

static std::vector<TestReportEntry> g_testReportEntries;


void writeTestReportCSV(const std::string& filePath) {
    std::ofstream csvFile(filePath);
    if (!csvFile.is_open()) return;
    csvFile << "Topic,Inputs,Expected Result,Result (Passed/Failed)\n";
    for (const auto& entry : g_testReportEntries) {
        csvFile << entry.topic << ",\""
                << entry.inputsStr << "\",\""
                << entry.expectedResultStr << "\","
                << entry.statusStr << "\n";
    }
    csvFile.close();
}

// ----------------------------------------------------------------------------
// Static helpers for sequential tick-based simulation testing
// ----------------------------------------------------------------------------
static AlertManager& getPersistentAlertManager() {
    static AlertManager manager;
    return manager;
}

static std::vector<std::unique_ptr<Sensor>>& getPersistentSensors() {
    static std::vector<std::unique_ptr<Sensor>> sensors;
    if (sensors.empty()) {
        sensors.push_back(std::make_unique<EngineTemperatureSensor>());
        sensors.push_back(std::make_unique<BatteryVoltageSensor>());
        sensors.push_back(std::make_unique<SpeedSensor>());
        sensors.push_back(std::make_unique<TirePressureSensor>());
        sensors.push_back(std::make_unique<DoorSensor>());
        sensors.push_back(std::make_unique<SeatbeltSensor>());
    }
    return sensors;
}

static bool verifyTick(int tickNum, const JsonValue& config, EventLogger<std::string>& tlog) {
    auto& manager = getPersistentAlertManager();
    auto& sensors = getPersistentSensors();

    // 1. Log input state
    const auto& inputs = config["inputs"];
    tlog.log("STEP 1: Tick " + std::to_string(tickNum) + " values set: Temp=" + std::to_string(inputs["engineTemp"].asNumber()) +
             " C, Battery=" + std::to_string(inputs["batteryVoltage"].asNumber()) + " V, Speed=" + std::to_string(inputs["speed"].asNumber()) +
             " km/h, Tire=" + std::to_string(inputs["tirePressure"].asNumber()) + " PSI, Door=" + std::string(inputs["doorOpen"].asBool() ? "OPEN" : "CLOSED") +
             ", Seatbelt=" + std::string(inputs["seatbeltLocked"].asBool() ? "LOCKED" : "UNLOCKED"), "INPUT");

    // 2. Extract expected outputs from JSON config
    const auto& expectedOutput = config["expectedOutput"];
    const auto& expectedAlerts = expectedOutput["activeAlerts"].asArray();
    std::string expectedStatus = expectedOutput["systemStatus"].asString();

    bool testPassed = true;
    std::vector<std::string> mismatches;

    // Check 1: Active Alert Count
    size_t actualAlertCount = manager.getActiveAlertCount();
    if (actualAlertCount != expectedAlerts.size()) {
        testPassed = false;
        mismatches.push_back("Alert count mismatch: Expected " + std::to_string(expectedAlerts.size()) +
                             " active alert(s), but got " + std::to_string(actualAlertCount) + ".");
    }

    // Check 2: Individual expected alerts presence
    for (size_t i = 0; i < expectedAlerts.size(); ++i) {
        std::string expSeverity = expectedAlerts[i]["severity"].asString();
        std::string expPrefix = expectedAlerts[i]["messagePrefix"].asString();

        bool found = false;
        for (const auto& activeAlert : manager.getActiveAlerts()) {
            if (severityToString(activeAlert.getSeverity()) == expSeverity &&
                activeAlert.getMessage().find(expPrefix) != std::string::npos) {
                found = true;
                break;
            }
        }
        if (!found) {
            testPassed = false;
            mismatches.push_back("Expected alert not found active: [" + expSeverity + "] " + expPrefix);
        }
    }

    // Check 3: System Status
    std::string actualStatus = manager.getSystemStatus();
    if (actualStatus != expectedStatus) {
        testPassed = false;
        mismatches.push_back("System status mismatch: Expected \"" + expectedStatus +
                             "\", but got \"" + actualStatus + "\".");
    }

    if (!testPassed) {
        tlog.log("TEST CASE " + std::to_string(tickNum) + ": " + config["description"].asString() + " -> FAILED", "STATUS");
        tlog.log("========================================", "FAIL_DETAILS");
        for (const auto& mismatch : mismatches) {
            tlog.log("  * " + mismatch, "FAIL_DETAILS");
        }
        tlog.log("----------------------------------------", "FAIL_DETAILS");
        tlog.log("RESULT: FAILED", "TEST_END");

        std::cout << "TEST CASE " << tickNum << ": " << config["description"].asString() << " -> [FAIL]" << std::endl;
        std::cout << "  --> Mismatches:" << std::endl;
        for (const auto& mismatch : mismatches) {
            std::cout << "      * " << mismatch << std::endl;
        }
        std::cout << std::endl;
    } else {
        tlog.log("TEST CASE " + std::to_string(tickNum) + ": " + config["description"].asString() + " -> PASSED", "STATUS");
        tlog.log("RESULT: PASSED", "TEST_END");

        std::cout << "TEST CASE " << tickNum << ": " << config["description"].asString() << " -> [PASS]" << std::endl;
    }

    return testPassed;
}

// ----------------------------------------------------------------------------
// GENERIC MODULE-WISE TEST IMPLEMENTATION
// ----------------------------------------------------------------------------
static bool runGenericTestCase(int tickNum, const JsonValue& config, EventLogger<std::string>& logger) {
    EventLogger<std::string>& tlog = logger;
    printAndLogHeader(tickNum, config["description"].asString(), logger, tlog);

    auto& manager = getPersistentAlertManager();
    auto& sensors = getPersistentSensors();

    const auto& inputs = config["inputs"];
    double engineTemp = inputs["engineTemp"].asNumber();
    double batteryVoltage = inputs["batteryVoltage"].asNumber();
    double speed = inputs["speed"].asNumber();
    double tirePressure = inputs["tirePressure"].asNumber();
    bool doorOpen = inputs["doorOpen"].asBool();
    bool seatbeltLocked = inputs["seatbeltLocked"].asBool();

    std::string testCaseName = "Tick " + std::to_string(tickNum) + ": " + config["description"].asString();

    profileBlock(testCaseName, "SensorManager", 4096, 128, [&]() {
        sensors[0]->setValue(engineTemp);
        sensors[1]->setValue(batteryVoltage);
        sensors[2]->setValue(speed);
        sensors[3]->setValue(tirePressure);
        sensors[4]->setValue(doorOpen ? 1.0 : 0.0);
        sensors[5]->setValue(seatbeltLocked ? 1.0 : 0.0);
    });

    profileBlock(testCaseName, "AlertManager", 8192, 256, [&]() {
        manager.evaluateSensors(sensors);
    });

    return verifyTick(tickNum, config, logger);
}

// Expose individual test case endpoints bound to the generic module-wise runner
bool runTestCase1(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(1, config, logger); }
bool runTestCase2(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(2, config, logger); }
bool runTestCase3(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(3, config, logger); }
bool runTestCase4(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(4, config, logger); }
bool runTestCase5(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(5, config, logger); }
bool runTestCase6(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(6, config, logger); }
bool runTestCase7(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(7, config, logger); }
bool runTestCase8(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(8, config, logger); }
bool runTestCase9(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(9, config, logger); }
bool runTestCase10(const JsonValue& config, EventLogger<std::string>& logger) { return runGenericTestCase(10, config, logger); }

// ============================================================================
// MAIN RUNNER
// ============================================================================
#ifndef CONSOLIDATED_BUILD
int main() {
    // Delete log files if they exist to start fresh
    const std::string logPath = "logs/test_suite_log.txt";
    try {
        if (std::filesystem::exists(logPath)) {
            std::filesystem::remove(logPath);
        }
    } catch (...) {
        // Safe to ignore if locked by editor; EventLogger constructor truncates the file anyway.
    }
    // Per-test logs are consolidated into the main suite log file now.
    
    // Ensure logs directory exists
    std::filesystem::create_directories("logs");

    try {
        // Instantiate the template logger to output the execution logs
        EventLogger<std::string> suiteLogger(logPath);
        suiteLogger.log("Automated C++ Test Suite Session Initialised", "TEST_SUITE");

        std::string jsonPath = "data/test_cases.json";
        suiteLogger.log("Loading configuration JSON database: " + jsonPath, "TEST_SUITE");
        
        JsonValue testSuite = JsonParser::parseFile(jsonPath);
        const auto& testCases = testSuite.asArray();
        
        suiteLogger.log("Loaded " + std::to_string(testCases.size()) + " test cases successfully.", "TEST_SUITE");

        // Verify loaded tests and run them sequentially
        int passCount = 0;
        int totalTests = 10;
        
        std::cout << "\nRunning all 10 automated test cases...\n" << std::endl;

        if (runTestCase1(testCases[0], suiteLogger)) passCount++;
        if (runTestCase2(testCases[1], suiteLogger)) passCount++;
        if (runTestCase3(testCases[2], suiteLogger)) passCount++;
        if (runTestCase4(testCases[3], suiteLogger)) passCount++;
        if (runTestCase5(testCases[4], suiteLogger)) passCount++;
        if (runTestCase6(testCases[5], suiteLogger)) passCount++;
        if (runTestCase7(testCases[6], suiteLogger)) passCount++;
        if (runTestCase8(testCases[7], suiteLogger)) passCount++;
        if (runTestCase9(testCases[8], suiteLogger)) passCount++;
        if (runTestCase10(testCases[9], suiteLogger)) passCount++;

        suiteLogger.log("============================================================", "TEST_SUITE");
        suiteLogger.log("  TEST RUN SUMMARY: " + std::to_string(passCount) + " / " + std::to_string(totalTests) + " PASSED", "TEST_SUITE");
        suiteLogger.log("============================================================", "TEST_SUITE");

        writePerformanceReport();
        writeTestReportCSV("logs/test_report.csv");

        std::cout << "\n============================================================" << std::endl;
        std::cout << "TEST SUITE SUMMARY: " << passCount << " / " << totalTests << " Passed" << std::endl;
        std::cout << "============================================================" << std::endl;
        
        if (passCount == totalTests) {
            std::cout << "\nALL 10 TEST CASES PASSED SUCCESSFULLY! (Logs: " << logPath << ")\n" << std::endl;
            return 0;
        } else {
            std::cout << "\nTEST SUITE COMPLETED WITH " << (totalTests - passCount) << " FAILURE(S). (Logs: " << logPath << ")\n" << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << TermColor::RED << "\n  [FATAL EXCEPTION DURING TESTING]: " << e.what() << TermColor::RESET << std::endl;
        return 1;
    } catch (...) {
        std::cerr << TermColor::RED << "\n  [FATAL UNKNOWN EXCEPTION ENCOUNTERED]" << TermColor::RESET << std::endl;
        return 1;
    }
}
#endif
