/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QCheckBox>
#include <QDebug>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

class ChanSlider : public QGroupBox {
  Q_OBJECT
 public:
  ChanSlider(const QString &name, int id, int lvol, int rvol,
             bool canRecord = false, bool muteable = false, bool lrview = false,
             bool tray = false, QWidget *parent = 0);
  void setVol(int lvol, int rvol);
  void setLVol(int lvol);
  void setRVol(int rvol);
  void setRecSrc(bool state);
  void sliderSetToolTip(int lvol, int rvol);
  void setMute(bool mute);
  void setTicks(bool on);
  void addVol(int volinc);
  bool isMuted();
  bool isRecSrc();

 private:
  void initLRView();
  void initUView();
  void addRecCB(QBoxLayout *layout);
  void addMuteCB(QBoxLayout *layout);
 signals:
  void recSourceChanged(int id, int state);
  void lVolumeChanged(int id, int vol);
  void rVolumeChanged(int id, int vol);
  void volumeChanged(int id, int lvol, int rvol);
  void muteChanged(int id, int state);
  void lockChanged(int state);
 private slots:
  void emitVolumeChanged(int);
  void emitRecSourceChanged(int);
  void emitLVolumeChanged(int lvol);
  void emitRVolumeChanged(int rvol);
  void emitMuteChanged(int state);
  void emitLockChanged(int state);

 public:
  int id{};
  int vol{};
  int rvol{};
  int lvol{};

 private:
  bool lrview{false};
  bool muted{false};
  bool muteable{false};
  bool tray{false};
  bool canRec{false};
  bool recSrc{false};
  bool lrlocked{false};
  QLabel *volabel{};
  QLabel *volabell{};
  QLabel *volabelr{};
  QSlider *slider{};
  QSlider *rslider{};
  QSlider *lslider{};
  QCheckBox *recCB{};
  QCheckBox *muteCB{};
  QCheckBox *lockCB{};
};
