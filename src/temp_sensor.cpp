#include "temp_sensor.hpp"
#include "adc_hal.hpp"
#include <cmath>    // log, expf
#include <numeric>  // accumulate

namespace sensor {

TempSensor::TempSensor(const ThermistorConfig& cfg)
    : cfg_(cfg) {}

void TempSensor::init() {
    hal::adc_init();
}

// ── Moving-average filter ─────────────────────────────────────────────────────

uint16_t TempSensor::update_filter(uint16_t raw) {
    filter_buf_[filter_idx_] = raw;
    filter_idx_ = (filter_idx_ + 1) % FILTER_DEPTH;
    if (filter_idx_ == 0) filter_full_ = true;

    uint8_t count = filter_full_ ? FILTER_DEPTH : filter_idx_;
    uint32_t sum  = 0;
    for (uint8_t i = 0; i < count; ++i) sum += filter_buf_[i];
    return static_cast<uint16_t>(sum / count);
}

// ── Steinhart–Hart B-parameter equation ──────────────────────────────────────
//
//   1/T = 1/T25 + (1/B) * ln(R_ntc / R25)
//
//   R_ntc derived from voltage divider:
//   V_adc = Vref * R_ntc / (R_series + R_ntc)
//   → R_ntc = R_series * V_adc / (Vref - V_adc)
//           = R_series * raw / (ADC_MAX - raw)
// ─────────────────────────────────────────────────────────────────────────────

std::optional<float> TempSensor::counts_to_celsius(uint16_t avg_raw) const {
    // Guard against divide-by-zero (open/short circuit)
    if (avg_raw == 0 || avg_raw >= hal::ADC_RESOLUTION - 1) {
        return std::nullopt;
    }

    // Resistance of the NTC from the voltage-divider equation
    float r_ntc = cfg_.r_series_ohm *
                  static_cast<float>(avg_raw) /
                  static_cast<float>(hal::ADC_RESOLUTION - 1 - avg_raw);

    // Steinhart–Hart B-parameter form
    float inv_t = (1.0f / cfg_.t25_kelvin) +
                  (1.0f / cfg_.b_coefficient) * std::log(r_ntc / cfg_.r25_ohm);

    float temp_k = 1.0f / inv_t;
    float temp_c = temp_k - 273.15f;

    if (temp_c < TEMP_MIN_C || temp_c > TEMP_MAX_C) {
        return std::nullopt;  // out-of-range → treat as fault
    }
    return temp_c;
}

// ── Public read ───────────────────────────────────────────────────────────────

std::optional<float> TempSensor::read_celsius() {
    last_raw_        = hal::adc_read_raw();
    uint16_t avg_raw = update_filter(last_raw_);
    return counts_to_celsius(avg_raw);
}

} // namespace sensor
