/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QObject>
#include <QTimer>

class SoundSettings: public QObject {
  Q_OBJECT
public:
  SoundSettings(int unitCheckIvalMs = 3000, QObject *parent = nullptr);
  void update();
  void setAmplification(int amplification);
  void setFeederRateQuality(int quality);
  void setDefaultUnit(int unit);
  void setMaxAutoVchans(int nchans);
  void setLatency(int latency);
  void setBypassMixer(bool bypass);
  void setUnitCheckIval(int ms);
  void suspendUnitCheck();
  void resumeUnitCheck();
  void applySettings();
  int getAmplification() const;
  int getFeederRateQuality() const;
  int getDefaultUnit() const;
  int getMaxAutoVchans() const;
  int getLatency() const;
  int getUnitCheckIval() const;
  bool getBypassMixer() const;

signals:
  void settingsChanged();
  void defaultUnitChanged(int unit);

private slots:
  void checkDefaultUnitChanged();

private:
  template <typename T>
  void setter(T &member, T val);

 private:
  int unitCheckIvalMs{3000};
  int amplification{};
  int feederRateQuality{};
  int defaultUnit{};
  int maxAutoVchans{};
  int latency{};
  bool bypassMixer{false};
  bool changed{false};
  bool unitCheckSuspended{false};
  QTimer *unitCheckTimer{nullptr};
};
