/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QObject>

class MixerSettings : public QObject {
  Q_OBJECT
 public:
  MixerSettings(bool lrView = false, bool scaleTicks = true,
                int chanMask = 0xff, int pollIvalMs = 900,
                int unitChkIval = 3000, int volInc = 3,
                bool inverseScroll = false, QObject *parent = nullptr);

  void setLRView(bool on);
  void setScaleTicks(bool on);
  void setChanMask(int mask);
  void setPollIval(int ms);
  void setUnitChkIval(int ms);
  void setVolInc(int volInc);
  void setInverseScroll(bool on);
  bool scaleTicksEnabled() const;
  bool lrViewEnabled() const;
  bool inverseScrollEnabled() const;
  int getChanMask() const;
  int getPollIval() const;
  int getUnitChkIval() const;
  int getVolInc() const;

 signals:
  void settingsChanged();
  void pollIvalChanged(int ms);
  void unitChkIvalChanged(int ms);
  void lrViewChanged(bool on);
  void chanMaskChanged(int mask);
  void scaleTicksChanged(bool on);
  void volIncChanged(int volInc);
  void inverseScrollChanged(bool on);

 private:
  int volInc{3};
  int chanMask{0xff};
  int pollIvalMs{900};
  int unitChkIvalMs{3000};
  bool lrView{false};
  bool scaleTicks{true};
  bool inverseScroll{false};
};
