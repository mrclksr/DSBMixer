/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "restartapps.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "libdsbmixer.h"
#include "qt-helper/qt-helper.h"

RestartApps::RestartApps(dsbmixer_audio_proc_t *ap, size_t nap, QWidget *parent)
    : QDialog(parent) {
  this->ap = ap;
  this->nap = nap;
  QVBoxLayout *vbox{new QVBoxLayout};
  QHBoxLayout *bbox{new QHBoxLayout};
  QHBoxLayout *hbox{new QHBoxLayout};
  QLabel *pic{new QLabel};
  QLabel *header{new QLabel(tr("<b>Default audio device changed</b>"))};
  QLabel *text{new QLabel(tr("Restart audio applications:"))};
  QIcon winIcon{qh::loadIcon(QStringList("audio-speakers"))};
  QIcon restartIcon{qh::loadStockIcon(QStyle::SP_DialogOkButton)};
  QIcon cancelIcon{qh::loadStockIcon(QStyle::SP_DialogCancelButton)};
  QPushButton *cancel{new QPushButton(cancelIcon, tr("&Cancel"))};
  restartPb = new QPushButton(restartIcon, tr("&Restart"));
  if (winIcon.isNull())
    winIcon = QIcon(":/icons/preferences-desktop-multimedia.png");
  qApp->setQuitOnLastWindowClosed(false);
  setWindowIcon(winIcon);
  setWindowTitle(tr("DSBMixer - Restart audio applications"));
  if (parent) {
    int width{parent->width() * 80 / 100};
    if (width > 0) resize(width, height());
  }
  vbox->setContentsMargins(15, 15, 15, 15);
  setLayout(vbox);
  pic->setPixmap(winIcon.pixmap(48));
  hbox->addWidget(pic, 0, Qt::AlignLeft);
  hbox->addWidget(header, 1, Qt::AlignCenter);
  hbox->addStretch(1);
  vbox->addLayout(hbox);
  vbox->addWidget(text);
  vbox->addLayout(createAppCbs());
  bbox->addWidget(restartPb, 1, Qt::AlignRight);
  bbox->addWidget(cancel, 0, Qt::AlignRight);
  vbox->addStretch(1);
  vbox->addLayout(bbox);
  connect(restartPb, SIGNAL(clicked()), this, SLOT(acceptSlot()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(rejectSlot()));
}

void RestartApps::acceptSlot() {
  for (int i{0}; i < cbmap.size(); i++) {
    if (cbmap.at(i)->cb->isChecked())
      (void)dsbmixer_restart_audio_proc(cbmap.at(i)->proc);
  }
  cleanup();
  this->accept();
}

void RestartApps::rejectSlot() {
  cleanup();
  this->reject();
}

void RestartApps::cleanup() {
  for (int i{0}; i < cbmap.size(); i++) delete (cbmap.at(i));
  free(ap);
}

void RestartApps::checkCanRestart() {
  bool canRestart{false};

  for (int i{0}; i < cbmap.size() && !canRestart; i++) {
    if (cbmap.at(i)->cb->isChecked()) canRestart = true;
  }
  restartPb->setEnabled(canRestart);
}

QVBoxLayout *RestartApps::createAppCbs() {
  QVBoxLayout *vbox{new QVBoxLayout};

  for (size_t i{0}; i < this->nap; i++) {
    QCheckBox *cb{new QCheckBox(
      QString("%1 (PID %2)").arg(dsbmixer_get_audio_proc_cmd(&ap[i])).arg(dsbmixer_get_audio_proc_pid(&ap[i])), this)};
    CbProcMap *mp{new CbProcMap};
    mp->cb = cb;
    mp->proc = &ap[i];
    this->cbmap.append(mp);
    cb->setTristate(false);
    cb->setCheckState(Qt::Checked);
    connect(cb, SIGNAL(clicked()), this, SLOT(checkCanRestart()));
    vbox->addWidget(cb);
  }
  return (vbox);
}
