#pragma once
#include <cstdint>

/**
 * @file adc_hal.hpp
 * @brief Hardware Abstraction Layer for ADC peripheral
 *
 * Abstracts raw ADC register access behind a clean interface.
 * On real hardware (STM32, etc.), this would write to memory-mapped
 * peripheral registers. Here it is simulated for host testing.
 */

namespace hal {

/// ADC resolution in bits (12-bit = 0..4095)
constexpr uint16_t ADC_RESOLUTION = 4096;

/// Reference voltage in millivolts (3.3 V supply)
constexpr uint32_t VREF_MV = 3300;

/**
 * @brief Initialise the ADC peripheral.
 * On real hardware: enables clock, sets sampling time, calibrates.
 */
void adc_init();

/**
 * @brief Trigger a single conversion and return the raw 12-bit result.
 * @return Raw ADC value in [0, 4095]
 *
 * Blocking — waits for EOC (end-of-conversion) flag.
 * On real hardware this would poll or use an interrupt/DMA.
 */
uint16_t adc_read_raw();

/**
 * @brief Convert a raw ADC value to millivolts.
 * @param raw  12-bit ADC sample
 * @return Voltage in millivolts
 */
inline uint32_t adc_to_mv(uint16_t raw) {
    return (static_cast<uint32_t>(raw) * VREF_MV) / (ADC_RESOLUTION - 1);
}

} // namespace hal
