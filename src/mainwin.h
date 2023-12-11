/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QCloseEvent>
#include <QIcon>
#include <QMainWindow>
#include <QMoveEvent>
#include <QWidget>
#include <QMutex>

#include "appsmixer.h"
#include "mixerlist.h"
#include "mixersettings.h"
#include "soundsettings.h"
#include "mixertabs.h"
#include "mixertrayicon.h"

class MainWin : public QMainWindow {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.dsb.dsbmixer")
 public:
  MainWin(dsbcfg_t *cfg, QWidget *parent = 0);
  QMenu *menu();

 public slots:
  void showConfigMenu();
  void toggleWin();
  void quit();
  Q_SCRIPTABLE void incVol(uint amount);
  Q_SCRIPTABLE void decVol(uint amount);
  Q_SCRIPTABLE void setVol(uint lvol, uint rvol);
  Q_SCRIPTABLE void mute(bool on);
  Q_SCRIPTABLE void toggleMute();
  Q_SCRIPTABLE void toggleAppsWin();

 private slots:
  void catchDefaultMixerChanged(Mixer *mixer);
  void catchMixerRemoved();
  void catchMixerAdded();
  void catchCurrentMixerChanged(Mixer *mixer);
  void catchScrGeomChanged();
  void catchAppsMixerClosed();
  void catchVolIncChanged(int steps);
  void catchDefaultUnitCheckIvalChanged(int ms);
  void restartAudioApps();
  void showAppsMixer();
  void setTabIndex(int index);

 protected:
  void keyPressEvent(QKeyEvent *event);
  void closeEvent(QCloseEvent *event);
  void moveEvent(QMoveEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  void createMainMenu();
  void createTrayIcon();
  void setDefaultTab(int);
  void setCurrentMixer();
  void updateTrayIcon();
  void updateTrayMenu();
  void saveGeometry();
  void registerDBusService();
  QAction *createQuitAction();
  QAction *createPrefsAction();
  QAction *createAppsMixerAction();

 private:
  int trayCheckCounter{60};
  QMenu *mainMenu;
  QMenu *trayMenu;
  QTimer *traytimer;
  QMutex prefsWinMutex;
  dsbcfg_t *cfg;
  MixerSettings *mixerSettings{nullptr};
  SoundSettings *soundSettings{nullptr};
  IconLoader *iconLoader{nullptr};
  MixerList *mixerList{nullptr};
  MixerTabs *mixerTabs{nullptr};
  TrayIcon *trayIcon{nullptr};
  AppsMixer *appsMixer{nullptr};
  Mixer *currentMixer;
  Mixer *defaultMixer;
  const int trayCheckIvalMs{500};
  const int trayCheckTries{60};
};
