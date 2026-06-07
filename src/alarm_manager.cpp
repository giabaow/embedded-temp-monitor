#include "alarm_manager.hpp"
#include <optional>

namespace monitor {

AlarmManager::AlarmManager(const AlarmConfig& cfg)
    : cfg_(cfg) {}

void AlarmManager::set_callback(StateCallback cb) {
    callback_ = std::move(cb);
}

void AlarmManager::transition_to(AlarmState next, float temp_c) {
    if (next == state_) return;

    AlarmState prev = state_;
    state_          = next;

    if (next == AlarmState::ALARM) ++alarm_count_;
    if (callback_) callback_(prev, next, temp_c);
}

void AlarmManager::update(std::optional<float> temp_opt) {
    if (!temp_opt.has_value()) {
        // Sensor fault: any state → FAULT
        transition_to(AlarmState::FAULT, 0.0f);
        return;
    }

    float temp = *temp_opt;

    switch (state_) {
        case AlarmState::NORMAL:
        case AlarmState::FAULT:
            if (temp > cfg_.high_thresh_c) {
                transition_to(AlarmState::ALARM, temp);
            } else {
                transition_to(AlarmState::NORMAL, temp);
            }
            break;

        case AlarmState::ALARM:
            // Hysteresis: only leave ALARM when temp drops below low_thresh
            if (temp < cfg_.low_thresh_c) {
                transition_to(AlarmState::NORMAL, temp);
            }
            break;
    }
}

} // namespace monitor
