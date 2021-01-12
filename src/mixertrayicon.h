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
#include <QDebug>
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
		if (ev->type() == QEvent::Wheel) {
			QWheelEvent *we = static_cast<QWheelEvent *>(ev);
			if (mixer == 0)
				return (true);
			int vol = mixer->getMasterVol();
			if (we->angleDelta().y() < 0) {
				if ((vol -= 3) < 0)
					vol = 0;
			} else if ((vol += 3) > 100)
				vol = 100;
			showSlider(vol);
			if (mixer->muted)
				return (true);
			mixer->setVol(DSBMIXER_MASTER, vol);
			return (true);
		}
		return (QSystemTrayIcon::event(ev));
	}

private slots:
	void initSlider(int vol);
	void showSlider(int vol);
	void hideSlider(void);

private:
	Mixer *mixer;
	ChanSlider *slider = 0;
	QTimer *slider_timer = 0;
};
