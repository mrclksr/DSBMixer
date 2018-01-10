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

#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGridLayout>
#include <QCloseEvent>
#include <QBoxLayout>

#include "preferences.h"
#include "qt-helper/qt-helper.h"

Preferences::Preferences(int chanMask, int amplify, int feederRateQuality,
	int defaultUnit, bool lrView, bool showTicks, QWidget *parent)
	: QDialog(parent) {
	this->chanMask = chanMask;
	this->amplify = amplify;
	this->feederRateQuality = feederRateQuality;
	this->defaultUnit = defaultUnit;
	this->lrView = lrView;
	this->showTicks = showTicks;

	qApp->setQuitOnLastWindowClosed(false);

	QVBoxLayout *vbox = new QVBoxLayout();
	QHBoxLayout *hbox = new QHBoxLayout();

	QTabWidget *tabs = new QTabWidget(this);

	QIcon icon = qh_loadStockIcon(QStyle::SP_DialogOkButton);
	QPushButton *apply = new QPushButton(icon, tr("&Ok"));
	connect(apply, SIGNAL(clicked()), this, SLOT(acceptSlot()));

	icon = qh_loadStockIcon(QStyle::SP_DialogCancelButton);
	QPushButton *close = new QPushButton(icon, tr("&Cancel"));
	connect(close, SIGNAL(clicked()), this, SLOT(rejectSlot()));

	tabs->addTab(createViewTab(), tr("View"));
	tabs->addTab(createDefaultDeviceTab(), tr("Default device"));
	tabs->addTab(createAdvancedTab(), tr("Advanced"));

	vbox->addWidget(tabs);
	hbox->addWidget(apply, 1, Qt::AlignRight);
	hbox->addWidget(close, 0, Qt::AlignRight);
	vbox->addLayout(hbox);
	vbox->setContentsMargins(15, 15, 15, 15);
	this->setLayout(vbox);

	icon = qh_loadIcon("preferences-desktop-multimedia", NULL);
	if (icon.isNull())
		icon = QIcon(":/icons/preferences-desktop-multimedia.png");
	setWindowIcon(icon);
	setWindowTitle(tr("Preferences"));
}

void
Preferences::acceptSlot()
{
	for (int i = 0; i < DSBMIXER_MAX_CHANNELS; i++) {
		if (viewTabCb[i]->checkState() == Qt::Checked)
			chanMask |=  (1 << i);
		else
			chanMask &= ~(1 << i);
	}
	showTicks = showTicksCb->checkState() == Qt::Checked ? : false;
	if (lrViewCb->checkState() == Qt::Checked)
		lrView = true;
	else
		lrView = false;
	for (int i = 0; i < defaultDeviceRb.count(); i++) {
		if (defaultDeviceRb.at(i)->isChecked())
			defaultUnit = i;
	}
	feederRateQuality = feederRateQualitySb->value();
	amplify = amplifySb->value();
	this->accept();
}

void
Preferences::rejectSlot()
{
	this->reject();
}

QWidget *
Preferences::createViewTab()
{
	QWidget	    *widget = new QWidget(this);
	const char  **names = dsbmixer_getchanames();
	QVBoxLayout *vbox = new QVBoxLayout();
	QGridLayout *grid = new QGridLayout();
	
	for (int i = 0; i < DSBMIXER_MAX_CHANNELS; i++) {
		viewTabCb[i] = new QCheckBox(QString(names[i]), this);
		if (chanMask & (1 << i))
			viewTabCb[i]->setCheckState(Qt::Checked);
	}
	for (int row = 0; row < 5; row++) {
		for (int col = 0; col < 5; col++) {
			grid->addWidget(viewTabCb[row * 5 + col], row, col);
		}
	}
	QLabel *label = new QLabel(tr("Select mixer devices to be visible\n"));
	label->setStyleSheet("font-weight: bold;");
	vbox->addWidget(label, 0, Qt::AlignCenter);
	vbox->addLayout(grid);

	QFrame* frame = new QFrame();
	frame->setFrameShape(QFrame::HLine);
	vbox->addWidget(frame);

	lrViewCb = new QCheckBox(tr("Show left and right channel"), this);
	lrViewCb->setCheckState(lrView ? Qt::Checked : Qt::Unchecked);

	showTicksCb = new QCheckBox(tr("Show ticks"), this);
	showTicksCb->setCheckState(showTicks ? Qt::Checked : Qt::Unchecked);

	vbox->addWidget(lrViewCb);
	vbox->addWidget(showTicksCb);

	widget->setLayout(vbox);

	return (widget);
}

QWidget *
Preferences::createDefaultDeviceTab()
{
	QWidget *widget = new QWidget();
	QVBoxLayout *vbox = new QVBoxLayout();
	QLabel *label = new QLabel(tr("Select default sound device\n"));
	label->setStyleSheet("font-weight: bold;");
	vbox->addWidget(label, 0, Qt::AlignCenter);

	for (int i = 0; i < dsbmixer_getndevs(); i++) {
		dsbmixer_t *dev = dsbmixer_getmixer(i);
		QRadioButton *rb = new QRadioButton(QString(dev->cardname),
		    this);
		defaultDeviceRb.append(rb);
		if (i == defaultUnit)
			rb->setChecked(true);
		vbox->addWidget(rb);
	}
	widget->setLayout(vbox);

	return (widget);
}

QWidget *
Preferences::createAdvancedTab()
{
	QWidget	    *widget = new QWidget(this);
	QVBoxLayout *vbox = new QVBoxLayout();
	QGridLayout *grid = new QGridLayout();

	QLabel *label = new QLabel(tr("Advanced settings\n"));
	label->setStyleSheet("font-weight: bold;");
	vbox->addWidget(label, 1, Qt::AlignCenter);
	vbox->addStretch(1);

	amplifySb	    = new QSpinBox(this);
	feederRateQualitySb = new QSpinBox(this);
	pVchansSb	    = new QSpinBox(this);
	rVchansSb	    = new QSpinBox(this);
	
	pVchansSb->setRange(0, 16);
	rVchansSb->setRange(0, 16);

	amplifySb->setRange(0, 100);
	amplifySb->setValue(amplify);

	feederRateQualitySb->setRange(1, 4);
	feederRateQualitySb->setValue(feederRateQuality);

	label = new QLabel(tr("Amplification:"));
	grid->addWidget(label, 0, 0);
	grid->addWidget(amplifySb, 0, 1);
	label = new QLabel("dB");
	grid->addWidget(label, 0, 2);
	
	label = new QLabel(tr("Feeder rate quality:"));
	grid->addWidget(label, 1, 0);
	grid->addWidget(feederRateQualitySb, 1, 1);

	QString s = QString(tr("Vchans for playback (pcm%1):")).arg(defaultUnit);
	label = new QLabel(s);
	grid->addWidget(label, 2, 0);
	grid->addWidget(pVchansSb, 2, 1);
	
	label = new QLabel(tr("Vchans for recording:"));
	grid->addWidget(label, 3, 0);
	grid->addWidget(rVchansSb, 3, 1);
	
	vbox->addLayout(grid);
	vbox->addStretch(1);	
	widget->setLayout(vbox);

	return (widget);
}

