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
#include <QDialog>

#include "mixer.h"

class Preferences : public QDialog
{
	Q_OBJECT
public:
	Preferences(int chanMask, int amplify, int feederRateQuality,
		int defaultUnit, int maxAutoVchans, int latency,
		bool bypassMixer, bool lrView, bool showTicks,
		QWidget *parent = 0);
public slots:
	void acceptSlot();
	void rejectSlot();
public:
	int  chanMask;
	int  amplify;
	int  feederRateQuality;
	int  defaultUnit;
	int  maxAutoVchans;
	int  latency;
	bool bypassMixer;
	bool lrView;
	bool showTicks;
private:
	QWidget	  *createViewTab();
	QWidget	  *createDefaultDeviceTab();
	QWidget	  *createAdvancedTab();
private:
	QSpinBox  *amplifySb;
	QSpinBox  *feederRateQualitySb;
	QSpinBox  *maxAutoVchansSb;
	QSpinBox  *latencySb;
	QCheckBox *viewTabCb[DSBMIXER_MAX_CHANNELS];
	QCheckBox *lrViewCb;
	QCheckBox *showTicksCb;
	QCheckBox *bypassMixerCb;
	QList<QRadioButton *> defaultDeviceRb;
};
#endif // PREFERENCES_H
