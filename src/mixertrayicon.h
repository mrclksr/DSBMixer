/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <memory.h>

#include <QEvent>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <QtDBus/QDBusConnection>

#include "chanslider.h"
#include "iconloader.h"
#include "mixer.h"
#include "mixersettings.h"

class TrayIcon : public QSystemTrayIcon {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.dsb.dsbmixer")
 public:
  TrayIcon(const IconLoader &iconLoader, const MixerSettings &mixerSettings,
           QMenu &trayMenu, Mixer *mixer, QObject *parent = nullptr);
  int getVolInc() const;
  void setMixer(Mixer *mixer);
  bool event(QEvent *ev);
 private slots:
  void initSlider(int lvol, int rvol);
  void showSlider(int lvol, int rvol);
  void hideSlider();
  void update();
  void updateIcon(int lvol, int rvol);
  void updateToolTip(int lvol, int rvol);
  void deleteSlider();
  void catchMixerDestroyed();
  void catchTrayIconThemeChanged();
  void catchLRViewChanged(bool on);
  void catchScaleTicksChanged(bool on);
  void catchVolIncChanged(int volInc);

 public slots:
  Q_SCRIPTABLE void incVol(uint amount);
  Q_SCRIPTABLE void decVol(uint amount);

 private:
  void registerDBusService();
  void updateSlider();

 private:
  int volInc{3};
  bool lrView{false};
  Mixer *mixer{nullptr};
  ChanSlider *slider{nullptr};
  QTimer *sliderTimer{nullptr};
  const IconLoader *iconLoader{nullptr};
  const MixerSettings *mixerSettings{nullptr};
};
