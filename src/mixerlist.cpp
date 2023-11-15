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
    : QWidget(parent) {
  this->mixerSettings = &mixerSettings;
  unitCheckTimer = new QTimer(this);
  pollTimer = new QTimer(this);
  createMixerList();

  connect(unitCheckTimer, SIGNAL(timeout()), this,
          SLOT(checkDefaultUnitChanged()));
  connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollMixers()));
#ifndef WITHOUT_DEVD
  devdWatcher = new Thread();
  connect(devdWatcher, SIGNAL(sendNewMixer(dsbmixer_t *)), this,
          SLOT(addNewMixer(dsbmixer_t *)));
  connect(devdWatcher, SIGNAL(sendRemoveMixer(dsbmixer_t *)), this,
          SLOT(removeMixer(dsbmixer_t *)));
  devdWatcher->start();
#endif
  unitCheckTimer->start(this->mixerSettings->getUnitChkIval());
  pollTimer->start(this->mixerSettings->getPollIval());
  connect(this->mixerSettings, SIGNAL(pollIvalChanged(int)), this,
          SLOT(setPollIval(int)));
  connect(this->mixerSettings, SIGNAL(unitChkIvalChanged(int)), this,
          SLOT(setDefaultUnitCheckIval(int)));
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
  int unit{dsbmixer_poll_default_unit()};
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

int MixerList::setDefaultMixer(const Mixer *mixer) {
  return (setDefaultMixer(mixer->getUnit()));
}

int MixerList::setDefaultMixer(int unit) {
  if (dsbmixer_set_default_unit(unit) != 0) return (-1);
  Mixer *mixer{unitToMixer(unit)};
  if (!mixer) return (-1);
  defaultMixer = mixer;
  emit defaultMixerChanged(defaultMixer);
  return (0);
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
  int unit{dsbmixer_poll_default_unit()};
  if (unit < 0) return;
  if (defaultMixer->getUnit() == unit) return;
  defaultMixer = unitToMixer(unit);
  if (defaultMixer) emit defaultMixerChanged(defaultMixer);
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

Mixer *MixerList::getDefaultMixer() const { return (defaultMixer); }

Mixer *MixerList::at(qsizetype idx) const { return (mixers.at(idx)); }

qsizetype MixerList::count() const { return (mixers.count()); }

void MixerList::setDefaultUnitCheckIval(int ms) {
  if (ms <= 0)
    unitCheckTimer->stop();
  else
    unitCheckTimer->start(ms);
}

void MixerList::setPollIval(int ms) {
  if (ms <= 0)
    pollTimer->stop();
  else
    pollTimer->start(ms);
}
