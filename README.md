# Embedded Temperature Monitor

A C++17 embedded systems project that simulates a real-time NTC thermistor monitor with hysteresis-based thermal alarms — the kind of safety subsystem found in automotive ECUs, motor controllers, and industrial sensors.

The project is structured as it would be on real hardware (STM32, Arduino, RP2040): a Hardware Abstraction Layer (HAL) sits between the application logic and the peripheral, unit tests run on the host, and a CI pipeline validates every commit.

---

## Features

| Feature | Detail |
|---|---|
| **NTC thermistor driver** | Steinhart–Hart B-parameter equation (B = 3950 K, R₂₅ = 10 kΩ) |
| **ADC HAL** | Simulates 12-bit ADC with voltage-divider circuit; swap for real register writes on hardware |
| **Moving-average filter** | 8-sample circular buffer for noise rejection |
| **Hysteresis alarm** | Two-threshold state machine (NORMAL / ALARM / FAULT) prevents chattering |
| **State callbacks** | `std::function` callback on every state transition (models GPIO / UART / CAN output) |
| **Unit tests** | 11 tests, no external framework — build and run anywhere |
| **CI** | GitHub Actions: build → test → smoke-run on every push |

---

## Project Structure

```
embedded-temp-monitor/
├── include/
│   ├── adc_hal.hpp          # ADC peripheral abstraction interface
│   ├── temp_sensor.hpp      # NTC thermistor driver
│   └── alarm_manager.hpp    # Hysteresis alarm state machine
├── src/
│   ├── adc_hal.cpp          # Simulated ADC (replace with MCU HAL on hardware)
│   ├── temp_sensor.cpp      # Steinhart–Hart conversion + moving-average filter
│   ├── alarm_manager.cpp    # State machine implementation
│   └── main.cpp             # Simulation entry point (models RTOS periodic task)
├── tests/
│   └── test_main.cpp        # Lightweight unit tests (no external deps)
├── .github/
│   └── workflows/ci.yml     # GitHub Actions build & test pipeline
└── CMakeLists.txt           # CMake build system (host + cross-compile ready)
```

---

## Quick Start

**Requirements:** CMake ≥ 3.16, a C++17-capable compiler (GCC 9+, Clang 10+, MSVC 2019+).

```bash
# Clone and build
git clone https://github.com/giabaow/embedded-temp-monitor.git
cd embedded-temp-monitor
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run unit tests
ctest --test-dir build --output-on-failure

# Run the simulation
./build/temp_monitor
```

### Expected output (abridged)

```
┌─────────────────────────────────────────────────────┐
│    Embedded Temperature Monitor — Simulation        │
│    NTC 10kΩ B=3950 | Alarm >76°C, Clear <70°C      │
└─────────────────────────────────────────────────────┘

Tick  Raw ADC   Temp (°C)  State     Bar
------------------------------------------------------------
0     2043      25.14      NORMAL    ############
5     2021      25.72      NORMAL    ############
...
95    1163      80.73      NORMAL    ########################################

  *** STATE CHANGE: NORMAL → ALARM  (T = 82.1 °C) ***

100   1147      81.92      ALARM     #########################################
...
185   1847      50.31      ALARM     #########################

  *** STATE CHANGE: ALARM  → NORMAL (T = 74.6 °C) ***

220   2047      25.00      NORMAL    ############

============================================================
Simulation complete.
Total ALARM events: 1
============================================================
```

---

## Architecture

### Hardware Abstraction Layer

```
┌─────────────────────────────────────────────────────────────────┐
│  Application (main.cpp)                                         │
│    └── AlarmManager ◄─── TempSensor ◄─── HAL (adc_hal.hpp)     │
│              │                 │               │                │
│         State machine    Steinhart–Hart    ADC register         │
│         + callbacks      + filter          read/write           │
└─────────────────────────────────────────────────────────────────┘
```

Separating the HAL means the business logic (thermistor math, alarm decisions) is **hardware-independent** and fully testable on a host machine. Porting to a new MCU only requires rewriting `adc_hal.cpp`.

### Alarm State Machine

```
          temp > 80 °C
  NORMAL ──────────────► ALARM
    ▲                      │
    │    temp < 75 °C      │
    └──────────────────────┘
    ▲
    │  valid reading
  FAULT ◄────────────── (any state, nullopt reading)
```

Hysteresis prevents alarm chatter when temperature hovers near the threshold — a standard requirement in automotive safety standards (ISO 26262).

---

## Porting to Real Hardware

1. **Replace `src/adc_hal.cpp`** with your MCU's peripheral driver:

```cpp
// STM32 HAL example
uint16_t hal::adc_read_raw() {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    return HAL_ADC_GetValue(&hadc1);
}
```

2. **Configure the thermistor constants** in `ThermistorConfig` to match your part's datasheet (B value, R₂₅).

3. **Cross-compile** using an ARM toolchain:

```bash
cmake -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm
```

4. **Replace `main()`** with an RTOS task:

```cpp
void temp_monitor_task(void*) {
    sensor.init();
    for (;;) {
        alarm.update(sensor.read_celsius());
        vTaskDelay(pdMS_TO_TICKS(100));  // FreeRTOS 100 ms tick
    }
}
```

---

## Testing

The test suite covers:

- Temperature plausibility across the simulated ADC ramp
- Fault detection on open-circuit (nullopt) readings
- Alarm threshold crossing (NORMAL → ALARM)
- Hysteresis: stays ALARM between thresholds
- Alarm clearing (ALARM → NORMAL below low threshold)
- Fault recovery on valid reading
- Callback invocation count on transitions
- Alarm counter increments

Run with:
```bash
ctest --test-dir build -V
```

---

## Concepts Demonstrated

- **Hardware abstraction** — peripheral interface decoupled from application logic
- **Embedded math** — Steinhart–Hart equation, voltage-divider, ADC conversion
- **Digital filtering** — circular-buffer moving average for ADC noise rejection
- **State machine design** — hysteresis, fault handling, observer callbacks
- **C++17 features** — `std::optional`, `std::function`, `constexpr`, scoped enums
- **CMake** — library/executable split, CTest integration, cross-compile readiness
- **CI/CD** — GitHub Actions pipeline mirroring embedded SIL test practice

---

## License

MIT — see [LICENSE](LICENSE) for details.
