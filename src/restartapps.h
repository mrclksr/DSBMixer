/*-
 * Copyright (c) 2020 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QGroupBox>
#include <QList>
#include <QRadioButton>
#include <QVBoxLayout>

#include "libdsbmixer.h"

enum ButtonId { MOVE_BUTTON_ID = 1, RESTART_BUTTON_ID, IGNORE_BUTTON_ID };

struct BGProcMap {
  QButtonGroup *group;
  dsbmixer_audio_proc_t *proc;
};

class RestartApps : public QDialog {
  Q_OBJECT
 public:
  RestartApps(dsbmixer_audio_proc_t apps[], size_t napps, QWidget *parent = 0);
 private slots:
  void acceptSlot();
  void rejectSlot();

 private:
  void cleanup(void);
  void addButtons(dsbmixer_audio_proc_t *proc, QWidget *parent = nullptr);
  void addRadioButton(QString text, ButtonId id, BGProcMap *map, QVBoxLayout *layout);
  QWidget *createAppList();
  QGroupBox *createAppGroupBox(dsbmixer_audio_proc_t *proc, QWidget *parent = nullptr);
 private:
  size_t napps{0};
  QList<BGProcMap *> bgmap;
  dsbmixer_audio_proc_t *apps;
};
