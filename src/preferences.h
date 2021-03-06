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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QRadioButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QList>
#include <QTabWidget>
#include <QDialog>
#include <QProcess>
#include <QLineEdit>
#include <QComboBox>

#include "mixer.h"
#include "settings.h"

class Preferences : public QDialog
{
	Q_OBJECT
public:
	Preferences(Settings& oldSettings, QWidget *parent = 0);
public slots:
	void acceptSlot();
	void rejectSlot();
	void toggleTestSound();
	void playSound(int unit);
	void stopSound();
	void commandChanged(const QString &);
	void soundPlayerFinished(int, QProcess::ExitStatus);
private slots:
	void changeTheme(int idx);
protected:
	void keyPressEvent(QKeyEvent *event);
public:
	Settings    settings;
private:
	QWidget	    *createViewTab();
	QWidget	    *createDefaultDeviceTab();
	QWidget	    *createAdvancedTab();
	void	    createThemeComboBox();
private:
	bool	    testSoundPlaying;
	QProcess    *soundPlayer;
	QSpinBox    *amplifySb;
	QSpinBox    *feederRateQualitySb;
	QSpinBox    *maxAutoVchansSb;
	QSpinBox    *latencySb;
	QSpinBox    *pollIvalSb;
	QLineEdit   *commandEdit;
	QCheckBox   *viewTabCb[DSBMIXER_MAX_CHANNELS];
	QCheckBox   *lrViewCb;
	QCheckBox   *showTicksCb;
	QCheckBox   *bypassMixerCb;
	QComboBox   *themeBox;
	QTabWidget  *tabs;
	QPushButton *testBt;
	QList<QRadioButton *> defaultDeviceRb;
};
#endif // PREFERENCES_H
