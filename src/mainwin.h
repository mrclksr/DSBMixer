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

#include "appsmixer.h"
#include "config.h"
#include "mixerlist.h"
#include "mixersettings.h"
#include "mixertabs.h"
#include "mixertrayicon.h"

class MainWin : public QMainWindow {
  Q_OBJECT
 public:
  MainWin(dsbcfg_t *cfg, QWidget *parent = 0);
  QMenu *menu();

 public slots:
  void showConfigMenu();
  void toggleWin();
  void quit();

 private slots:
  void catchDefaultMixerChanged(Mixer *mixer);
  void catchMixerRemoved();
  void catchMixerAdded();
  void catchCurrentMixerChanged(Mixer *mixer);
  void catchScrGeomChanged();
  void catchAppsMixerClosed();
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
  void updateMixerSettings();
  void updateTrayIcon();
  void updateTrayMenu();
  void initMixerSettings();
  void storeMixerSettings();
  void saveGeometry();
  QAction *createQuitAction();
  QAction *createPrefsAction();
  QAction *createAppsMixerAction();

 private:
  int trayCheckCounter{60};
  QMenu *mainMenu;
  QMenu *trayMenu;
  QTimer *traytimer;
  dsbcfg_t *cfg;
  MixerSettings *mixerSettings{nullptr};
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
