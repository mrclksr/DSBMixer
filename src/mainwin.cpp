/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#else
#include <QStringConverter>
#endif
#include <QTabWidget>
#include <QTimer>

#include "appsmixer.h"
#include "iconloader.h"
#include "libdsbmixer.h"
#include "mainwin.h"
#include "mixerlist.h"
#include "mixertabs.h"
#include "preferences.h"
#include "qt-helper/qt-helper.h"
#include "restartapps.h"
#include "settings.h"

MainWin::MainWin(dsbcfg_t *cfg, QWidget *parent) : QMainWindow(parent) {
  this->cfg = cfg;
  initMixerSettings();
  QString trayThemeStr(dsbcfg_getval(cfg, CFG_TRAY_THEME).string);
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
  createTrayIcon();
  createMainMenu();
  setWindowIcon(iconLoader->mixerIcon);
  setWindowTitle("DSBMixer");
  registerDBusService();
  resize(dsbcfg_getval(cfg, CFG_WIDTH).integer,
         dsbcfg_getval(cfg, CFG_HEIGHT).integer);
  move(dsbcfg_getval(cfg, CFG_POS_X).integer,
       dsbcfg_getval(cfg, CFG_POS_Y).integer);
  statusBar()->setSizeGripEnabled(true);
  setCentralWidget(mixerTabs);
  qApp->setQuitOnLastWindowClosed(false);
}

void MainWin::initMixerSettings() {
  mixerSettings =
      new MixerSettings(dsbcfg_getval(cfg, CFG_LRVIEW).boolean,
                        dsbcfg_getval(cfg, CFG_TICKS).boolean,
                        dsbcfg_getval(cfg, CFG_MASK).integer,
                        dsbcfg_getval(cfg, CFG_POLL_IVAL).integer,
                        dsbcfg_getval(cfg, CFG_UNIT_CHK_IVAL).integer,
                        dsbcfg_getval(cfg, CFG_VOL_INC).integer, this);
  qApp->setWheelScrollLines(mixerSettings->getVolInc());
}

void MainWin::storeMixerSettings() {
  dsbcfg_set_bool(cfg, CFG_LRVIEW, mixerSettings->lrViewEnabled());
  dsbcfg_set_bool(cfg, CFG_TICKS, mixerSettings->scaleTicksEnabled());
  dsbcfg_set_int(cfg, CFG_MASK, mixerSettings->getChanMask());
  dsbcfg_set_int(cfg, CFG_POLL_IVAL, mixerSettings->getPollIval());
  dsbcfg_set_int(cfg, CFG_UNIT_CHK_IVAL, mixerSettings->getUnitChkIval());
  dsbcfg_set_int(cfg, CFG_VOL_INC, mixerSettings->getVolInc());
}

void MainWin::catchScrGeomChanged() { traytimer->start(trayCheckIvalMs); }
void MainWin::setTabIndex(int index) { mixerTabs->setCurrentIndex(index); }

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
  size_t nap{0};
  dsbmixer_audio_proc_t *ap{dsbmixer_get_audio_procs(&nap)};
  if (ap == NULL) return;
  RestartApps raWin(ap, nap, this);
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
  Settings settings;

  settings.chanMask = mixerSettings->getChanMask();
  settings.lrView = mixerSettings->lrViewEnabled();
  settings.showTicks = mixerSettings->scaleTicksEnabled();
  settings.pollIval = mixerSettings->getPollIval();
  settings.unitChkIval = mixerSettings->getUnitChkIval();
  settings.volInc = mixerSettings->getVolInc();

  settings.amplify = dsbmixer_amplification();
  settings.feederRateQuality = dsbmixer_feeder_rate_quality(),
  settings.defaultUnit = dsbmixer_default_unit();
  settings.maxAutoVchans = dsbmixer_maxautovchans();
  settings.latency = dsbmixer_latency();
  settings.bypassMixer = dsbmixer_bypass_mixer();

  if (dsbcfg_getval(cfg, CFG_PLAY_CMD).string != NULL)
    settings.playCmd = QString(dsbcfg_getval(cfg, CFG_PLAY_CMD).string);
  else
    settings.playCmd = QString();
  if (dsbcfg_getval(cfg, CFG_TRAY_THEME).string != NULL)
    settings.themeName = QString(dsbcfg_getval(cfg, CFG_TRAY_THEME).string);
  else
    settings.themeName = QString();
  Preferences prefs(settings, this);

  if (prefs.exec() != QDialog::Accepted) return;
  if (settings.defaultUnit != prefs.settings.defaultUnit)
    mixerList->setDefaultMixer(prefs.settings.defaultUnit);
  mixerSettings->setLRView(prefs.settings.lrView);
  mixerSettings->setScaleTicks(prefs.settings.showTicks);
  mixerSettings->setChanMask(prefs.settings.chanMask);
  mixerSettings->setPollIval(prefs.settings.pollIval);
  mixerSettings->setUnitChkIval(prefs.settings.unitChkIval);
  mixerSettings->setVolInc(prefs.settings.volInc);
  qApp->setWheelScrollLines(prefs.settings.volInc);
  storeMixerSettings();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  auto *codec{QTextCodec::codecForLocale()};
  QByteArray playCmdStr{codec->fromUnicode(prefs.settings.playCmd)};
#else
  auto encoder{QStringEncoder(QStringEncoder::Utf8)};
  QByteArray playCmdStr{encoder(prefs.settings.playCmd)};
#endif
  if (settings.playCmd != prefs.settings.playCmd) {
    if (prefs.settings.playCmd.isNull())
      dsbcfg_set_string(cfg, CFG_PLAY_CMD, "");
    else
      dsbcfg_set_string(cfg, CFG_PLAY_CMD, playCmdStr.data());
  }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QByteArray trayThemeStr{codec->fromUnicode(prefs.settings.themeName)};
#else
  QByteArray trayThemeStr{encoder(prefs.settings.themeName)};
#endif
  if (prefs.settings.themeName != settings.themeName) {
    if (prefs.settings.themeName.isNull())
      dsbcfg_set_string(cfg, CFG_TRAY_THEME, "");
    else {
      dsbcfg_set_string(cfg, CFG_TRAY_THEME, trayThemeStr.data());
      iconLoader->setTheme(prefs.settings.themeName);
    }
  }
  dsbcfg_write(PROGRAM, "config", cfg);
  if (prefs.settings.amplify != settings.amplify ||
      prefs.settings.defaultUnit != settings.defaultUnit ||
      prefs.settings.feederRateQuality != settings.feederRateQuality ||
      prefs.settings.latency != settings.latency ||
      prefs.settings.maxAutoVchans != settings.maxAutoVchans ||
      prefs.settings.bypassMixer != settings.bypassMixer) {
    if (dsbmixer_change_settings(
            prefs.settings.defaultUnit, prefs.settings.amplify,
            prefs.settings.feederRateQuality, prefs.settings.latency,
            prefs.settings.maxAutoVchans, prefs.settings.bypassMixer) == -1)
      qh::warnx(this, dsbmixer_error());
  }
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
  dsbcfg_set_int(cfg, CFG_POS_X, this->x());
  dsbcfg_set_int(cfg, CFG_POS_Y, this->y());
  dsbcfg_set_int(cfg, CFG_WIDTH, this->width());
  dsbcfg_set_int(cfg, CFG_HEIGHT, this->height());
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

void MainWin::updateTrayMenu() {
  if (!currentMixer || !defaultMixer) return;
  QAction *toggle{
      new QAction(iconLoader->windowIcon, tr("Show/hide window"), this)};
  QSignalMapper *mapper{new QSignalMapper(this)};

  trayMenu->clear();
  trayMenu->addAction(toggle);
  trayMenu->addAction(createAppsMixerAction());
  trayMenu->addSection(tr("Sound devices"));

  for (auto i{0}; i < mixerTabs->count(); i++) {
    Mixer *mixer{mixerTabs->mixer(i)};
    QString label(mixer->getName());
    if (mixer->getID() == defaultMixer->getID()) label.append("*");
    QAction *action{new QAction(iconLoader->mixerIcon, label)};
    if (mixer->getID() == currentMixer->getID()) action->setEnabled(false);
    mapper->setMapping(action, i);
    trayMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
  }
  trayMenu->addSeparator();
  trayMenu->addAction(createPrefsAction());
  trayMenu->addAction(createQuitAction());
  connect(mapper, SIGNAL(mappedInt(int)), this, SLOT(setTabIndex(int)));
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
        "/Vol", this, QDBusConnection::ExportScriptableSlots))
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
  currentMixer->setMute(DSBMIXER_MASTER, on);
}

void MainWin::toggleMute() {
  if (!currentMixer) return;
  currentMixer->toggleMute(DSBMIXER_MASTER);
}
