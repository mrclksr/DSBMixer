/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QList>
#include <QTimer>
#include <QWidget>
#include <cstddef>

#include "mixer.h"
#include "mixersettings.h"
#include "thread.h"

class MixerList : public QWidget {
  Q_OBJECT
 public:
  MixerList(const MixerSettings &mixerSettings, QWidget *parent = nullptr);
  int setDefaultMixer(const Mixer *mixer);
  int setDefaultMixer(int unit);
  Mixer *at(qsizetype idx) const;
  Mixer *getDefaultMixer() const;
  qsizetype count() const;
  void suspendUnitChecker();
  void resumeUnitChecker();

 private:
  void initDefaultMixer();
  void createMixerList();
  Mixer *unitToMixer(int unit);

 signals:
  void defaultMixerChanged(Mixer *);
  void mixerAdded(Mixer *);
  void mixerRemoved(Mixer *);

 private slots:
  void checkDefaultUnitChanged();
  void addNewMixer(dsbmixer_t *dev);
  void removeMixer(dsbmixer_t *dev);
  void pollMixers();
  void catchPollIvalChanged(int ms);
  void catchUnitCheckIvalChanged(int ms);

 private:
  bool unitCheckSuspended{false};
  Thread *devdWatcher;
  QTimer *pollTimer{nullptr};
  QTimer *unitCheckTimer{nullptr};
  Mixer *defaultMixer{nullptr};
  QList<Mixer *> mixers;
  const MixerSettings *mixerSettings{nullptr};
};
