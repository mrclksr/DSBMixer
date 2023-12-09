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

#include "qt-helper/qt-helper.h"

RestartApps::RestartApps(dsbmixer_audio_proc_t *apps, size_t napps,
                         QWidget *parent)
    : QDialog(parent) {
  this->apps = apps;
  this->napps = napps;
  QVBoxLayout *vbox{new QVBoxLayout};
  QHBoxLayout *bbox{new QHBoxLayout};
  QHBoxLayout *hbox{new QHBoxLayout};
  QLabel *pic{new QLabel};
  QLabel *header{new QLabel(tr("<b>Default audio device changed</b>"))};
  QLabel *text{
      new QLabel(tr("What do you want to do with the audio applications?"))};
  QIcon winIcon{qh::loadIcon(QStringList("audio-speakers"))};
  QIcon restartIcon{qh::loadStockIcon(QStyle::SP_DialogOkButton)};
  QIcon cancelIcon{qh::loadStockIcon(QStyle::SP_DialogCancelButton)};
  QPushButton *cancel{new QPushButton(cancelIcon, tr("&Cancel"))};
  QPushButton *ok = new QPushButton(restartIcon, tr("&Ok"));
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
  vbox->addWidget(createAppList());
  bbox->addWidget(ok, 1, Qt::AlignRight);
  bbox->addWidget(cancel, 0, Qt::AlignRight);
  vbox->addStretch(1);
  vbox->addLayout(bbox);
  connect(ok, SIGNAL(clicked()), this, SLOT(acceptSlot()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(rejectSlot()));
}

void RestartApps::acceptSlot() {
  for (auto bg : bgmap) {
    switch (bg->group->checkedId()) {
      case MOVE_BUTTON_ID:
        (void)dsbmixer_audio_proc_move(bg->proc);
        break;
      case RESTART_BUTTON_ID:
        (void)dsbmixer_audio_proc_restart(bg->proc);
    }
  }
  cleanup();
  this->accept();
}

void RestartApps::rejectSlot() {
  cleanup();
  this->reject();
}

void RestartApps::cleanup() { free(apps); }

QWidget *RestartApps::createAppList() {
  QWidget *container{new QWidget(this)};
  QVBoxLayout *vbox{new QVBoxLayout(container)};
  for (size_t i{0}; i < this->napps; i++)
    vbox->addWidget(createAppGroupBox(&apps[i], container));
  return (container);
}

QGroupBox *RestartApps::createAppGroupBox(dsbmixer_audio_proc_t *proc,
                                          QWidget *parent) {
  QString title{QString("%1 (PID %2)")
                    .arg(dsbmixer_get_audio_proc_cmd(proc))
                    .arg(dsbmixer_get_audio_proc_pid(proc))};
  QGroupBox *gb{new QGroupBox(title, parent)};
  addButtons(proc, gb);
  return (gb);
}

void RestartApps::addButtons(dsbmixer_audio_proc_t *proc, QWidget *parent) {
  BGProcMap *mp{new BGProcMap};
  QVBoxLayout *vbox{new QVBoxLayout};

  mp->group = new QButtonGroup(parent);
  mp->proc = proc;
  if (dsbmixer_audio_proc_can_move(proc)) {
    addRadioButton(tr("Move audio streams to new default audio device"),
                   MOVE_BUTTON_ID, mp, vbox);
  }
  if (dsbmixer_audio_proc_can_restart(proc))
    addRadioButton(tr("Restart"), RESTART_BUTTON_ID, mp, vbox);
  addRadioButton(tr("Do nothing"), IGNORE_BUTTON_ID, mp, vbox);
  this->bgmap.append(mp);
  parent->setLayout(vbox);
}

void RestartApps::addRadioButton(QString text, ButtonId id, BGProcMap *map,
                                 QVBoxLayout *layout) {
  QRadioButton *rb{new QRadioButton(text)};
  map->group->addButton(rb, id);
  layout->addWidget(rb);
  if (map->group->checkedId() == -1) rb->setChecked(true);
}
