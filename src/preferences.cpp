/*-
 * Copyright (c) 2016 Marcel Kaiser. All rights reserved.
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

#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGridLayout>
#include <QCloseEvent>
#include <QBoxLayout>

#include "preferences.h"
#include "qt-helper/qt-helper.h"

Preferences::Preferences(int chanMask, int amplify, int feederRateQuality,
	int defaultUnit, int maxAutoVchans, int latency, bool bypassMixer,
	bool lrView, bool showTicks, int pollIval, const char *playCmd,
	QWidget *parent) : QDialog(parent) {

	this->chanMask = chanMask;
	this->amplify = amplify;
	this->feederRateQuality = feederRateQuality;
	this->defaultUnit = defaultUnit;
	this->maxAutoVchans = maxAutoVchans;
	this->latency = latency;
	this->bypassMixer = bypassMixer;
	this->lrView = lrView;
	this->showTicks = showTicks;
	this->pollIval = pollIval;
	this->playCmd = QString(playCmd);

	qApp->setQuitOnLastWindowClosed(false);

	QVBoxLayout *vbox = new QVBoxLayout();
	QHBoxLayout *hbox = new QHBoxLayout();

	QTabWidget *tabs = new QTabWidget(this);

	QIcon icon = qh_loadStockIcon(QStyle::SP_DialogOkButton);
	QPushButton *apply = new QPushButton(icon, tr("&Ok"));
	connect(apply, SIGNAL(clicked()), this, SLOT(acceptSlot()));

	icon = qh_loadStockIcon(QStyle::SP_DialogCancelButton);
	QPushButton *close = new QPushButton(icon, tr("&Cancel"));
	connect(close, SIGNAL(clicked()), this, SLOT(rejectSlot()));

	tabs->addTab(createViewTab(), tr("View"));
	tabs->addTab(createDefaultDeviceTab(), tr("Default device"));
	tabs->addTab(createAdvancedTab(), tr("Advanced"));

	vbox->addWidget(tabs);
	hbox->addWidget(apply, 1, Qt::AlignRight);
	hbox->addWidget(close, 0, Qt::AlignRight);
	vbox->addLayout(hbox);
	vbox->setContentsMargins(15, 15, 15, 15);
	this->setLayout(vbox);

	icon = qh_loadIcon("preferences-desktop-multimedia", NULL);
	if (icon.isNull())
		icon = QIcon(":/icons/preferences-desktop-multimedia.png");
	setWindowIcon(icon);
	setWindowTitle(tr("Preferences"));
}

void
Preferences::acceptSlot()
{
	for (int i = 0; i < DSBMIXER_MAX_CHANNELS; i++) {
		if (viewTabCb[i]->checkState() == Qt::Checked)
			chanMask |=  (1 << i);
		else
			chanMask &= ~(1 << i);
	}
	showTicks = showTicksCb->checkState() == Qt::Checked ? true : false;
	if (lrViewCb->checkState() == Qt::Checked)
		lrView = true;
	else
		lrView = false;
	for (int i = 0; i < defaultDeviceRb.count(); i++) {
		if (defaultDeviceRb.at(i)->isChecked())
			defaultUnit = i;
	}
	feederRateQuality = feederRateQualitySb->value();
	amplify = amplifySb->value();
	maxAutoVchans = maxAutoVchansSb->value();
	latency = latencySb->value();
	bypassMixer = bypassMixerCb->checkState() == Qt::Checked ? \
	    true : false;
	pollIval = pollIvalSb->value();
	if (testSoundPlaying)
		stopSound();
	this->accept();
}

void
Preferences::rejectSlot()
{
	if (testSoundPlaying)
		stopSound();
	this->reject();
}

QWidget *
Preferences::createViewTab()
{
	QWidget	    *widget = new QWidget(this);
	const char  **names = dsbmixer_getchanames();
	QVBoxLayout *vbox = new QVBoxLayout();
	QGridLayout *grid = new QGridLayout();
	
	for (int i = 0; i < DSBMIXER_MAX_CHANNELS; i++) {
		viewTabCb[i] = new QCheckBox(QString(names[i]), this);
		if (chanMask & (1 << i))
			viewTabCb[i]->setCheckState(Qt::Checked);
	}
	for (int row = 0; row < 5; row++) {
		for (int col = 0; col < 5; col++) {
			grid->addWidget(viewTabCb[row * 5 + col], row, col);
		}
	}
	QLabel *label = new QLabel(tr("Select mixer devices to be visible\n"));
	label->setStyleSheet("font-weight: bold;");
	vbox->addWidget(label, 0, Qt::AlignCenter);
	vbox->addLayout(grid);

	QFrame* frame = new QFrame();
	frame->setFrameShape(QFrame::HLine);
	vbox->addWidget(frame);

	lrViewCb = new QCheckBox(tr("Show left and right channel"), this);
	lrViewCb->setCheckState(lrView ? Qt::Checked : Qt::Unchecked);

	showTicksCb = new QCheckBox(tr("Show ticks"), this);
	showTicksCb->setCheckState(showTicks ? Qt::Checked : Qt::Unchecked);

	vbox->addWidget(lrViewCb);
	vbox->addWidget(showTicksCb);

	widget->setLayout(vbox);

	return (widget);
}

QWidget *
Preferences::createDefaultDeviceTab()
{
	QWidget	    *widget = new QWidget(this);
	QVBoxLayout *vbox   = new QVBoxLayout();
	QHBoxLayout *hbox   = new QHBoxLayout();
	QLabel	    *label  = new QLabel(tr("Select default sound device\n"));
	testBt		    = new QPushButton(tr("Test sound"));
	commandEdit	    = new QLineEdit;
	soundPlayer	    = new QProcess(this);

	testSoundPlaying = false;

	label->setStyleSheet("font-weight: bold;");
	vbox->addWidget(label, 0, Qt::AlignCenter);

	for (int i = 0; i < dsbmixer_getndevs(); i++) {
		dsbmixer_t *dev = dsbmixer_getmixer(i);
		QRadioButton *rb = new QRadioButton(QString(dev->cardname),
		    this);
		defaultDeviceRb.append(rb);
		if (i == defaultUnit)
			rb->setChecked(true);
		vbox->addWidget(rb);
	}
	commandEdit->setText(playCmd);
	commandEdit->setToolTip(tr("Enter a command which plays a sound"));

	hbox->addWidget(commandEdit);
	hbox->addWidget(testBt, 0, Qt::AlignRight);
	vbox->addLayout(hbox);

	widget->setLayout(vbox);
	connect(testBt, SIGNAL(clicked()), this, SLOT(toggleTestSound()));
	connect(commandEdit, SIGNAL(textEdited(const QString &)), this,
	    SLOT(commandChanged(const QString &)));
	return (widget);
}

void
Preferences::toggleTestSound()
{
	if (testSoundPlaying) {
		stopSound();
		return;
	}
	for (int i = 0; i < dsbmixer_getndevs(); i++) {
		QRadioButton *rb = defaultDeviceRb.at(i);
		if (rb->isChecked()) {
			playSound(i);
			return;
		}
	}
}

void
Preferences::playSound(int unit)
{
	if (testSoundPlaying)
		return;
	if (dsbmixer_set_default_unit(unit) == -1) {
		qh_warn(this, "Couldn't set default sound unit to %d", unit);
		return;
	}
	soundPlayer->start(playCmd);
	soundPlayer->closeReadChannel(QProcess::StandardOutput);
	soundPlayer->closeReadChannel(QProcess::StandardError);
	soundPlayer->waitForStarted(-1);

	if (soundPlayer->state() == QProcess::NotRunning) {
		qh_warnx(this, "Couldn't execute '%s': %s",
		    playCmd.toLocal8Bit().data(),
		    soundPlayer->errorString().toLocal8Bit().data());
		return;
        }
	connect(soundPlayer, SIGNAL(finished(int, QProcess::ExitStatus)),
	    this, SLOT(soundPlayerFinished(int, QProcess::ExitStatus)));
	testBt->setText(tr("Stop"));
	testSoundPlaying = true;
}

void
Preferences::stopSound()
{
	if (!testSoundPlaying)
		return;
	disconnect(soundPlayer,
	    SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
	soundPlayer->close();
	testSoundPlaying = false;
	testBt->setText(tr("Test sound"));
	if (dsbmixer_set_default_unit(defaultUnit) == -1) {
		qh_warn(this, "Couldn't reset default sound unit to %d",
		    defaultUnit);
	}
}

void
Preferences::soundPlayerFinished(int exitCode, QProcess::ExitStatus status)
{
	if (status == QProcess::CrashExit)
		qh_warnx(this, "Command crashed");
	else if (exitCode != 0)
		qh_warnx(this, "Command returned with exit code %d", exitCode);
	stopSound();
}

void
Preferences::commandChanged(const QString &text)
{
	playCmd = text;
}

QWidget *
Preferences::createAdvancedTab()
{
	QWidget	    *widget = new QWidget(this);
	QVBoxLayout *vbox   = new QVBoxLayout();
	QGridLayout *grid   = new QGridLayout();

	QLabel *label = new QLabel(tr("Advanced settings\n"));
	label->setStyleSheet("font-weight: bold;");
	vbox->addWidget(label, 1, Qt::AlignCenter);
	vbox->addStretch(1);

	amplifySb	    = new QSpinBox(this);
	feederRateQualitySb = new QSpinBox(this);
	maxAutoVchansSb	    = new QSpinBox(this);
	latencySb	    = new QSpinBox(this);
	pollIvalSb	    = new QSpinBox(this);
	bypassMixerCb	    = new QCheckBox(tr("Bypass mixer"));

	bypassMixerCb->setToolTip(tr(
		"Enable this to allow applications to use\n"   \
		"their own existing mixer logic to control\n" \
		"their own channel volume."));
	maxAutoVchansSb->setRange(0, 256);
	maxAutoVchansSb->setValue(maxAutoVchans);

	latencySb->setRange(0, 10);
	latencySb->setValue(latency);

	amplifySb->setRange(0, 100);
	amplifySb->setValue(amplify);
	amplifySb->setSuffix(" dB");

	feederRateQualitySb->setRange(1, 4);
	feederRateQualitySb->setValue(feederRateQuality);

	pollIvalSb->setRange(10, 10000);
	pollIvalSb->setValue(pollIval);
	pollIvalSb->setSuffix(" ms");

	bypassMixerCb->setCheckState(bypassMixer ? Qt::Checked : \
	    Qt::Unchecked);

	label = new QLabel(tr("Amplification:"));
	grid->addWidget(label, 0, 0);
	grid->addWidget(amplifySb, 0, 1);

	label = new QLabel(tr("Feeder rate quality:"));
	grid->addWidget(label, 1, 0);
	grid->addWidget(feederRateQualitySb, 1, 1);

	label = new QLabel(tr("Max. auto VCHANS:"));
	grid->addWidget(label, 2, 0);
	grid->addWidget(maxAutoVchansSb, 2, 1);

	label = new QLabel(tr("Latency (0 low, 10 high):"));
	grid->addWidget(label, 3, 0);
	grid->addWidget(latencySb, 3, 1);

	label = new QLabel(tr("Poll mixers every"));
	grid->addWidget(label, 4, 0);
	grid->addWidget(pollIvalSb, 4, 1);

	grid->addWidget(new QLabel(""), 5, 0);
	grid->addWidget(bypassMixerCb, 6, 0);

	vbox->addLayout(grid);
	vbox->addStretch(1);	
	widget->setLayout(vbox);

	return (widget);
}

