/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "mixertrayicon.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QString>
#include <QTimer>

TrayIcon::TrayIcon(const IconLoader &iconLoader,
                   const MixerSettings &mixerSettings, QMenu &trayMenu,
                   Mixer *mixer, QObject *parent)
    : KStatusNotifierItem(parent) {
  this->iconLoader = &iconLoader;
  this->mixerSettings = &mixerSettings;
  lrView = this->mixerSettings->lrViewEnabled();
  volInc = this->mixerSettings->getVolInc();
  direction = this->mixerSettings->inverseScrollEnabled() ? -1 : 1;
  setIconByPixmap(this->iconLoader->mixerIcon);
  setMixer(mixer);
  setContextMenu(&trayMenu);
  setStatus(KStatusNotifierItem::Active);

  connect(this->iconLoader, SIGNAL(trayIconThemeChanged()), this,
          SLOT(catchTrayIconThemeChanged()));
  connect(this->mixerSettings, SIGNAL(lrViewChanged(bool)), this,
          SLOT(catchLRViewChanged(bool)));
  connect(this->mixerSettings, SIGNAL(volIncChanged(int)), this,
          SLOT(catchVolIncChanged(int)));
  connect(this->mixerSettings, SIGNAL(inverseScrollChanged(bool)), this,
          SLOT(catchInverseScrollChanged(bool)));
  connect(this, SIGNAL(scrollRequested(int, Qt::Orientation)), this,
          SLOT(catchScrollRequest(int, Qt::Orientation)));
}

void TrayIcon::catchLRViewChanged(bool on) {
  lrView = on;
  update();
}

void TrayIcon::catchInverseScrollChanged(bool on) {
  direction = on ? -1 : 1;
}

void TrayIcon::catchVolIncChanged(int volInc) {
  if (volInc <= 0 || volInc >= 100) return;
  this->volInc = volInc;
}

void TrayIcon::catchTrayIconThemeChanged() { update(); }

int TrayIcon::getVolInc() const { return (volInc); }

void TrayIcon::setMixer(Mixer *mixer) {
  if (this->mixer == mixer) return;
  if (this->mixer) this->mixer->disconnect(this);
  this->mixer = mixer;
  if (mixer) {
    update();
    connect(this->mixer, SIGNAL(destroyed()), this,
            SLOT(catchMixerDestroyed()));
    connect(this->mixer, SIGNAL(masterVolChanged(int, int, int)), this,
            SLOT(update()));
    connect(this->mixer, SIGNAL(muteStateChanged(int, bool)), this,
            SLOT(update()));
  } else
    setIconByPixmap(iconLoader->noMixerIcon);
}

void TrayIcon::catchMixerDestroyed() {
  this->mixer = nullptr;
  setIconByPixmap(iconLoader->noMixerIcon);
}

void TrayIcon::update() {
  if (!mixer) return;
  const int lvol{mixer->getMasterLVol()};
  const int rvol{mixer->getMasterRVol()};
  updateIcon(lvol, rvol);
  updateToolTip(lvol, rvol);
}

void TrayIcon::updateIcon(int lvol, int rvol) {
  int vol{(lvol + rvol) >> 1};
  if (vol == 0 || mixer->isMuted(DSBMIXER_MASTER)) {
    setIconByPixmap(iconLoader->muteIcon);
    return;
  }
  switch (vol * 3 / 100) {
    case 0:
      setIconByPixmap(iconLoader->lVolIcon);
      break;
    case 1:
      setIconByPixmap(iconLoader->mVolIcon);
      break;
    case 2:
    case 3:
      setIconByPixmap(iconLoader->hVolIcon);
  }
}

void TrayIcon::updateToolTip(int lvol, int rvol) {
  QString tt;
  int vol{(lvol + rvol) >> 1};

  if (vol == 0 || mixer->isMuted(DSBMIXER_MASTER))
    tt = QString(tr("Muted"));
  else {
    if (lrView)
      tt = QString("Vol %1:%2").arg(lvol).arg(rvol);
    else
      tt = QString("Vol %1%").arg(vol);
  }
  setToolTip(iconLoader->mixerIcon, tt, "");
}

void TrayIcon::catchScrollRequest(int delta, Qt::Orientation orientation) {
  if (!mixer) return;
  int inc{};
  if (delta < 0)
    inc = -volInc;
  else
    inc = volInc;
  mixer->changeMasterVol(inc * direction);
}
