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
    : QSystemTrayIcon(iconLoader.mixerIcon, parent) {
  this->iconLoader = &iconLoader;
  this->mixerSettings = &mixerSettings;
  lrView = this->mixerSettings->lrViewEnabled();
  volInc = this->mixerSettings->getVolInc();

  sliderTimer = new QTimer();
  setMixer(mixer);
  setContextMenu(&trayMenu);
  registerDBusService();
  connect(this->iconLoader, SIGNAL(trayIconThemeChanged()), this,
          SLOT(catchTrayIconThemeChanged()));
  connect(this->mixerSettings, SIGNAL(lrViewChanged(bool)), this,
          SLOT(catchLRViewChanged(bool)));
  connect(this->mixerSettings, SIGNAL(scaleTicksChanged(bool)), this,
          SLOT(catchScaleTicksChanged(bool)));
  connect(this->mixerSettings, SIGNAL(volIncChanged(int)), this,
          SLOT(catchVolIncChanged(int)));
  show();
}

void TrayIcon::catchLRViewChanged(bool on) {
  lrView = on;
  if (slider) deleteSlider();
  update();
}

void TrayIcon::catchScaleTicksChanged(bool on) {
  if (slider) slider->setTicks(on);
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
  deleteSlider();
  if (mixer) {
    update();
    connect(this->mixer, SIGNAL(destroyed()), this,
            SLOT(catchMixerDestroyed()));
    connect(this->mixer, SIGNAL(masterVolChanged(int, int, int)), this,
            SLOT(update()));
    connect(this->mixer, SIGNAL(muteStateChanged(int, bool)), this,
            SLOT(update()));
  } else
    setIcon(iconLoader->noMixerIcon);
}

void TrayIcon::catchMixerDestroyed() {
  this->mixer = nullptr;
  setIcon(iconLoader->noMixerIcon);
}

void TrayIcon::showSlider(int lvol, int rvol) {
  int sx{0}, sy{0};
  QRect rect(geometry());
  QPoint tray_center(geometry().center());
  QRect screen_rect(qApp->screenAt(tray_center)->geometry());

  if (!slider) initSlider(lvol, rvol);
  QSize scrsize(slider->screen()->availableSize());
  slider->setVol(lvol, rvol);
  sliderTimer->start(1000);
  if (!slider->isVisible()) {
    sx = rect.x() - slider->width() / 2 + rect.width() / 2;
    if (2 * (rect.y() - screen_rect.y()) < scrsize.height())
      sy = rect.y() + rect.height();
    else
      sy = rect.y() - slider->size().height();
    slider->move(sx, sy);
    slider->show();
  }
}

void TrayIcon::initSlider(int lvol, int rvol) {
  constexpr bool canRec{false}, chanMutable{false}, isTraySlider{true};
  slider =
      new ChanSlider("Vol", DSBMIXER_MASTER, lvol, rvol, canRec, chanMutable,
                     mixerSettings->lrViewEnabled(), isTraySlider);
  slider->setTicks(mixerSettings->scaleTicksEnabled());
  slider->setWindowFlags(Qt::ToolTip);
  connect(sliderTimer, SIGNAL(timeout()), this, SLOT(hideSlider()));
  slider->show();
  slider->hide();
}

void TrayIcon::hideSlider() {
  if (!slider) return;
  slider->hide();
  sliderTimer->stop();
}

void TrayIcon::deleteSlider() {
  if (!slider) return;
  delete slider;
  slider = nullptr;
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
    setIcon(iconLoader->muteIcon);
    return;
  }
  switch (vol * 3 / 100) {
    case 0:
      setIcon(iconLoader->lVolIcon);
      break;
    case 1:
      setIcon(iconLoader->mVolIcon);
      break;
    case 2:
    case 3:
      setIcon(iconLoader->hVolIcon);
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
  setToolTip(tt);
}

bool TrayIcon::event(QEvent *ev) {
  int inc{};

  if (ev->type() != QEvent::Wheel) return (QSystemTrayIcon::event(ev));
  if (!mixer) return (true);
  QWheelEvent *we = static_cast<QWheelEvent *>(ev);
  if (we->angleDelta().y() < 0)
    inc = -volInc;
  else
    inc = volInc;
  mixer->changeMasterVol(inc);
  updateSlider();

  return (true);
}

void TrayIcon::registerDBusService() {
  if (!QDBusConnection::sessionBus().isConnected()) return;
  if (!QDBusConnection::sessionBus().registerService("org.dsb.dsbmixer"))
    qDebug() << "registerService() failed";
  if (!QDBusConnection::sessionBus().registerObject(
          "/Vol", this, QDBusConnection::ExportScriptableSlots))
    qDebug() << "registerObject() failed";
}

void TrayIcon::incVol(uint amount) {
  if (!mixer) return;
  mixer->changeMasterVol(amount);
  updateSlider();
}

void TrayIcon::decVol(uint amount) {
  if (!mixer) return;
  mixer->changeMasterVol(-amount);
  updateSlider();
}

void TrayIcon::updateSlider() {
  const int lvol{mixer->getMasterLVol()};
  const int rvol{mixer->getMasterRVol()};
  showSlider(lvol, rvol);
}
