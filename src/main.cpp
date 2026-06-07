#include "temp_sensor.hpp"
#include "alarm_manager.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>

/**
 * @file main.cpp
 * @brief Application entry point — simulates an embedded monitor main loop.
 *
 * On real hardware this would be:
 *   - An RTOS task woken by a periodic timer interrupt (e.g. every 100 ms)
 *   - Output routed to a UART or CAN bus instead of stdout
 *   - Alarm driving a GPIO pin or sending a CAN DTC frame
 *
 * For the simulation we run 220 iterations at 50 ms intervals, which is
 * enough to observe the full ramp-up → alarm → cooldown → normal cycle.
 */

namespace {

const char* state_name(monitor::AlarmState s) {
    switch (s) {
        case monitor::AlarmState::NORMAL: return "NORMAL";
        case monitor::AlarmState::ALARM:  return "ALARM ";
        case monitor::AlarmState::FAULT:  return "FAULT ";
    }
    return "?";
}

void print_header() {
    std::cout << "\n"
              << "┌─────────────────────────────────────────────────────┐\n"
              << "│    Embedded Temperature Monitor — Simulation        │\n"
              << "│    NTC 10kΩ B=3950 | Alarm >76°C, Clear <70°C      │\n"
              << "└─────────────────────────────────────────────────────┘\n"
              << "\n"
              << std::left
              << std::setw(6)  << "Tick"
              << std::setw(10) << "Raw ADC"
              << std::setw(12) << "Temp (°C)"
              << std::setw(10) << "State"
              << "Bar\n"
              << std::string(60, '-') << "\n";
}

void print_row(int tick, uint16_t raw, std::optional<float> temp,
               monitor::AlarmState state)
{
    // Temperature bar: ░ per 2 °C, max 50 chars, red zone > 80 °C
    std::string bar;
    if (temp) {
        int blocks = static_cast<int>(*temp / 2.0f);
        if (blocks > 50) blocks = 50;
        bar = std::string(static_cast<size_t>(blocks), '#');
    }

    std::cout << std::left
              << std::setw(6)  << tick
              << std::setw(10) << raw
              << std::setw(12) << (temp ? std::to_string(static_cast<int>(*temp * 10) / 10.0f).substr(0,5)
                                        : "FAULT")
              << std::setw(10) << state_name(state)
              << bar << "\n";
}

} // namespace

int main() {
    // ── Sensor setup ──────────────────────────────────────────────────
    sensor::ThermistorConfig therm_cfg;
    sensor::TempSensor       sensor(therm_cfg);
    sensor.init();

    // ── Alarm manager setup ───────────────────────────────────────────
    monitor::AlarmConfig alarm_cfg;
    alarm_cfg.high_thresh_c = 76.0f;
    alarm_cfg.low_thresh_c  = 70.0f;
    monitor::AlarmManager alarm(alarm_cfg);

    // Register state-change callback (simulates ISR / CAN frame / GPIO)
    alarm.set_callback([](monitor::AlarmState prev,
                          monitor::AlarmState next,
                          float temp_c)
    {
        std::cout << "\n  *** STATE CHANGE: "
                  << state_name(prev) << " → " << state_name(next)
                  << "  (T = " << std::fixed << std::setprecision(1)
                  << temp_c << " °C) ***\n\n";
    });

    print_header();

    // ── Main loop (simulates periodic timer task) ──────────────────────
    constexpr int    ITERATIONS   = 220;
    constexpr int    LOG_EVERY    = 5;     // print every Nth tick to reduce noise
    constexpr auto   TICK_PERIOD  = std::chrono::milliseconds(20);

    for (int tick = 0; tick < ITERATIONS; ++tick) {
        auto reading = sensor.read_celsius();
        alarm.update(reading);

        if (tick % LOG_EVERY == 0) {
            print_row(tick, sensor.last_raw(), reading, alarm.current_state());
        }

        std::this_thread::sleep_for(TICK_PERIOD);
    }

    std::cout << "\n" << std::string(60, '=') << "\n"
              << "Simulation complete.\n"
              << "Total ALARM events: " << alarm.alarm_count() << "\n"
              << std::string(60, '=') << "\n";

    return 0;
}
