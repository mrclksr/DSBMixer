/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "mixersettings.h"

MixerSettings::MixerSettings(bool lrView, bool scaleTicks, int chanMask,
                             int pollIvalMs, int unitChkIvalMs, int volInc,
                             bool inverseScroll, QObject *parent)
    : QObject(parent) {
  this->chanMask = chanMask;
  this->pollIvalMs = pollIvalMs;
  this->volInc = volInc;
  this->unitChkIvalMs = unitChkIvalMs;
  this->lrView = lrView;
  this->scaleTicks = scaleTicks;
  this->inverseScroll = inverseScroll;
}

void MixerSettings::setLRView(bool on) {
  if (lrView == on) return;
  lrView = on;
  emit lrViewChanged(on);
}

void MixerSettings::setScaleTicks(bool on) {
  if (scaleTicks == on) return;
  scaleTicks = on;
  emit scaleTicksChanged(on);
}

void MixerSettings::setChanMask(int mask) {
  if (chanMask == mask) return;
  chanMask = mask;
  emit chanMaskChanged(mask);
}

void MixerSettings::setPollIval(int ms) {
  if (pollIvalMs == ms) return;
  pollIvalMs = ms;
  emit pollIvalChanged(ms);
}

void MixerSettings::setUnitChkIval(int ms) {
  if (unitChkIvalMs == ms) return;
  unitChkIvalMs = ms;
  emit unitChkIvalChanged(ms);
}

void MixerSettings::setVolInc(int inc) {
  if (volInc == inc) return;
  volInc = inc;
  emit volIncChanged(inc);
}

void MixerSettings::setInverseScroll(bool on) {
  if (inverseScroll == on) return;
  inverseScroll = on;
  emit inverseScrollChanged(on);
}

bool MixerSettings::scaleTicksEnabled() const { return (scaleTicks); }

bool MixerSettings::lrViewEnabled() const { return (lrView); }

bool MixerSettings::inverseScrollEnabled() const { return (inverseScroll); }

int MixerSettings::getChanMask() const { return (chanMask); }

int MixerSettings::getPollIval() const { return (pollIvalMs); }

int MixerSettings::getUnitChkIval() const { return (unitChkIvalMs); }

int MixerSettings::getVolInc() const { return (volInc); }
