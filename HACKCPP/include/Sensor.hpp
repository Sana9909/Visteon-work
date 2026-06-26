#pragma once

// ============================================================================
// Sensor.hpp — Sensor Framework for Adaptive Smart Cabin & Vehicle Health
// ============================================================================
// Design:
//   - Abstract base class `Sensor` enforces a uniform interface via pure
//     virtual functions (runtime polymorphism).
//   - Six concrete derived classes model real vehicle sensors.
//   - A static counter (`sensorCount_`) tracks lifetime object count (RAII —
//     incremented in ctor, decremented in dtor).
//   - Each sensor owns a `std::mt19937` RNG seeded from `std::random_device`
//     so simulated readings are independent and thread-local-safe when each
//     thread operates on its own sensor object.
//   - `operator<<` is overloaded as a friend to delegate to `display()`.
// ============================================================================

#include <string>
#include <random>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <atomic>

// ========================== Enumerations ====================================

/// Identifies the kind of physical quantity a sensor measures.
enum class SensorType {
    ENGINE_TEMP,
    BATTERY_VOLTAGE,
    SPEED,
    TIRE_PRESSURE,
    DOOR,
    SEATBELT
};

/// Binary state for a vehicle door.
enum class DoorState { OPEN, CLOSED };

/// Binary state for a seatbelt latch.
enum class SeatbeltState { LOCKED, UNLOCKED };

// ========================== Abstract Base Class =============================

/// @brief Polymorphic base for every sensor in the system.
///
/// Clients interact with sensors exclusively through `Sensor*` or
/// `Sensor&`, enabling open/closed extension: new sensor types can be
/// added without modifying existing consumer code.
class Sensor {
protected:
    const std::string& name_;    ///< Reference to static human-readable sensor label
    SensorType   type_;          ///< Discriminator tag
    static int   sensorCount_;   ///< Total live Sensor objects (all types)
    mutable std::mt19937 rng_;   ///< Per-object Mersenne Twister engine
    mutable bool rngSeeded_ = false; ///< Lazy seeding flag
    bool isOverridden_ = false;  ///< Manual override lock flag
    void ensureRngSeeded() const; ///< Lazily seeds the RNG if not already done

public:
    /// Construct with a display name and type tag.
    Sensor(const std::string& name, SensorType type);

    /// Virtual destructor — ensures correct cleanup through base pointer.
    virtual ~Sensor();

    // ----- Pure virtual interface (runtime polymorphism) --------------------

    /// Advance the sensor simulation by one tick (generate a new reading).
    virtual void update() = 0;

    /// Pretty-print the current reading to an output stream.
    virtual void display(std::ostream& os) const = 0;

    /// Return the current reading as a raw double.
    [[nodiscard]] virtual double getValue() const = 0;

    /// Return the current reading formatted as a human-readable string.
    [[nodiscard]] virtual std::string getValueString() const = 0;

    /// Return the unit label (e.g. "°C", "V", "km/h").
    [[nodiscard]] virtual std::string getUnit() const = 0;

    /// Set a custom mock value for testing or manual overrides.
    virtual void setValue(double value) = 0;

    /// Query or clear manual override state
    bool isOverridden() const { return isOverridden_; }
    void clearOverride() { isOverridden_ = false; }

    // ----- Non-virtual accessors -------------------------------------------

    [[nodiscard]] const std::string& getName() const;
    [[nodiscard]] SensorType  getType() const;

    /// Query the global live-sensor count.
    [[nodiscard]] static int getSensorCount();

    // ----- Operator overloading --------------------------------------------

    /// Delegates to `display()` so sensors work with any ostream.
    friend std::ostream& operator<<(std::ostream& os, const Sensor& sensor);
};

// ========================== Derived Classes ==================================

// ---------------------------------------------------------------------------
// EngineTemperatureSensor — models coolant temperature (°C)
// ---------------------------------------------------------------------------
class EngineTemperatureSensor : public Sensor {
    double temperature_;  ///< Current reading in degrees Celsius
public:
    EngineTemperatureSensor();
    void        update() override;
    void        display(std::ostream& os) const override;
    [[nodiscard]] double      getValue() const override;
    [[nodiscard]] std::string getValueString() const override;
    [[nodiscard]] std::string getUnit() const override;
    void        setValue(double value) override;
};

// ---------------------------------------------------------------------------
// BatteryVoltageSensor — models 12 V lead-acid / LFP battery (Volts)
// ---------------------------------------------------------------------------
class BatteryVoltageSensor : public Sensor {
    double voltage_;  ///< Current reading in Volts
public:
    BatteryVoltageSensor();
    void        update() override;
    void        display(std::ostream& os) const override;
    [[nodiscard]] double      getValue() const override;
    [[nodiscard]] std::string getValueString() const override;
    [[nodiscard]] std::string getUnit() const override;
    void        setValue(double value) override;
};

// ---------------------------------------------------------------------------
// SpeedSensor — models vehicle ground speed (km/h)
// ---------------------------------------------------------------------------
class SpeedSensor : public Sensor {
    double speed_;  ///< Current reading in km/h
public:
    SpeedSensor();
    void        update() override;
    void        display(std::ostream& os) const override;
    [[nodiscard]] double      getValue() const override;
    [[nodiscard]] std::string getValueString() const override;
    [[nodiscard]] std::string getUnit() const override;
    void        setValue(double value) override;
};

// ---------------------------------------------------------------------------
// TirePressureSensor — models TPMS reading (PSI)
// ---------------------------------------------------------------------------
class TirePressureSensor : public Sensor {
    double pressure_;  ///< Current reading in PSI
public:
    TirePressureSensor();
    void        update() override;
    void        display(std::ostream& os) const override;
    [[nodiscard]] double      getValue() const override;
    [[nodiscard]] std::string getValueString() const override;
    [[nodiscard]] std::string getUnit() const override;
    void        setValue(double value) override;
};

// ---------------------------------------------------------------------------
// DoorSensor — binary open / closed state
// ---------------------------------------------------------------------------
class DoorSensor : public Sensor {
    DoorState state_;
public:
    DoorSensor();
    void        update() override;
    void        display(std::ostream& os) const override;
    [[nodiscard]] double      getValue() const override;   ///< 1.0 = OPEN, 0.0 = CLOSED
    [[nodiscard]] std::string getValueString() const override;
    [[nodiscard]] std::string getUnit() const override;
    void        setValue(double value) override;
    [[nodiscard]] DoorState   getState() const;
};

// ---------------------------------------------------------------------------
// SeatbeltSensor — binary locked / unlocked state
// ---------------------------------------------------------------------------
class SeatbeltSensor : public Sensor {
    SeatbeltState state_;
public:
    SeatbeltSensor();
    void          update() override;
      void          display(std::ostream& os) const override;
    [[nodiscard]] double        getValue() const override;  ///< 1.0 = LOCKED, 0.0 = UNLOCKED
    [[nodiscard]] std::string   getValueString() const override;
    [[nodiscard]] std::string   getUnit() const override;
    void          setValue(double value) override;
    [[nodiscard]] SeatbeltState getState() const;
};
