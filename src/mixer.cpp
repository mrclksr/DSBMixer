/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "mixer.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QRegExp>
#else
#include <QRegularExpression>
#endif

#include "libdsbmixer.h"

Mixer::Mixer(dsbmixer_t &mixer, const MixerSettings &mixerSettings,
             QWidget *parent)
    : QWidget(parent), mixer{&mixer}, mixerSettings{&mixerSettings} {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QRegExp rx("[<>]+");
#else
  QRegularExpression rx("[<>]+");
#endif
  QStringList tokens{QString(dsbmixer_get_card_name(this->mixer))
                         .split(rx, Qt::SkipEmptyParts)};
  if (tokens.size() > 1)
    cardName = tokens[0] + tokens[1];
  else
    cardName = QString(dsbmixer_get_card_name(this->mixer));
  devName = QString(dsbmixer_get_name(this->mixer));
  layout = new QHBoxLayout;
  setLayout(layout);
  createChannels();
  connect(this->mixerSettings, SIGNAL(lrViewChanged(bool)), this,
          SLOT(redrawMixer()));
  connect(this->mixerSettings, SIGNAL(scaleTicksChanged(bool)), this,
          SLOT(redrawMixer()));
  connect(this->mixerSettings, SIGNAL(chanMaskChanged(int)), this,
          SLOT(redrawMixer()));
}

Mixer::~Mixer() { dsbmixer_del_mixer(mixer); }

void Mixer::createChannels() {
  for (int i = 0; i < dsbmixer_get_nchans(mixer); i++) {
    const int chan{dsbmixer_get_chan_id(mixer, i)};
    const bool muteable{chan == DSBMIXER_MASTER};
    if (!((1 << chan) & mixerSettings->getChanMask())) continue;
    const int lvol{DSBMIXER_CHAN_LEFT(dsbmixer_get_vol(mixer, chan))};
    const int rvol{DSBMIXER_CHAN_RIGHT(dsbmixer_get_vol(mixer, chan))};
    const char *name{dsbmixer_get_chan_name(mixer, chan)};
    ChanSlider *cs{new ChanSlider(QString(name), chan, lvol, rvol,
                                  dsbmixer_can_rec(mixer, chan), muteable,
                                  mixerSettings->lrViewEnabled(), false)};
    cs->setTicks(mixerSettings->scaleTicksEnabled());
    connect(cs, SIGNAL(lVolumeChanged(int, int)), this,
            SLOT(setLVol(int, int)));
    connect(cs, SIGNAL(rVolumeChanged(int, int)), this,
            SLOT(setRVol(int, int)));
    connect(cs, SIGNAL(volumeChanged(int, int, int)), this,
            SLOT(setVol(int, int, int)));
    connect(cs, SIGNAL(recSourceChanged(int, int)), this,
            SLOT(setRecSrc(int, int)));

    if (chan == DSBMIXER_MASTER) {
      cs->setMute(dsbmixer_is_muted(mixer, chan));
      connect(cs, SIGNAL(muteChanged(int, int)), this, SLOT(setMute(int, int)));
    }
    cs->setRecSrc(dsbmixer_is_recsrc(mixer, chan));
    channels.append(cs);
    layout->addWidget(cs, 0, Qt::AlignLeft);
  }
}

void Mixer::redrawMixer() {
  deleteChannels();
  createChannels();
  for (auto &chan : channels)
    chan->setTicks(mixerSettings->scaleTicksEnabled());
}

int Mixer::channelIndex(int chan) const {
  for (int idx = 0; idx < channels.count(); idx++) {
    if (channels.at(idx)->id == chan) return (idx);
  }
  return (-1);
}

void Mixer::setLVol(int chan, int lvol) {
  dsbmixer_set_lvol(this->mixer, chan, lvol);
  updateSlider(chan);
}

void Mixer::setRVol(int chan, int rvol) {
  dsbmixer_set_rvol(this->mixer, chan, rvol);
  updateSlider(chan);
}

void Mixer::setVol(int chan, int lvol, int rvol) {
  dsbmixer_set_vol(this->mixer, chan, DSBMIXER_CHAN_CONCAT(lvol, rvol));
  updateSlider(chan);
}

void Mixer::setRecSrc(int chan, int state) {
  dsbmixer_set_rec(mixer, chan, (state == Qt::Checked));
  //
  // Enabling/Disabling of a record source can lead to changes
  // in other record sources.
  //
  for (auto &c : channels) c->setRecSrc(dsbmixer_is_recsrc(mixer, c->id));
}

void Mixer::setMute(int chan, int state) {
  bool mute{(state == Qt::Checked)};
  dsbmixer_set_mute(mixer, chan, mute);
  if (mute == dsbmixer_is_muted(mixer, chan))
    emit muteStateChanged(dsbmixer_get_unit(mixer), mute);
}

void Mixer::setMute(int chan, bool on) {
  const int idx{channelIndex(chan)};
  if (idx < 0) return;
  dsbmixer_set_mute(mixer, chan, on);
  if (on == dsbmixer_is_muted(mixer, chan))
    emit muteStateChanged(dsbmixer_get_unit(mixer), on);
  channels.at(idx)->setMute(on);
}

void Mixer::toggleMute(int chan) {
  setMute(chan, !dsbmixer_is_muted(mixer, chan));
}

void Mixer::muteMaster(bool on) { setMute(DSBMIXER_MASTER, on); }

void Mixer::toggleMasterMute() { toggleMute(DSBMIXER_MASTER); }

void Mixer::updateSlider(int chan) {
  const int idx{channelIndex(chan)};
  if (idx < 0) return;
  int lvol{DSBMIXER_CHAN_LEFT(dsbmixer_get_vol(mixer, chan))};
  int rvol{DSBMIXER_CHAN_RIGHT(dsbmixer_get_vol(mixer, chan))};
  channels.at(idx)->setVol(lvol, rvol);
  if (chan == DSBMIXER_MASTER)
    emit masterVolChanged(dsbmixer_get_unit(this->mixer), lvol, rvol);
}

void Mixer::update() {
  for (auto &cs : channels) {
    const int chan{cs->id};
    const int lvol{DSBMIXER_CHAN_LEFT(dsbmixer_get_vol(mixer, chan))};
    const int rvol{DSBMIXER_CHAN_RIGHT(dsbmixer_get_vol(mixer, chan))};
    if (chan == DSBMIXER_MASTER)
      emit masterVolChanged(dsbmixer_get_unit(this->mixer), lvol, rvol);
    cs->setVol(lvol, rvol);
    if (dsbmixer_can_rec(mixer, chan))
      cs->setRecSrc(dsbmixer_is_recsrc(mixer, chan));
    cs->setMute(dsbmixer_is_muted(this->mixer, chan));
  }
}

void Mixer::changeMasterVol(int volinc) {
  const int chan{DSBMIXER_MASTER};
  const int idx{channelIndex(chan)};
  if (idx < 0) return;
  if (dsbmixer_is_muted(mixer, chan)) {
    dsbmixer_set_mute(mixer, chan, false);
    if (!dsbmixer_is_muted(mixer, chan)) {
      emit muteStateChanged(dsbmixer_get_unit(this->mixer), false);
      channels.at(idx)->setMute(false);
      return;
    }
  }
  channels.at(idx)->addVol(volinc);
  int lvol{channels.at(idx)->lvol};
  int rvol{channels.at(idx)->rvol};
  dsbmixer_set_lvol(this->mixer, chan, lvol);
  dsbmixer_set_rvol(this->mixer, chan, rvol);
  updateSlider(chan);
}

void Mixer::deleteChannels() {
  for (auto &chan : channels) {
    chan->blockSignals(true);
    delete chan;
  }
  channels.clear();
}

bool Mixer::isMuted(int chan) const {
  const int idx{channelIndex(chan)};
  if (idx < 0) return (false);
  return (dsbmixer_is_muted(mixer, chan));
}

int Mixer::getUnit() const { return (dsbmixer_get_unit(mixer)); }

int Mixer::getMasterLVol() {
  return (DSBMIXER_CHAN_LEFT(dsbmixer_get_vol(mixer, DSBMIXER_MASTER)));
}

int Mixer::getMasterRVol() {
  return (DSBMIXER_CHAN_RIGHT(dsbmixer_get_vol(mixer, DSBMIXER_MASTER)));
}

int Mixer::getID() const { return (dsbmixer_get_id(mixer)); }

dsbmixer_t *Mixer::getDev() const { return (mixer); }

QString Mixer::getName() const { return (cardName); }

QString Mixer::getDevName() const { return (devName); }
