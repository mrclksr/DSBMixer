/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "chanslider.h"

#include <QGroupBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QString>

#include "qt-helper/qt-helper.h"

ChanSlider::ChanSlider(const QString &name, int id, int lvol, int rvol,
                       bool canRecord, bool muteable, bool lrview, bool tray,
                       QWidget *parent)
    : QGroupBox(name, parent) {
  this->tray = tray;
  this->lrview = lrview;
  this->muted = false;
  this->muteable = muteable;
  this->canRec = canRecord;
  this->id = id;
  this->lvol = lvol;
  this->rvol = rvol;
  this->vol = (lvol + rvol) >> 1;
  if (lrview)
    initLRView();
  else
    initUView();
}

void ChanSlider::initLRView() {
  volabell = new QLabel(this);
  volabelr = new QLabel(this);
  lslider = new QSlider(Qt::Vertical);
  rslider = new QSlider(Qt::Vertical);

  QVBoxLayout *vboxl{new QVBoxLayout};
  QVBoxLayout *vboxr{new QVBoxLayout};
  QVBoxLayout *layout{new QVBoxLayout};
  QHBoxLayout *hbox{new QHBoxLayout};

  volabell->setText(QString("100%"));
  volabelr->setText(QString("100%"));

  QSize sz{volabell->sizeHint()};

  volabell->setMinimumWidth(sz.width());
  volabelr->setMinimumWidth(sz.width());
  volabell->setMaximumWidth(sz.width());
  volabelr->setMaximumWidth(sz.width());

  volabell->setText(QString("%1%").arg(lvol));
  volabelr->setText(QString("%1%").arg(rvol));
  if (!tray) addRecCB(layout);

  lslider->setMinimum(0);
  lslider->setMaximum(100);
  lslider->setValue(lvol);
  lslider->setTickPosition(QSlider::TicksLeft);
  vboxl->addWidget(volabell);
  vboxl->addWidget(lslider);

  rslider->setMinimum(0);
  rslider->setMaximum(100);
  rslider->setTickPosition(QSlider::TicksRight);
  vboxr->addWidget(volabelr, 0, Qt::AlignHCenter);
  vboxr->addWidget(rslider, 0, Qt::AlignHCenter);
  setVol(lvol, rvol);
  hbox->addLayout(vboxl);
  hbox->addLayout(vboxr);

  layout->addLayout(hbox, 0);
  if (!tray) {
    lockCB = new QCheckBox("🔒");
    layout->addWidget(lockCB, 0, Qt::AlignCenter);
    addMuteCB(layout);
    connect(lockCB, SIGNAL(stateChanged(int)), this,
            SLOT(emitLockChanged(int)));
  }
  setLayout(layout);
  connect(lslider, SIGNAL(valueChanged(int)), this,
          SLOT(emitLVolumeChanged(int)));
  connect(rslider, SIGNAL(valueChanged(int)), this,
          SLOT(emitRVolumeChanged(int)));
}

void ChanSlider::initUView() {
  volabel = new QLabel(this);
  volabel->setText(QString("100%"));
  QSize sz{volabel->sizeHint()};
  volabel->setMinimumWidth(sz.width());
  volabel->setMaximumWidth(sz.width());

  QVBoxLayout *layout{new QVBoxLayout};
  if (!tray) addRecCB(layout);
  layout->addWidget(volabel, 0, Qt::AlignHCenter);

  slider = new QSlider(Qt::Vertical);
  slider->setMinimum(0);
  slider->setMaximum(100);
  slider->setTickPosition(QSlider::TicksLeft);
  setVol(lvol, rvol);
  layout->addWidget(slider, 0, Qt::AlignHCenter);
  if (!tray) addMuteCB(layout);
  setLayout(layout);
  connect(slider, SIGNAL(valueChanged(int)), this,
          SLOT(emitVolumeChanged(int)));
}

void ChanSlider::addRecCB(QBoxLayout *layout) {
  QHBoxLayout *micHbox{new QHBoxLayout};
  QLabel *micPic{new QLabel(this)};
  QWidget *recElem{new QWidget(this)};
  QIcon micIcon{qh::loadIcon("audio-input-microphone-high")};
  recCB = new QCheckBox(this);
  micPic->setPixmap(micIcon.pixmap(16));
  micHbox->addWidget(recCB);
  micHbox->addWidget(micPic);
  recElem->setLayout(micHbox);
  if (!canRec) {
    // Padding space
    QSizePolicy sp;
    sp.setRetainSizeWhenHidden(true);
    recElem->setSizePolicy(sp);
    recElem->hide();
  } else {
    recCB->setToolTip(tr("Set/unset recording source"));
    connect(recCB, SIGNAL(stateChanged(int)), this,
            SLOT(emitRecSourceChanged(int)));
  }
  layout->addWidget(recElem, 0, Qt::AlignHCenter);
}

void ChanSlider::addMuteCB(QBoxLayout *layout) {
  muteCB = new QCheckBox(this);
  QLabel *mutePic{new QLabel(this)};
  QWidget *muteElem{new QWidget(this)};
  QHBoxLayout *muteHbox{new QHBoxLayout};
  QIcon muteIcon{qh::loadIcon("audio-volume-muted")};
  if (muteIcon.isNull()) muteIcon = QIcon(":/icons/audio-volume-muted.png");

  mutePic->setPixmap(muteIcon.pixmap(16));
  muteHbox->addWidget(muteCB);
  muteHbox->addWidget(mutePic);
  muteElem->setLayout(muteHbox);
  if (!muteable) {
    // Padding space
    QSizePolicy sp;
    sp.setRetainSizeWhenHidden(true);
    muteElem->setSizePolicy(sp);
    muteElem->hide();
  } else {
    muteElem->setToolTip(tr("mute"));
    connect(muteCB, SIGNAL(stateChanged(int)), this,
            SLOT(emitMuteChanged(int)));
  }
  layout->addWidget(muteElem, 0, Qt::AlignHCenter);
}

void ChanSlider::emitVolumeChanged(int val) {
  if (muted) return;
  addVol(val - vol);
  emit volumeChanged(this->id, lvol, rvol);
}

void ChanSlider::emitLVolumeChanged(int val) {
  if (muted) return;
  if (lrlocked) {
    addVol(val - lvol);
    emit volumeChanged(this->id, lvol, rvol);
  } else {
    lvol = val;
    emit lVolumeChanged(this->id, lvol);
  }
}

void ChanSlider::emitRVolumeChanged(int val) {
  if (muted) return;
  if (lrlocked) {
    addVol(val - rvol);
    emit volumeChanged(id, lvol, rvol);
  } else {
    rvol = val;
    emit rVolumeChanged(id, rvol);
  }
}

void ChanSlider::emitRecSourceChanged(int state) {
  emit recSourceChanged(id, state);
}

void ChanSlider::emitMuteChanged(int state) {
  muted = (state == Qt::Checked);
  if (lrview) {
    lslider->setEnabled(!muted);
    rslider->setEnabled(!muted);
  } else
    slider->setEnabled(!muted);
  emit muteChanged(id, state);
}

void ChanSlider::emitLockChanged(int state) {
  lrlocked = (state == Qt::Checked);
  emit lockChanged(state);
}

bool ChanSlider::isMuted() {
  return (muted);
}

bool ChanSlider::isRecSrc() {
  return (recSrc);
}

void ChanSlider::setVol(int lvol, int rvol) {
  if (lvol > 100 || lvol < 0) return;
  if (rvol > 100 || rvol < 0) return;
  this->lvol = lvol;
  this->rvol = rvol;
  if (lrview) {
    const QSignalBlocker blocker(this);
    lslider->setValue(lvol);
    rslider->setValue(rvol);
    sliderSetToolTip(lvol, rvol);
    volabell->setText(QString("%1%").arg(lvol));
    volabelr->setText(QString("%1%").arg(rvol));
  } else {
    const QSignalBlocker blocker(this);
    vol = (lvol + rvol) >> 1;
    slider->setValue(vol);
    sliderSetToolTip(lvol, rvol);
    volabel->setText(QString("%1%").arg(vol));
  }
}

void ChanSlider::setRecSrc(bool state) {
  if (!recCB)
    return;
  const QSignalBlocker blocker(recCB);
  recCB->setCheckState(state ? Qt::Checked : Qt::Unchecked);
  recSrc = state;
}

void ChanSlider::setMute(bool state) {
  if (!muteCB)
    return;
  muted = state;
  muteCB->setCheckState(state ? Qt::Checked : Qt::Unchecked);
}

void ChanSlider::sliderSetToolTip(int lvol, int rvol) {
  if (!lrview) {
    QString str{QString("%1%").arg(vol)};
    slider->setToolTip(str);
    return;
  }
  QString lstr{QString("%1%").arg(lvol)};
  QString rstr{QString("%1%").arg(rvol)};

  lslider->setToolTip(lstr);
  rslider->setToolTip(rstr);
}

void ChanSlider::setTicks(bool on) {
  if (lrview) {
    lslider->setTickPosition(on ? QSlider::TicksLeft : QSlider::NoTicks);
    rslider->setTickPosition(on ? QSlider::TicksRight : QSlider::NoTicks);
    return;
  }
  slider->setTickPosition(on ? QSlider::TicksLeft : QSlider::NoTicks);
}

void ChanSlider::addVol(int volinc) {
  int minvol, maxvol;

  if (lrlocked) {
    if (volinc < 0) {
      minvol = std::min(rvol, lvol);
      if (minvol + volinc < 0) volinc = -minvol;
    } else {
      maxvol = std::max(rvol, lvol);
      if (maxvol + volinc > 100) volinc = 100 - maxvol;
    }
    rvol += volinc;
    lvol += volinc;
  } else {
    if (rvol + volinc < 0)
      rvol = 0;
    else if (rvol + volinc > 100)
      rvol = 100;
    else
      rvol += volinc;
    if (lvol + volinc < 0)
      lvol = 0;
    else if (lvol + volinc > 100)
      lvol = 100;
    else
      lvol += volinc;
  }
  vol = (lvol + rvol) >> 1;
}
