#include "progress_pill.h"

#include <algorithm>

#include <QPainter>
#include <QPainterPath>

ProgressPill::ProgressPill(QWidget *parent) : QWidget(parent) {
  setMinimumHeight(30);
}

void ProgressPill::setValue(int value01to100) {
  const int next = std::clamp(value01to100, 0, 100);
  if (next == value_) {
    return;
  }
  value_ = next;
  update();
}

void ProgressPill::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
  const qreal radius = r.height() * 0.5;

  // Track
  QPainterPath trackPath;
  trackPath.addRoundedRect(r, radius, radius);
  p.setPen(QPen(QColor(255, 255, 255, 26), 1.0));
  p.setBrush(QColor(255, 255, 255, 31));
  p.drawPath(trackPath);

  // Fill (clip to rounded track so it "fills" instead of looking like a blob that grows)
  const qreal w = r.width() * (value_ / 100.0);
  if (w > 0.0) {
    QRectF fill = r;
    fill.setWidth(std::max(w, 1.0));

    p.save();
    p.setClipPath(trackPath);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 140));
    p.drawRect(fill);
    p.restore();
  }

  // Percent text
  p.setPen(QColor(255, 255, 255, 190));
  QFont f = font();
  f.setBold(true);
  f.setPixelSize(std::max(10, int(r.height() * 0.45)));
  p.setFont(f);
  p.drawText(r, Qt::AlignCenter, QString::number(value_) + "%");
}
