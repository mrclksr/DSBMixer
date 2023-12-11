/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
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
#include <QHBoxLayout>
#include <QList>
#include <QString>
#include <QWidget>

#include "chanslider.h"
#include "libdsbmixer.h"
#include "mixersettings.h"

class Mixer : public QWidget {
  Q_OBJECT
 public:
  Mixer(dsbmixer_t &mixer, const MixerSettings &mixerSettings,
        QWidget *parent = nullptr);
  ~Mixer();
  void update();
  void muteMaster(bool on);
  void toggleMasterMute();
  bool isMuted(int chan) const;
  int getUnit() const;
  int getMasterLVol();
  int getMasterRVol();
  int getID() const;
  QString getDevName() const;
  dsbmixer_t *getDev() const;
  QString getName() const;

 public slots:
  void setVol(int chan, int lvol, int rvol);
  void setMute(int chan, bool on);
  void toggleMute(int chan);

 private slots:
  void setLVol(int chan, int lvol);
  void setRVol(int chan, int rvol);
  void setMute(int chan, int state);
  void setRecSrc(int chan, int state);
  void redrawMixer();

 signals:
  void masterVolChanged(int unit, int lvol, int rvol);
  void muteStateChanged(int unit, bool muted);

 public slots:
  void changeMasterVol(int volinc);

 private:
  int channelIndex(int chan) const;
  void deleteChannels();
  void createChannels();
  void updateSlider(int chan);

 private:
  QString cardName;
  QString devName;
  QHBoxLayout *layout{};
  dsbmixer_t *mixer{nullptr};
  QList<ChanSlider *> channels;
  const MixerSettings *mixerSettings{nullptr};
};
