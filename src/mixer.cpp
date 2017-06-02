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

#include <QWidget>
#include "mixer.h"
#include <iostream>

Mixer::Mixer(dsbmixer_t *mixer, int chanMask, bool lrview, QWidget *parent)
	: QWidget(parent)
{
	ChanSlider  *cs;
	QHBoxLayout *hbox = new QHBoxLayout();

	if (mixer == NULL)
		return;
	this->mixer  = mixer;
	this->lrview = lrview;
	muted = false;

	for (int i = 0; i < dsbmixer_getnchans(mixer); i++) {
		int chan = dsbmixer_getchanid(mixer, i);
		if (!((1 << chan) & chanMask))
			continue;
		int lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
		int rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
		int uvol = (lvol + rvol) >> 1;
		const char *name = dsbmixer_getchaname(mixer, chan);

		if (lrview) {
			cs = new ChanSlider(QString(name), chan, lvol, rvol,
			    dsbmixer_canrec(mixer, chan),
			    chan == DSBMIXER_MASTER ? true : false);
			connect(cs, SIGNAL(lVolumeChanged(int, int)), this,
			    SLOT(setLVol(int, int)));
			connect(cs, SIGNAL(rVolumeChanged(int, int)), this,
			    SLOT(setRVol(int, int)));
		} else {
			cs = new ChanSlider(QString(name), chan, uvol,
			    dsbmixer_canrec(mixer, chan),
			    chan == DSBMIXER_MASTER ? true : false);
			connect(cs, SIGNAL(VolumeChanged(int, int)), this,
			    SLOT(setVol(int, int)));
		}
		connect(cs, SIGNAL(recSourceChanged(int, int)), this,
		    SLOT(setRecSrc(int, int)));
		if (chan == DSBMIXER_MASTER) {
			muted = dsbmixer_getmute(mixer);
			cs->setMute(muted);
			connect(cs, SIGNAL(muteChanged(int)), this,
			    SLOT(setMute(int)));
		}
		cs->setRecSrc(dsbmixer_getrec(mixer, chan));
		channel.append(cs);
		hbox->addWidget(cs, 0, Qt::AlignLeft);
	}
	hbox->addStretch(1);
	setLayout(hbox);
}

dsbmixer_t *
Mixer::getDev() const
{
	return (mixer);
}

void
Mixer::setVol(int chan, int vol)
{
	dsbmixer_setvol(this->mixer, chan, DSBMIXER_CHAN_CONCAT(vol, vol));

	if (chan == DSBMIXER_MASTER)
		emit masterVolChanged(vol);
}

void
Mixer::setLVol(int chan, int lvol)
{
	dsbmixer_setlvol(this->mixer, chan, lvol);

	if (chan == DSBMIXER_MASTER) {
		int lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
		int rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
		emit masterVolChanged((lvol + rvol) >> 1);
	}
}

void
Mixer::setRVol(int chan, int rvol)
{
	dsbmixer_setrvol(this->mixer, chan, rvol);
	if (chan == DSBMIXER_MASTER) {
		int lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
		int rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
		emit masterVolChanged((lvol + rvol) >> 1);
	}
}

void
Mixer::setRecSrc(int chan, int state)
{
	dsbmixer_setrec(mixer, chan, state == Qt::Checked ? true : false);

	for (int i = 0; i < channel.count(); i++) {
		channel.at(i)->setRecSrc(dsbmixer_getrec(mixer,
		    channel.at(i)->id));
	}
}

void
Mixer::setMute(int state)
{
	dsbmixer_setmute(mixer, state == Qt::Checked ? true : false);
	muted = dsbmixer_getmute(mixer);
	update();
}

void
Mixer::update()
{
	for (int i = 0; i < channel.count(); i++) {
		int chan = channel.at(i)->id;
		int lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
		int rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
		int uvol = (lvol + rvol) >> 1;
		if (chan == DSBMIXER_MASTER) {
			if (uvol > 0) {
				channel.at(i)->setMute(false);
				emit masterVolChanged(uvol);
			} else {
				channel.at(i)->setMute(true);
				emit masterVolChanged(0);
			}
		}
		if (lrview)
			channel.at(i)->setVol(lvol, rvol);
		else
			channel.at(i)->setVol(uvol);
		if (dsbmixer_canrec(mixer, chan))
			channel.at(i)->setRecSrc(dsbmixer_getrec(mixer, chan));
	}
}

