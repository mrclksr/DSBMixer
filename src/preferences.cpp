/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "preferences.h"

#include <paths.h>

#include <QApplication>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QDirIterator>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "libdsbmixer.h"
#include "qnamespace.h"
#include "qt-helper/qt-helper.h"

Preferences::Preferences(Settings &oldSettings, QWidget *parent)
    : QDialog(parent) {
  settings = oldSettings;

  qApp->setQuitOnLastWindowClosed(false);

  QVBoxLayout *vbox{new QVBoxLayout};
  QHBoxLayout *hbox{new QHBoxLayout};

  tabs = new QTabWidget(this);

  QIcon icon = qh::loadStockIcon(QStyle::SP_DialogOkButton);
  QPushButton *apply = new QPushButton(icon, tr("&Ok"));
  connect(apply, SIGNAL(clicked()), this, SLOT(acceptSlot()));

  icon = qh::loadStockIcon(QStyle::SP_DialogCancelButton);
  QPushButton *close = new QPushButton(icon, tr("&Cancel"));
  connect(close, SIGNAL(clicked()), this, SLOT(rejectSlot()));

  tabs->addTab(createViewTab(), tr("View"));
  tabs->addTab(createDefaultDeviceTab(), tr("Default device"));
  tabs->addTab(createBehaviorTab(), tr("Behavior"));
  tabs->addTab(createAdvancedTab(), tr("Advanced"));

  vbox->addWidget(tabs);
  hbox->addWidget(apply, 1, Qt::AlignRight);
  hbox->addWidget(close, 0, Qt::AlignRight);
  vbox->addLayout(hbox);
  vbox->setContentsMargins(15, 15, 15, 15);
  this->setLayout(vbox);

  icon = qh::loadIcon(QStringList("preferences-desktop-multimedia"));
  if (icon.isNull()) icon = QIcon(":/icons/preferences-desktop-multimedia.png");
  setWindowIcon(icon);
  setWindowTitle(tr("Preferences"));
}

void Preferences::acceptSlot() {
  for (int i{0}; i < DSBMIXER_MAX_CHANNELS; i++) {
    if (viewTabCb[i]->checkState() == Qt::Checked)
      settings.chanMask |= (1 << i);
    else
      settings.chanMask &= ~(1 << i);
  }
  settings.showTicks = showTicksCb->checkState() == Qt::Checked ? true : false;
  settings.inverseScroll =
      inverseScrollCb->checkState() == Qt::Checked ? true : false;
  if (lrViewCb->checkState() == Qt::Checked)
    settings.lrView = true;
  else
    settings.lrView = false;
  for (int i{0}; i < defaultDeviceRb.count(); i++) {
    if (defaultDeviceRb.at(i)->isChecked()) settings.defaultUnit = i;
  }
  settings.feederRateQuality = feederRateQualitySb->value();
  settings.amplify = amplifySb->value();
  settings.maxAutoVchans = maxAutoVchansSb->value();
  settings.latency = latencySb->value();
  settings.bypassMixer =
      bypassMixerCb->checkState() == Qt::Checked ? true : false;
  settings.pollIval = pollIvalSb->value();
  settings.unitChkIval = unitChkIvalSb->value();
  settings.volInc = granularitySb->value();
  if (testSoundPlaying) stopSound();
  this->accept();
}

void Preferences::rejectSlot() {
  if (testSoundPlaying) stopSound();
  this->reject();
}

void Preferences::createThemeComboBox() {
  themeBox = new QComboBox;
  QStringList paths{QIcon::themeSearchPaths()};
  QStringList names;

  for (auto &p : paths) {
    QDirIterator it(p);
    while (it.hasNext()) {
      QString indexPath{QString("%1/index.theme").arg(it.next())};
      if (!it.fileInfo().isDir()) continue;
      QString name{it.fileName()};
      if (name == "." || name == "..") continue;
      QFile indexFile(indexPath);
      if (!indexFile.exists()) continue;
      indexFile.close();
      names.append(name);
    }
  }
  names.sort(Qt::CaseInsensitive);
  names.removeDuplicates();
  themeBox->addItems(names);

  int index{themeBox->findText(QString(settings.themeName), Qt::MatchExactly)};
  if (index != -1) themeBox->setCurrentIndex(index);
  connect(themeBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeTheme(int)));
}

void Preferences::changeTheme(int index) {
  if (index != -1) settings.themeName = themeBox->currentText();
}

QWidget *Preferences::createViewTab() {
  QWidget *widget{new QWidget(this)};
  QGroupBox *mdGrp{new QGroupBox(tr("Select mixer devices to be visible\n"))};
  QGroupBox *slGrp{new QGroupBox(tr("Slider settings"))};
  QGroupBox *thGrp{new QGroupBox(tr("Tray icon theme"))};
  const char **names{dsbmixer_get_chan_names()};
  QVBoxLayout *vbox{new QVBoxLayout};
  QVBoxLayout *slBox{new QVBoxLayout};
  QGridLayout *grid{new QGridLayout};
  QHBoxLayout *hbox{new QHBoxLayout};

  for (int i{0}; i < DSBMIXER_MAX_CHANNELS; i++) {
    viewTabCb[i] = new QCheckBox(QString(names[i]), this);
    if (settings.chanMask & (1 << i)) viewTabCb[i]->setCheckState(Qt::Checked);
  }

  for (int row{0}; row < 5; row++) {
    for (int col{0}; col < 5; col++)
      grid->addWidget(viewTabCb[row * 5 + col], row, col);
  }
  mdGrp->setLayout(grid);
  vbox->addWidget(mdGrp);

  lrViewCb = new QCheckBox(tr("Show left and right channel"), this);
  lrViewCb->setCheckState(settings.lrView ? Qt::Checked : Qt::Unchecked);

  showTicksCb = new QCheckBox(tr("Show ticks"), this);
  showTicksCb->setCheckState(settings.showTicks ? Qt::Checked : Qt::Unchecked);
  slBox->addWidget(lrViewCb);
  slBox->addWidget(showTicksCb);

  slGrp->setLayout(slBox);
  vbox->addWidget(slGrp);
  createThemeComboBox();
  hbox->addWidget(themeBox);
  thGrp->setLayout(hbox);
  vbox->addWidget(thGrp);
  vbox->addStretch(1);
  widget->setLayout(vbox);

  return (widget);
}

QWidget *Preferences::createDefaultDeviceTab() {
  QWidget *widget{new QWidget(this)};
  QVBoxLayout *vbox{new QVBoxLayout};
  QHBoxLayout *hbox{new QHBoxLayout};
  QVBoxLayout *ddBox{new QVBoxLayout};
  QGroupBox *ddGrp{new QGroupBox(tr("Select default sound device\n"))};
  testBt = new QPushButton(tr("Test sound"));
  commandEdit = new QLineEdit;
  soundPlayer = new QProcess(this);

  testSoundPlaying = false;
  for (int i{0}; i < dsbmixer_get_ndevs(); i++) {
    dsbmixer_t *dev{dsbmixer_get_mixer(i)};
    QRadioButton *rb{
        new QRadioButton(QString(dsbmixer_get_card_name(dev)), this)};
    defaultDeviceRb.append(rb);
    if (i == settings.defaultUnit) rb->setChecked(true);
    ddBox->addWidget(rb);
  }
  ddBox->addStretch(1);
  ddGrp->setLayout(ddBox);
  commandEdit->setText(settings.playCmd);
  commandEdit->setToolTip(tr("Enter a command which plays a sound"));

  hbox->addWidget(commandEdit);
  hbox->addWidget(testBt, 0, Qt::AlignRight);
  vbox->addWidget(ddGrp);
  vbox->addLayout(hbox);

  widget->setLayout(vbox);
  connect(testBt, SIGNAL(clicked()), this, SLOT(toggleTestSound()));
  connect(commandEdit, SIGNAL(textEdited(const QString &)), this,
          SLOT(commandChanged(const QString &)));
  return (widget);
}

void Preferences::toggleTestSound() {
  if (testSoundPlaying) {
    stopSound();
    return;
  }
  for (int i{0}; i < dsbmixer_get_ndevs(); i++) {
    QRadioButton *rb{defaultDeviceRb.at(i)};
    if (rb->isChecked()) {
      playSound(i);
      return;
    }
  }
}

void Preferences::playSound(int unit) {
  if (testSoundPlaying) return;
  if (dsbmixer_set_default_unit(unit) == -1) {
    qh::warn(this, QString("Couldn't set default sound unit to %1").arg(unit));
    return;
  }
  soundPlayer->start(_PATH_BSHELL,
                     QStringList() << "-c" << settings.playCmd.toUtf8().data());
  soundPlayer->closeReadChannel(QProcess::StandardOutput);
  soundPlayer->closeReadChannel(QProcess::StandardError);
  soundPlayer->waitForStarted(-1);

  if (soundPlayer->state() == QProcess::NotRunning) {
    qh::warnx(this, QString("Couldn't execute '%1': %2")
                        .arg(settings.playCmd)
                        .arg(soundPlayer->errorString()));
    return;
  }
  connect(soundPlayer, SIGNAL(finished(int, QProcess::ExitStatus)), this,
          SLOT(soundPlayerFinished(int, QProcess::ExitStatus)));
  testBt->setText(tr("Stop"));
  testSoundPlaying = true;
}

void Preferences::stopSound() {
  if (!testSoundPlaying) return;
  disconnect(soundPlayer, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
  soundPlayer->close();
  testSoundPlaying = false;
  testBt->setText(tr("Test sound"));
  if (dsbmixer_set_default_unit(settings.defaultUnit) == -1) {
    qh::warn(this, QString("Couldn't reset default sound unit to %1")
                       .arg(settings.defaultUnit));
  }
}

void Preferences::soundPlayerFinished(int exitCode,
                                      QProcess::ExitStatus status) {
  if (status == QProcess::CrashExit)
    qh::warnx(this, "Command crashed");
  else if (exitCode != 0)
    qh::warnx(this,
              QString("Command returned with exit code %1").arg(exitCode));
  stopSound();
}

void Preferences::commandChanged(const QString &text) {
  settings.playCmd = text;
}

QWidget *Preferences::createBehaviorTab() {
  QWidget *widget{new QWidget(this)};
  QVBoxLayout *vbox{new QVBoxLayout};
  QGridLayout *grid{new QGridLayout};
  QGroupBox *grpBox{new QGroupBox(tr("Behavior"))};

  pollIvalSb = new QSpinBox(this);
  unitChkIvalSb = new QSpinBox(this);
  granularitySb = new QSpinBox(this);
  inverseScrollCb = new QCheckBox(tr("Inverse scroll direction of tray"));
  inverseScrollCb->setToolTip(
      tr("On some panels the scroll direction is inversed."));
  inverseScrollCb->setChecked(settings.inverseScroll);
  pollIvalSb->setRange(10, 10000);
  pollIvalSb->setValue(settings.pollIval);
  pollIvalSb->setSuffix(" ms");
  pollIvalSb->setToolTip(
      tr("Defines the time interval in milliseconds mixer\n"
         "devices are polled. Higher values mean less CPU usage.\n"
         "Lower values mean less latency when showing the\n"
         "current volume changed by other programs."));
  unitChkIvalSb->setRange(10, 1000000);
  unitChkIvalSb->setValue(settings.unitChkIval);
  unitChkIvalSb->setSuffix(" ms");
  unitChkIvalSb->setToolTip(
      tr("Defines the time interval in milliseconds to\n"
         "check whether the default sound device was changed.\n"
         "Higher values mean less CPU usage.\n"));

  granularitySb->setRange(1, 10);
  granularitySb->setValue(settings.volInc);
  granularitySb->setToolTip(
      tr("Defines the mouse wheel scroll lines for changing the volume."));

  QLabel *label{new QLabel(tr("Poll mixers every"))};
  grid->addWidget(label, 1, 0);
  grid->addWidget(pollIvalSb, 1, 1);

  label = new QLabel(tr("Check for default sound device change every"));
  grid->addWidget(label, 2, 0);
  grid->addWidget(unitChkIvalSb, 2, 1);

  label = new QLabel(tr("Mouse wheel scroll lines"));
  grid->addWidget(label, 3, 0);
  grid->addWidget(granularitySb, 3, 1);

  grid->addWidget(inverseScrollCb, 4, 0);

  grpBox->setLayout(grid);
  vbox->addWidget(grpBox);
  vbox->addStretch(1);
  widget->setLayout(vbox);

  return (widget);
}

QWidget *Preferences::createAdvancedTab() {
  QWidget *widget{new QWidget(this)};
  QVBoxLayout *vbox{new QVBoxLayout};
  QGridLayout *grid{new QGridLayout};
  QGroupBox *grpBox{new QGroupBox(tr("Advanced settings"))};

  amplifySb = new QSpinBox(this);
  feederRateQualitySb = new QSpinBox(this);
  maxAutoVchansSb = new QSpinBox(this);
  latencySb = new QSpinBox(this);

  bypassMixerCb = new QCheckBox(tr("Bypass mixer"));

  bypassMixerCb->setToolTip(
      tr("Enable this to allow applications to use\n"
         "their own existing mixer logic to control\n"
         "their own channel volume."));
  maxAutoVchansSb->setRange(0, 256);
  maxAutoVchansSb->setValue(settings.maxAutoVchans);
  maxAutoVchansSb->setToolTip(
      tr("Defines the max. number of virtual playback\n"
         "and recording channels that can be created.\n"
         "Virtual channels allow programs to use more playback\n"
         "and recording channels than the physical hardware\n"
         "provides."));

  latencySb->setRange(0, 10);
  latencySb->setValue(settings.latency);
  latencySb->setToolTip(tr("Lower values mean less buffering and latency."));
  amplifySb->setRange(0, 100);
  amplifySb->setValue(settings.amplify);
  amplifySb->setSuffix(" dB");
  amplifySb->setToolTip(
      tr("Lower values mean more amplification, but can\n"
         "produce sound clipping when chosen too low.\n"
         "Higher values mean finer volume control."));
  feederRateQualitySb->setRange(1, 4);
  feederRateQualitySb->setValue(settings.feederRateQuality);
  feederRateQualitySb->setToolTip(
      tr("Higher values mean better sample rate conversion,\n"
         "but more memory and CPU usage."));
  bypassMixerCb->setCheckState(settings.bypassMixer ? Qt::Checked
                                                    : Qt::Unchecked);

  QLabel *label{new QLabel(tr("Amplification:"))};
  grid->addWidget(label, 0, 0);
  grid->addWidget(amplifySb, 0, 1);

  label = new QLabel(tr("Sample rate converter quality:"));
  grid->addWidget(label, 1, 0);
  grid->addWidget(feederRateQualitySb, 1, 1);

  label = new QLabel(tr("Max. auto VCHANS:"));
  grid->addWidget(label, 2, 0);
  grid->addWidget(maxAutoVchansSb, 2, 1);

  label = new QLabel(tr("Latency (0 low, 10 high):"));
  grid->addWidget(label, 3, 0);
  grid->addWidget(latencySb, 3, 1);

  grid->addWidget(new QLabel(""), 4, 0);
  grid->addWidget(bypassMixerCb, 5, 0);

  grpBox->setLayout(grid);
  vbox->addWidget(grpBox);
  vbox->addStretch(1);
  widget->setLayout(vbox);

  return (widget);
}

void Preferences::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
    case Qt::Key_Escape:
      rejectSlot();
      break;
    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
      int tabno{e->text().toInt()};
      if (e->modifiers() == Qt::AltModifier) {
        if (tabno == 0) tabno = tabs->count();
        tabs->setCurrentIndex(tabno - 1);
      }
  }
}
