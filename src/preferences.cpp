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
#include <paths.h>

#include "preferences.h"
#include "qt-helper/qt-helper.h"
#include "iconthemeselector.h"

Preferences::Preferences(int chanMask, int amplify, int feederRateQuality,
	int defaultUnit, int maxAutoVchans, int latency, bool bypassMixer,
	bool lrView, bool showTicks, int pollIval, const char *playCmd,
	const char *trayTheme, QWidget *parent) : QDialog(parent) {

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
	this->themeName = QString(trayTheme);

	qApp->setQuitOnLastWindowClosed(false);

	QVBoxLayout *vbox = new QVBoxLayout;
	QHBoxLayout *hbox = new QHBoxLayout;

	tabs = new QTabWidget(this);

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
	themeName = themeEdit->text();
	this->accept();
}

void
Preferences::rejectSlot()
{
	if (testSoundPlaying)
		stopSound();
	this->reject();
}

void
Preferences::selectTheme()
{
	IconThemeSelector selector(this);
	QString name = selector.getTheme();
	if (name != "") {
		themeEdit->setText(name);
		themeName = name;
	}
}

QWidget *
Preferences::createViewTab()
{
	QWidget	    *widget	= new QWidget(this);
	QGroupBox   *mdGrp	= new QGroupBox(tr("Select mixer devices to be visible\n"));
	QGroupBox   *slGrp	= new QGroupBox(tr("Slider settings"));
	QGroupBox   *thGrp	= new QGroupBox(tr("Tray icon theme"));
	const char  **names	= dsbmixer_getchanames();
	QVBoxLayout *vbox	= new QVBoxLayout;
	QVBoxLayout *slBox	= new QVBoxLayout;
	QGridLayout *grid	= new QGridLayout;
	themeEdit		= new QLineEdit(themeName);
	QLabel	    *themeLabel	= new QLabel(tr("Tray icon theme:"));
	QHBoxLayout *hbox	= new QHBoxLayout;
	QPushButton *openButton = new QPushButton(tr("Select theme"));

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
	mdGrp->setLayout(grid);
	vbox->addWidget(mdGrp);

	lrViewCb = new QCheckBox(tr("Show left and right channel"), this);
	lrViewCb->setCheckState(lrView ? Qt::Checked : Qt::Unchecked);

	showTicksCb = new QCheckBox(tr("Show ticks"), this);
	showTicksCb->setCheckState(showTicks ? Qt::Checked : Qt::Unchecked);

	slBox->addWidget(lrViewCb);
	slBox->addWidget(showTicksCb);

	slGrp->setLayout(slBox);
	vbox->addWidget(slGrp);

	hbox->addWidget(themeLabel);
	hbox->addWidget(themeEdit);
	hbox->addWidget(openButton);
	thGrp->setLayout(hbox);
	vbox->addWidget(thGrp);
	vbox->addStretch(1);
	widget->setLayout(vbox);
	connect(openButton, &QPushButton::clicked, this,
	    &Preferences::selectTheme);
	return (widget);
}

QWidget *
Preferences::createDefaultDeviceTab()
{
	QWidget	    *widget = new QWidget(this);
	QVBoxLayout *vbox   = new QVBoxLayout;
	QHBoxLayout *hbox   = new QHBoxLayout;
	QVBoxLayout *ddBox  = new QVBoxLayout;
	QGroupBox   *ddGrp  = new QGroupBox(tr("Select default sound device\n"));
	testBt		    = new QPushButton(tr("Test sound"));
	commandEdit	    = new QLineEdit;
	soundPlayer	    = new QProcess(this);

	testSoundPlaying = false;
	for (int i = 0; i < dsbmixer_getndevs(); i++) {
		dsbmixer_t *dev = dsbmixer_getmixer(i);
		QRadioButton *rb = new QRadioButton(QString(dev->cardname),
		    this);
		defaultDeviceRb.append(rb);
		if (i == defaultUnit)
			rb->setChecked(true);
		ddBox->addWidget(rb);
	}
	ddBox->addStretch(1);
	ddGrp->setLayout(ddBox);
	commandEdit->setText(playCmd);
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
	soundPlayer->start(_PATH_BSHELL, QStringList() << "-c" << playCmd);
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
	QVBoxLayout *vbox   = new QVBoxLayout;
	QGridLayout *grid   = new QGridLayout;
	QGroupBox   *grpBox = new QGroupBox(tr("Advanced settings"));

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
	maxAutoVchansSb->setToolTip(tr(
		"Defines the max. number of virtual playback\n" \
		"and recording channels that can be created.\n" \
		"Virtual channels allow programs to use more playback\n" \
		"and recording channels than the physical hardware\n" \
		"provides."));

	latencySb->setRange(0, 10);
	latencySb->setValue(latency);
	latencySb->setToolTip(tr(
		"Lower values mean less buffering and latency."));
	amplifySb->setRange(0, 100);
	amplifySb->setValue(amplify);
	amplifySb->setSuffix(" dB");
	amplifySb->setToolTip(tr(
		"Lower values mean more amplification, but can\n" \
		"produce sound clipping when chosen too low.\n" \
		"Higher values mean finer volume control."));
	feederRateQualitySb->setRange(1, 4);
	feederRateQualitySb->setValue(feederRateQuality);
	feederRateQualitySb->setToolTip(tr(
		"Higher values mean better sample rate conversion,\n" \
		"but more memory and CPU usage."));

	pollIvalSb->setRange(10, 10000);
	pollIvalSb->setValue(pollIval);
	pollIvalSb->setSuffix(" ms");
	pollIvalSb->setToolTip(tr(
		"Defines the time interval in milliseconds mixer\n" \
		"devices are polled. Higher values mean less CPU usage.\n" \
		"Lower values mean less latency when showing the\n" \
		"current volume changed by other programs."));
	bypassMixerCb->setCheckState(bypassMixer ? Qt::Checked : \
	    Qt::Unchecked);

	QLabel *label = new QLabel(tr("Amplification:"));
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

	label = new QLabel(tr("Poll mixers every"));
	grid->addWidget(label, 4, 0);
	grid->addWidget(pollIvalSb, 4, 1);

	grid->addWidget(new QLabel(""), 5, 0);
	grid->addWidget(bypassMixerCb, 6, 0);

	grpBox->setLayout(grid);
	vbox->addWidget(grpBox);
	vbox->addStretch(1);	
	widget->setLayout(vbox);

	return (widget);
}

void
Preferences::keyPressEvent(QKeyEvent *e)
{
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
		int tabno = e->text().toInt();
		if (e->modifiers() == Qt::AltModifier) {
			if (tabno == 0)
				tabno = tabs->count();
			tabs->setCurrentIndex(tabno - 1);
		}
	}
}
