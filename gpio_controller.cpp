#include "gpio_controller.h"

#include "amust_config.h"

#include <algorithm>
#include <utility>
#include <string>
#include <vector>

#include <QDebug>

#if defined(AMUST_HAVE_GPIOD)
#include <gpiod.h>
#endif

#if defined(AMUST_HAVE_GPIOD) && defined(GPIOD_LINE_BULK_MAX_LINES)
#define AMUST_GPIOD_LEGACY_API 1
#endif

struct GpioController::Impl {
  bool initialized = false;

#if defined(AMUST_HAVE_GPIOD)
  gpiod_chip *chip = nullptr;

#if defined(AMUST_GPIOD_LEGACY_API)
  gpiod_line *laser = nullptr;
  gpiod_line *led1 = nullptr;
  gpiod_line *led2 = nullptr;
  gpiod_line *xray = nullptr;
  std::vector<std::pair<int, gpiod_line *>> requestedLines;

  void setLine(gpiod_line *line, bool on) {
    if (!line)
      return;
    if (gpiod_line_set_value(line, on ? 1 : 0) < 0) {
      qWarning() << "gpiod_line_set_value failed";
    }
  }
#else
  gpiod_line_request *request = nullptr;
  int laser = -1;
  int led1 = -1;
  int led2 = -1;
  int xray = -1;
  std::vector<int> requestedLines;

  void setLine(int lineOffset, bool on) {
    if (!request || lineOffset < 0)
      return;
    if (gpiod_line_request_set_value(
            request, static_cast<unsigned int>(lineOffset),
            on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE) < 0) {
      qWarning() << "gpiod_line_request_set_value failed";
    }
  }
#endif
#endif
};

GpioController::GpioController() : impl_(std::make_unique<Impl>()) {
#if defined(AMUST_HAVE_GPIOD)
  const char *consumer = "amust_v0.2.0";

#if defined(AMUST_GPIOD_LEGACY_API)
  impl_->chip = gpiod_chip_open_by_name(AmustConfig::kGpioChipName);
  if (!impl_->chip) {
    qWarning() << "GPIO: failed to open chip" << AmustConfig::kGpioChipName;
    return;
  }

  auto requestOut = [&](int lineNum, gpiod_line **outLine) -> bool {
    auto sameLineIt = std::find_if(impl_->requestedLines.begin(), impl_->requestedLines.end(),
                                   [&](const auto &pair) { return pair.first == lineNum; });
    if (sameLineIt != impl_->requestedLines.end()) {
      *outLine = sameLineIt->second;
      return true;
    }

    gpiod_line *line = gpiod_chip_get_line(impl_->chip, lineNum);
    if (!line) {
      qWarning() << "GPIO: failed to get line" << lineNum;
      return false;
    }
    if (gpiod_line_request_output(line, consumer, 0) < 0) {
      qWarning() << "GPIO: failed to request output line" << lineNum;
      return false;
    }

    impl_->requestedLines.emplace_back(lineNum, line);
    *outLine = line;
    return true;
  };
#else
  const std::string chipPath = [chipPath = std::string(AmustConfig::kGpioChipName)]() {
    return chipPath.empty() || chipPath[0] == '/' ? chipPath : "/dev/" + chipPath;
  }();
  impl_->chip = gpiod_chip_open(chipPath.c_str());
  if (!impl_->chip) {
    qWarning() << "GPIO: failed to open chip" << chipPath;
    return;
  }

  auto requestOut = [&](int lineNum, int *outLine) -> bool {
    if (lineNum < 0) {
      *outLine = -1;
      return false;
    }

    const bool alreadyRequested =
        std::find(impl_->requestedLines.begin(), impl_->requestedLines.end(), lineNum) !=
        impl_->requestedLines.end();
    if (!alreadyRequested) {
      impl_->requestedLines.push_back(lineNum);
    }
    *outLine = lineNum;
    return true;
  };
#endif

  const bool hasLaser = requestOut(AmustConfig::kGpioLaserLine, &impl_->laser);
  const bool hasLed1 = requestOut(AmustConfig::kGpioLed1Line, &impl_->led1);
  const bool hasLed2 = requestOut(AmustConfig::kGpioLed2Line, &impl_->led2);
  const bool hasXray = requestOut(AmustConfig::kGpioXrayEnableLine, &impl_->xray);

#if defined(AMUST_GPIOD_LEGACY_API)
  impl_->initialized = hasLaser || hasLed1 || hasLed2 || hasXray;
#else
  if (!impl_->requestedLines.empty()) {
    gpiod_request_config *requestCfg = gpiod_request_config_new();
    if (!requestCfg) {
      qWarning() << "GPIO: failed to allocate request config";
      return;
    }
    gpiod_request_config_set_consumer(requestCfg, consumer);

    gpiod_line_config *lineCfg = gpiod_line_config_new();
    if (!lineCfg) {
      gpiod_request_config_free(requestCfg);
      qWarning() << "GPIO: failed to allocate line config";
      return;
    }

    gpiod_line_settings *lineSettings = gpiod_line_settings_new();
    if (!lineSettings) {
      gpiod_line_config_free(lineCfg);
      gpiod_request_config_free(requestCfg);
      qWarning() << "GPIO: failed to allocate line settings";
      return;
    }

    gpiod_line_settings_set_direction(lineSettings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(lineSettings, GPIOD_LINE_VALUE_INACTIVE);

    bool allConfigured = true;
    for (const int lineNum : impl_->requestedLines) {
      const unsigned int lineOffset = static_cast<unsigned int>(lineNum);
      if (gpiod_line_config_add_line_settings(lineCfg, &lineOffset, 1, lineSettings) < 0) {
        allConfigured = false;
        qWarning() << "GPIO: failed to configure output line" << lineNum;
      }
    }

    gpiod_line_settings_free(lineSettings);
    if (allConfigured) {
      impl_->request = gpiod_chip_request_lines(impl_->chip, requestCfg, lineCfg);
    }
    gpiod_line_config_free(lineCfg);
    gpiod_request_config_free(requestCfg);
  }
  impl_->initialized = impl_->request != nullptr;
#endif

  if (!impl_->initialized) {
    qWarning() << "GPIO: init failed; no output lines available";
  } else {
    qInfo() << "GPIO: init" << (hasLaser ? "laser" : "no-laser") << (hasLed1 ? "led1" : "no-led1")
            << (hasLed2 ? "led2" : "no-led2") << (hasXray ? "xray" : "no-xray");
  }
#else
  qInfo() << "GPIO: libgpiod not enabled; outputs are no-op";
#endif
}

GpioController::~GpioController() {
  setAllOff();

#if defined(AMUST_HAVE_GPIOD)
  if (!impl_)
    return;

#if defined(AMUST_GPIOD_LEGACY_API)
  auto release = [](gpiod_line *line) {
    if (line)
      gpiod_line_release(line);
  };
  for (const auto &entry : impl_->requestedLines) {
    release(entry.second);
  }
#else
  if (impl_->request) {
    gpiod_line_request_release(impl_->request);
    impl_->request = nullptr;
  }
#endif

  if (impl_->chip)
    gpiod_chip_close(impl_->chip);
#endif
}

void GpioController::setLaser(bool on) {
  if (!impl_)
    return;
#if defined(AMUST_HAVE_GPIOD)
  impl_->setLine(impl_->laser, on);
#else
  Q_UNUSED(on);
#endif
}

void GpioController::setLed1(bool on) {
  if (!impl_)
    return;
#if defined(AMUST_HAVE_GPIOD)
  impl_->setLine(impl_->led1, on);
#else
  Q_UNUSED(on);
#endif
}

void GpioController::setLed2(bool on) {
  if (!impl_)
    return;
#if defined(AMUST_HAVE_GPIOD)
  impl_->setLine(impl_->led2, on);
#else
  Q_UNUSED(on);
#endif
}

void GpioController::setXrayEnable(bool on) {
  if (!impl_)
    return;
#if defined(AMUST_HAVE_GPIOD)
  impl_->setLine(impl_->xray, on);
#else
  Q_UNUSED(on);
#endif
}

void GpioController::setAllOff() {
  if (!impl_)
    return;
#if defined(AMUST_HAVE_GPIOD)
  impl_->setLine(impl_->laser, false);
  impl_->setLine(impl_->led1, false);
  impl_->setLine(impl_->led2, false);
  impl_->setLine(impl_->xray, false);
#endif
}

bool GpioController::isInitialized() const {
  if (!impl_)
    return false;
#if defined(AMUST_HAVE_GPIOD)
  return impl_->initialized;
#else
  return false;
#endif
}
