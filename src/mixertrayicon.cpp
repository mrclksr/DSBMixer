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

#include <QWidget>
#include <QScreen>

#include "mixertrayicon.h"

MixerTrayIcon::MixerTrayIcon(Mixer *mixer, const QIcon &icon,
    QObject *parent) : QSystemTrayIcon(icon, parent) {

	this->mixer = mixer;
}

void
MixerTrayIcon::setMixer(Mixer *mixer)
{
	this->mixer = mixer;
}

void
MixerTrayIcon::showSlider(int vol)
{
	int   sx, sy;
	QRect rect = geometry();

	if (slider == 0)
		initSlider(vol);
	QSize scrsize = slider->screen()->availableSize();
	slider->setVol(vol);
	slider_timer->start(1000);
	if (!slider->isVisible()) {
		sx = rect.x() - slider->width() / 2 + rect.width() / 2;
		if (rect.y() < scrsize.height() - rect.y())
			sy = rect.y() + rect.height();
		else
			sy = rect.y() - slider->size().height();
		slider->move(sx, sy);
		slider->show();
	}
}

void
MixerTrayIcon::initSlider(int vol)
{
	slider = new ChanSlider(QString("Vol"), DSBMIXER_MASTER, vol);
	slider->setWindowFlags(Qt::ToolTip);
	slider_timer = new QTimer(this);
	connect(slider_timer, SIGNAL(timeout()), this, SLOT(hideSlider()));
	slider->show();
	slider->hide();
}

void
MixerTrayIcon::hideSlider()
{
	if (slider == 0)
		return;
	slider->hide();
	slider_timer->stop();
}
