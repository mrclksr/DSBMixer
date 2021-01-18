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
#include <QLabel>

class ChanSlider : public QGroupBox {
	Q_OBJECT
public:
	ChanSlider(const QString &name, int id, int lvol, int rvol, bool rec,
	    bool muteable, bool lrview, bool tray, QWidget *parent = 0);
	void setVol(int lvol, int rvol);
	void setLVol(int lvol);
	void setRVol(int rvol);
	void setRecSrc(bool state);
	void sliderSetToolTip(int lvol, int rvol);
	void setMute(bool mute);
	void setTicks(bool on);
	void addVol(int volinc);
private:
	void initLRView(QWidget *parent);
	void initUView(QWidget *parent);
signals:
	void VolumeChanged(int id, int val);
	void recSourceChanged(int id, int state);
	void lVolumeChanged(int id, int vol);
	void rVolumeChanged(int id, int vol);
	void volumeChanged(int id, int lvol, int rvol);
	void muteChanged(int state);
	void lockChanged(int state);
private slots:
	void emitVolumeChanged(int);
	void emitRecSourceChanged(int);
	void emitLVolumeChanged(int lvol);
	void emitRVolumeChanged(int rvol);
	void emitMuteChanged(int state);
	void emitLockChanged(int state);
public:
	int	    id;
	int	    vol;
	int	    rvol;
	int	    lvol;
private:
	bool	    lrview, mute, muteable, tray, rec;
	bool	    lrlocked = false;
	QLabel	    *volabel;
	QLabel	    *volabell;
	QLabel	    *volabelr;
	QSlider	    *slider, *rslider, *lslider;;
	QCheckBox   *recCB, *muteCB, *lockCB;
	QVBoxLayout *layout;
};
#endif
