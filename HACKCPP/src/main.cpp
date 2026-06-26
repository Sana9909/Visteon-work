// =============================================================================
// main.cpp — Entry Point for the Adaptive Smart Cabin & Vehicle Health Monitor
// =============================================================================
// This file demonstrates the following modern C++17 features:
//
//   • Smart Pointers & Polymorphism  — std::unique_ptr<Sensor> with derived types
//   • RAII                           — ThreadManager joins threads in destructor;
//                                      EventLogger closes files in destructor
//   • Move Semantics                 — sensor vector is moved into SharedVehicleData
//   • Templates                      — EventLogger<std::string>, SafeQueue<std::string>
//   • std::filesystem                — portable directory creation
//   • Exception Safety               — top-level try/catch ensures clean error reporting
//   • Multithreading                 — ThreadManager spawns 4 worker threads
//
// Startup Sequence:
//   1. Print banner
//   2. Ensure logs/ directory exists
//   3. Instantiate all subsystems (sensors, alerts, stats, logger, dashboard)
//   4. Pack shared state into SharedVehicleData
//   5. Launch ThreadManager (4 threads)
//   6. Wait for user to press ENTER
//   7. Graceful shutdown with final statistics
// =============================================================================

#include "Sensor.hpp"
#include "Alert.hpp"
#include "Logger.hpp"
#include "Dashboard.hpp"
#include "ThreadManager.hpp"
#include "ServiceBus.hpp"
#include "CanBus.hpp"
#include "DtcManager.hpp"
#include "EcuMonitor.hpp"
#include "OtaSimulator.hpp"
#include "EdrRecorder.hpp"

#include <iostream>
#include <memory>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <limits>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include "JsonParser.hpp"
#include "DriverProfile.hpp"


// Forward declarations for test cases defined in tests.cpp
bool runTestCase1(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase2(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase3(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase4(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase5(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase6(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase7(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase8(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase9(const JsonValue& config, EventLogger<std::string>& logger);
bool runTestCase10(const JsonValue& config, EventLogger<std::string>& logger);

extern "C" void writePerformanceReport();

// =============================================================================
// printBanner — Display a startup splash screen
// =============================================================================
static void printBanner()
{
    std::cout
        << "\n"
        << "  ================================================================\n"
        << "  |                                                              |\n"
        << "  |     ADAPTIVE SMART CABIN & VEHICLE HEALTH MONITORING        |\n"
        << "  |                      SYSTEM v1.0                            |\n"
        << "  |                                                              |\n"
        << "  |  Modern C++17 - Threads, RAII, Smart Pointers, Templates    |\n"
        << "  |                                                              |\n"
        << "  ================================================================\n"
        << "\n";
}

// =============================================================================
// printShutdownSummary — Display final statistics before exit
// =============================================================================
static void printShutdownSummary(const AlertManager& alertManager,
                                 const EventLogger<std::string>& logger)
{
    std::cout
        << "\n"
        << "  ================================================================\n"
        << "  |                    SHUTDOWN SUMMARY                          |\n"
        << "  ================================================================\n"
        << "  | Total Sensors Created  : " << Sensor::getSensorCount()        << "\n"
        << "  | Total Alerts Generated : " << alertManager.getTotalAlertCount() << "\n"
        << "  | Active Alerts          : " << alertManager.getActiveAlertCount()<< "\n"
        << "  | Log Entries Written    : " << logger.getLogCount()            << "\n"
        << "  ================================================================\n"
        << "  |          System shut down gracefully. Goodbye!               |\n"
        << "  ================================================================\n"
        << "\n";
}

static bool parseNumericToken(const std::string& rawToken, double& val) {
    std::string clean = "";
    bool seenDot = false;
    bool seenMinus = false;
    for (char c : rawToken) {
        if (std::isdigit(c)) {
            clean += c;
        } else if (c == '.' && !seenDot) {
            clean += c;
            seenDot = true;
        } else if (c == '-' && clean.empty() && !seenMinus) {
            clean += c;
            seenMinus = true;
        }
    }
    if (clean.empty()) return false;
    try {
        size_t idx = 0;
        val = std::stod(clean, &idx);
        return idx > 0;
    } catch (...) {
        return false;
    }
}

static void parseUserText(const std::string& text, double& temp, double& volt, double& speed, double& tire, int& door, int& seatbelt) {
    std::string cleanText = text;
    for (char& c : cleanText) {
        if (c == ',' || c == ':' || c == '=' || c == ';' || c == '(' || c == ')' || c == '"' || c == '\'') {
            c = ' ';
        }
    }

    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(cleanText);
    while (ss >> token) {
        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) { return std::tolower(c); });
        tokens.push_back(token);
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::string t = tokens[i];

        if (t == "temp" || t == "temperature" || t == "coolant" || t == "heat" || t == "hot") {
            for (size_t j = i + 1; j < tokens.size() && j <= i + 3; ++j) {
                double val;
                if (parseNumericToken(tokens[j], val)) {
                    temp = val;
                    break;
                }
            }
        }
        else if (t == "volt" || t == "voltage" || t == "battery" || t == "v") {
            for (size_t j = i + 1; j < tokens.size() && j <= i + 3; ++j) {
                double val;
                if (parseNumericToken(tokens[j], val)) {
                    volt = val;
                    break;
                }
            }
        }
        else if (t == "speed" || t == "velocity" || t == "speedometer") {
            for (size_t j = i + 1; j < tokens.size() && j <= i + 3; ++j) {
                double val;
                if (parseNumericToken(tokens[j], val)) {
                    speed = val;
                    break;
                }
            }
        }
        else if (t == "tire" || t == "tires" || t == "pressure" || t == "psi") {
            for (size_t j = i + 1; j < tokens.size() && j <= i + 3; ++j) {
                double val;
                if (parseNumericToken(tokens[j], val)) {
                    tire = val;
                    break;
                }
            }
        }
        else if (t == "door" || t == "doors" || t == "ajar") {
            for (size_t j = i + 1; j < tokens.size() && j <= i + 3; ++j) {
                std::string nextT = tokens[j];
                if (nextT == "open" || nextT == "1" || nextT == "ajar" || nextT == "unlocked" || nextT == "opened" || nextT == "true") {
                    door = 1;
                    break;
                }
                if (nextT == "closed" || nextT == "0" || nextT == "locked" || nextT == "shut" || nextT == "false") {
                    door = 0;
                    break;
                }
            }
        }
        else if (t == "seatbelt" || t == "belt" || t == "seatbelts" || t == "seat-belt") {
            for (size_t j = i + 1; j < tokens.size() && j <= i + 3; ++j) {
                std::string nextT = tokens[j];
                if (nextT == "unlocked" || nextT == "0" || nextT == "unbuckled" || nextT == "released" || nextT == "open" || nextT == "off" || nextT == "false") {
                    seatbelt = 0;
                    break;
                }
                if (nextT == "locked" || nextT == "1" || nextT == "buckled" || nextT == "fastened" || nextT == "on" || nextT == "true") {
                    seatbelt = 1;
                    break;
                }
            }
        }
    }
}

static int runCliTestMode(int argc, char* argv[]) {
    bool isUserMode = false;
    std::string userText = "";
    
    if (argc > 2 && std::string(argv[2]) == "-user") {
        isUserMode = true;
        for (int i = 3; i < argc; ++i) {
            userText += argv[i];
            if (i < argc - 1) {
                userText += " ";
            }
        }
    } else if (argc > 1 && std::string(argv[1]) == "-user") {
        isUserMode = true;
        for (int i = 2; i < argc; ++i) {
            userText += argv[i];
            if (i < argc - 1) {
                userText += " ";
            }
        }
    }

    double temp = 85.0;
    double volt = 12.6;
    double speed = 50.0;
    double tire = 32.0;
    int door = 0;
    int seatbelt = 1;

    if (isUserMode) {
        std::cout << "\n  \033[1;36m[USER-DEFINED TEST MODE]\033[0m Parsing custom input text:\n"
                  << "    \"" << userText << "\"\n\n";
        parseUserText(userText, temp, volt, speed, tire, door, seatbelt);
    } else {
        if (argc < 8) {
            std::cout << "\n  \033[1;31m[ERROR] Missing parameters for -testmode!\033[0m\n\n"
                      << "  Usage:\n"
                      << "    SmartVehicleMonitor.exe -testmode <temp> <volt> <speed> <tire> <door: 0/1> <seatbelt: 0/1>\n"
                      << "    OR:\n"
                      << "    SmartVehicleMonitor.exe -testmode -user <descriptive input text>\n\n"
                      << "  Parameters (Positional):\n"
                      << "    temp     : Engine Temperature (C)\n"
                      << "    volt     : Battery Voltage (V)\n"
                      << "    speed    : Vehicle Speed (km/h)\n"
                      << "    tire     : Tire Pressure (PSI)\n"
                      << "    door     : Door State (0 = CLOSED, 1 = OPEN)\n"
                      << "    seatbelt : Seatbelt State (0 = UNLOCKED, 1 = LOCKED)\n\n"
                      << "  Example (Positional):\n"
                      << "    .\\SmartVehicleMonitor.exe -testmode 115.0 12.5 50.0 32.0 0 1\n\n"
                      << "  Example (User Defined Text):\n"
                      << "    .\\SmartVehicleMonitor.exe -testmode -user temp 115.0 volt 12.5 speed 50.0 door open\n\n";
            return 1;
        }

        try {
            temp = std::stod(argv[2]);
            volt = std::stod(argv[3]);
            speed = std::stod(argv[4]);
            tire = std::stod(argv[5]);
            door = std::stoi(argv[6]);
            seatbelt = std::stoi(argv[7]);
        } catch (const std::exception& e) {
            std::cout << "\n  \033[1;31m[ERROR] Failed to parse positional parameters: " << e.what() << "\033[0m\n\n";
            return 1;
        }
    }

    try {
        std::vector<std::unique_ptr<Sensor>> sensors;
        sensors.push_back(std::make_unique<EngineTemperatureSensor>());
        sensors.push_back(std::make_unique<BatteryVoltageSensor>());
        sensors.push_back(std::make_unique<SpeedSensor>());
        sensors.push_back(std::make_unique<TirePressureSensor>());
        sensors.push_back(std::make_unique<DoorSensor>());
        sensors.push_back(std::make_unique<SeatbeltSensor>());

        sensors[0]->setValue(temp);
        sensors[1]->setValue(volt);
        sensors[2]->setValue(speed);
        sensors[3]->setValue(tire);
        sensors[4]->setValue(door ? 1.0 : 0.0);
        sensors[5]->setValue(seatbelt ? 1.0 : 0.0);

        AlertManager alertManager;
        alertManager.evaluateSensors(sensors);
        alertManager.prioritizeAlerts(speed);

        for (const auto& alert : alertManager.getActiveAlerts()) {
            if (alert.getSource() == "Engine Temperature") {
                DtcManager::getInstance().triggerDtc("P0117", "Engine Coolant Temperature Circuit Low (Engine Overheating)", 
                                                   temp, volt, speed, tire, door, seatbelt);
            } else if (alert.getSource() == "Battery Voltage") {
                DtcManager::getInstance().triggerDtc("P0562", "System Voltage Low (Low Battery Charge)", 
                                                   temp, volt, speed, tire, door, seatbelt);
            } else if (alert.getSource() == "Speed") {
                DtcManager::getInstance().triggerDtc("P0219", "Engine Overspeed Condition (Safety Velocity Breach)", 
                                                   temp, volt, speed, tire, door, seatbelt);
            } else if (alert.getSource() == "Door" && speed > 10.0) {
                DtcManager::getInstance().triggerDtc("P0300", "Cabin Compartment Door Ajar (Door Open While Moving)", 
                                                   temp, volt, speed, tire, door, seatbelt);
            }
        }

        std::cout << "\n  ================================================================"
                  << "\n  |                 CLI TESTMODE EVALUATION REPORT               |"
                  << "\n  ================================================================"
                  << "\n    [INPUT PARAMETERS]:"
                  << "\n      * Engine Temp      : " << temp << " C"
                  << "\n      * Battery Voltage  : " << volt << " V"
                  << "\n      * Vehicle Speed    : " << speed << " km/h"
                  << "\n      * Tire Pressure    : " << tire << " PSI"
                  << "\n      * Door State       : " << (door ? "OPEN" : "CLOSED")
                  << "\n      * Seatbelt State   : " << (seatbelt ? "LOCKED" : "UNLOCKED")
                  << "\n"
                  << "\n    [EVALUATION STATUS]:"
                  << "\n      * Overall Status   : " << alertManager.getSystemStatus()
                  << "\n      * Active Alerts    : " << alertManager.getActiveAlertCount()
                  << "\n";

        if (alertManager.getActiveAlertCount() > 0) {
            std::cout << "      --------------------------------------------------------\n";
            for (const auto& a : alertManager.getActiveAlerts()) {
                std::cout << "        - [" << severityToString(a.getSeverity()) << "] " 
                          << a.getSource() << ": " << a.getMessage() 
                          << " (Priority Score: " << std::fixed << std::setprecision(1) << a.getPriorityScore() << ")\n";
            }
        }

        auto activeDtcs = DtcManager::getInstance().getActiveDtcs();
        std::cout << "\n    [DIAGNOSTIC FAULTS (DTCs)]:\n";
        if (activeDtcs.empty()) {
            std::cout << "      * No OBD-II trouble codes triggered (MIL OFF).\n";
        } else {
            for (const auto& d : activeDtcs) {
                std::cout << "      * " << d.code << " [" << (d.status == DtcStatus::CONFIRMED ? "CONFIRMED" : "PENDING") << "]: "
                          << d.description << "\n";
            }
        }
        std::cout << "  ================================================================\n\n";

        if (isUserMode) {
            std::filesystem::create_directories("logs");
            EventLogger<std::string> userLogger("logs/user_defined_test_log.txt");
            userLogger.log("============================================================", "USER_TEST");
            userLogger.log("USER-DEFINED TEST RUN COMPLETED", "USER_TEST");
            userLogger.log("Input text: " + userText, "USER_TEST");
            userLogger.log("Parsed Inputs: Temp=" + std::to_string(temp) + " C, Battery=" + std::to_string(volt) + 
                           " V, Speed=" + std::to_string(speed) + " km/h, Tire=" + std::to_string(tire) + 
                           " PSI, Door=" + std::string(door ? "OPEN" : "CLOSED") + 
                           ", Seatbelt=" + std::string(seatbelt ? "LOCKED" : "UNLOCKED"), "USER_TEST");
            userLogger.log("Evaluation Status: " + alertManager.getSystemStatus() + 
                           " (Active Alerts: " + std::to_string(alertManager.getActiveAlertCount()) + ")", "USER_TEST");
            for (const auto& a : alertManager.getActiveAlerts()) {
                userLogger.log("  - Alert: [" + severityToString(a.getSeverity()) + "] " + a.getSource() + ": " + a.getMessage(), "USER_TEST");
            }
            for (const auto& d : activeDtcs) {
                userLogger.log("  - DTC: " + d.code + ": " + d.description, "USER_TEST");
            }
            userLogger.log("============================================================\n", "USER_TEST");
            std::cout << "  [INFO] Independent test details successfully logged to logs/user_defined_test_log.txt\n\n";
        }

        return 0;

    } catch (const std::exception& e) {
        std::cout << "\n  \033[1;31m[ERROR] Failed to run test mode: " << e.what() << "\033[0m\n\n";
        return 1;
    }
}

// =============================================================================
// main
// =============================================================================
int main(int argc, char* argv[])
{
    if (argc > 1 && (std::string(argv[1]) == "-testmode" || std::string(argv[1]) == "-user")) {
        return runCliTestMode(argc, argv);
    }

    try {
        // ----- Step 1: Startup banner -----
        printBanner();

        // ----- Step 2: Ensure the logs directory exists -----
        // std::filesystem::create_directories is a no-op if the directory
        // already exists, so this is safe to call unconditionally.
        const std::string logDir  = "logs";
        const std::string logFile = "logs/vehicle_log.txt";

        if (std::filesystem::create_directories(logDir)) {
            std::cout << "  [INIT] Created log directory: " << logDir << "\n";
        } else {
            std::cout << "  [INIT] Log directory already exists: " << logDir << "\n";
        }

        // ----- Step 2.5: Interactive Startup Selection Menu -----
        std::cout << "\n";
        std::cout << "  \033[1;36m================================================================\033[0m\n";
        std::cout << "  |                     STARTUP SELECTION                        |\n";
        std::cout << "  \033[1;36m================================================================\033[0m\n";
        std::cout << "    Please select an option to execute before the Dashboard starts:\n";
        std::cout << "    \033[1;32m[1]\033[0m Run all 10 automated test cases\n";
        std::cout << "    \033[1;32m[2]\033[0m Select and run a specific test case\n";
        std::cout << "    \033[1;32m[3]\033[0m Skip tests and start Dashboard directly\n";
        std::cout << "  \033[1;36m----------------------------------------------------------------\033[0m\n";
        std::cout << "    Enter option [1-3]: " << std::flush;

        int menuOption = 3;
        if (std::cin >> menuOption) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            menuOption = 3;
        }

        if (menuOption == 1 || menuOption == 2) {
            // Load test cases configuration
            std::string jsonPath = "data/test_cases.json";
            JsonValue testSuite = JsonParser::parseFile(jsonPath);
            const auto& testCases = testSuite.asArray();

            EventLogger<std::string> suiteLogger("logs/test_suite_log.txt");

            if (menuOption == 1) {
                std::cout << "\n  Running all 10 automated test cases...\n";
                runTestCase1(testCases[0], suiteLogger);
                runTestCase2(testCases[1], suiteLogger);
                runTestCase3(testCases[2], suiteLogger);
                runTestCase4(testCases[3], suiteLogger);
                runTestCase5(testCases[4], suiteLogger);
                runTestCase6(testCases[5], suiteLogger);
                runTestCase7(testCases[6], suiteLogger);
                runTestCase8(testCases[7], suiteLogger);
                runTestCase9(testCases[8], suiteLogger);
                runTestCase10(testCases[9], suiteLogger);

                writePerformanceReport();

                std::cout << "\n  \033[1;32mAll 10 test cases completed! Press ENTER to start the Dashboard...\033[0m";
                std::cin.get();
            } else if (menuOption == 2) {
                std::cout << "\n  \033[1;36m================================================================\033[0m\n";
                std::cout << "  |                      SELECT A TEST CASE                      |\n";
                std::cout << "  \033[1;36m================================================================\033[0m\n";
                for (size_t i = 0; i < testCases.size(); ++i) {
                    std::cout << "    \033[1;32m[" << (i + 1) << "]\033[0m " << testCases[i]["name"].asString() << "\n";
                }
                std::cout << "  \033[1;36m----------------------------------------------------------------\033[0m\n";
                std::cout << "    Enter test case number [1-" << testCases.size() << "]: " << std::flush;

                int tcChoice = 1;
                if (std::cin >> tcChoice) {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                } else {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    tcChoice = 1;
                }

                if (tcChoice >= 1 && tcChoice <= 10) {
                    std::cout << "\n  Running Test Case " << tcChoice << "...\n";
                    switch (tcChoice) {
                        case 1: runTestCase1(testCases[0], suiteLogger); break;
                        case 2: runTestCase2(testCases[1], suiteLogger); break;
                        case 3: runTestCase3(testCases[2], suiteLogger); break;
                        case 4: runTestCase4(testCases[3], suiteLogger); break;
                        case 5: runTestCase5(testCases[4], suiteLogger); break;
                        case 6: runTestCase6(testCases[5], suiteLogger); break;
                        case 7: runTestCase7(testCases[6], suiteLogger); break;
                        case 8: runTestCase8(testCases[7], suiteLogger); break;
                        case 9: runTestCase9(testCases[8], suiteLogger); break;
                        case 10: runTestCase10(testCases[9], suiteLogger); break;
                    }
                    writePerformanceReport();
                    std::cout << "\n  \033[1;32mTest Case " << tcChoice << " completed! Press ENTER to start the Dashboard...\033[0m";
                    std::cin.get();
                } else {
                    std::cout << "  Invalid test case number. Skipping to Dashboard...\n";
                }
            }
        }

        // ----- Step 3: Create all 6 sensors using smart pointers -----
        // Each sensor is a derived class of the abstract Sensor base.
        // std::unique_ptr provides exclusive ownership and automatic cleanup.
        // Polymorphism allows us to store them in a single vector and call
        // virtual methods (update, getValue, getName, etc.) uniformly.
        std::cout << "  [INIT] Creating sensors...\n";

        std::vector<std::unique_ptr<Sensor>> sensors;
        sensors.reserve(6);  // Avoid reallocations

        sensors.push_back(std::make_unique<EngineTemperatureSensor>());
        sensors.push_back(std::make_unique<BatteryVoltageSensor>());
        sensors.push_back(std::make_unique<SpeedSensor>());
        sensors.push_back(std::make_unique<TirePressureSensor>());
        sensors.push_back(std::make_unique<DoorSensor>());
        sensors.push_back(std::make_unique<SeatbeltSensor>());

        std::cout << "  [INIT] " << sensors.size()
                  << " sensors created (total sensor instances: "
                  << Sensor::getSensorCount() << ")\n";

        // ----- Step 3.5: Load driver comfort profiles -----
        std::cout << "  [INIT] Loading driver comfort profiles...\n";
        std::vector<DriverProfile> loadedProfiles;
        try {
            std::string profilesPath = "data/driver_profiles.json";
            JsonValue profilesSuite = JsonParser::parseFile(profilesPath);
            const auto& profilesArray = profilesSuite.asArray();
            for (size_t i = 0; i < profilesArray.size(); ++i) {
                DriverProfile dp;
                dp.name = profilesArray[i]["name"].asString();
                dp.seatHeightCm = profilesArray[i]["seatHeightCm"].asNumber();
                dp.mirrorAngleDeg = profilesArray[i]["mirrorAngleDeg"].asNumber();
                dp.temperaturePreferenceC = profilesArray[i]["temperaturePreferenceC"].asNumber();
                dp.maxSpeedThresholdKmh = profilesArray[i]["maxSpeedThresholdKmh"].asNumber();
                dp.lowBatteryThresholdV = profilesArray[i]["lowBatteryThresholdV"].asNumber();
                dp.engineTempThresholdC = profilesArray[i]["engineTempThresholdC"].asNumber();
                loadedProfiles.push_back(dp);
            }
            std::cout << "  [INIT] Loaded " << loadedProfiles.size() << " driver profiles successfully.\n";
        } catch (const std::exception& e) {
            std::cerr << "  [WARN] Failed to load driver profiles: " << e.what() << ". Using fallback defaults.\n";
            loadedProfiles.push_back(DriverProfile{});
        }

        // ----- Step 4: Create subsystem managers -----
        // AlertManager   : evaluates sensor readings against thresholds
        // VehicleStats   : accumulates min/max/avg statistics per sensor
        // Dashboard      : renders the console UI
        // EventLogger    : writes timestamped log entries to disk
        std::cout << "  [INIT] Initialising subsystems...\n";

        AlertManager alertManager;
        if (!loadedProfiles.empty()) {
            alertManager.setActiveProfile(loadedProfiles[0]);
        }
        std::cout << "  [INIT]   AlertManager      - OK\n";

        VehicleStatistics vehicleStats;
        std::cout << "  [INIT]   VehicleStatistics  - OK\n";

        Dashboard dashboard;
        std::cout << "  [INIT]   Dashboard          - OK\n";

        // EventLogger constructor opens the file and throws on failure.
        // RAII ensures the file is closed when the logger goes out of scope.
        EventLogger<std::string> logger(logFile);
        std::cout << "  [INIT]   EventLogger        - OK (file: " << logFile << ")\n";

        // Log the startup event
        logger.log("System startup - all subsystems initialised", "SYSTEM");

        // ----- Step 5: Create SharedVehicleData -----
        // The sensor vector is *moved* into the shared data struct.
        // After this point, the local `sensors` vector is empty (moved-from).
        auto sharedData = std::make_shared<SharedVehicleData>(
            std::move(sensors),
            alertManager,
            vehicleStats,
            logger,
            dashboard
        );
        sharedData->loadedProfiles = loadedProfiles;

        std::cout << "  [INIT] SharedVehicleData assembled (sensors moved)\n";

        // ----- Step 6: Create and start the ThreadManager -----
        // The ThreadManager takes a shared_ptr to the data so that the data
        // outlives all threads even in edge cases.
        ThreadManager threadManager(sharedData);
        threadManager.start();

        std::cout << "  [INIT] ThreadManager started - 4 worker threads launched\n";
        std::cout << "\n";

        // Log that threads are running
        logger.log("ThreadManager started - 4 threads active", "SYSTEM");
        logger.processQueue();

        // Small delay so the dashboard renders at least once before the
        // "Press ENTER" message appears (the dashboard thread will overwrite
        // the console anyway, but this makes the first render cleaner).
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // ----- Step 7: Wait for user input (Interactive Command Menu) -----
        // The main thread runs a command menu loop. Worker threads continue in
        // the background, updating sensors, evaluating alerts, rendering the
        // dashboard, and writing logs.
        std::string command;
        while (sharedData->running.load()) {
            // Read standard input line portably
            if (!std::getline(std::cin, command)) {
                sharedData->running.store(false);
                break;
            }

            // Trim leading/trailing whitespace
            std::string cmdClean = command;
            size_t first = cmdClean.find_first_not_of(" \t\r\n");
            if (first != std::string::npos) {
                size_t last = cmdClean.find_last_not_of(" \t\r\n");
                cmdClean = cmdClean.substr(first, (last - first + 1));
            } else {
                cmdClean = "";
            }

            // Immediately exit gracefully if user types 'q' / 'quit'
            if (cmdClean == "q" || cmdClean == "Q" || cmdClean == "quit" || cmdClean == "QUIT" || cmdClean == "exit" || cmdClean == "EXIT") {
                std::cout << "    Initiating graceful shutdown procedure...\n";
                sharedData->running.store(false);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            }

            // Suspend dashboard rendering
            sharedData->isMenuMode.store(true);

            // Determine if we perform a direct shortcut action or open the selection menu
            int choice = 0;
            if (cmdClean == "p" || cmdClean == "P") {
                choice = 1;
            } else if (cmdClean == "o" || cmdClean == "O") {
                choice = 2;
            } else if (cmdClean == "r" || cmdClean == "R") {
                choice = 3;
            } else if (cmdClean.empty()) {
                // Draw a styled command menu if user just pressed ENTER
                std::cout << "\033[2J\033[H";
                std::cout << "  ================================================================\n";
                std::cout << "  |                 INTERACTIVE COMMAND MENU                     |\n";
                std::cout << "  ================================================================\n";
                std::cout << "    Please select a command option below:\n";
                std::cout << "    \033[1;32m[1 / P]\033[0m Change Active Driver Profile (Hot-swaps comfort & safety targets)\n";
                std::cout << "    \033[1;32m[2 / O]\033[0m Inject Manual Sensor Override (Simulate extreme conditions)\n";
                std::cout << "    \033[1;32m[3 / R]\033[0m Clear and Reset All Accumulating Statistics\n";
                std::cout << "    \033[1;32m[4]\033[0m     OBD-II Diagnostics Menu (Request DTCs, Freeze Frames, OBD Services)\n";
                std::cout << "    \033[1;32m[5]\033[0m     Trigger OTA Software Update (Downloads & flashes with speed/DTC checks)\n";
                std::cout << "    \033[1;32m[6]\033[0m     Simulate Severe Vehicle Collision (Forces deceleration to trigger EDR)\n";
                std::cout << "    \033[1;32m[7]\033[0m     Resume Real-time Dashboard Rendering\n";
                std::cout << "    \033[1;32m[8 / Q]\033[0m Shutdown System and Terminate Monitor Process\n";
                std::cout << "  ----------------------------------------------------------------\n";
                std::cout << "    Enter choice [1-8]: " << std::flush;

                if (std::cin >> choice) {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                } else {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    choice = 7;
                }
            } else {
                std::cout << "    \033[1;31mUnknown command option \"" << cmdClean << "\". Resuming dashboard...\033[0m\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                choice = 7;
            }

            if (choice == 1) {
                std::cout << "\n  \033[1;36m================================================================\033[0m\n";
                std::cout << "  |                     SELECT DRIVER PROFILE                    |\n";
                std::cout << "  \033[1;36m================================================================\033[0m\n";
                for (size_t i = 0; i < sharedData->loadedProfiles.size(); ++i) {
                    std::cout << "    \033[1;32m[" << (i + 1) << "]\033[0m " << sharedData->loadedProfiles[i].name << "\n";
                }
                std::cout << "  \033[1;36m----------------------------------------------------------------\033[0m\n";
                std::cout << "    Enter profile number [1-" << sharedData->loadedProfiles.size() << "]: " << std::flush;

                size_t pChoice = 1;
                if (std::cin >> pChoice) {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                } else {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    pChoice = 1;
                }

                if (pChoice >= 1 && pChoice <= sharedData->loadedProfiles.size()) {
                    const auto& selectedProfile = sharedData->loadedProfiles[pChoice - 1];
                    {
                        std::unique_lock<std::shared_mutex> lock(sharedData->dataMutex);
                        sharedData->alertManager.setActiveProfile(selectedProfile);
                    }
                    std::cout << "\n  \033[1;32mActive profile successfully switched to " << selectedProfile.name << "!\033[0m\n";
                } else {
                    std::cout << "    Invalid choice. Profile unchanged.\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));

            } else if (choice == 2) {
                std::cout << "\n  \033[1;36m================================================================\033[0m\n";
                std::cout << "  |                    SELECT SENSOR TO OVERRIDE                 |\n";
                std::cout << "  \033[1;36m================================================================\033[0m\n";
                std::vector<std::string> sensorNames;
                {
                    std::shared_lock<std::shared_mutex> lock(sharedData->dataMutex);
                    for (size_t i = 0; i < sharedData->sensors.size(); ++i) {
                        sensorNames.push_back(sharedData->sensors[i]->getName());
                        std::cout << "    \033[1;32m[" << (i + 1) << "]\033[0m " << sharedData->sensors[i]->getName() 
                                  << " (Current: " << sharedData->sensors[i]->getValueString() << " " << sharedData->sensors[i]->getUnit() << ")\n";
                    }
                }
                std::cout << "  \033[1;36m----------------------------------------------------------------\033[0m\n";
                std::cout << "    Enter sensor number [1-" << sensorNames.size() << "]: " << std::flush;

                size_t sChoice = 1;
                if (std::cin >> sChoice) {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                } else {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    sChoice = 1;
                }

                if (sChoice >= 1 && sChoice <= sensorNames.size()) {
                    std::cout << "    Enter new override value: " << std::flush;
                    double overrideVal = 0.0;
                    if (std::cin >> overrideVal) {
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        {
                            std::unique_lock<std::shared_mutex> lock(sharedData->dataMutex);
                            sharedData->sensors[sChoice - 1]->setValue(overrideVal);
                            sharedData->statistics.update(sharedData->sensors[sChoice - 1]->getName(), overrideVal);
                        }
                        std::cout << "\n  \033[1;32mValue overridden successfully!\033[0m\n";
                    } else {
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "    Invalid override value.\n";
                    }
                } else {
                    std::cout << "    Invalid choice.\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));

            } else if (choice == 3) {
                {
                    std::unique_lock<std::shared_mutex> lock(sharedData->dataMutex);
                    sharedData->statistics.reset();
                    for (auto& s : sharedData->sensors) {
                        s->clearOverride();
                    }
                }
                std::cout << "\n  \033[1;32mVehicle statistics cleared and manual sensor overrides reset!\033[0m\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));

            } else if (choice == 4) {
                // OBD-II Diagnostics Menu
                std::cout << "\n  \033[1;36m================================================================\033[0m\n";
                std::cout << "  |                   OBD-II DIAGNOSTIC TELEMETRY                |\n";
                std::cout << "  \033[1;36m================================================================\033[0m\n";
                std::cout << "    [1] Service $01: Read current telemetry PID data & VIN\n";
                std::cout << "    [2] Service $03: Request confirmed emission DTCs\n";
                std::cout << "    [3] Service $07: Request pending DTCs\n";
                std::cout << "    [4] Service $09: Request Calibration and VIN info\n";
                std::cout << "    [5] Service $04: Clear active DTC codes & Freeze Frames\n";
                std::cout << "    [6] Cancel and Return\n";
                std::cout << "  \033[1;36m----------------------------------------------------------------\033[0m\n";
                std::cout << "    Enter service option [1-6]: " << std::flush;

                int obdChoice = 6;
                if (std::cin >> obdChoice) {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                } else {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    obdChoice = 6;
                }

                if (obdChoice >= 1 && obdChoice <= 5) {
                    uint8_t svId = 0x01;
                    if (obdChoice == 2) svId = 0x03;
                    else if (obdChoice == 3) svId = 0x07;
                    else if (obdChoice == 4) svId = 0x09;
                    else if (obdChoice == 5) svId = 0x04;

                    std::cout << DtcManager::getInstance().executeObdService(svId);
                    std::cout << "\n  Press ENTER to return to menu...";
                    std::cin.get();
                }

            } else if (choice == 5) {
                // Trigger OTA Software Update
                double currentSpeed = 0.0;
                {
                    std::shared_lock<std::shared_mutex> lock(sharedData->dataMutex);
                    for (const auto& s : sharedData->sensors) {
                        if (s->getType() == SensorType::SPEED) {
                            currentSpeed = s->getValue();
                        }
                    }
                }
                size_t activeDtcCount = DtcManager::getInstance().getActiveDtcs().size();
                std::string errMsg;
                bool success = OtaSimulator::getInstance().startUpdate(currentSpeed, activeDtcCount, errMsg);
                if (!success) {
                    std::cout << "\n  \033[1;31m[OTA FAILED] " << errMsg << "\033[0m\n";
                } else {
                    std::cout << "\n  \033[1;32m[OTA STARTED] Download and flash sequence initiated!\033[0m\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));

            } else if (choice == 6) {
                // Simulate Vehicle Collision
                {
                    std::unique_lock<std::shared_mutex> lock(sharedData->dataMutex);
                    for (auto& s : sharedData->sensors) {
                        if (s->getType() == SensorType::SPEED) {
                            s->setValue(0.0); // Simulate extreme deceleration G force
                        }
                    }
                }
                EdrRecorder::getInstance().triggerCrashDump("Simulated Severe Accident Event via Menu");
                std::cout << "\n  \033[1;31m[EDR EMERGENCY] Crash impact detected!\033[0m\n";
                std::cout << "  \033[1;32mEvent Data Recorder has locked history and saved reports to logs/edr_log.txt & logs/crash_edr.bin.\033[0m\n";
                std::cout << "\n  Press ENTER to return to menu...";
                std::cin.get();

            } else if (choice == 7) {
                std::cout << "    Resuming console dashboard...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(600));

            } else if (choice == 8) {
                std::cout << "    Initiating shutdown procedure...\n";
                sharedData->running.store(false);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            }

            // Resume console dashboard thread rendering
            sharedData->isMenuMode.store(false);
        }

        // ----- Step 8: Graceful shutdown -----
        std::cout << "\n  [SHUTDOWN] Stopping all threads...\n";
        threadManager.stop();
        std::cout << "  [SHUTDOWN] All threads joined successfully.\n";

        // Log the shutdown event
        logger.log("System shutdown - all threads joined", "SYSTEM");
        logger.processQueue();

        // ----- Step 9: Print final statistics -----
        printShutdownSummary(alertManager, logger);

        return 0;

    } catch (const std::exception& ex) {
        // Top-level exception handler. This catches:
        //   - EventLogger file-open failures
        //   - Any unexpected std::exception from subsystems
        std::cerr << "\n  [FATAL] Unhandled exception: " << ex.what() << "\n";
        std::cerr << "  [FATAL] The system will now exit.\n\n";
        return 1;

    } catch (...) {
        // Catch-all for non-std exceptions (should never happen in well-behaved
        // C++ code, but defence in depth is good practice).
        std::cerr << "\n  [FATAL] Unknown exception caught. Exiting.\n\n";
        return 1;
    }
}
