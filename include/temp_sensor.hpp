#pragma once
#include <cstdint>
#include <optional>

/**
 * @file temp_sensor.hpp
 * @brief NTC thermistor driver — converts ADC voltage to °C
 *
 * Models a 10 kΩ NTC thermistor in a voltage-divider circuit
 * (R_series = 10 kΩ, V_ref = 3.3 V), using the Steinhart–Hart
 * simplified B-parameter equation.
 *
 * Schematic:
 *   VCC ──┬── R_series ──┬── NTC ── GND
 *         │              └── ADC_IN
 *
 * Typical part: Murata NCP18XH103 (B = 3950 K, R25 = 10 kΩ)
 */

namespace sensor {

/// Configuration constants for the thermistor circuit
struct ThermistorConfig {
    float r_series_ohm  = 10000.0f; ///< Series resistor (Ω)
    float r25_ohm       = 10000.0f; ///< Thermistor resistance at 25 °C (Ω)
    float b_coefficient = 3950.0f;  ///< B constant (K) from datasheet
    float t25_kelvin    = 298.15f;  ///< 25 °C in Kelvin
};

/**
 * @brief Driver class for an NTC thermistor connected via ADC.
 *
 * Responsibilities:
 *  - Read raw ADC sample via HAL
 *  - Apply voltage-divider and Steinhart–Hart equations
 *  - Detect out-of-range readings (open/short circuit)
 *  - Maintain a simple N-sample moving average for noise rejection
 */
class TempSensor {
public:
    static constexpr uint8_t  FILTER_DEPTH   = 8;    ///< Moving-average window
    static constexpr float    TEMP_MIN_C     = -40.0f;
    static constexpr float    TEMP_MAX_C     = 125.0f;

    /**
     * @brief Construct the driver with a given circuit configuration.
     * @param cfg  Thermistor + circuit parameters
     */
    explicit TempSensor(const ThermistorConfig& cfg = {});

    /**
     * @brief Initialise underlying ADC hardware.
     * Must be called once before any read.
     */
    void init();

    /**
     * @brief Perform one ADC conversion and return filtered temperature.
     * @return Temperature in °C, or std::nullopt on sensor fault.
     *
     * Returns nullopt when:
     *  - ADC reads 0 (short circuit / broken wire)
     *  - ADC reads 4095 (open circuit)
     *  - Computed temperature outside TEMP_MIN_C..TEMP_MAX_C
     */
    std::optional<float> read_celsius();

    /**
     * @brief Return the most recent unfiltered raw reading (debug aid).
     */
    uint16_t last_raw() const { return last_raw_; }

private:
    ThermistorConfig cfg_;
    uint16_t         filter_buf_[FILTER_DEPTH]{};
    uint8_t          filter_idx_  = 0;
    bool             filter_full_ = false;
    uint16_t         last_raw_    = 0;

    /// Push a sample into the circular buffer; return the current average.
    uint16_t update_filter(uint16_t raw);

    /// Convert averaged ADC counts → °C via Steinhart–Hart.
    std::optional<float> counts_to_celsius(uint16_t avg_raw) const;
};

} // namespace sensor
