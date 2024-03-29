/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */
#include "soundsettings.h"

#include <string.h>

#include <QDebug>
#include <stdexcept>
#include <string>

#include "libdsbmixer.h"

SoundSettings::SoundSettings(QObject *parent) : QObject(parent) { update(); }

void SoundSettings::update() {
  amplification = dsbmixer_amplification();
  feederRateQuality = dsbmixer_feeder_rate_quality();
  defaultUnit = dsbmixer_default_unit();
  maxAutoVchans = dsbmixer_maxautovchans();
  latency = dsbmixer_latency();
  bypassMixer = dsbmixer_bypass_mixer();
}

void SoundSettings::applySettings() {
  if (!checkSettingsChanged()) return;
  if (dsbmixer_change_settings(defaultUnit, amplification, feederRateQuality,
                               latency, maxAutoVchans, bypassMixer) == -1) {
    std::string error{std::string{"dsbmixer_change_settings() failed:"} +
                      std::string{dsbmixer_error()}};
    throw std::runtime_error{error};
  }
  update();
  emit settingsChanged();
}

bool SoundSettings::inRange(Range range, int val) {
  return (range.min <= val && range.max >= val);
}

bool SoundSettings::checkSettingsChanged() {
  return (amplification != dsbmixer_amplification() ||
          feederRateQuality != dsbmixer_feeder_rate_quality() ||
          defaultUnit != dsbmixer_default_unit() ||
          maxAutoVchans != dsbmixer_maxautovchans() ||
          latency != dsbmixer_latency() ||
          bypassMixer != dsbmixer_bypass_mixer());
}

void SoundSettings::setAmplification(int amplification) {
  if (!inRange(amplificationRange, amplification)) return;
  this->amplification = amplification;
}

void SoundSettings::setFeederRateQuality(int quality) {
  if (!inRange(feederRateQualityRange, quality)) return;
  this->feederRateQuality = quality;
}

void SoundSettings::setDefaultUnit(int unit) {
  if (!inRange(unitRange, unit)) return;
  this->defaultUnit = unit;
}

void SoundSettings::setMaxAutoVchans(int nchans) {
  if (!inRange(maxAutoVchansRange, nchans)) return;
  this->maxAutoVchans = nchans;
}

void SoundSettings::setLatency(int latency) {
  if (!inRange(latencyRange, latency)) return;
  this->latency = latency;
}

void SoundSettings::setBypassMixer(bool bypass) { this->bypassMixer = bypass; }

int SoundSettings::getAmplification() const { return (amplification); }

int SoundSettings::getFeederRateQuality() const { return (feederRateQuality); }

int SoundSettings::getDefaultUnit() const { return (defaultUnit); }

int SoundSettings::getMaxAutoVchans() const { return (maxAutoVchans); }

int SoundSettings::getLatency() const { return (latency); }

bool SoundSettings::getBypassMixer() const { return (bypassMixer); }

Range SoundSettings::getAplificationRange() const {
  return (amplificationRange);
}

Range SoundSettings::getFeederRateQualityRange() const {
  return (feederRateQualityRange);
}

Range SoundSettings::getLatencyRange() const { return (latencyRange); }

Range SoundSettings::getMaxAutoVchansRange() const {
  return (maxAutoVchansRange);
}
