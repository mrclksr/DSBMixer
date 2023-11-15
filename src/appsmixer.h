/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QDialog>
#include <QHBoxLayout>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "chanslider.h"
#include "iconloader.h"
#include "libdsbmixer.h"
#include "mixersettings.h"

class AppsMixer : public QDialog {
  Q_OBJECT
 public:
  AppsMixer(dsbappsmixer_t &mixer, const IconLoader &iconLoader,
            const MixerSettings &mixerSettings, QWidget *parent = nullptr);
  ~AppsMixer();
  dsbappsmixer_t *getDev() const;
 private slots:
  void setMute(int chan, int state);
  void setLVol(int chan, int lvol);
  void setRVol(int chan, int rvol);
  void setVol(int chan, int lvol, int rvol);
  void update();
  void redrawMixer();
  void catchPollIvalChanged();

 private:
  void deleteChannels();
  void createChannels();
  void updateVolumes();
  void adjustWinSize();
  void setWinSize(QWidget *parent);

 private:
  const int defaultWidth{200};
  const int defaultHeight{200};
  dsbappsmixer_t *mixer{nullptr};
  QTimer *timer{nullptr};
  QHBoxLayout *layout{nullptr};
  QWidget *channelContainer{nullptr};
  QList<ChanSlider *> channels;
  const MixerSettings *mixerSettings{nullptr};
};
