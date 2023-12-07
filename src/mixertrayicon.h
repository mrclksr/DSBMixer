/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <memory.h>

#include <KStatusNotifierItem>
#include <QMenu>
#include <QObject>
#include <QWidget>
#include <QtDBus/QDBusConnection>

#include "iconloader.h"
#include "mixer.h"
#include "mixersettings.h"

class TrayIcon : public KStatusNotifierItem {
  Q_OBJECT
 public:
  TrayIcon(const IconLoader &iconLoader, const MixerSettings &mixerSettings,
           QMenu &trayMenu, Mixer *mixer, QObject *parent = nullptr);
  int getVolInc() const;
  void setMixer(Mixer *mixer);
 private slots:
  void update();
  void updateIcon(int lvol, int rvol);
  void updateToolTip(int lvol, int rvol);
  void catchMixerDestroyed();
  void catchTrayIconThemeChanged();
  void catchInverseScrollChanged(bool on);
  void catchLRViewChanged(bool on);
  void catchVolIncChanged(int volInc);
  void catchScrollRequest(int delta, Qt::Orientation orientation);

 private:
  void registerDBusService();

 private:
  int volInc{3};
  int direction{1};
  bool lrView{false};
  Mixer *mixer{nullptr};
  const IconLoader *iconLoader{nullptr};
  const MixerSettings *mixerSettings{nullptr};
};
