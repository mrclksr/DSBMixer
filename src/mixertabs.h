/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QTabWidget>
#include <list>

#include "mixerlist.h"

class MixerTabs : public QWidget {
  Q_OBJECT
 public:
  MixerTabs(MixerList &mixerList, QWidget *parent = nullptr);
  int count();
  void setCurrent(Mixer *mixer);
  void setCurrentIndex(int index);
  Mixer *getCurrent() const;
  Mixer *mixer(int index) const;

 signals:
  void currentMixerChanged(Mixer *mixer);

 private slots:
  void catchMixerRemoved(Mixer *mixer);
  void catchMixerAdded(Mixer *mixer);
  void catchDefaultMixerChanged(Mixer *mixer);
  void catchCurrentTabChanged(int index);
  void markDefaultMixerTab(Mixer *dflt);

 private:
  MixerList *mixerList;
  QTabWidget *tabs{nullptr};
};
