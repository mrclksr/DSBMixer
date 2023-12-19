/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QObject>

#include "config.h"
#include "dsbcfg.h"
#include "range.h"

class MixerSettings : public QObject {
  Q_OBJECT
 public:
  MixerSettings(dsbcfg_t &cfg, QObject *parent = nullptr);
  void setLRView(bool on);
  void setScaleTicks(bool on);
  void setChanMask(int mask);
  void setPollIval(int ms);
  void setUnitChkIval(int ms);
  void setVolInc(int volInc);
  void setInverseScroll(bool on);
  void setTrayThemeName(QString theme);
  void setPlayCmd(QString cmd);
  void setWindowGeometry(int x, int y, int width, int height);
  void storeSettings();
  bool scaleTicksEnabled() const;
  bool lrViewEnabled() const;
  bool inverseScrollEnabled() const;
  int getChanMask() const;
  int getPollIval() const;
  int getUnitChkIval() const;
  int getVolInc() const;
  int getWinPosX() const;
  int getWinPosY() const;
  int getWinWidth() const;
  int getWinHeight() const;
  QString getPlayCmd() const;
  QString getTrayThemeName() const;
  Range getPollIvalRange() const;
  Range getUnitChkIvalRange() const;
  Range getVolIncRange() const;

 private:
  template <typename T>
  bool setter(T &member, T val);
  bool inRange(Range range, int val);
 signals:
  void settingsChanged();
  void pollIvalChanged(int ms);
  void unitChkIvalChanged(int ms);
  void lrViewChanged(bool on);
  void chanMaskChanged(int mask);
  void scaleTicksChanged(bool on);
  void volIncChanged(int volInc);
  void inverseScrollChanged(bool on);
  void trayThemeChanged(QString theme);
  void playCmdChanged(QString cmd);

 private:
  int volInc{3};
  int chanMask{0xff};
  int pollIvalMs{900};
  int unitChkIvalMs{3000};
  int x{};
  int y{};
  int width{};
  int height{};
  bool lrView{false};
  bool scaleTicks{true};
  bool inverseScroll{false};
  dsbcfg_t *cfg{nullptr};
  QString trayThemeName;
  QString playCmd;
  Range pollIvalRange{10, 999999999};
  Range unitChkIvalRange{10, 999999999};
  Range volIncRange{1, 50};
};
