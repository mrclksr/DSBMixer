/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "mixertabs.h"

#include <QObject>
#include <QVBoxLayout>
#include <cstddef>

MixerTabs::MixerTabs(MixerList &mixerList, QWidget *parent)
    : QWidget(parent), mixerList{&mixerList} {
  tabs = new QTabWidget(this);
  for (auto i{0}; i < this->mixerList->count(); i++) {
    Mixer *mixer{this->mixerList->at(i)};
    QString label{mixer->getName()};
    if (this->mixerList->getDefaultMixer()->getID() == mixer->getID())
      label.append("*");
    tabs->addTab(mixer, label);
  }
  Mixer *currMixer{this->mixerList->getDefaultMixer()};
  if (currMixer) setCurrent(currMixer);
  QVBoxLayout *layout{new QVBoxLayout};
  layout->addWidget(tabs);
  setLayout(layout);

  connect(this->mixerList, SIGNAL(mixerRemoved(Mixer *)), this,
          SLOT(catchMixerRemoved(Mixer *)));
  connect(this->mixerList, SIGNAL(mixerAdded(Mixer *)), this,
          SLOT(catchMixerAdded(Mixer *)));
  connect(this->mixerList, SIGNAL(defaultMixerChanged(Mixer *)), this,
          SLOT(catchDefaultMixerChanged(Mixer *)));
  connect(tabs, SIGNAL(currentChanged(int)), this,
          SLOT(catchCurrentTabChanged(int)));
}

int MixerTabs::count() { return (tabs->count()); }

void MixerTabs::setCurrent(Mixer *mixer) { tabs->setCurrentWidget(mixer); }
void MixerTabs::setCurrentIndex(int index) { tabs->setCurrentIndex(index); }

Mixer *MixerTabs::getCurrent() const {
  return (qobject_cast<Mixer *>(tabs->currentWidget()));
}

Mixer *MixerTabs::mixer(int index) const {
  return (qobject_cast<Mixer *>(tabs->widget(index)));
}

void MixerTabs::catchMixerRemoved(Mixer *mixer) {
  for (auto i{0}; i < tabs->count(); i++) {
    Mixer *m{qobject_cast<Mixer *>(tabs->widget(i))};
    if (mixer->getID() == m->getID()) {
      tabs->removeTab(i);
      return;
    }
  }
}

void MixerTabs::catchMixerAdded(Mixer *mixer) {
  tabs->addTab(mixer, mixer->getName());
}

void MixerTabs::catchDefaultMixerChanged(Mixer *mixer) {
  tabs->setCurrentWidget(mixer);
  markDefaultMixerTab(mixer);
}

void MixerTabs::catchCurrentTabChanged(int index) {
  Mixer *mixer{qobject_cast<Mixer *>(tabs->widget(index))};
  emit currentMixerChanged(mixer);
}

void MixerTabs::markDefaultMixerTab(Mixer *dflt) {
  for (auto i{0}; i < tabs->count(); i++) {
    Mixer *mixer{qobject_cast<Mixer *>(tabs->widget(i))};
    QString l{tabs->tabText(i)};
    if (l.isEmpty()) continue;
    if (l.back() == '*') l.remove(l.size() - 1, 1);
    if (mixer->getID() == dflt->getID()) l.append("*");
    tabs->setTabText(i, l);
  }
}
