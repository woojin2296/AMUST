#include "boot_screen_widget.h"

#include <algorithm>
#include <cmath>

#include <QDateTime>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {

constexpr double kPi = 3.14159265358979323846;

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

} // namespace

BootScreenWidget::BootScreenWidget(QWidget *parent) : QWidget(parent) {
  setAttribute(Qt::WA_OpaquePaintEvent);
  setAutoFillBackground(false);

  progressTimer_.setInterval(50);
  connect(&progressTimer_, &QTimer::timeout, this, [this]() {
    progress_ = (progress_ >= 100) ? 0 : (progress_ + 1);
    update();
  });
  progressTimer_.start();

  clockTimer_.setInterval(1000);
  connect(&clockTimer_, &QTimer::timeout, this, [this]() { update(); });
  clockTimer_.start();

  pulseTimer_.setInterval(16);
  connect(&pulseTimer_, &QTimer::timeout, this, [this]() {
    // ~1.6s period (matches web)
    pulsePhase_ += (2.0 * kPi) * (pulseTimer_.interval() / 1600.0);
    if (pulsePhase_ > 2.0 * kPi)
      pulsePhase_ -= 2.0 * kPi;
    update();
  });
  pulseTimer_.start();
}

QString BootScreenWidget::timeText() const {
  return QTime::currentTime().toString("hh:mm");
}

void BootScreenWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    emit continueRequested();
  }
  QWidget::mousePressEvent(event);
}

void BootScreenWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  // Full-screen blue background (no outer frame/bezel)
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

  // Content paddings (match web-ish)
  const qreal padXMain = screen.width() * 0.06;
  const qreal padXBar = screen.width() * 0.04;
  const qreal topPad = screen.height() * 0.03;
  const qreal bottomPad = screen.height() * 0.02;

  const QRectF contentMain =
      screen.adjusted(padXMain, topPad, -padXMain, -bottomPad);
  const QRectF contentBar = screen.adjusted(padXBar, topPad, -padXBar, -bottomPad);

  // Top bar
  const qreal topBarH = contentBar.height() * 0.06;
  const QRectF topBar =
      QRectF(contentBar.left(), contentBar.top(), contentBar.width(), topBarH);
  const qreal topLineY = topBar.bottom();

  // Top left: state
  {
    p.setFont(fixedFont(14));
    p.setPen(withAlpha(Qt::white, 0.72));
    const QString left = QString("STATE %1").arg(deviceState_);
    p.drawText(QRectF(topBar.left(), topBar.top(), topBar.width() * 0.45, topBarH),
               Qt::AlignVCenter | Qt::AlignLeft, left);
  }

  // Top center: time (absolute center)
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
    QLinearGradient line(QPointF(contentBar.left(), topLineY),
                         QPointF(contentBar.right(), topLineY));
    line.setColorAt(0.0, withAlpha(Qt::white, 0.55));
    line.setColorAt(0.5, withAlpha(Qt::white, 0.8));
    line.setColorAt(1.0, withAlpha(Qt::white, 0.55));
    QPen pen(QBrush(line), 1.0);
    p.setPen(pen);
    p.drawLine(QPointF(contentBar.left(), topLineY), QPointF(contentBar.right(), topLineY));
  }

  // Bottom bar layout
  const qreal bottomBarH = contentBar.height() * 0.12;
  const qreal bottomLift = 0.0;
  const QRectF bottomBar = QRectF(contentBar.left(),
                                  contentBar.bottom() - bottomBarH - bottomLift,
                                  contentBar.width(), bottomBarH);
  const qreal footerHeight = 18.0;
  const qreal footerTopY = bottomBar.bottom() - footerHeight;
  const qreal topLineOffset = topLineY - topBar.top();
  qreal bottomLineY = bottomBar.bottom() - topLineOffset;
  bottomLineY = std::min(bottomLineY, footerTopY - 10.0);

  // Main area (logo + text)
  const QRectF mainArea =
      QRectF(contentMain.left(), topLineY + contentMain.height() * 0.02, contentMain.width(),
             (bottomBar.top() - (topLineY + contentMain.height() * 0.02)));

  // Logo (simple rings + hex + A)
  {
    const QPointF c(mainArea.center().x(), mainArea.top() + mainArea.height() * 0.42);
    const qreal size = qMin(mainArea.width(), mainArea.height()) * 0.23;

    auto drawRing = [&](qreal r, const QColor &col, qreal w) {
      QPen pen(withAlpha(col, col.alphaF()), w);
      pen.setCapStyle(Qt::RoundCap);
      p.setPen(pen);
      p.setBrush(Qt::NoBrush);
      p.drawEllipse(c, r, r);
    };

    // Soft plate
    {
      QRadialGradient g(c, size * 0.95);
      g.setColorAt(0.0, withAlpha(Qt::white, 0.12));
      g.setColorAt(0.7, withAlpha(QColor("#a8d5f7"), 0.07));
      g.setColorAt(1.0, withAlpha(Qt::white, 0.0));
      p.setPen(Qt::NoPen);
      p.setBrush(g);
      p.drawEllipse(c, size * 0.95, size * 0.95);
      p.setBrush(withAlpha(Qt::white, 0.03));
      p.drawEllipse(c, size * 0.8, size * 0.8);
    }

    drawRing(size * 0.86, withAlpha(Qt::white, 0.95), 4.0);
    drawRing(size * 0.67, withAlpha(QColor("#a8d5f7"), 0.7), 3.0);

    // Hex
    auto hexPath = [&](qreal r) {
      QPainterPath path;
      for (int i = 0; i < 6; i++) {
        const double a = (kPi / 3.0) * i - kPi / 2.0;
        const QPointF pt(c.x() + r * std::cos(a), c.y() + r * std::sin(a));
        if (i == 0)
          path.moveTo(pt);
        else
          path.lineTo(pt);
      }
      path.closeSubpath();
      return path;
    };

    {
      p.save();
      p.setRenderHint(QPainter::Antialiasing, true);
      p.setBrush(Qt::NoBrush);
      p.setPen(QPen(withAlpha(Qt::white, 0.95), 6.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p.drawPath(hexPath(size * 0.5));
      p.setPen(QPen(withAlpha(QColor("#a8d5f7"), 0.55), 3.0, Qt::SolidLine, Qt::RoundCap,
                    Qt::RoundJoin));
      p.drawPath(hexPath(size * 0.38));
      p.restore();
    }

    // "A" monogram
    {
      QPainterPath a;
      const qreal topY = c.y() - size * 0.34;
      const qreal botY = c.y() + size * 0.34;
      const qreal leftX = c.x() - size * 0.20;
      const qreal rightX = c.x() + size * 0.20;
      const qreal midY = c.y() + size * 0.06;
      a.moveTo(c.x(), topY);
      a.lineTo(rightX, botY);
      a.lineTo(c.x() + size * 0.11, botY);
      a.lineTo(c.x() + size * 0.06, botY - size * 0.14);
      a.lineTo(c.x() - size * 0.06, botY - size * 0.14);
      a.lineTo(c.x() - size * 0.11, botY);
      a.lineTo(leftX, botY);
      a.closeSubpath();

      p.setBrush(Qt::NoBrush);
      p.setPen(QPen(withAlpha(Qt::white, 0.95), 6.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p.drawPath(a);
      p.setPen(QPen(withAlpha(QColor("#a8d5f7"), 0.75), 6.0, Qt::SolidLine, Qt::RoundCap,
                    Qt::RoundJoin));
      p.drawLine(QPointF(c.x() - size * 0.06, midY), QPointF(c.x() + size * 0.06, midY));
    }
  }

  // Device info text
  {
    const qreal infoY = mainArea.bottom() - mainArea.height() * 0.28;
    p.setPen(Qt::white);
    QFont title = font();
    title.setBold(true);
    title.setPixelSize(24);
    p.setFont(title);
    p.drawText(QRectF(contentMain.left(), infoY, contentMain.width(), 30), Qt::AlignHCenter, "AMUST");

    p.setFont(fixedFont(13));
    p.setPen(withAlpha(Qt::white, 0.8));
    p.drawText(QRectF(contentMain.left(), infoY + 30, contentMain.width(), 22), Qt::AlignHCenter,
               "High-Voltage Energy Emission Device");
  }

  // Click to continue (pulse)
  {
    const double s = 0.5 * (1.0 + std::sin(pulsePhase_));
    const double alpha = 0.15 + (0.85 * s);
    p.setFont(fixedFont(12));
    p.setPen(withAlpha(Qt::white, 0.7 * alpha));
    p.drawText(QRectF(contentMain.left(), mainArea.bottom() - mainArea.height() * 0.12,
                      contentMain.width(), 18),
               Qt::AlignHCenter, "CLICK TO CONTINUE");
  }

  // Bottom border line
  {
    QLinearGradient line(QPointF(contentBar.left(), bottomLineY),
                         QPointF(contentBar.right(), bottomLineY));
    line.setColorAt(0.0, withAlpha(Qt::white, 0.55));
    line.setColorAt(0.5, withAlpha(Qt::white, 0.8));
    line.setColorAt(1.0, withAlpha(Qt::white, 0.55));
    QPen pen(QBrush(line), 1.0);
    p.setPen(pen);
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
        u8"UMUSTR&D | Model AMUST-A001RPIW | HW Rev.C | FIRMWARE v1.0.0");
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
