/*-
 * Copyright (c) 2017 Marcel Kaiser. All rights reserved.
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

#include <QObject>
#include <QEvent>
#include <QSystemTrayIcon>
#include <QWheelEvent>
#include <QWidget>
#include <QTimer>

#include "mixer.h"
#include "chanslider.h"

class MixerTrayIcon : public QSystemTrayIcon
{
	Q_OBJECT

public:
	MixerTrayIcon(Mixer *mixer, const QIcon &icon, QObject *parent = 0);
	
	void setMixer(Mixer *mixer);

	bool event(QEvent* ev)
	{
		int volinc, lvol, rvol;

		if (ev->type() != QEvent::Wheel)
			return (QSystemTrayIcon::event(ev));
		if (mixer == 0)
			return (true);
		if (mixer->muted)
			return (true);
		QWheelEvent *we = static_cast<QWheelEvent *>(ev);
		if (we->angleDelta().y() < 0)
			volinc = - 3;
		else
			volinc = 3;
		mixer->changeMasterVol(volinc);
		mixer->getMasterVol(&lvol, &rvol);
		showSlider(lvol, rvol);

		return (true);
	}

private slots:
	void initSlider(int lvol, int rvol);
	void showSlider(int lvol, int rvol);
	void hideSlider(void);

private:
	Mixer *mixer;
	ChanSlider *slider = 0;
	QTimer *slider_timer = 0;
};
