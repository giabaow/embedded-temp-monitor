#pragma once
#include <cstdint>
#include <functional>
#include <optional>

/**
 * @file alarm_manager.hpp
 * @brief Hysteresis-based thermal alarm manager
 *
 * Implements a two-threshold state machine with hysteresis to prevent
 * rapid on/off toggling near the alarm boundary — a common requirement
 * in automotive and industrial embedded systems.
 *
 * State diagram:
 *
 *   NORMAL ──(temp > high_thresh)──► ALARM
 *   ALARM  ──(temp < low_thresh) ──► NORMAL
 *   ANY    ──(nullopt reading)   ──► FAULT
 *   FAULT  ──(valid reading)     ──► NORMAL
 */

namespace monitor {

enum class AlarmState : uint8_t {
    NORMAL = 0,
    ALARM  = 1,
    FAULT  = 2,
};

/// Callback signature: invoked whenever state changes.
using StateCallback = std::function<void(AlarmState prev, AlarmState next, float temp_c)>;

struct AlarmConfig {
    float high_thresh_c = 80.0f;  ///< Enter ALARM above this
    float low_thresh_c  = 75.0f;  ///< Return to NORMAL below this (hysteresis)
};

/**
 * @brief Stateful alarm manager with hysteresis and fault detection.
 */
class AlarmManager {
public:
    explicit AlarmManager(const AlarmConfig& cfg = {});

    /**
     * @brief Register a callback to be invoked on state transitions.
     * @param cb  Callable matching StateCallback signature
     */
    void set_callback(StateCallback cb);

    /**
     * @brief Feed a new temperature reading into the state machine.
     * @param temp_opt  Result from TempSensor::read_celsius()
     */
    void update(std::optional<float> temp_opt);

    AlarmState current_state() const { return state_; }

    /// Number of ALARM transitions since boot (useful for logging).
    uint32_t alarm_count() const { return alarm_count_; }

private:
    AlarmConfig    cfg_;
    AlarmState     state_       = AlarmState::NORMAL;
    uint32_t       alarm_count_ = 0;
    StateCallback  callback_;

    void transition_to(AlarmState next, float temp_c);
};

} // namespace monitor
