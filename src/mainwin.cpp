/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "mainwin.h"

#include <stdlib.h>
#include <unistd.h>

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>
#include <QRect>
#include <QScreen>
#include <QSignalMapper>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <cstdlib>

#include "appsmixer.h"
#include "iconloader.h"
#include "libdsbmixer.h"
#include "mixerlist.h"
#include "mixertabs.h"
#include "preferences.h"
#include "qt-helper/qt-helper.h"
#include "restartapps.h"

MainWin::MainWin(dsbcfg_t *cfg, QWidget *parent)
    : QMainWindow(parent), cfg{cfg} {
  mixerSettings = new MixerSettings(*cfg, this);
  QString trayThemeStr(mixerSettings->getTrayThemeName());
  iconLoader = new IconLoader(trayThemeStr);
  mixerList = new MixerList(*mixerSettings, this);
  mixerTabs = new MixerTabs(*mixerList, this);
  defaultMixer = mixerList->getDefaultMixer();
  currentMixer = defaultMixer;
  connect(mixerList, SIGNAL(mixerAdded(Mixer *)), this,
          SLOT(catchMixerAdded()));
  connect(mixerList, SIGNAL(mixerRemoved(Mixer *)), this,
          SLOT(catchMixerRemoved()));
  connect(mixerList, SIGNAL(defaultMixerChanged(Mixer *)), this,
          SLOT(catchDefaultMixerChanged(Mixer *)));
  connect(mixerTabs, SIGNAL(currentMixerChanged(Mixer *)), this,
          SLOT(catchCurrentMixerChanged(Mixer *)));
  connect(QGuiApplication::primaryScreen(),
          SIGNAL(geometryChanged(const QRect &)), this,
          SLOT(catchScrGeomChanged()));
  connect(mixerSettings, SIGNAL(volIncChanged(int)), this,
          SLOT(catchVolIncChanged(int)));
  createTrayIcon();
  createMainMenu();
  setWindowIcon(iconLoader->mixerIcon);
  setWindowTitle("DSBMixer");
  registerDBusService();
  resize(mixerSettings->getWinWidth(), mixerSettings->getWinHeight());
  move(mixerSettings->getWinPosX(), mixerSettings->getWinPosY());
  statusBar()->setSizeGripEnabled(true);
  setCentralWidget(mixerTabs);
  qApp->setQuitOnLastWindowClosed(false);
  qApp->setWheelScrollLines(mixerSettings->getVolInc());
}

void MainWin::catchScrGeomChanged() { traytimer->start(trayCheckIvalMs); }

void MainWin::setTabIndex(int index) { mixerTabs->setCurrentIndex(index); }

void MainWin::catchVolIncChanged(int steps) {
  qApp->setWheelScrollLines(steps);
}

void MainWin::catchDefaultMixerChanged(Mixer *mixer) {
  defaultMixer = mixer;
  updateTrayMenu();
  restartAudioApps();
}

void MainWin::catchCurrentMixerChanged(Mixer *mixer) {
  currentMixer = mixer;
  if (!trayIcon) return;
  updateTrayMenu();
  trayIcon->setMixer(currentMixer);
}

void MainWin::catchMixerRemoved() { updateTrayMenu(); }

void MainWin::catchMixerAdded() { updateTrayMenu(); }

void MainWin::showAppsMixer() {
  if (appsMixer) {
    appsMixer->show();
    return;
  }
  dsbappsmixer_t *mixer{dsbappsmixer_create_mixer()};
  if (!mixer) {
    qh::warnx(
        this,
        QString("Couldn't open applications mixer: %1").arg(dsbmixer_error()));
    return;
  }
  appsMixer = new AppsMixer(*mixer, *iconLoader, *mixerSettings, this);
  connect(appsMixer, SIGNAL(destroyed()), this, SLOT(catchAppsMixerClosed()));
  appsMixer->show();
}

void MainWin::restartAudioApps() {
  size_t napps{0};
  dsbmixer_audio_proc_t *apps{dsbmixer_get_audio_procs(&napps)};
  if (apps == NULL) return;
  RestartApps raWin(apps, napps, this);
  (void)raWin.exec();
}

void MainWin::catchAppsMixerClosed() { appsMixer = nullptr; }

void MainWin::closeEvent(QCloseEvent *event) {
  setVisible(false);
  event->ignore();
}

void MainWin::moveEvent(QMoveEvent *event) {
  saveGeometry();
  event->accept();
}

void MainWin::resizeEvent(QResizeEvent *event) {
  saveGeometry();
  event->accept();
}

void MainWin::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
    case Qt::Key_Escape:
      hide();
      break;
    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
      int tabno{e->text().toInt()};
      if (e->modifiers() == Qt::AltModifier) {
        if (tabno == 0) tabno = mixerTabs->count();
        mixerTabs->setCurrentIndex(tabno - 1);
      }
  }
}

void MainWin::showConfigMenu() {
  if (!prefsWinMutex.try_lock()) return;
  mixerList->suspendUnitChecker();
  Preferences prefs{*mixerSettings, this};
  (void)prefs.exec();
  mixerList->resumeUnitChecker();
  prefsWinMutex.unlock();
}

void MainWin::toggleWin() {
  if (isVisible())
    hide();
  else
    show();
}

void MainWin::quit() {
  saveGeometry();
  QApplication::quit();
}

void MainWin::saveGeometry() {
  if (!isVisible()) return;
  mixerSettings->setWindowGeometry(this->x(), this->y(), this->width(),
                                   this->height());
  mixerSettings->storeSettings();
}

QAction *MainWin::createQuitAction() {
  QAction *action{new QAction(iconLoader->quitIcon, tr("&Quit"), this)};
  connect(action, SIGNAL(triggered()), this, SLOT(quit()));
  return (action);
}

QAction *MainWin::createPrefsAction() {
  QAction *action{new QAction(iconLoader->prefsIcon, tr("&Preferences"), this)};
  connect(action, SIGNAL(triggered()), this, SLOT(showConfigMenu()));
  return (action);
}

QAction *MainWin::createAppsMixerAction() {
  QAction *action{
      new QAction(iconLoader->appsMixerIcon, tr("&Open applications mixer"))};
  connect(action, SIGNAL(triggered()), this, SLOT(showAppsMixer()));
  return (action);
}

void MainWin::createMainMenu() {
  mainMenu = menuBar()->addMenu(tr("&File"));
  mainMenu->addAction(createAppsMixerAction());
  mainMenu->addAction(createPrefsAction());
  mainMenu->addAction(createQuitAction());
}

void MainWin::addMixerChooser(QMenu *menu) {
  QSignalMapper *mapper{new QSignalMapper(this)};

  for (auto i{0}; i < mixerTabs->count(); i++) {
    Mixer *mixer{mixerTabs->mixer(i)};
    QString label(mixer->getName());
    if (mixer->getID() == defaultMixer->getID()) label.append("*");
    QAction *action{new QAction(iconLoader->mixerIcon, label)};
    if (mixer->getID() == currentMixer->getID()) action->setEnabled(false);
    mapper->setMapping(action, i);
    menu->addAction(action);
    connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
  }
  connect(mapper, SIGNAL(mappedInt(int)), this, SLOT(setTabIndex(int)));
}

void MainWin::updateTrayMenu() {
  if (!currentMixer || !defaultMixer) return;
  QAction *toggle{
      new QAction(iconLoader->windowIcon, tr("Show/hide window"), this)};
  trayMenu->clear();
  trayMenu->addAction(toggle);
  trayMenu->addAction(createAppsMixerAction());
  trayMenu->addSection(tr("Sound devices"));
  addMixerChooser(trayMenu);
  trayMenu->addSeparator();
  trayMenu->addAction(createPrefsAction());
  trayMenu->addAction(createQuitAction());
  connect(toggle, SIGNAL(triggered()), this, SLOT(toggleWin()));
}

void MainWin::createTrayIcon() {
  trayMenu = new QMenu(this);
  updateTrayMenu();
  trayIcon =
      new TrayIcon(*iconLoader, *mixerSettings, *trayMenu, currentMixer, this);
}

void MainWin::registerDBusService() {
  if (!QDBusConnection::sessionBus().isConnected()) return;
  if (!QDBusConnection::sessionBus().registerService("org.dsb.dsbmixer"))
    qDebug() << "registerService() failed";
  if (!QDBusConnection::sessionBus().registerObject(
          "/", this, QDBusConnection::ExportScriptableSlots))
    qDebug() << "registerObject() failed";
}

void MainWin::incVol(uint amount) {
  if (!currentMixer) return;
  currentMixer->changeMasterVol(amount);
}

void MainWin::decVol(uint amount) {
  if (!currentMixer) return;
  currentMixer->changeMasterVol(-amount);
}

void MainWin::setVol(uint lvol, uint rvol) {
  if (!currentMixer) return;
  currentMixer->setVol(DSBMIXER_MASTER, lvol, rvol);
}

void MainWin::mute(bool on) {
  if (!currentMixer) return;
  currentMixer->muteMaster(on);
}

void MainWin::toggleMute() {
  if (!currentMixer) return;
  currentMixer->toggleMasterMute();
}

void MainWin::toggleAppsWin() {
  if (appsMixer) {
    delete appsMixer;
    appsMixer = nullptr;
  } else
    showAppsMixer();
}
