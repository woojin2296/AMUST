#pragma once

namespace AmustConfig {

// ToF distance guidance window
inline constexpr int kTofMinMm = 150;
inline constexpr int kTofMaxMm = 200;

// ToF simulation clamp (placeholder until real sensor wired)
inline constexpr int kTofSimMinMm = 100;
inline constexpr int kTofSimMaxMm = 350;

// ToF sensor reader (v0.1.0 compatible; runs `python3 TOF.py`)
// Enable with env: AMUST_ENABLE_TOF=1
inline constexpr bool kTofEnableByDefault = false;
inline constexpr double kTofPollIntervalSeconds = 0.5;

// Output time defaults / limits
inline constexpr int kOutputDefaultMs = 300'000; // 5 minutes
inline constexpr int kOutputMinMs = 10'000;      // 10 seconds
inline constexpr int kOutputMaxMs = 600'000;     // 10 minutes

// GPIO (libgpiod) configuration
inline constexpr const char *kGpioChipName = "gpiochip0";
// NOTE: Update these line numbers to match your wiring.
inline constexpr int kGpioLed1Line = 17;
inline constexpr int kGpioLed2Line = 27;
inline constexpr int kGpioLaserLine = 22;
inline constexpr int kGpioXrayEnableLine = 23;

} // namespace AmustConfig
