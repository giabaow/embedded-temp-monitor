#include "adc_hal.hpp"
#include <cstdlib>

/**
 * @file adc_hal.cpp
 * @brief Simulated ADC HAL for host-side testing.
 *
 * On real hardware (e.g. STM32H7) this file would contain:
 *   - RCC clock enable for ADC peripheral
 *   - GPIO pin configuration (analog mode, no pull)
 *   - ADC calibration sequence
 *   - HAL_ADC_PollForConversion() or DMA setup
 *
 * Simulation models a 10 kΩ NTC thermistor ramping from ~25 °C to ~90 °C
 * then cooling back down, exercising the alarm state machine.
 */

namespace hal {

namespace {
    uint32_t tick = 0;
}

void adc_init() {
    std::srand(42);
    tick = 0;
}

uint16_t adc_read_raw() {
    // Voltage-divider: V_adc = Vref * R_ntc / (R_series + R_ntc)
    // raw = V_adc / Vref * 4095 = R_ntc / (R_series + R_ntc) * 4095
    //
    // At 25 °C → R_ntc = 10kΩ → raw ≈ 2047
    // At 90 °C → R_ntc ≈ 1.7kΩ → raw = 1700/11700*4095 ≈ 595
    //
    // Ramp: 0..59 rise, 60..119 hold at peak, 120..219 fall

    float target_raw;
    if (tick < 60) {
        float t = static_cast<float>(tick) / 59.0f;
        target_raw = 2047.0f - t * 1550.0f;   // 2047 → 497  (~92 °C)
    } else if (tick < 120) {
        target_raw = 497.0f;                   // hold at peak
    } else if (tick < 220) {
        float t = static_cast<float>(tick - 120) / 99.0f;
        target_raw = 497.0f + t * 1550.0f;    // 497 → 2047
    } else {
        target_raw = 2047.0f;
    }

    // ±8 LSB noise
    int noise = (std::rand() % 17) - 8;
    int raw   = static_cast<int>(target_raw) + noise;
    if (raw < 1)    raw = 1;
    if (raw > 4094) raw = 4094;

    ++tick;
    return static_cast<uint16_t>(raw);
}

} // namespace hal
