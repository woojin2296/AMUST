#pragma once

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

class BootScreenWidget final : public QWidget {
  Q_OBJECT

public:
  explicit BootScreenWidget(QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

signals:
  void continueRequested();

private:
  QString timeText() const;

  QTimer progressTimer_;
  QTimer clockTimer_;
  QTimer pulseTimer_;

  int progress_ = 0;
  double pulsePhase_ = 0.0;

  QString deviceState_ = "BOOT";
};

