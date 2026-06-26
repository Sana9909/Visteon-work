// ============================================================================
// Sensor.cpp — Full implementation of the Sensor framework
// ============================================================================
// Implementation notes:
//   • Every sensor seeds its own mt19937 from std::random_device so that
//     separate sensor objects produce independent, non-deterministic streams
//     even when constructed on the same thread.
//   • Speed uses a "random walk" model: each update() adds a small delta
//     (±5 km/h) and clamps to [0, 160], giving a realistic gradual change.
//   • Engine temperature uses a normal distribution centred at 85 °C with
//     σ = 10, then a 5 % spike chance pushes it up to 130 °C — simulating
//     an overheating fault.
//   • Battery voltage uses a normal distribution centred at 12.8 V with
//     σ = 0.8, plus a 5 % chance of a low-voltage dip to [8, 10] V.
//   • Tire pressure uses a normal distribution centred at 32 PSI with
//     σ = 2, plus a 5 % flat-tire dip to [20, 25] PSI.
//   • Door and Seatbelt sensors use weighted Bernoulli trials.
// ============================================================================
    
#include "Sensor.hpp"

#include <algorithm>  // std::clamp
#include <cmath>      // (available if needed)

// ========================== Static Member Init ==============================

/// Tracks total number of live Sensor-derived objects across the program.
int Sensor::sensorCount_ = 0;

static const std::string ENGINE_TEMP_NAME = "Engine Temperature";
static const std::string BATTERY_VOLTAGE_NAME = "Battery Voltage";
static const std::string SPEED_NAME = "Speed";
static const std::string TIRE_PRESSURE_NAME = "Tire Pressure";
static const std::string DOOR_NAME = "Door";
static const std::string SEATBELT_NAME = "Seatbelt";

// ========================== Sensor (Base) ===================================

Sensor::Sensor(const std::string& name, SensorType type)
    : name_{name}
    , type_{type}
    , rngSeeded_{false}
{
    ++sensorCount_;
}

void Sensor::ensureRngSeeded() const {
    if (!rngSeeded_) {
        thread_local std::mt19937 seedRng([]() {
            std::random_device rd;
            return rd();
        }());
        rng_.seed(seedRng());
        rngSeeded_ = true;
    }
}

Sensor::~Sensor() {
    --sensorCount_;
}

const std::string& Sensor::getName() const {
    return name_;
}

SensorType Sensor::getType() const {
    return type_;
}

int Sensor::getSensorCount() {
    return sensorCount_;
}

/// Friend operator<< simply delegates to the polymorphic display().
std::ostream& operator<<(std::ostream& os, const Sensor& sensor) {
    sensor.display(os);
    return os;
}

// ========================== EngineTemperatureSensor =========================

EngineTemperatureSensor::EngineTemperatureSensor()
    : Sensor(ENGINE_TEMP_NAME, SensorType::ENGINE_TEMP)
    , temperature_{85.0}                // Start in comfortable mid-range
{
}

void EngineTemperatureSensor::update() {
    if (isOverridden_) return;
    ensureRngSeeded();
    // 5 % chance of an overheating spike (fault injection)
    std::uniform_real_distribution<double> faultDist(0.0, 1.0);
    if (faultDist(rng_) < 0.05) {
        // Spike: temperature shoots into the danger zone [110, 130] °C
        std::uniform_real_distribution<double> spikeDist(110.0, 130.0);
        temperature_ = spikeDist(rng_);
    } else {
        // Normal operation: Gaussian centred at 85 °C, σ = 10
        std::normal_distribution<double> normalDist(85.0, 10.0);
        temperature_ = normalDist(rng_);
        // Clamp to a physically plausible range [50, 130] °C
        temperature_ = std::clamp(temperature_, 50.0, 130.0);
    }
}

void EngineTemperatureSensor::display(std::ostream& os) const {
    os << "[" << name_ << "] "
       << std::fixed << std::setprecision(1) << temperature_
       << " " << getUnit();
}

double EngineTemperatureSensor::getValue() const {
    return temperature_;
}

std::string EngineTemperatureSensor::getValueString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << temperature_;
    return oss.str();
}

std::string EngineTemperatureSensor::getUnit() const {
    return "\u00B0C";   // Unicode degree-Celsius: °C
}

void EngineTemperatureSensor::setValue(double value) {
    temperature_ = value;
    isOverridden_ = true;
}

// ========================== BatteryVoltageSensor ============================

BatteryVoltageSensor::BatteryVoltageSensor()
    : Sensor(BATTERY_VOLTAGE_NAME, SensorType::BATTERY_VOLTAGE)
    , voltage_{12.8}                    // Healthy resting voltage
{
}

void BatteryVoltageSensor::update() {
    if (isOverridden_) return;
    ensureRngSeeded();
    // 5 % chance of a low-voltage dip (simulates parasitic drain / aging cell)
    std::uniform_real_distribution<double> faultDist(0.0, 1.0);
    if (faultDist(rng_) < 0.05) {
        std::uniform_real_distribution<double> lowDist(8.0, 10.0);
        voltage_ = lowDist(rng_);
    } else {
        // Normal: Gaussian centred at 12.8 V, σ = 0.8
        std::normal_distribution<double> normalDist(12.8, 0.8);
        voltage_ = normalDist(rng_);
        voltage_ = std::clamp(voltage_, 8.0, 14.5);
    }
}

void BatteryVoltageSensor::display(std::ostream& os) const {
    os << "[" << name_ << "] "
       << std::fixed << std::setprecision(1) << voltage_
       << " " << getUnit();
}

double BatteryVoltageSensor::getValue() const {
    return voltage_;
}

std::string BatteryVoltageSensor::getValueString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << voltage_;
    return oss.str();
}

std::string BatteryVoltageSensor::getUnit() const {
    return "V";
}

void BatteryVoltageSensor::setValue(double value) {
    voltage_ = value;
    isOverridden_ = true;
}

// ========================== SpeedSensor =====================================

SpeedSensor::SpeedSensor()
    : Sensor(SPEED_NAME, SensorType::SPEED)
    , speed_{60.0}                      // Start at a moderate cruising speed
{
}

void SpeedSensor::update() {
    if (isOverridden_) return;
    ensureRngSeeded();
    // Random-walk model: add a small delta ∈ [-5, +5] km/h each tick.
    // This produces realistic gradual acceleration / deceleration.
    std::uniform_real_distribution<double> deltaDist(-5.0, 5.0);
    speed_ += deltaDist(rng_);
    speed_ = std::clamp(speed_, 0.0, 160.0);
}

void SpeedSensor::display(std::ostream& os) const {
    os << "[" << name_ << "] "
       << std::fixed << std::setprecision(1) << speed_
       << " " << getUnit();
}

double SpeedSensor::getValue() const {
    return speed_;
}

std::string SpeedSensor::getValueString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << speed_;
    return oss.str();
}

std::string SpeedSensor::getUnit() const {
    return "km/h";
}

void SpeedSensor::setValue(double value) {
    speed_ = value;
    isOverridden_ = true;
}

// ========================== TirePressureSensor ==============================

TirePressureSensor::TirePressureSensor()
    : Sensor(TIRE_PRESSURE_NAME, SensorType::TIRE_PRESSURE)
    , pressure_{32.0}                   // Ideal cold pressure for most cars
{
}

void TirePressureSensor::update() {
    if (isOverridden_) return;
    ensureRngSeeded();
    // 5 % chance of a sudden deflation event
    std::uniform_real_distribution<double> faultDist(0.0, 1.0);
    if (faultDist(rng_) < 0.05) {
        std::uniform_real_distribution<double> lowDist(20.0, 25.0);
        pressure_ = lowDist(rng_);
    } else {
        // Normal fluctuation: Gaussian centred at 32 PSI, σ = 2
        std::normal_distribution<double> normalDist(32.0, 2.0);
        pressure_ = normalDist(rng_);
        pressure_ = std::clamp(pressure_, 20.0, 40.0);
    }
}

void TirePressureSensor::display(std::ostream& os) const {
    os << "[" << name_ << "] "
       << std::fixed << std::setprecision(1) << pressure_
       << " " << getUnit();
}

double TirePressureSensor::getValue() const {
    return pressure_;
}

std::string TirePressureSensor::getValueString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << pressure_;
    return oss.str();
}

std::string TirePressureSensor::getUnit() const {
    return "PSI";
}

void TirePressureSensor::setValue(double value) {
    pressure_ = value;
    isOverridden_ = true;
}

// ========================== DoorSensor ======================================

DoorSensor::DoorSensor()
    : Sensor(DOOR_NAME, SensorType::DOOR)
    , state_{DoorState::CLOSED}         // Doors default to closed
{
}

void DoorSensor::update() {
    if (isOverridden_) return;
    ensureRngSeeded();
    // 80 % CLOSED, 20 % OPEN — weighted Bernoulli trial
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    state_ = (dist(rng_) < 0.20) ? DoorState::OPEN : DoorState::CLOSED;
}

void DoorSensor::display(std::ostream& os) const {
    os << "[" << name_ << "] " << getValueString();
}

double DoorSensor::getValue() const {
    // Convention: 1.0 = OPEN (potentially hazardous), 0.0 = CLOSED (safe)
    return (state_ == DoorState::OPEN) ? 1.0 : 0.0;
}

std::string DoorSensor::getValueString() const {
    return (state_ == DoorState::OPEN) ? "OPEN" : "CLOSED";
}

std::string DoorSensor::getUnit() const {
    // Binary sensor — no physical unit applies
    return "";
}

void DoorSensor::setValue(double value) {
    state_ = (value != 0.0 ? DoorState::OPEN : DoorState::CLOSED);
    isOverridden_ = true;
}

DoorState DoorSensor::getState() const {
    return state_;
}

// ========================== SeatbeltSensor ==================================

SeatbeltSensor::SeatbeltSensor()
    : Sensor(SEATBELT_NAME, SensorType::SEATBELT)
    , state_{SeatbeltState::LOCKED}     // Seatbelt defaults to latched
{
}

void SeatbeltSensor::update() {
    if (isOverridden_) return;
    ensureRngSeeded();
    // 70 % LOCKED, 30 % UNLOCKED — weighted Bernoulli trial
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    state_ = (dist(rng_) < 0.30) ? SeatbeltState::UNLOCKED : SeatbeltState::LOCKED;
}

void SeatbeltSensor::display(std::ostream& os) const {
    os << "[" << name_ << "] " << getValueString();
}

double SeatbeltSensor::getValue() const {
    // Convention: 1.0 = LOCKED (safe), 0.0 = UNLOCKED (alert-worthy)
    return (state_ == SeatbeltState::LOCKED) ? 1.0 : 0.0;
}

std::string SeatbeltSensor::getValueString() const {
    return (state_ == SeatbeltState::LOCKED) ? "LOCKED" : "UNLOCKED";
}

std::string SeatbeltSensor::getUnit() const {
    // Binary sensor — no physical unit applies
    return "";
}

void SeatbeltSensor::setValue(double value) {
    state_ = (value != 0.0 ? SeatbeltState::LOCKED : SeatbeltState::UNLOCKED);
    isOverridden_ = true;
}

SeatbeltState SeatbeltSensor::getState() const {
    return state_;
}
