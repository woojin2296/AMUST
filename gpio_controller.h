#pragma once

#include <memory>

class GpioController final {
public:
  GpioController();
  ~GpioController();

  GpioController(const GpioController &) = delete;
  GpioController &operator=(const GpioController &) = delete;
  GpioController(GpioController &&) = delete;
  GpioController &operator=(GpioController &&) = delete;

  void setLaser(bool on);
  void setLed1(bool on);
  void setLed2(bool on);
  void setXrayEnable(bool on);

  void setAllOff();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

