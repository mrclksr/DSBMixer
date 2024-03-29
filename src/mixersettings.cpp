/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include <QApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#else
#include <QStringConverter>
#endif
#include "mixersettings.h"

MixerSettings::MixerSettings(dsbcfg_t &cfg, QObject *parent)
    : QObject(parent), cfg{&cfg} {
  lrView = dsbcfg_getval(this->cfg, CFG_LRVIEW).boolean;
  scaleTicks = dsbcfg_getval(this->cfg, CFG_TICKS).boolean;
  chanMask = dsbcfg_getval(this->cfg, CFG_MASK).integer;
  inverseScroll = dsbcfg_getval(this->cfg, CFG_INVERSE_SCROLL).boolean;
  width = dsbcfg_getval(this->cfg, CFG_WIDTH).integer;
  height = dsbcfg_getval(this->cfg, CFG_HEIGHT).integer;
  x = dsbcfg_getval(this->cfg, CFG_POS_X).integer;
  y = dsbcfg_getval(this->cfg, CFG_POS_Y).integer;
  if (dsbcfg_getval(this->cfg, CFG_PLAY_CMD).string != NULL)
    playCmd = QString(dsbcfg_getval(this->cfg, CFG_PLAY_CMD).string);
  else
    playCmd = "";
  if (dsbcfg_getval(this->cfg, CFG_TRAY_THEME).string != NULL)
    trayThemeName = QString(dsbcfg_getval(this->cfg, CFG_TRAY_THEME).string);
  else
    trayThemeName = "";
  setPollIval(dsbcfg_getval(this->cfg, CFG_POLL_IVAL).integer);
  setUnitChkIval(dsbcfg_getval(this->cfg, CFG_UNIT_CHK_IVAL).integer);
  setVolInc(dsbcfg_getval(this->cfg, CFG_VOL_INC).integer);
}

void MixerSettings::storeSettings() {
  dsbcfg_set_bool(this->cfg, CFG_LRVIEW, lrView);
  dsbcfg_set_bool(this->cfg, CFG_TICKS, scaleTicks);
  dsbcfg_set_bool(this->cfg, CFG_INVERSE_SCROLL, inverseScroll);
  dsbcfg_set_int(this->cfg, CFG_MASK, chanMask);
  dsbcfg_set_int(this->cfg, CFG_POLL_IVAL, pollIvalMs);
  dsbcfg_set_int(this->cfg, CFG_UNIT_CHK_IVAL, unitChkIvalMs);
  dsbcfg_set_int(this->cfg, CFG_VOL_INC, volInc);
  dsbcfg_set_int(this->cfg, CFG_POS_X, x);
  dsbcfg_set_int(this->cfg, CFG_POS_Y, y);
  dsbcfg_set_int(this->cfg, CFG_WIDTH, width);
  dsbcfg_set_int(this->cfg, CFG_HEIGHT, height);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  auto *codec{QTextCodec::codecForLocale()};
  QByteArray playCmdStr{codec->fromUnicode(playCmd)};
  QByteArray trayThemeStr{codec->fromUnicode(trayThemeName)};
#else
  auto encoder{QStringEncoder(QStringEncoder::Utf8)};
  QByteArray playCmdStr{encoder(playCmd)};
  QByteArray trayThemeStr{encoder(trayThemeName)};
#endif
  dsbcfg_set_string(this->cfg, CFG_TRAY_THEME, trayThemeStr.data());
  dsbcfg_set_string(this->cfg, CFG_PLAY_CMD, playCmdStr.data());
}

bool MixerSettings::inRange(Range range, int val) {
  return (range.min <= val && range.max >= val);
}

template <typename T>
bool MixerSettings::setter(T &member, T val) {
  if (member == val) return (false);
  member = val;
  return (true);
}

void MixerSettings::setLRView(bool on) {
  if (setter(this->lrView, on)) emit lrViewChanged(on);
}

void MixerSettings::setScaleTicks(bool on) {
  if (setter(this->scaleTicks, on)) emit scaleTicksChanged(on);
}

void MixerSettings::setChanMask(int mask) {
  if (setter(this->chanMask, mask)) emit chanMaskChanged(mask);
}

void MixerSettings::setPollIval(int ms) {
  if (!inRange(pollIvalRange, ms)) return;
  if (setter(this->pollIvalMs, ms)) emit pollIvalChanged(ms);
}

void MixerSettings::setUnitChkIval(int ms) {
  if (!inRange(unitChkIvalRange, ms)) return;
  if (setter(this->unitChkIvalMs, ms)) emit unitChkIvalChanged(ms);
}

void MixerSettings::setVolInc(int inc) {
  if (!inRange(volIncRange, inc)) return;
  if (setter(volInc, inc)) emit volIncChanged(inc);
}

void MixerSettings::setInverseScroll(bool on) {
  if (setter(this->inverseScroll, on)) emit inverseScrollChanged(on);
}

void MixerSettings::setTrayThemeName(QString theme) {
  if (setter(this->trayThemeName, theme)) emit trayThemeChanged(theme);
}

void MixerSettings::setPlayCmd(QString cmd) {
  if (setter(playCmd, cmd)) emit playCmdChanged(cmd);
}

void MixerSettings::setWindowGeometry(int x, int y, int width, int height) {
  this->x = x;
  this->y = y;
  this->width = width;
  this->height = height;
}

bool MixerSettings::scaleTicksEnabled() const { return (scaleTicks); }

bool MixerSettings::lrViewEnabled() const { return (lrView); }

bool MixerSettings::inverseScrollEnabled() const { return (inverseScroll); }

int MixerSettings::getChanMask() const { return (chanMask); }

int MixerSettings::getPollIval() const { return (pollIvalMs); }

int MixerSettings::getUnitChkIval() const { return (unitChkIvalMs); }

int MixerSettings::getVolInc() const { return (volInc); }

int MixerSettings::getWinPosX() const { return (x); }

int MixerSettings::getWinPosY() const { return (y); }

int MixerSettings::getWinWidth() const { return (width); }

int MixerSettings::getWinHeight() const { return (height); }

QString MixerSettings::getPlayCmd() const { return (playCmd); }

QString MixerSettings::getTrayThemeName() const { return (trayThemeName); }

Range MixerSettings::getPollIvalRange() const { return (pollIvalRange); }

Range MixerSettings::getUnitChkIvalRange() const { return (unitChkIvalRange); }

Range MixerSettings::getVolIncRange() const { return (volIncRange); }
