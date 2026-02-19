#pragma once

#include <memory>

#include <QElapsedTimer>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "amust_config.h"
#include "gpio_controller.h"
#include "hw/tof_sensor_controller.h"

class ProgressPill;

class MainMenuWidget final : public QWidget {
  Q_OBJECT

public:
  explicit MainMenuWidget(QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  enum class DeviceState { Ready, Running, Paused, Done };

  void setState(DeviceState next);
  void startXray();
  void pauseOrResume();
  void enterDone();
  void stopAndReset();

  void updateToFUi();
  void updateIndicators();
  void updateControlsEnabled();

  QString timeText() const;

  QTimer tickTimer_;
  QTimer clockTimer_;
  QTimer tofTimer_;
  TofSensorController tofSensor_;
  bool usingRealTof_ = false;

  int progress_ = 0;
  int tofDistanceMm_ = -1;

  DeviceState state_ = DeviceState::Ready;
  bool xrayActive_ = false;
  int outputSetDurationMs_ = AmustConfig::kOutputDefaultMs;
  int outputRunDurationMs_ = AmustConfig::kOutputDefaultMs;
  int outputRemainingMs_ = AmustConfig::kOutputDefaultMs;
  QElapsedTimer outputElapsed_;
  int outputElapsedAccumMs_ = 0;

  QString deviceState_ = "READY";

  QLabel *tofValueLabel_ = nullptr;
  QLabel *tofStatusLabel_ = nullptr;
  QLabel *tofHintLabel_ = nullptr;

  QLabel *laserValueLabel_ = nullptr;
  QLabel *led1ValueLabel_ = nullptr;
  QLabel *xrayValueLabel_ = nullptr;

  QLabel *outputTimeLabel_ = nullptr;
  ProgressPill *outputProgressBar_ = nullptr;
  QPushButton *outputMinus10sButton_ = nullptr;
  QPushButton *outputPlus10sButton_ = nullptr;
  QPushButton *outputMinus1mButton_ = nullptr;
  QPushButton *outputPlus1mButton_ = nullptr;

  QPushButton *startButton_ = nullptr;
  QPushButton *pauseButton_ = nullptr;
  QPushButton *stopButton_ = nullptr;

  GpioController gpio_;
};
