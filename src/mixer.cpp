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

Mixer::Mixer(dsbmixer_t *mixer, int chanMask, bool lrview, QWidget *parent)
	: QWidget(parent)
{
	ChanSlider  *cs;
	QHBoxLayout *hbox = new QHBoxLayout();

	if (mixer == NULL)
		return;
	QStringList tokens = QString(mixer->cardname).split(QRegExp("[<>]+"),
	    Qt::SkipEmptyParts);
	if (tokens.size() > 1)
		this->cardname = tokens[0] + tokens[1];
	else
		this->cardname = QString(mixer->cardname);
	this->mixer  = mixer;
	this->lrview = lrview;
	muted = false;

	for (int i = 0; i < dsbmixer_getnchans(mixer); i++) {
		int chan = dsbmixer_getchanid(mixer, i);
		if (!((1 << chan) & chanMask))
			continue;
		int lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
		int rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
		const char *name = dsbmixer_getchaname(mixer, chan);
		cs = new ChanSlider(QString(name), chan, lvol, rvol,
				    dsbmixer_canrec(mixer, chan),
				    chan == DSBMIXER_MASTER ? true : false,
				    lrview, false);
		connect(cs, SIGNAL(lVolumeChanged(int, int)), this,
		    SLOT(setLVol(int, int)));
		connect(cs, SIGNAL(rVolumeChanged(int, int)), this,
		    SLOT(setRVol(int, int)));
		connect(cs, SIGNAL(volumeChanged(int, int, int)), this,
		    SLOT(setVol(int, int, int)));
		connect(cs, SIGNAL(recSourceChanged(int, int)), this,
		    SLOT(setRecSrc(int, int)));
		if (chan == DSBMIXER_MASTER) {
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

int
Mixer::find_idx(int chan)
{
	int idx;

	for (idx = 0; idx < channel.count(); idx++) {
		if (channel.at(idx)->id == chan)
			return (idx);
	}
	return (-1);
}

void
Mixer::setLVol(int chan, int lvol)
{
	int rvol, idx = find_idx(chan);
	if (idx < 0)
		return;
	dsbmixer_setlvol(this->mixer, chan, lvol);
	lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
	rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
	channel.at(idx)->setVol(lvol, rvol);
	if (chan == DSBMIXER_MASTER)
		emit masterVolChanged(this->mixer->unit, lvol, rvol);
}

void
Mixer::setRVol(int chan, int rvol)
{
	int lvol, idx = find_idx(chan);
	if (idx < 0)
		return;
	dsbmixer_setrvol(this->mixer, chan, rvol);
	rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
	lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
	channel.at(idx)->setVol(lvol, rvol);
	if (chan == DSBMIXER_MASTER)
		emit masterVolChanged(this->mixer->unit, lvol, rvol);
}

void
Mixer::setVol(int chan, int lvol, int rvol)
{
	int idx = find_idx(chan);
	if (idx < 0)
		return;
	dsbmixer_setvol(this->mixer, chan, DSBMIXER_CHAN_CONCAT(lvol, rvol));
	lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
	rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
	channel.at(idx)->setVol(lvol, rvol);
	if (chan == DSBMIXER_MASTER)
		emit masterVolChanged(this->mixer->unit, lvol, rvol);
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
	muted = state == Qt::Checked ? true : false;
	dsbmixer_setmute(mixer, muted);
	channel.at(DSBMIXER_MASTER)->setMute(muted);
	int lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, DSBMIXER_MASTER));
	int rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, DSBMIXER_MASTER));
	emit masterVolChanged(mixer->unit, lvol, rvol);
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
			if (uvol > 0)
				channel.at(i)->setMute(false);
			emit masterVolChanged(this->mixer->unit, lvol, rvol);
		}
		channel.at(i)->setVol(lvol, rvol);
		if (dsbmixer_canrec(mixer, chan))
			channel.at(i)->setRecSrc(dsbmixer_getrec(mixer, chan));
	}
}

void
Mixer::getMasterVol(int *lvol, int *rvol)
{
	int vol = dsbmixer_getvol(mixer, DSBMIXER_MASTER);

	*lvol = DSBMIXER_CHAN_LEFT(vol);
	*rvol = DSBMIXER_CHAN_RIGHT(vol);
}

void
Mixer::setTicks(bool on)
{
	for (int i = 0; i < channel.count(); i++)
		channel.at(i)->setTicks(on);
}

void
Mixer::changeMasterVol(int volinc)
{
	int chan = DSBMIXER_MASTER;
	int idx = find_idx(chan);
	if (idx < 0)
		return;
	channel.at(idx)->addVol(volinc);
	int lvol = channel.at(idx)->lvol;
	int rvol = channel.at(idx)->rvol;
	dsbmixer_setlvol(this->mixer, chan, lvol);
	dsbmixer_setrvol(this->mixer, chan, rvol);
	lvol = DSBMIXER_CHAN_LEFT(dsbmixer_getvol(mixer, chan));
	rvol = DSBMIXER_CHAN_RIGHT(dsbmixer_getvol(mixer, chan));
	channel.at(idx)->setVol(lvol, rvol);
	emit masterVolChanged(this->mixer->unit, lvol, rvol);
}
