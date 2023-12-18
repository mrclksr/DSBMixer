/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "mixerlist.h"

#include <memory>
#include <stdexcept>

#include "libdsbmixer.h"

MixerList::MixerList(const MixerSettings &mixerSettings, QWidget *parent)
    : QWidget(parent), mixerSettings{&mixerSettings} {
  pollTimer = new QTimer(this);
  unitCheckTimer = new QTimer(this);
  createMixerList();
  connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollMixers()));
  connect(unitCheckTimer, SIGNAL(timeout()), this,
          SLOT(checkDefaultUnitChanged()));
  pollTimer->start(this->mixerSettings->getPollIval());
  unitCheckTimer->start(this->mixerSettings->getUnitChkIval());
  connect(this->mixerSettings, SIGNAL(pollIvalChanged(int)), this,
          SLOT(catchPollIvalChanged(int)));
  connect(this->mixerSettings, SIGNAL(unitChkIvalChanged(int)), this,
          SLOT(catchUnitCheckIvalChanged(int)));
#ifndef WITHOUT_DEVD
  devdWatcher = new Thread();
  connect(devdWatcher, SIGNAL(sendNewMixer(dsbmixer_t *)), this,
          SLOT(addNewMixer(dsbmixer_t *)));
  connect(devdWatcher, SIGNAL(sendRemoveMixer(dsbmixer_t *)), this,
          SLOT(removeMixer(dsbmixer_t *)));
  devdWatcher->start();
#endif
}

void MixerList::createMixerList() {
  for (int i = 0; i < dsbmixer_get_ndevs(); i++) {
    dsbmixer_t *dev = dsbmixer_get_mixer(i);
    auto mixer{new Mixer(*dev, *mixerSettings, this)};
    mixers.append(mixer);
  }
  initDefaultMixer();
}

void MixerList::initDefaultMixer() {
  int unit{dsbmixer_default_unit()};
  if (unit < 0) throw std::runtime_error{"Failed to detect default mixer unit"};
  defaultMixer = unitToMixer(unit);
  if (!defaultMixer) throw std::runtime_error{"Couldn't find default mixer"};
}

Mixer *MixerList::unitToMixer(int unit) {
  auto result =
      std::find_if(mixers.cbegin(), mixers.cend(),
                   [unit](const auto m) { return (m->getUnit() == unit); });
  return (*result);
}

void MixerList::addNewMixer(dsbmixer_t *dev) {
  auto mixer{new Mixer(*dev, *mixerSettings, this)};
  mixers.append(mixer);
  emit mixerAdded(mixer);
}

void MixerList::removeMixer(dsbmixer_t *dev) {
  for (auto &m : mixers) {
    if (m->getID() == dsbmixer_get_id(dev)) {
      emit mixerRemoved(m);
      mixers.removeOne(m);
      return;
    }
  }
}

void MixerList::checkDefaultUnitChanged() {
  int unit{dsbmixer_default_unit()};
  if (defaultMixer->getUnit() == unit) return;
  initDefaultMixer();
  emit defaultMixerChanged(defaultMixer);
}

void MixerList::pollMixers() {
  dsbmixer_t *mixer{dsbmixer_poll_mixers()};
  if (mixer == NULL) return;
  for (auto &m : mixers) {
    if (mixer == m->getDev()) {
      m->update();
      return;
    }
  }
}
void MixerList::suspendUnitChecker() {
  unitCheckTimer->stop();
  unitCheckSuspended = true;
}

void MixerList::resumeUnitChecker() {
  unitCheckTimer->start(mixerSettings->getUnitChkIval());
  unitCheckSuspended = false;
  checkDefaultUnitChanged();
}

void MixerList::catchPollIvalChanged(int ms) {
  if (ms <= 0)
    pollTimer->stop();
  else
    pollTimer->start(ms);
}

void MixerList::catchUnitCheckIvalChanged(int ms) {
  if (unitCheckSuspended) return;
  if (ms <= 0)
    unitCheckTimer->stop();
  else
    unitCheckTimer->start(ms);
}

Mixer *MixerList::getDefaultMixer() const { return (defaultMixer); }

Mixer *MixerList::at(qsizetype idx) const { return (mixers.at(idx)); }

qsizetype MixerList::count() const { return (mixers.count()); }
