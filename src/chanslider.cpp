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

#include "chanslider.h"
#include <QGroupBox>
#include <QLabel>
#include <QString>
#include "qt-helper/qt-helper.h"

ChanSlider::ChanSlider(const QString &name, int id, int vol, bool rec,
    bool muteable, QWidget *parent)
	: QGroupBox(name, parent)
{
	mute	     = false;
	this->id     = id;
	this->vol    = vol;
	this->lrview = false;
	layout		     = new QVBoxLayout(parent);
	recCB		     = new QCheckBox;
	volabel		     = new QLabel;
	recCB		     = new QCheckBox;
	muteCB		     = new QCheckBox;
	slider		     = new QSlider(Qt::Vertical);
	QLabel	    *micPic  = new QLabel;
	QWidget	    *recElem = new QWidget(this);
	QHBoxLayout *micHbox = new QHBoxLayout;
	QIcon       micIcon  = qh_loadIcon("audio-input-microphone-high", NULL);

	micPic->setPixmap(micIcon.pixmap(16));
	micHbox->addWidget(micPic);
	micHbox->addWidget(recCB);
	recElem->setLayout(micHbox);

	if (!rec) {
		/* Padding space */
		QSizePolicy sp;
		sp.setRetainSizeWhenHidden(true);
		recElem->setSizePolicy(sp);
		recElem->hide();
		layout->addWidget(recElem, 0, Qt::AlignHCenter);
	} else {
		recCB->setToolTip(tr("Set/unset recording source"));
		layout->addWidget(recElem, 0, Qt::AlignHCenter);
		connect(recCB, SIGNAL(stateChanged(int)), this,
		    SLOT(emitRecSourceChanged(int)));
	}
	volabel->setText(QString("100%"));
	layout->addWidget(volabel, 0, Qt::AlignHCenter);

	slider->setMinimum(0);
	slider->setMaximum(100);
	slider->setTickPosition(QSlider::TicksLeft);
	setVol(vol);

	layout->addWidget(slider, 0, Qt::AlignHCenter);

	if (!muteable) {
		/* Padding space */
		QSizePolicy sp;
		sp.setRetainSizeWhenHidden(true);
		muteCB->setSizePolicy(sp);
		layout->addWidget(muteCB, 0, Qt::AlignHCenter);
		muteCB->hide();
	} else {
		muteCB->setToolTip(tr("Mute"));
		layout->addWidget(muteCB, 0, Qt::AlignHCenter);
		connect(muteCB, SIGNAL(stateChanged(int)), this,
		    SLOT(emitMuteChanged(int)));
	}
	setLayout(layout);

	connect(slider, SIGNAL(valueChanged(int)), this,
	    SLOT(emitVolumeChanged(int)));
}

ChanSlider::ChanSlider(const QString &name, int id, int lvol, int rvol,
    bool rec, bool muteable, QWidget *parent)
	: QGroupBox(name, parent)
{
	mute	   = false;
	lrview	   = true;
	this->id   = id;
	this->lvol = lvol;
	this->rvol = rvol;
	volabell	     = new QLabel;
	volabelr	     = new QLabel;
	lslider		     = new QSlider(Qt::Vertical);
	rslider		     = new QSlider(Qt::Vertical);
	recCB		     = new QCheckBox;
	QHBoxLayout *micHbox = new QHBoxLayout;
	QLabel 	    *micPic  = new QLabel;
	QWidget	    *recElem = new QWidget(this);
	QVBoxLayout *vboxl   = new QVBoxLayout(parent);
	QVBoxLayout *vboxr   = new QVBoxLayout(parent);
	QVBoxLayout *layout  = new QVBoxLayout(parent);
	QHBoxLayout *hbox    = new QHBoxLayout(parent);
	QIcon micIcon	     = qh_loadIcon("audio-input-microphone-high", NULL);

	micPic->setPixmap(micIcon.pixmap(16));
	micHbox->addWidget(micPic);
	micHbox->addWidget(recCB);
	recElem->setLayout(micHbox);

	if (!rec) {
		/* Padding space */
		QSizePolicy sp;
		sp.setRetainSizeWhenHidden(true);
		recElem->setSizePolicy(sp);
		recElem->hide();
		layout->addWidget(recElem, 0, Qt::AlignHCenter);
	} else {
		recCB->setToolTip(tr("Set/unset recording source"));
		layout->addWidget(recElem, 0, Qt::AlignHCenter);
		connect(recCB, SIGNAL(stateChanged(int)), this,
		    SLOT(emitRecSourceChanged(int)));
	}
	lslider->setMinimum(0);
	lslider->setMaximum(100);
	lslider->setValue(lvol);
	lslider->setTickPosition(QSlider::TicksLeft);
	vboxl->addWidget(volabell);
	vboxl->addWidget(lslider);

	rslider->setMinimum(0);
	rslider->setMaximum(100);
	rslider->setTickPosition(QSlider::TicksRight);
	vboxr->addWidget(volabelr, 0, Qt::AlignHCenter);
	vboxr->addWidget(rslider, 0, Qt::AlignHCenter);

	setVol(lvol, rvol);
	hbox->addLayout(vboxl);
	hbox->addLayout(vboxr);

	layout->addLayout(hbox, 0);

	muteCB = new QCheckBox;
	if (!muteable) {
		/* Padding space */
		QSizePolicy sp;
		sp.setRetainSizeWhenHidden(true);
		muteCB->setSizePolicy(sp);
		layout->addWidget(muteCB, 0, Qt::AlignHCenter);
		muteCB->hide();
	} else {
		muteCB->setToolTip(tr("Mute"));
		layout->addWidget(muteCB, 0, Qt::AlignHCenter);
		connect(muteCB, SIGNAL(stateChanged(int)), this,
		    SLOT(emitMuteChanged(int)));
	}
	volabell->setText(QString("%1%").arg(lvol));
	volabelr->setText(QString("%1%").arg(rvol));

	setLayout(layout);

	connect(lslider, SIGNAL(valueChanged(int)), this,
	    SLOT(emitLVolumeChanged(int)));
	connect(rslider, SIGNAL(valueChanged(int)), this,
	    SLOT(emitRVolumeChanged(int)));
}


ChanSlider::ChanSlider(const QString &name, int id, int vol, QWidget *parent)
	: QGroupBox(name, parent)
{
	mute	     = false;
	this->id     = id;
	this->vol    = vol;
	this->lrview = false;
	layout	     = new QVBoxLayout(parent);
	volabel	     = new QLabel;

	volabel->setText(QString("%1%").arg(vol));
	slider = new QSlider(Qt::Vertical);
	slider->setMinimum(0);
	slider->setMaximum(100);
	slider->setTickPosition(QSlider::TicksLeft);
	setVol(vol);
	layout->addWidget(volabel, 0, Qt::AlignHCenter);
	layout->addWidget(slider, 1, Qt::AlignHCenter);
	setLayout(layout);
	connect(slider, SIGNAL(valueChanged(int)), this,
	    SLOT(emitVolumeChanged(int)));
}

void
ChanSlider::emitVolumeChanged(int val)
{
	if (muteCB != 0 && mute) {
		slider->setValue(vol);
		return;
	}
	this->vol = val;
	sliderSetToolTip(val);
	emit VolumeChanged(this->id, val);
}

void
ChanSlider::emitLVolumeChanged(int vol)
{
	if (muteCB != 0 && mute) {
		lslider->setValue(lvol);
		return;
	}
	this->lvol = vol;
	sliderSetToolTip(this->lvol, this->rvol);
	volabell->setText(QString("%1%").arg(this->lvol));
	emit lVolumeChanged(this->id, vol);
}

void
ChanSlider::emitRVolumeChanged(int vol)
{
	if (muteCB != 0 && mute) {
		rslider->setValue(rvol);
		return;
	}
	this->rvol = vol;
	sliderSetToolTip(this->lvol, this->rvol);
	volabelr->setText(QString("%1%").arg(this->rvol));
	emit rVolumeChanged(this->id, vol);
}

void
ChanSlider::emitRecSourceChanged(int state)
{
	emit recSourceChanged(this->id, state);
}

void
ChanSlider::emitMuteChanged(int state)
{
	if (state == Qt::Unchecked)
		mute = false;
	else
		mute = true;
	if (lrview) {
		lslider->setEnabled(!mute);
		rslider->setEnabled(!mute);
	} else
		slider->setEnabled(!mute);
	emit muteChanged(state);
}

void
ChanSlider::setVol(int vol)
{
	if (mute)
		return;
	if (vol > 100 || vol < 0)
		return;
	slider->setValue(vol);
	sliderSetToolTip(vol);
	volabel->setText(QString("%1%").arg(vol));
}

void
ChanSlider::setVol(int lvol, int rvol)
{
	if (mute)
		return;
	if (lvol > 100 || lvol < 0)
		return;
	if (rvol > 100 || rvol < 0)
		return;
	lslider->setValue(lvol);
	rslider->setValue(rvol);
	sliderSetToolTip(lvol, rvol);
}

void
ChanSlider::setRecSrc(bool state)
{
	if (recCB)
		recCB->setCheckState(state ? Qt::Checked : Qt::Unchecked);
}

void
ChanSlider::setMute(bool state)
{
	if (muteCB)
		muteCB->setCheckState(state ? Qt::Checked : Qt::Unchecked);
}

void
ChanSlider::sliderSetToolTip(int vol)
{
	QString str = QString("%1%").arg(vol);
	slider->setToolTip(str);
}

void
ChanSlider::sliderSetToolTip(int lvol, int rvol)
{
	QString lstr = QString("%1%").arg(lvol);
	lslider->setToolTip(lstr);

	QString rstr = QString("%1%").arg(rvol);
	rslider->setToolTip(rstr);
}

void
ChanSlider::setTicks(bool on)
{
	if (lrview) {
		lslider->setTickPosition(on ? QSlider::TicksLeft  : \
		    QSlider::NoTicks);
		rslider->setTickPosition(on ? QSlider::TicksRight : \
		    QSlider::NoTicks);
		return;
	}
	slider->setTickPosition(on ? QSlider::TicksLeft : QSlider::NoTicks);
}

