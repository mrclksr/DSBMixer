/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QObject>
#include <QTimer>

class SoundSettings : public QObject {
  Q_OBJECT
 public:
  SoundSettings(QObject *parent = nullptr);
  void update();
  void setAmplification(int amplification);
  void setFeederRateQuality(int quality);
  void setDefaultUnit(int unit);
  void setMaxAutoVchans(int nchans);
  void setLatency(int latency);
  void setBypassMixer(bool bypass);
  void applySettings();
  int getAmplification() const;
  int getFeederRateQuality() const;
  int getDefaultUnit() const;
  int getMaxAutoVchans() const;
  int getLatency() const;
  bool getBypassMixer() const;

 signals:
  void settingsChanged();

 private:
  template <typename T>
  void setter(T &member, T val);

 private:
  int amplification{};
  int feederRateQuality{};
  int defaultUnit{};
  int maxAutoVchans{};
  int latency{};
  bool bypassMixer{false};
  bool changed{false};
};
