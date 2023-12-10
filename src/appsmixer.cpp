/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "appsmixer.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QWidget>

AppsMixer::AppsMixer(dsbappsmixer_t &mixer, const IconLoader &iconLoader,
                     const MixerSettings &mixerSettings, QWidget *parent)
    : QDialog(parent), mixer{&mixer}, mixerSettings{&mixerSettings} {
  timer = new QTimer(this);
  timer->start(this->mixerSettings->getPollIval());
  layout = new QHBoxLayout;

  createChannels();
  setLayout(layout);
  setWinSize(parent);
  setWindowIcon(iconLoader.appsMixerIcon);
  setWindowTitle(tr("Applications mixer"));
  setAttribute(Qt::WA_DeleteOnClose);

  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  connect(this->mixerSettings, SIGNAL(lrViewChanged(bool)), this,
          SLOT(redrawMixer()));
  connect(this->mixerSettings, SIGNAL(scaleTicksChanged(bool)), this,
          SLOT(redrawMixer()));
  connect(this->mixerSettings, SIGNAL(pollIvalChanged(int)), this,
          SLOT(catchPollIvalChanged()));
}

void AppsMixer::setWinSize(QWidget *parent) {
  int h{defaultHeight};
  if (!parent) {
    resize(QSize(minimumWidth(), h));
    return;
  }
  if (parent->height() > 0) h = (parent->height() * 70 / 100);
  resize(QSize(minimumWidth(), h));
}

void AppsMixer::adjustWinSize() {
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  QSize s{sizeHint()};
  s.setHeight(height());
  resize(s);
}

AppsMixer::~AppsMixer() {
  dsbappsmixer_destroy(mixer);
  deleteChannels();
}

dsbappsmixer_t *AppsMixer::getDev() const { return (mixer); }

void AppsMixer::catchPollIvalChanged() {
  timer->start(mixerSettings->getPollIval());
}

void AppsMixer::setLVol(int chan, int lvol) {
  dsbappsmixer_set_lvol(this->mixer, chan, lvol);
  lvol = DSBMIXER_CHAN_LEFT(dsbappsmixer_get_vol(mixer, chan));
  const int rvol{DSBMIXER_CHAN_RIGHT(dsbappsmixer_get_vol(mixer, chan))};
  channels.at(chan)->setVol(lvol, rvol);
}

void AppsMixer::setRVol(int chan, int rvol) {
  dsbappsmixer_set_rvol(this->mixer, chan, rvol);
  rvol = DSBMIXER_CHAN_RIGHT(dsbappsmixer_get_vol(mixer, chan));
  const int lvol{DSBMIXER_CHAN_LEFT(dsbappsmixer_get_vol(mixer, chan))};
  channels.at(chan)->setVol(lvol, rvol);
}

void AppsMixer::setVol(int chan, int lvol, int rvol) {
  dsbappsmixer_set_vol(this->mixer, chan, DSBMIXER_CHAN_CONCAT(lvol, rvol));
  lvol = DSBMIXER_CHAN_LEFT(dsbappsmixer_get_vol(mixer, chan));
  rvol = DSBMIXER_CHAN_RIGHT(dsbappsmixer_get_vol(mixer, chan));
  channels.at(chan)->setVol(lvol, rvol);
}

void AppsMixer::setMute(int chan, int state) {
  dsbappsmixer_set_mute(mixer, chan, (state == Qt::Checked));
  channels.at(chan)->setMute(dsbappsmixer_is_muted(mixer, chan));
}

void AppsMixer::updateVolumes() {
  for (auto i{0}; i < channels.count(); i++) {
    if (dsbappsmixer_is_muted(mixer, i)) continue;
    const int lvol{DSBMIXER_CHAN_LEFT(dsbappsmixer_get_vol(mixer, i))};
    const int rvol{DSBMIXER_CHAN_RIGHT(dsbappsmixer_get_vol(mixer, i))};
    ChanSlider *cs{channels.at(i)};
    cs->setVol(lvol, rvol);
    if (cs->isMuted()) cs->setMute(false);
  }
}

void AppsMixer::update() {
  switch (dsbappsmixer_poll(mixer)) {
    case 0:
      updateVolumes();
      break;
    case DSBAPPSMIXER_CHANS_CHANGED:
      deleteChannels();
      dsbappsmixer_reinit(mixer);
      createChannels();
      adjustWinSize();
      break;
  }
}

void AppsMixer::createChannels() {
  channelContainer = new QWidget(this);
  QHBoxLayout *hbox{new QHBoxLayout(channelContainer)};
  constexpr bool canRec{false}, chanMutable{true}, isTraySlider{false};
  for (auto i{0}; i < dsbappsmixer_get_nchans(mixer); i++) {
    const int lvol{DSBMIXER_CHAN_LEFT(dsbappsmixer_get_vol(mixer, i))};
    const int rvol{DSBMIXER_CHAN_RIGHT(dsbappsmixer_get_vol(mixer, i))};
    const char *name{dsbappsmixer_get_name(mixer, i)};
    const pid_t pid{dsbappsmixer_get_pid(mixer, i)};
    ChanSlider *cs{new ChanSlider(
        QString("%1 (%2)").arg(name).arg(pid), i, lvol, rvol, canRec,
        chanMutable, mixerSettings->lrViewEnabled(), isTraySlider)};
    cs->setTicks(mixerSettings->scaleTicksEnabled());
    connect(cs, SIGNAL(lVolumeChanged(int, int)), this,
            SLOT(setLVol(int, int)));
    connect(cs, SIGNAL(rVolumeChanged(int, int)), this,
            SLOT(setRVol(int, int)));
    connect(cs, SIGNAL(volumeChanged(int, int, int)), this,
            SLOT(setVol(int, int, int)));
    connect(cs, SIGNAL(muteChanged(int, int)), this, SLOT(setMute(int, int)));
    channels.append(cs);
    hbox->addWidget(cs, 0, Qt::AlignLeft);
  }
  layout->addWidget(channelContainer);
  if (!channels.count()) hbox->addWidget(new QLabel(tr("No audio apps found")));
}

void AppsMixer::deleteChannels() {
  channels.clear();
  delete channelContainer;
}

void AppsMixer::redrawMixer() {
  deleteChannels();
  createChannels();
  for (auto &chan : channels)
    chan->setTicks(mixerSettings->scaleTicksEnabled());
}
