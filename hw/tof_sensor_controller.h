#pragma once

#include <QObject>
#include <QProcess>

#include <functional>

class TofSensorController final : public QObject {
  Q_OBJECT

public:
  explicit TofSensorController(QObject *parent = nullptr);
  ~TofSensorController() override;

  bool start(double intervalSeconds, std::function<void(int mm)> onDistanceUpdate,
             double durationSeconds = 0.0);
  void stop();
  bool isRunning() const;

private:
  QString resolveTofScriptPath() const;
  void attachProcessLogging(QProcess *process, const QString &label);

  QProcess *process_ = nullptr;
  std::function<void(int mm)> distanceCallback_;
};

