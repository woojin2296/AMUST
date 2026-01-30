#pragma once

#include <QWidget>

class ProgressPill final : public QWidget {
  Q_OBJECT

public:
  explicit ProgressPill(QWidget *parent = nullptr);

  void setValue(int value01to100);
  int value() const { return value_; }

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  int value_ = 0;
};

