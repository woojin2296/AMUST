#include "tof_sensor_controller.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace {
QString sanitizeLine(const QByteArray &data) {
  return QString::fromUtf8(data).trimmed();
}

bool envTruthy(const QByteArray &v) {
  if (v.isEmpty())
    return false;
  const QByteArray lower = v.toLower();
  return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}
} // namespace

TofSensorController::TofSensorController(QObject *parent) : QObject(parent) {}

TofSensorController::~TofSensorController() {
  stop();
}

bool TofSensorController::start(double intervalSeconds, std::function<void(int mm)> onDistanceUpdate,
                                double durationSeconds) {
  stop();

  distanceCallback_ = std::move(onDistanceUpdate);
  process_ = new QProcess(this);
  attachProcessLogging(process_, QStringLiteral("TOF.py"));

  connect(process_, &QProcess::readyReadStandardOutput, this, [this]() {
    if (!process_)
      return;
    const QList<QByteArray> lines = process_->readAllStandardOutput().split('\n');
    for (const QByteArray &line : lines) {
      const QString trimmed = sanitizeLine(line);
      if (trimmed.isEmpty())
        continue;
      bool ok = false;
      const int mm = trimmed.toInt(&ok);
      if (!ok)
        continue;
      const int normalizedMm = std::max(-1, mm);
      if (distanceCallback_)
        distanceCallback_(normalizedMm);
    }
  });

  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this](int exitCode, QProcess::ExitStatus status) {
            if (status != QProcess::NormalExit || exitCode != 0) {
              qWarning() << "TOF.py exited" << exitCode << "status" << status;
            }
            if (process_) {
              process_->deleteLater();
              process_ = nullptr;
            }
          });

  QString script = resolveTofScriptPath();
  if (script.isEmpty())
    script = QStringLiteral("TOF.py");

  QStringList args;
  args << script;

  const QByteArray envBus = qgetenv("AMUST_TOF_BUS");
  const QByteArray envAddr = qgetenv("AMUST_TOF_ADDR");
  if (!envBus.isEmpty()) {
    args << "--bus" << QString::fromUtf8(envBus);
  }
  if (!envAddr.isEmpty()) {
    args << "--addr" << QString::fromUtf8(envAddr);
  }

  args << "--interval" << QString::number(std::max(0.05, intervalSeconds));
  if (durationSeconds > 0.0)
    args << "--duration" << QString::number(durationSeconds);

  const QString python = QStringLiteral("python3");
  process_->start(python, args);
  if (!process_->waitForStarted(3000)) {
    qWarning() << "Failed to start TOF.py:" << process_->errorString();
    stop();
    return false;
  }

  // Optionally force-disable to avoid noisy failures in dev environments.
  if (envTruthy(qgetenv("AMUST_DISABLE_TOF"))) {
    stop();
    return false;
  }

  return true;
}

void TofSensorController::stop() {
  distanceCallback_ = nullptr;
  if (!process_)
    return;

  QObject::disconnect(process_, nullptr, this, nullptr);
  if (process_->state() != QProcess::NotRunning) {
    process_->terminate();
    if (!process_->waitForFinished(1000)) {
      process_->kill();
      process_->waitForFinished();
    }
  }
  process_->deleteLater();
  process_ = nullptr;
}

bool TofSensorController::isRunning() const {
  return process_ && process_->state() != QProcess::NotRunning;
}

QString TofSensorController::resolveTofScriptPath() const {
  const QByteArray env = qgetenv("AMUST_TOF_SCRIPT");
  if (!env.isEmpty()) {
    const QString p = QString::fromUtf8(env);
    if (QFileInfo::exists(p))
      return QFileInfo(p).canonicalFilePath();
  }

  const QString dir = QCoreApplication::applicationDirPath();
  QStringList candidates;
  candidates << (dir + QDir::separator() + QStringLiteral("TOF.py"));
  candidates << (dir + QDir::separator() + QStringLiteral("../TOF.py"));
  candidates << (dir + QDir::separator() + QStringLiteral("../../TOF.py"));
  candidates << (dir + QDir::separator() + QStringLiteral("../Resources/TOF.py"));
  candidates << (dir + QDir::separator() + QStringLiteral("../share/amust/TOF.py"));
  candidates << (dir + QDir::separator() + QStringLiteral("../../share/amust/TOF.py"));
  for (const QString &p : candidates) {
    if (QFileInfo::exists(p))
      return QFileInfo(p).canonicalFilePath();
  }
  return QString();
}

void TofSensorController::attachProcessLogging(QProcess *process, const QString &label) {
  connect(process, &QProcess::readyReadStandardError, this, [process, label]() {
    if (!process)
      return;
    const QString line = sanitizeLine(process->readAllStandardError());
    if (!line.isEmpty())
      qWarning().noquote() << label << ":" << line;
  });
  connect(process, &QProcess::errorOccurred, this, [process, label](QProcess::ProcessError error) {
    if (!process)
      return;
    qWarning() << label << "error" << error << process->errorString();
  });
}
