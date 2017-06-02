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

#ifndef CHANSLIDER_H
#define CHANSLIDER_H
#include <QGroupBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>
#include <QSlider>

class ChanSlider : public QGroupBox {
	Q_OBJECT
public:
	ChanSlider(const QString &name, int id, int vol, bool rec,
	    bool muteable, QWidget *parent = 0);
	ChanSlider(const QString &name, int id, int lvol, int rvol, bool rec,
	    bool muteable, QWidget *parent = 0);
	void setVol(int vol);
	void setVol(int lvol, int rvol);
	void setRecSrc(bool state);
	void sliderSetToolTip(int vol);
	void sliderSetToolTip(int lvol, int rvol);
	void setMute(bool mute);
signals:
	void VolumeChanged(int id, int val);
	void recSourceChanged(int id, int state);
	void lVolumeChanged(int id, int vol);
	void rVolumeChanged(int id, int vol);
	void muteChanged(int state);
private slots:
	void emitVolumeChanged(int);
	void emitRecSourceChanged(int);
	void emitLVolumeChanged(int lvol);
	void emitRVolumeChanged(int rvol);
	void emitMuteChanged(int state);
public:
	int id;
private:
	int vol;
	int rvol;
	int lvol;
	bool lrview, mute;
	QCheckBox *recCB, *muteCB;
	QSlider *slider, *rslider, *lslider;;
	QVBoxLayout *layout;
};
#endif

