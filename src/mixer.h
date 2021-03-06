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

#ifndef MIXER_H
#define MIXER_H

#include <QWidget>
#include <QList>

#include "chanslider.h"
#include "libdsbmixer.h"

class Mixer : public QWidget
{
	Q_OBJECT
public:
	Mixer(dsbmixer_t *mixer, int chanMask, bool lrview, QWidget *parent = 0);
	dsbmixer_t *getDev() const;
signals:
	void masterVolChanged(int unit, int lvol, int rvol);
public slots:
	void setLVol(int chan, int lvol);
	void setRVol(int chan, int rvol);
	void setVol(int chan, int lvol, int rvol);
	void setRecSrc(int chan, int state);
	void setMute(int state);
	void changeMasterVol(int volinc);
	void setTicks(bool on);
	void update();
public:
	void getMasterVol(int *lvol, int *rvol);
public:
	bool muted;
	bool lrview;
	QString cardname;
private:
	int find_idx(int chan);

	dsbmixer_t *mixer;
	QList<ChanSlider *> channel;
};
#endif // MIXER_H
