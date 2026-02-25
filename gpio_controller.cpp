#include "gpio_controller.h"

#include "amust_config.h"

#include <utility>

#include <QDebug>

#if defined(AMUST_HAVE_GPIOD)
#include <gpiod.h>
#endif

struct GpioController::Impl {
  bool initialized = false;

#if defined(AMUST_HAVE_GPIOD)
  gpiod_chip *chip = nullptr;
  gpiod_line *laser = nullptr;
  gpiod_line *led1 = nullptr;
  gpiod_line *led2 = nullptr;
  gpiod_line *xray = nullptr;
#endif

  void setLine(void *linePtr, bool on) {
#if defined(AMUST_HAVE_GPIOD)
    auto *line = static_cast<gpiod_line *>(linePtr);
    if (!line)
      return;
    if (gpiod_line_set_value(line, on ? 1 : 0) < 0) {
      qWarning() << "gpiod_line_set_value failed";
    }
#else
    Q_UNUSED(linePtr);
    Q_UNUSED(on);
#endif
  }
};

GpioController::GpioController() : impl_(std::make_unique<Impl>()) {
#if defined(AMUST_HAVE_GPIOD)
  const char *consumer = "amust_v0.2.0";
  impl_->chip = gpiod_chip_open_by_name(AmustConfig::kGpioChipName);
  if (!impl_->chip) {
    qWarning() << "GPIO: failed to open chip" << AmustConfig::kGpioChipName;
    return;
  }

  auto requestOut = [&](int lineNum, gpiod_line **outLine) -> bool {
    gpiod_line *line = gpiod_chip_get_line(impl_->chip, lineNum);
    if (!line) {
      qWarning() << "GPIO: failed to get line" << lineNum;
      return false;
    }
    if (gpiod_line_request_output(line, consumer, 0) < 0) {
      qWarning() << "GPIO: failed to request output line" << lineNum;
      return false;
    }
    *outLine = line;
    return true;
  };

  const bool hasLaser = requestOut(AmustConfig::kGpioLaserLine, &impl_->laser);
  const bool hasLed1 = requestOut(AmustConfig::kGpioLed1Line, &impl_->led1);
  const bool hasLed2 = requestOut(AmustConfig::kGpioLed2Line, &impl_->led2);
  const bool hasXray = requestOut(AmustConfig::kGpioXrayEnableLine, &impl_->xray);

  impl_->initialized = hasLaser || hasLed1 || hasLed2 || hasXray;
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

  auto release = [](gpiod_line *line) {
    if (line)
      gpiod_line_release(line);
  };
  release(impl_->laser);
  release(impl_->led1);
  release(impl_->led2);
  release(impl_->xray);

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
