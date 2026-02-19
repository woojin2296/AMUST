#include "main_menu_widget.h"

#include <algorithm>
#include <cmath>

#include <QFrame>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QSizePolicy>
#include <QTime>
#include <QVBoxLayout>

#include "progress_pill.h"
#include "amust_config.h"

namespace {

QRectF fitAspect(const QRectF &outer, double aspectW, double aspectH) {
  const double target = aspectW / aspectH;
  const double actual = outer.width() / outer.height();
  if (actual > target) {
    const double w = outer.height() * target;
    const double x = outer.x() + (outer.width() - w) * 0.5;
    return QRectF(x, outer.y(), w, outer.height());
  }
  const double h = outer.width() / target;
  const double y = outer.y() + (outer.height() - h) * 0.5;
  return QRectF(outer.x(), y, outer.width(), h);
}

QFont fixedFont(int pixelSize) {
  QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  f.setPixelSize(pixelSize);
  return f;
}

QColor withAlpha(const QColor &c, double a01) {
  QColor out = c;
  out.setAlphaF(std::clamp(a01, 0.0, 1.0));
  return out;
}

QString pillStyle(const QString &bgRgba, const QString &borderRgba, const QString &textRgba = QString()) {
  QString style =
      "QLabel {"
      "  background: %1;"
      "  border: 1px solid %2;"
      "  border-radius: 10px;"
      "  padding: 6px 10px;";
  if (!textRgba.isEmpty())
    style += "  color: %3;";
  style += "}";

  if (textRgba.isEmpty())
    return QString(style).arg(bgRgba, borderRgba);
  return QString(style).arg(bgRgba, borderRgba, textRgba);
}

QString cardStyle() {
  return QString(
      "QFrame {"
      "  background: rgba(0,0,0,0.18);"
      "  border: 1px solid rgba(255,255,255,0.10);"
      "  border-radius: 12px;"
      "}"
      "QLabel { color: rgba(255,255,255,0.86); }"
      "QPushButton {"
      "  background: rgba(255,255,255,0.10);"
      "  border: 1px solid rgba(255,255,255,0.12);"
      "  border-radius: 10px;"
      "  padding: 10px 14px;"
      "  color: rgba(255,255,255,0.9);"
      "  font-weight: 600;"
      "}"
      "QPushButton:disabled {"
      "  background: rgba(255,255,255,0.06);"
      "  color: rgba(255,255,255,0.35);"
      "  border-color: rgba(255,255,255,0.08);"
      "}"
      "QPushButton#primary {"
      "  background: rgba(255,255,255,0.16);"
      "  border-color: rgba(255,255,255,0.20);"
      "}");
}

QString smallLabelStyle() {
  return "QLabel { color: rgba(255,255,255,0.60); }";
}

QString monoStyle(int px, bool bold = false) {
  return QString("QLabel { font-family: \"%1\"; font-size: %2px; font-weight: %3; }")
      .arg(QFontDatabase::systemFont(QFontDatabase::FixedFont).family())
      .arg(px)
      .arg(bold ? 700 : 400);
}

QString titleStyle() {
  return monoStyle(12, true) + smallLabelStyle() + "QLabel { padding: 6px 0 6px 6px; }";
}

bool envTruthy(const QByteArray &v) {
  if (v.isEmpty())
    return false;
  const QByteArray lower = v.toLower();
  return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

} // namespace

MainMenuWidget::MainMenuWidget(QWidget *parent) : QWidget(parent), tofSensor_(this) {
  setAttribute(Qt::WA_OpaquePaintEvent);
  setAutoFillBackground(false);

  // Layout area: keep away from top/bottom bars (which are painted).
  // Use a centering wrapper so the whole block sits vertically centered.
  auto *centerWrap = new QVBoxLayout(this);
  // Larger overall layout (another ≈10% more usable area).
  // Add a bit more top margin so the whole block feels vertically centered.
  centerWrap->setContentsMargins(65, 88, 65, 95);
  centerWrap->addStretch(1);

  auto *gridHost = new QWidget(this);
  gridHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  auto *root = new QGridLayout(gridHost);
  root->setContentsMargins(0, 0, 0, 0);
  root->setHorizontalSpacing(18);
  root->setVerticalSpacing(18);

  // ToF card
  auto *tofCard = new QFrame(this);
  tofCard->setStyleSheet(cardStyle());
  auto *tofLayout = new QVBoxLayout(tofCard);
  tofLayout->setContentsMargins(18, 16, 18, 16);
  tofLayout->setSpacing(10);

  auto *tofTitle = new QLabel("TOF DISTANCE", tofCard);
  tofTitle->setStyleSheet(titleStyle());

  tofValueLabel_ = new QLabel("-- mm", tofCard);
  tofValueLabel_->setAlignment(Qt::AlignCenter);
  tofValueLabel_->setMinimumHeight(52);
  tofValueLabel_->setStyleSheet("QLabel { padding: 6px 0; }" + monoStyle(28, true));

  tofStatusLabel_ = new QLabel("—", tofCard);
  tofStatusLabel_->setAlignment(Qt::AlignCenter);
  tofStatusLabel_->setFixedHeight(34);
  tofStatusLabel_->setStyleSheet(pillStyle("rgba(255,255,255,0.10)", "rgba(255,255,255,0.12)") +
                                 monoStyle(12, true));

  tofHintLabel_ = new QLabel("Maintain target distance (e.g. 350–450 mm)", tofCard);
  tofHintLabel_->setMinimumHeight(28);
  tofHintLabel_->setStyleSheet(smallLabelStyle() + monoStyle(12, false) +
                               "QLabel { padding-left: 6px; }");

  tofLayout->addWidget(tofTitle);
  tofLayout->addWidget(tofValueLabel_);
  tofLayout->addSpacing(24);
  tofLayout->addWidget(tofStatusLabel_);
  tofLayout->addSpacing(8);
  tofLayout->addWidget(tofHintLabel_);
  tofLayout->addStretch(1);

  // Indicators card
  auto *ioCard = new QFrame(this);
  ioCard->setStyleSheet(cardStyle());
  auto *ioLayout = new QGridLayout(ioCard);
  ioLayout->setContentsMargins(18, 16, 18, 16);
  ioLayout->setHorizontalSpacing(12);
  ioLayout->setVerticalSpacing(10);

  auto *ioTitle = new QLabel("INDICATORS", ioCard);
  ioTitle->setStyleSheet(titleStyle());
  ioLayout->addWidget(ioTitle, 0, 0, 1, 2);

  auto mkName = [&](const QString &t) {
    auto *l = new QLabel(t, ioCard);
    l->setAlignment(Qt::AlignCenter);
    l->setMinimumHeight(28);
    l->setFixedHeight(34);
    l->setStyleSheet(smallLabelStyle() + monoStyle(12, true));
    return l;
  };
  auto mkValue = [&](const QString &t) {
    auto *l = new QLabel(t, ioCard);
    l->setAlignment(Qt::AlignCenter);
    l->setFixedHeight(34);
    l->setStyleSheet(pillStyle("rgba(255,255,255,0.10)", "rgba(255,255,255,0.12)") +
                     monoStyle(12, true));
    return l;
  };

  ioLayout->addWidget(mkName("LASER"), 1, 0);
  laserValueLabel_ = mkValue("OFF");
  ioLayout->addWidget(laserValueLabel_, 1, 1);

  ioLayout->addWidget(mkName("LED"), 2, 0);
  led1ValueLabel_ = mkValue("OFF");
  ioLayout->addWidget(led1ValueLabel_, 2, 1);

  ioLayout->addWidget(mkName("X-RAY"), 3, 0);
  xrayValueLabel_ = mkValue("IDLE");
  ioLayout->addWidget(xrayValueLabel_, 3, 1);

  // Controls row
  auto *controlsCard = new QFrame(this);
  controlsCard->setStyleSheet(cardStyle());
  auto *controlsOuter = new QVBoxLayout(controlsCard);
  controlsOuter->setContentsMargins(18, 16, 18, 16);
  controlsOuter->setSpacing(14);
  // Keep controls content vertically centered within the card.
  controlsOuter->addStretch(1);

  auto *outTitle = new QLabel("OUTPUT TIME", controlsCard);
  outTitle->setStyleSheet(titleStyle());
  controlsOuter->addWidget(outTitle);

  outputTimeLabel_ = new QLabel(QTime(0, 0).addMSecs(outputSetDurationMs_).toString("m:ss"), controlsCard);
  outputTimeLabel_->setAlignment(Qt::AlignCenter);
  outputTimeLabel_->setMinimumHeight(72);
  outputTimeLabel_->setStyleSheet(
      "QLabel {"
      "  background: rgba(0,0,0,0.18);"
      "  border: 1px solid rgba(255,255,255,0.14);"
      "  border-radius: 14px;"
      "  padding: 18px 16px;"
      "  color: rgba(255,255,255,0.92);"
      "  font-family: \"" +
      QFontDatabase::systemFont(QFontDatabase::FixedFont).family() +
      "\";"
      "  font-size: 28px;"
      "  font-weight: 700;"
      "}");
  controlsOuter->addWidget(outputTimeLabel_);

  outputProgressBar_ = new ProgressPill(controlsCard);
  outputProgressBar_->setFixedHeight(30);
  controlsOuter->addWidget(outputProgressBar_);

  auto *stepRow = new QHBoxLayout();
  stepRow->setSpacing(12);

  outputMinus10sButton_ = new QPushButton("− 10s", controlsCard);
  outputPlus10sButton_ = new QPushButton("+ 10s", controlsCard);
  outputMinus1mButton_ = new QPushButton("− 1m", controlsCard);
  outputPlus1mButton_ = new QPushButton("+ 1m", controlsCard);

  for (auto *b :
       {outputMinus10sButton_, outputPlus10sButton_, outputMinus1mButton_, outputPlus1mButton_}) {
    b->setMinimumHeight(46);
  }

  stepRow->addWidget(outputMinus1mButton_);
  stepRow->addWidget(outputMinus10sButton_);
  stepRow->addWidget(outputPlus10sButton_);
  stepRow->addWidget(outputPlus1mButton_);
  controlsOuter->addLayout(stepRow);

  auto *actionsTitle = new QLabel("CONTROL", controlsCard);
  actionsTitle->setStyleSheet(titleStyle());
  controlsOuter->addWidget(actionsTitle);

  auto *actions = new QGridLayout();
  actions->setHorizontalSpacing(12);
  actions->setVerticalSpacing(12);

  startButton_ = new QPushButton("START", controlsCard);
  startButton_->setObjectName("primary");
  pauseButton_ = new QPushButton("PAUSE", controlsCard);
  stopButton_ = new QPushButton("STOP", controlsCard);

  startButton_->setMinimumHeight(54);
  pauseButton_->setMinimumHeight(54);
  stopButton_->setMinimumHeight(54);

  actions->addWidget(startButton_, 0, 0);
  actions->addWidget(pauseButton_, 0, 1);
  actions->addWidget(stopButton_, 0, 2);
  controlsOuter->addLayout(actions);

  controlsOuter->addStretch(1);

  // Layout: TOF (left/top), Indicators (left/bottom), Controls (right)
  root->addWidget(tofCard, 0, 0);
  root->addWidget(ioCard, 1, 0);
  root->addWidget(controlsCard, 0, 1, 2, 1);
  // Vertical split (10 units): TOF 6 / Indicators 4. Controls spans full height (10).
  root->setRowStretch(0, 6);
  root->setRowStretch(1, 4);
  // Left ~35% / Right ~65%
  root->setColumnStretch(0, 35);
  root->setColumnStretch(1, 65);

  centerWrap->addWidget(gridHost);
  centerWrap->addStretch(1);
  centerWrap->setStretch(0, 1);
  centerWrap->setStretch(1, 8);
  centerWrap->setStretch(2, 1);

  clockTimer_.setInterval(1000);
  connect(&clockTimer_, &QTimer::timeout, this, [this]() { update(); });
  clockTimer_.start();

  tickTimer_.setInterval(50);
  connect(&tickTimer_, &QTimer::timeout, this, [this]() {
    if (state_ == DeviceState::Running && xrayActive_) {
      const int elapsedTotal =
          outputElapsedAccumMs_ + static_cast<int>(outputElapsed_.elapsed());
      outputRemainingMs_ = std::max(0, outputRunDurationMs_ - elapsedTotal);
      progress_ = static_cast<int>(100.0 *
                                   (elapsedTotal / double(std::max(1, outputRunDurationMs_))));
      if (outputRemainingMs_ <= 0) {
        enterDone();
      }
    } else if (state_ == DeviceState::Ready) {
      progress_ = 0;
    }

    if (outputTimeLabel_) {
      if (state_ == DeviceState::Ready) {
        outputTimeLabel_->setText(QTime(0, 0).addMSecs(outputSetDurationMs_).toString("m:ss"));
      } else if (state_ == DeviceState::Done) {
        outputTimeLabel_->setText("DONE");
      } else {
        outputTimeLabel_->setText(QTime(0, 0).addMSecs(outputRemainingMs_).toString("m:ss"));
      }
    }

    if (outputProgressBar_) {
      outputProgressBar_->setVisible(true);
      outputProgressBar_->setValue(state_ == DeviceState::Ready ? 0 : progress_);
    }

    updateControlsEnabled();
    updateIndicators();
    update();
  });
  tickTimer_.start();

  const bool enableTof =
      AmustConfig::kTofEnableByDefault || envTruthy(qgetenv("AMUST_ENABLE_TOF"));
  if (enableTof) {
    usingRealTof_ = tofSensor_.start(
        AmustConfig::kTofPollIntervalSeconds,
        [this](int mm) {
          tofDistanceMm_ = mm;
          updateToFUi();
        },
        /*durationSeconds=*/0.0);
  }
  if (!usingRealTof_) {
    tofDistanceMm_ = -1;
    updateToFUi();
  }

  auto clampSetDuration = [this]() {
    // 10s .. 10m
    outputSetDurationMs_ = std::clamp(outputSetDurationMs_, AmustConfig::kOutputMinMs,
                                      AmustConfig::kOutputMaxMs);
    if (state_ == DeviceState::Ready) {
      outputRunDurationMs_ = outputSetDurationMs_;
      outputRemainingMs_ = outputSetDurationMs_;
    }
  };
  auto refreshSetLabel = [this]() {
    if (!outputTimeLabel_)
      return;
    outputTimeLabel_->setText(QTime(0, 0).addMSecs(outputSetDurationMs_).toString("m:ss"));
  };

  connect(outputMinus10sButton_, &QPushButton::clicked, this, [this, clampSetDuration, refreshSetLabel]() {
    outputSetDurationMs_ -= 10'000;
    clampSetDuration();
    refreshSetLabel();
  });
  connect(outputPlus10sButton_, &QPushButton::clicked, this, [this, clampSetDuration, refreshSetLabel]() {
    outputSetDurationMs_ += 10'000;
    clampSetDuration();
    refreshSetLabel();
  });
  connect(outputMinus1mButton_, &QPushButton::clicked, this, [this, clampSetDuration, refreshSetLabel]() {
    outputSetDurationMs_ -= 60'000;
    clampSetDuration();
    refreshSetLabel();
  });
  connect(outputPlus1mButton_, &QPushButton::clicked, this, [this, clampSetDuration, refreshSetLabel]() {
    outputSetDurationMs_ += 60'000;
    clampSetDuration();
    refreshSetLabel();
  });

  connect(startButton_, &QPushButton::clicked, this, [this]() { startXray(); });
  connect(pauseButton_, &QPushButton::clicked, this, [this]() { pauseOrResume(); });
  connect(stopButton_, &QPushButton::clicked, this, [this]() { stopAndReset(); });

  updateToFUi();
  updateIndicators();
  updateControlsEnabled();
}

QString MainMenuWidget::timeText() const {
  return QTime::currentTime().toString("hh:mm");
}

void MainMenuWidget::setState(DeviceState next) {
  state_ = next;
  switch (state_) {
  case DeviceState::Ready:
    deviceState_ = "READY";
    break;
  case DeviceState::Running:
    deviceState_ = "RUNNING";
    break;
  case DeviceState::Paused:
    deviceState_ = "PAUSE";
    break;
  case DeviceState::Done:
    deviceState_ = "DONE";
    break;
  }
  updateIndicators();
  updateControlsEnabled();
  update();
}

void MainMenuWidget::startXray() {
  if (state_ != DeviceState::Ready)
    return;

  xrayActive_ = true;
  outputRunDurationMs_ = outputSetDurationMs_;
  outputRemainingMs_ = outputRunDurationMs_;
  outputElapsedAccumMs_ = 0;
  outputElapsed_.restart();
  setState(DeviceState::Running);
}

void MainMenuWidget::pauseOrResume() {
  if (state_ == DeviceState::Running) {
    // Pause: stop x-ray (LEDs off), keep remaining time.
    xrayActive_ = false;
    outputElapsedAccumMs_ += static_cast<int>(outputElapsed_.elapsed());
    outputRemainingMs_ = std::max(0, outputRunDurationMs_ - outputElapsedAccumMs_);
    setState(DeviceState::Paused);
  } else if (state_ == DeviceState::Paused) {
    // Resume: continue for remaining time.
    xrayActive_ = true;
    // Keep original total duration so progress continues, not reset.
    outputElapsed_.restart();
    setState(DeviceState::Running);
  }
}

void MainMenuWidget::enterDone() {
  if (state_ == DeviceState::Done)
    return;

  xrayActive_ = false;
  progress_ = 100;
  outputRemainingMs_ = 0;
  outputElapsedAccumMs_ = outputRunDurationMs_;
  setState(DeviceState::Done);
}

void MainMenuWidget::stopAndReset() {
  xrayActive_ = false;
  progress_ = 0;
  outputRunDurationMs_ = outputSetDurationMs_;
  outputRemainingMs_ = outputSetDurationMs_;
  outputElapsedAccumMs_ = 0;
  setState(DeviceState::Ready);
}

void MainMenuWidget::updateToFUi() {
  if (!tofValueLabel_ || !tofStatusLabel_ || !tofHintLabel_)
    return;

  if (tofDistanceMm_ < 0) {
    tofValueLabel_->setText("-- mm");
  } else {
    tofValueLabel_->setText(QString::number(tofDistanceMm_) + " mm");
  }

  // Target distance window (tune as needed).
  constexpr int kMin = AmustConfig::kTofMinMm;
  constexpr int kMax = AmustConfig::kTofMaxMm;

  QString status;
  QString bg;
  QString border;
  QString text;

  if (tofDistanceMm_ < 0) {
    status = "TOF SENSOR NOT DETECTED";
    bg = "rgba(255, 70, 70, 0.26)";
    border = "rgba(255, 70, 70, 0.55)";
    text = "rgba(255, 190, 190, 0.98)";
  } else if (tofDistanceMm_ < kMin) {
    status = "TOO CLOSE";
    bg = "rgba(255, 70, 70, 0.26)";
    border = "rgba(255, 70, 70, 0.55)";
    text = "rgba(255, 190, 190, 0.98)";
  } else if (tofDistanceMm_ > kMax) {
    status = "TOO FAR";
    bg = "rgba(255, 180, 40, 0.26)";
    border = "rgba(255, 180, 40, 0.55)";
    text = "rgba(255, 225, 170, 0.98)";
  } else {
    status = "OK";
    bg = "rgba(70, 255, 180, 0.22)";
    border = "rgba(70, 255, 180, 0.50)";
    text = "rgba(190, 255, 230, 0.98)";
  }

  tofStatusLabel_->setText(status);
  tofStatusLabel_->setStyleSheet(pillStyle(bg, border, text) + monoStyle(12, true));
  tofHintLabel_->setText(QString("Target: %1–%2 mm (adjust position)")
                             .arg(AmustConfig::kTofMinMm)
                             .arg(AmustConfig::kTofMaxMm));
  updateControlsEnabled();
}

void MainMenuWidget::updateIndicators() {
  if (!laserValueLabel_ || !led1ValueLabel_ || !xrayValueLabel_)
    return;

  // HW note: Laser / LEDs / X-RAY share the same power (cannot be controlled independently).
  const bool xrayOn = (state_ == DeviceState::Running && xrayActive_);
  const bool laserOn = xrayOn;
  const bool ledsOn = xrayOn;

  laserValueLabel_->setText(laserOn ? "ON" : "OFF");
  laserValueLabel_->setStyleSheet(
      pillStyle(laserOn ? "rgba(70,255,180,0.22)" : "rgba(255,255,255,0.06)",
                laserOn ? "rgba(70,255,180,0.50)" : "rgba(255,255,255,0.10)",
                laserOn ? "rgba(190,255,230,0.98)" : "rgba(255,255,255,0.55)") +
      monoStyle(12, true));

  auto ledStyle = [&](bool on) {
    return pillStyle(on ? "rgba(255,70,70,0.26)" : "rgba(255,255,255,0.06)",
                    on ? "rgba(255,70,70,0.55)" : "rgba(255,255,255,0.10)",
                    on ? "rgba(255,190,190,0.98)" : "rgba(255,255,255,0.55)") +
           monoStyle(12, true);
  };

  led1ValueLabel_->setText(ledsOn ? "ON" : "OFF");
  led1ValueLabel_->setStyleSheet(ledStyle(ledsOn));

  const QString xrayText = xrayOn ? "RUNNING" : "READY";

  xrayValueLabel_->setText(xrayText);
  xrayValueLabel_->setStyleSheet(
      pillStyle(xrayOn ? "rgba(255,70,70,0.26)" : "rgba(255,255,255,0.06)",
                xrayOn ? "rgba(255,70,70,0.55)" : "rgba(255,255,255,0.10)",
                xrayOn ? "rgba(255,190,190,0.98)" : "rgba(255,255,255,0.55)") +
      monoStyle(12, true));

  gpio_.setLaser(laserOn);
  gpio_.setLed1(ledsOn);
  gpio_.setLed2(ledsOn);
  gpio_.setXrayEnable(xrayOn);
}

void MainMenuWidget::updateControlsEnabled() {
  if (!startButton_ || !pauseButton_ || !stopButton_ || !outputMinus10sButton_ || !outputPlus10sButton_ ||
      !outputMinus1mButton_ || !outputPlus1mButton_)
    return;

  const bool canAdjust = (state_ == DeviceState::Ready);
  outputMinus10sButton_->setEnabled(canAdjust);
  outputPlus10sButton_->setEnabled(canAdjust);
  outputMinus1mButton_->setEnabled(canAdjust);
  outputPlus1mButton_->setEnabled(canAdjust);

  startButton_->setEnabled(state_ == DeviceState::Ready);
  pauseButton_->setEnabled(state_ == DeviceState::Running || state_ == DeviceState::Paused);
  pauseButton_->setText(state_ == DeviceState::Paused ? "RESUME" : "PAUSE");

  stopButton_->setEnabled(state_ != DeviceState::Ready);
  if (state_ == DeviceState::Done) {
    stopButton_->setText("DONE");
    pauseButton_->setEnabled(false);
  } else {
    stopButton_->setText("STOP");
  }
}

void MainMenuWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  // Full-screen blue background
  const QRectF screen = fitAspect(QRectF(rect()), 1024.0, 600.0);
  {
    QLinearGradient bg(screen.topLeft(), screen.bottomLeft());
    bg.setColorAt(0.0, QColor("#1a4d7a"));
    bg.setColorAt(0.5, QColor("#2b7bc4"));
    bg.setColorAt(1.0, QColor("#0f2d4a"));
    p.fillRect(screen, bg);

    QRadialGradient glow(screen.center(), screen.width() * 0.55);
    glow.setColorAt(0.0, withAlpha(QColor("#3a8dd8"), 0.22));
    glow.setColorAt(0.6, withAlpha(QColor("#3a8dd8"), 0.05));
    glow.setColorAt(1.0, withAlpha(QColor("#3a8dd8"), 0.0));
    p.fillRect(screen, glow);
  }

  // Content paddings (keep same chrome as BootScreenWidget)
  const qreal padXBar = screen.width() * 0.04;
  const qreal topPad = screen.height() * 0.03;
  const qreal bottomPad = screen.height() * 0.02;
  const QRectF contentBar = screen.adjusted(padXBar, topPad, -padXBar, -bottomPad);

  // Top bar
  const qreal topBarH = contentBar.height() * 0.06;
  const QRectF topBar(contentBar.left(), contentBar.top(), contentBar.width(), topBarH);
  const qreal topLineY = topBar.bottom();

  // Top left: state
  {
    p.setFont(fixedFont(14));
    p.setPen(withAlpha(Qt::white, 0.72));
    const QString left = QString("STATE %1").arg(deviceState_);
    p.drawText(QRectF(topBar.left(), topBar.top(), topBar.width() * 0.45, topBarH),
               Qt::AlignVCenter | Qt::AlignLeft, left);
  }

  // Top center: time
  {
    p.setFont(fixedFont(16));
    p.setPen(withAlpha(Qt::white, 0.82));
    p.drawText(QRectF(topBar.left(), topBar.top(), topBar.width(), topBarH),
               Qt::AlignVCenter | Qt::AlignHCenter, timeText());
  }

  // Top right: dots
  {
    const int dotCount = 7;
    const qreal r = 4.0;
    const qreal gap = 6.0;
    const qreal totalW = dotCount * (2 * r) + (dotCount - 1) * gap;
    const qreal x0 = topBar.right() - totalW;
    const qreal y0 = topBar.center().y() - r;
    p.setPen(Qt::NoPen);
    p.setBrush(withAlpha(Qt::white, 0.55));
    for (int i = 0; i < dotCount; i++) {
      p.drawEllipse(QRectF(x0 + i * (2 * r + gap), y0, 2 * r, 2 * r));
    }
  }

  // Top border line
  {
    QLinearGradient line(QPointF(contentBar.left(), topLineY), QPointF(contentBar.right(), topLineY));
    line.setColorAt(0.0, withAlpha(Qt::white, 0.55));
    line.setColorAt(0.5, withAlpha(Qt::white, 0.8));
    line.setColorAt(1.0, withAlpha(Qt::white, 0.55));
    p.setPen(QPen(QBrush(line), 1.0));
    p.drawLine(QPointF(contentBar.left(), topLineY), QPointF(contentBar.right(), topLineY));
  }

  // Bottom bar layout
  const qreal bottomBarH = contentBar.height() * 0.12;
  const QRectF bottomBar(contentBar.left(), contentBar.bottom() - bottomBarH, contentBar.width(),
                         bottomBarH);
  const qreal footerHeight = 18.0;
  const qreal footerTopY = bottomBar.bottom() - footerHeight;
  const qreal topLineOffset = topLineY - topBar.top();
  qreal bottomLineY = bottomBar.bottom() - topLineOffset;
  bottomLineY = std::min(bottomLineY, footerTopY - 10.0);

  // Bottom border line
  {
    QLinearGradient line(QPointF(contentBar.left(), bottomLineY),
                         QPointF(contentBar.right(), bottomLineY));
    line.setColorAt(0.0, withAlpha(Qt::white, 0.55));
    line.setColorAt(0.5, withAlpha(Qt::white, 0.8));
    line.setColorAt(1.0, withAlpha(Qt::white, 0.55));
    p.setPen(QPen(QBrush(line), 1.0));
    p.drawLine(QPointF(contentBar.left(), bottomLineY), QPointF(contentBar.right(), bottomLineY));
  }

  // Left decorative circles
  {
    const QPointF base(contentBar.left() + 20, bottomLineY);
    p.setPen(QPen(withAlpha(Qt::white, 0.7), 2.0));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(base, 4.0, 4.0);
    p.setBrush(withAlpha(Qt::white, 0.18));
    p.drawEllipse(QPointF(base.x() + 16, base.y()), 6.0, 6.0);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(base.x() + 32, base.y()), 4.0, 4.0);
  }

  // ECG line (right)
  {
    const QRectF ecg(contentBar.right() - 110, bottomLineY - 14, 100, 28);
    QPainterPath path;
    path.moveTo(ecg.left(), ecg.center().y());
    path.lineTo(ecg.left() + ecg.width() * 0.2, ecg.center().y());
    path.lineTo(ecg.left() + ecg.width() * 0.25, ecg.top() + ecg.height() * 0.25);
    path.lineTo(ecg.left() + ecg.width() * 0.3, ecg.bottom() - ecg.height() * 0.25);
    path.lineTo(ecg.left() + ecg.width() * 0.35, ecg.center().y());
    path.lineTo(ecg.right(), ecg.center().y());
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(withAlpha(Qt::white, 0.9), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);
  }

  // Footer text
  {
    p.setFont(fixedFont(11));
    p.setPen(withAlpha(Qt::white, 0.5));
    const QString footer = QString::fromUtf8(
        u8"(주)유머스트알엔디 | Model AMUST-A001RPIW | HW Rev.C | FIRMWARE v1.0.0");
    p.drawText(QRectF(contentBar.left(), footerTopY, contentBar.width(), 16),
               Qt::AlignHCenter, footer);
  }

  // Screen glare
  {
    QRectF glare(screen.left(), screen.top(), screen.width(), screen.height() * 0.22);
    QLinearGradient g(glare.topLeft(), glare.bottomLeft());
    g.setColorAt(0.0, withAlpha(Qt::white, 0.06));
    g.setColorAt(1.0, withAlpha(Qt::white, 0.0));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawRect(glare);
  }
}
