/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QList>
#include <QProcess>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>

#include "libdsbmixer.h"
#include "mixersettings.h"
#include "sndsettings.h"

class Preferences : public QDialog {
  Q_OBJECT
 public:
  Preferences(MixerSettings &mixerSettings, SoundSettings &soundSettings, QWidget *parent = 0);
   ~Preferences();
 public slots:
  void acceptSlot();
  void rejectSlot();
  void toggleTestSound();
  void playSound(int unit);
  void stopSound();
  void soundPlayerFinished(int, QProcess::ExitStatus);

 protected:
  void keyPressEvent(QKeyEvent *event);

 private:
  QWidget *createViewTab();
  QWidget *createDefaultDeviceTab();
  QWidget *createBehaviorTab();
  QWidget *createAdvancedTab();
  void createThemeComboBox();

 private:
  bool testSoundPlaying;
  QProcess *soundPlayer;
  QSpinBox *amplifySb;
  QSpinBox *feederRateQualitySb;
  QSpinBox *maxAutoVchansSb;
  QSpinBox *latencySb;
  QSpinBox *pollIvalSb;
  QSpinBox *unitChkIvalSb;
  QSpinBox *granularitySb;
  QLineEdit *commandEdit;
  QCheckBox *viewTabCb[DSBMIXER_MAX_CHANNELS];
  QCheckBox *lrViewCb;
  QCheckBox *showTicksCb;
  QCheckBox *bypassMixerCb;
  QCheckBox *inverseScrollCb;
  QComboBox *themeBox;
  QTabWidget *tabs;
  QPushButton *testBt;
  QList<QRadioButton *> defaultDeviceRb;
  MixerSettings *mixerSettings;
  SoundSettings *soundSettings;
};
