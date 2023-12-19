/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QObject>
#include <QTimer>

#include "range.h"

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
  Range getAplificationRange() const;
  Range getFeederRateQualityRange() const;
  Range getMaxAutoVchansRange() const;
  Range getLatencyRange() const;
 signals:
  void settingsChanged();

 private:
  template <typename T>
  void setter(T &member, T val);
  bool inRange(Range range, int val);

 private:
  int amplification{};
  int feederRateQuality{};
  int defaultUnit{};
  int maxAutoVchans{};
  int latency{};
  bool bypassMixer{false};
  bool changed{false};
  Range amplificationRange{1, 100};
  Range feederRateQualityRange{1, 4};
  Range maxAutoVchansRange{0, 999};
  Range latencyRange{0, 10};
  Range unitRange{0, 100};
};
