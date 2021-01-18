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

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

ChanSlider::ChanSlider(const QString &name, int id, int lvol, int rvol,
    bool rec, bool muteable, bool lrview, bool tray, QWidget *parent)
	: QGroupBox(name, parent)
{
	this->tray	= tray;
	this->lrview	= lrview;
	this->mute	= false;
	this->muteable	= muteable;
	this->rec	= rec;
	this->id	= id;
	this->lvol	= lvol;
	this->rvol	= rvol;
	this->vol	= (lvol + rvol) >> 1;
	if (lrview)
		initLRView(parent);
	else
		initUView(parent);
}

void
ChanSlider::initLRView(QWidget *parent)
{
	volabell	     = new QLabel;
	volabelr	     = new QLabel;
	lslider		     = new QSlider(Qt::Vertical);
	rslider		     = new QSlider(Qt::Vertical);
	recCB		     = new QCheckBox;
	muteCB		     = new QCheckBox(tr("mute"));
	lockCB		     = new QCheckBox("🔒");
	QHBoxLayout *micHbox = new QHBoxLayout;
	QLabel 	    *micPic  = new QLabel;
	QWidget	    *recElem = new QWidget(this);
	QVBoxLayout *vboxl   = new QVBoxLayout(parent);
	QVBoxLayout *vboxr   = new QVBoxLayout(parent);
	QVBoxLayout *layout  = new QVBoxLayout(parent);
	QHBoxLayout *hbox    = new QHBoxLayout(parent);
	QIcon micIcon	     = qh_loadIcon("audio-input-microphone-high", NULL);

	volabell->setText(QString("100%"));
	volabelr->setText(QString("100%"));

	QSize sz = volabell->sizeHint();

	volabell->setMinimumWidth(sz.width());
	volabelr->setMinimumWidth(sz.width());
	volabell->setMaximumWidth(sz.width());
	volabelr->setMaximumWidth(sz.width());

	volabell->setText(QString("%1%").arg(lvol));
	volabelr->setText(QString("%1%").arg(rvol));

	if (!tray) {
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
	if (!tray) {
		layout->addWidget(lockCB, 0, Qt::AlignCenter);
		if (!muteable) {
			/* Padding space */
			QSizePolicy sp;
			sp.setRetainSizeWhenHidden(true);
			muteCB->setSizePolicy(sp);
			layout->addWidget(muteCB, 0, Qt::AlignHCenter);
			muteCB->hide();
		} else {
			layout->addWidget(muteCB, 0, Qt::AlignHCenter);
			connect(muteCB, SIGNAL(stateChanged(int)), this,
			    SLOT(emitMuteChanged(int)));
		}
	}
	setLayout(layout);
	connect(lslider, SIGNAL(valueChanged(int)), this,
	    SLOT(emitLVolumeChanged(int)));
	connect(rslider, SIGNAL(valueChanged(int)), this,
	    SLOT(emitRVolumeChanged(int)));
	connect(lockCB, SIGNAL(stateChanged(int)), this,
	    SLOT(emitLockChanged(int)));
}

void
ChanSlider::initUView(QWidget *parent)
{
	layout		     = new QVBoxLayout(parent);
	recCB		     = new QCheckBox;
	volabel		     = new QLabel;
	recCB		     = new QCheckBox;
	muteCB		     = new QCheckBox(tr("mute"));
	slider		     = new QSlider(Qt::Vertical);
	QLabel	    *micPic  = new QLabel;
	QWidget	    *recElem = new QWidget(this);
	QHBoxLayout *micHbox = new QHBoxLayout;
	QIcon       micIcon  = qh_loadIcon("audio-input-microphone-high", NULL);

	volabel->setText(QString("100%"));
	QSize sz = volabel->sizeHint();

	volabel->setMinimumWidth(sz.width());
	volabel->setMaximumWidth(sz.width());

	if (!tray) {
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
	}
	layout->addWidget(volabel, 0, Qt::AlignHCenter);

	slider->setMinimum(0);
	slider->setMaximum(100);
	slider->setTickPosition(QSlider::TicksLeft);
	setVol(lvol, rvol);

	layout->addWidget(slider, 0, Qt::AlignHCenter);
	if (!tray) {
		if (!muteable) {
			/* Padding space */
			QSizePolicy sp;
			sp.setRetainSizeWhenHidden(true);
			muteCB->setSizePolicy(sp);
			layout->addWidget(muteCB, 0, Qt::AlignHCenter);
			muteCB->hide();
		} else {
			layout->addWidget(muteCB, 0, Qt::AlignHCenter);
			connect(muteCB, SIGNAL(stateChanged(int)), this,
			    SLOT(emitMuteChanged(int)));
		}
	}
	setLayout(layout);

	connect(slider, SIGNAL(valueChanged(int)), this,
	    SLOT(emitVolumeChanged(int)));
}

void
ChanSlider::emitVolumeChanged(int val)
{

	if (muteCB != 0 && mute) {
		slider->setValue(val);
		return;
	}
	addVol(val - vol);
	emit volumeChanged(this->id, lvol, rvol);
}

void
ChanSlider::emitLVolumeChanged(int val)
{

	if (muteCB != 0 && mute) {
		lslider->setValue(lvol);
		return;
	}
	if (lrlocked) {
		addVol(val - lvol);
		emit volumeChanged(this->id, lvol, rvol);
	} else {
		lvol = val;
		emit lVolumeChanged(this->id, lvol);
	}
}

void
ChanSlider::emitRVolumeChanged(int val)
{

	if (muteCB != 0 && mute) {
		rslider->setValue(rvol);
		return;
	}
	if (lrlocked) {
		addVol(val - rvol);
		emit volumeChanged(this->id, lvol, rvol);
	} else {
		rvol = val;
		emit rVolumeChanged(this->id, rvol);
	}
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
ChanSlider::emitLockChanged(int state)
{
	if (state == Qt::Unchecked)
		lrlocked = false;
	else
		lrlocked = true;
	emit lockChanged(state);
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
	if (lrview) {
		lslider->blockSignals(true);
		rslider->blockSignals(true);
		lslider->setValue(lvol);
		rslider->setValue(rvol);
		sliderSetToolTip(lvol, rvol);
		volabell->setText(QString("%1%").arg(lvol));
		volabelr->setText(QString("%1%").arg(rvol));
		lslider->blockSignals(false);
		rslider->blockSignals(false);
	} else {
		vol = (lvol + rvol) >> 1;
		slider->blockSignals(true);
		slider->setValue(vol);
		sliderSetToolTip(lvol, rvol);
		slider->blockSignals(false);
		volabel->setText(QString("%1%").arg(vol));
	}
}

void
ChanSlider::setRecSrc(bool state)
{
	if (recCB) {
		recCB->blockSignals(true);
		recCB->setCheckState(state ? Qt::Checked : Qt::Unchecked);
		recCB->blockSignals(false);
	}
}

void
ChanSlider::setMute(bool state)
{
	if (muteCB)
		muteCB->setCheckState(state ? Qt::Checked : Qt::Unchecked);
}

void
ChanSlider::sliderSetToolTip(int lvol, int rvol)
{
	if (!lrview) {
		QString str = QString("%1%").arg(vol);
		slider->setToolTip(str);
		return;
	}
	QString lstr = QString("%1%").arg(lvol);
	QString rstr = QString("%1%").arg(rvol);

	lslider->setToolTip(lstr);
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

void
ChanSlider::addVol(int volinc)
{
	int minvol, maxvol;

	if (lrlocked) {
		if (volinc < 0) {
			minvol = MIN(rvol, lvol);
			if (minvol + volinc < 0)
				volinc = -minvol;
		} else {
			maxvol = MAX(rvol, lvol);
			if (maxvol + volinc > 100)
				volinc = 100 - maxvol;
		}
		rvol += volinc;
		lvol += volinc;
	} else {
		if (rvol + volinc < 0)
			rvol = 0;
		else if (rvol + volinc > 100)
			rvol = 100;
		else
			rvol += volinc;
		if (lvol + volinc < 0)
			lvol = 0;
		else if (lvol + volinc > 100)
			lvol = 100;
		else
			lvol += volinc;
	}
	vol = (lvol + rvol) >> 1;
}
