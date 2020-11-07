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

#include <QTabWidget>
#include <QTimer>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTextCodec>
#include <unistd.h>
#include <stdlib.h>
#include <QRect>
#include <QScreen>

#include "mainwin.h"
#include "thread.h"
#include "mixer.h"
#include "preferences.h"
#include "qt-helper/qt-helper.h"

MainWin::MainWin(dsbcfg_t *cfg, QWidget *parent)
	: QMainWindow(parent) {
	timer	  = new QTimer(this);
	traytimer = new QTimer(this);
	tabs 	  = new QTabWidget(this);

	this->cfg = cfg;
	posX	  = &dsbcfg_getval(cfg, CFG_POS_X).integer;
	posY	  = &dsbcfg_getval(cfg, CFG_POS_Y).integer;
	wWidth	  = &dsbcfg_getval(cfg, CFG_WIDTH).integer;
	hHeight	  = &dsbcfg_getval(cfg, CFG_HEIGHT).integer;
	lrView    = &dsbcfg_getval(cfg, CFG_LRVIEW).boolean;
	showTicks = &dsbcfg_getval(cfg, CFG_TICKS).boolean;
	chanMask  = &dsbcfg_getval(cfg, CFG_MASK).integer;
	pollIval  = &dsbcfg_getval(cfg, CFG_POLL_IVAL).integer;
	playCmd   = &dsbcfg_getval(cfg, CFG_PLAY_CMD).string;
	trayAvailable = false;

	loadIcons();
	createMixerList();
	createTabs();
#ifndef WITHOUT_DEVD
	Thread *thread = new Thread();
	connect(thread, SIGNAL(sendNewMixer(dsbmixer_t*)), this,
	    SLOT(addNewMixer()));

	connect(thread, SIGNAL(sendRemoveMixer(dsbmixer_t*)), this,
	    SLOT(removeMixer(dsbmixer_t*)));
	thread->start();
#endif
	connect(timer, SIGNAL(timeout()), this, SLOT(updateMixers()));
	timer->start(*pollIval);

	connect(tabs, SIGNAL(currentChanged(int)), this,
	    SLOT(catchCurrentChanged()));

	connect(traytimer, SIGNAL(timeout()), this, SLOT(checkForSysTray()));
	traytimer->start(500);

	createMenuActions();
	createMainMenu();
	setWindowIcon(hVolIcon);
	setWindowTitle("DSBMixer");
	resize(*wWidth, *hHeight);	
	move(*posX, *posY);
	statusBar()->setSizeGripEnabled(true);

	connect(QGuiApplication::primaryScreen(),
	    SIGNAL(geometryChanged(const QRect &)), this,
	    SLOT(scrGeomChanged(const QRect &)));
}

void
MainWin::scrGeomChanged(const QRect &g)
{
	Q_UNUSED(g);
	traytimer->start(500);
}

void
MainWin::loadIcons()
{
	muteIcon  = qh_loadIcon("audio-volume-muted-panel",
				"audio-volume-muted", NULL);
	lVolIcon  = qh_loadIcon("audio-volume-low-panel",
				"audio-volume-low", NULL);
	mVolIcon  = qh_loadIcon("audio-volume-medium-panel",
				"audio-volume-medium", NULL);
	hVolIcon  = qh_loadIcon("audio-volume-high-panel",
				"audio-volume-high", NULL);
	quitIcon  = qh_loadIcon("application-exit", NULL); 
	prefsIcon = qh_loadIcon("preferences-desktop-multimedia", NULL);

	if (quitIcon.isNull())
		quitIcon = qh_loadStockIcon(QStyle::SP_DialogCloseButton);
	if (prefsIcon.isNull())
		prefsIcon = QIcon(":/icons/preferences-desktop-multimedia.png");
	if (muteIcon.isNull())
		muteIcon = QIcon(":/icons/audio-volume-muted.png");
	if (lVolIcon.isNull())
		lVolIcon = QIcon(":/icons/audio-volume-low.png");
	if (mVolIcon.isNull())
		mVolIcon = QIcon(":/icons/audio-volume-medium.png");
	if (hVolIcon.isNull())
		hVolIcon = QIcon(":/icons/audio-volume-high.png");
}

void
MainWin::createMixerList()
{
	for (int i = 0; i < dsbmixer_getndevs(); i++) {
		dsbmixer_t *dev = dsbmixer_getmixer(i);
		Mixer *mixer = new Mixer(dev, *chanMask, *lrView, this);
		mixers.append(mixer);
		mixer->setTicks(*showTicks);
		connect(mixer, SIGNAL(masterVolChanged(int)), this,
		    SLOT(catchMasterVolChanged(int)));
	}
}

void
MainWin::createTabs()
{
	int didx = mixerUnitToTabIndex(dsbmixer_default_unit());

	for (int i = 0; i < mixers.count(); i++) {
		dsbmixer_t *dev = mixers.at(i)->getDev();
		QString label(dev->cardname);

		if (i == didx)
			label.append("*");
		tabs->addTab(mixers.at(i), label);
		tabs->setTabToolTip(i, QString(dev->name));
	}
	setCentralWidget(tabs);
	tabs->setCurrentIndex(didx == -1 ? 0: didx);
}

void
MainWin::setDefaultTab(int unit)
{
	int idx = mixerUnitToTabIndex(unit);
	if (idx == -1)
		return;
	tabs->setCurrentIndex(idx);
	/* Update tab labels. */
	for (int i = 0; i < mixers.count(); i++) {
		QString label(mixers.at(i)->getDev()->cardname);
		if (i == idx)
			label.append("*");
		tabs->setTabText(i, label);
	}
}

void
MainWin::redrawMixers()
{
	for (int i = tabs->count() - 1; i >= 0; i--)
		tabs->removeTab(0);
	for (int i = mixers.count() - 1; i >= 0; i--) {
		delete mixers.at(0);
		mixers.removeAt(0);
	}
	createMixerList();
	createTabs();
	saveGeometry();
}

#ifndef WITHOUT_DEVD
void
MainWin::addNewMixer()
{
	redrawMixers();
}

void
MainWin::removeMixer(dsbmixer_t *mixer)
{
	for (int i = 0; i < mixers.count(); i++) {
		if (mixers.at(i)->getDev() == mixer) {
			tabs->removeTab(i);
			delete mixers.at(i);
			mixers.removeAt(i);
			dsbmixer_delmixer(mixer);
			return;
		}
  	}	
}
#endif

void
MainWin::updateMixers()
{
	int	   unit;
	static int prev_unit = 0;
	static int cnt = 0;
	dsbmixer_t *mixer;

	if (cnt++ % 5 == 0) {
		unit = dsbmixer_poll_default_unit();
		if (prev_unit != unit) {
			setDefaultTab(unit);
			prev_unit = unit;
		}
	}
	mixer = dsbmixer_pollmixers();
	if (mixer == NULL)
		return;
	for (int i = 0; i < mixers.count(); i++) {
		if (mixer == mixers.at(i)->getDev()) {
			mixers.at(i)->update();
			return;
		}
	}
}

void
MainWin::trayClicked(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger || 
	    reason == QSystemTrayIcon::DoubleClick) {
		if (!this->isVisible()) {
			this->setVisible(true);
			saveGeometry();
		} else {
			saveGeometry();
			this->setVisible(false);
		}
	}
}

void
MainWin::closeEvent(QCloseEvent *event)
{
	setVisible(false);
	event->ignore();
}

void
MainWin::moveEvent(QMoveEvent *event)
{
	saveGeometry();
	event->accept();
}

void
MainWin::resizeEvent(QResizeEvent *event)
{
	saveGeometry();
	event->accept();
}

void
MainWin::showConfigMenu()
{
	Preferences prefs(*chanMask, dsbmixer_amplification(),
			   dsbmixer_feeder_rate_quality(),
			   dsbmixer_default_unit(), dsbmixer_maxautovchans(),
			   dsbmixer_latency(), dsbmixer_bypass_mixer(),
			   *lrView, *showTicks, *pollIval, *playCmd, this);

	/* No need to poll the mixers while the config menu is showing */
	timer->stop();
	if (prefs.exec() != QDialog::Accepted) {
		timer->start(*pollIval);
		return;
	}

	if (prefs.defaultUnit != dsbmixer_default_unit())
		setDefaultTab(prefs.defaultUnit);
	if (*lrView    != prefs.lrView || *chanMask != prefs.chanMask ||
	    *showTicks != prefs.showTicks) {
		*lrView    = prefs.lrView;
		*chanMask  = prefs.chanMask;
		*showTicks = prefs.showTicks;
		
		redrawMixers();
	}
	if (*pollIval != prefs.pollIval) {
		timer->start(prefs.pollIval);
		*pollIval = prefs.pollIval;
	}
	QTextCodec *codec = QTextCodec::codecForLocale();
	QByteArray encstr = codec->fromUnicode(prefs.playCmd);

	if (encstr.data() == NULL || strcmp(encstr.data(), *playCmd) != 0) {
		free(*playCmd);
		if (encstr.data() == NULL)
			*playCmd = strdup("");
		else
			*playCmd = strdup(encstr.data());
		if (*playCmd == NULL)
			qh_err(this, EXIT_FAILURE, "strdup()");
	}
	dsbcfg_write(PROGRAM, "config", cfg);

	if (dsbmixer_amplification() != prefs.amplify ||
	    dsbmixer_default_unit() != prefs.defaultUnit ||
	    dsbmixer_feeder_rate_quality() != prefs.feederRateQuality ||
	    dsbmixer_latency() != prefs.latency ||
	    dsbmixer_maxautovchans() != prefs.maxAutoVchans ||
	    dsbmixer_bypass_mixer() != prefs.bypassMixer) {
		if (dsbmixer_change_settings(prefs.defaultUnit, prefs.amplify,
		    prefs.feederRateQuality, prefs.latency,
		    prefs.maxAutoVchans, prefs.bypassMixer) == -1)
			qh_warnx(0, dsbmixer_error());
	}
	timer->start(*pollIval);
}

void
MainWin::toggleWin()
{
	if (isVisible())
		hide();
	else
		show();
}

void
MainWin::quit()
{
	saveGeometry();
	dsbcfg_write(PROGRAM, "config", cfg);
	dsbmixer_cleanup();
	QApplication::quit();
}

void
MainWin::saveGeometry()
{
	if (isVisible()) {
		*posX = this->x(); *posY = this->y();
		*wWidth = this->width(); *hHeight = this->height();
		 dsbcfg_write(PROGRAM, "config", cfg);
	}
}

void
MainWin::createMenuActions()
{

	quitAction = new QAction(quitIcon, tr("&Quit"), this);
	preferencesAction = new QAction(prefsIcon, tr("&Preferences"), this);
	toggleAction = new QAction(hVolIcon, tr("Show/hide window"), this);

	connect(toggleAction, SIGNAL(triggered()), this, SLOT(toggleWin()));
	connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));
	connect(preferencesAction, SIGNAL(triggered()), this,
	    SLOT(showConfigMenu()));
}

void
MainWin::createMainMenu()
{
	mainMenu = menuBar()->addMenu(tr("&File"));
	mainMenu->addAction(preferencesAction);
	mainMenu->addAction(quitAction);
}

void
MainWin::checkForSysTray()
{
	static int tries = 60;

	if (trayAvailable) {
		tries = 60;
		delete trayIcon;
		trayAvailable = false;
	}
	if (QSystemTrayIcon::isSystemTrayAvailable()) {
		createTrayIcon();
		trayAvailable = true;
		tries = 60;
		traytimer->stop();
	} else if (tries-- <= 0) {
		traytimer->stop();
		trayAvailable = false;
		show();
	}
}

void
MainWin::createTrayIcon()
{
	int   idx   = tabs->currentIndex();
	QMenu *menu = new QMenu(this);
	trayIcon    = new MixerTrayIcon(idx == -1 ? 0 : mixers.at(idx),
					hVolIcon, this);
	updateTrayIcon();
	menu->addAction(toggleAction);
	menu->addAction(preferencesAction);
	menu->addAction(quitAction);


	trayIcon->setContextMenu(menu);
	trayIcon->show();

	connect(trayIcon,
	    SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	    this,
	    SLOT(trayClicked(QSystemTrayIcon::ActivationReason)));
}

void
MainWin::catchCurrentChanged()
{
	int idx = tabs->currentIndex();

	if (!trayAvailable)
		return;
	trayIcon->setMixer(idx == -1 ? 0 : mixers.at(idx));
	updateTrayIcon();
}

void
MainWin::catchMasterVolChanged(int vol)
{
	int	idx = tabs->currentIndex();
	QString trayToolTip;

	if (!trayAvailable)
		return;
	if (mixers.at(idx)->muted)
		trayToolTip = QString(tr("Muted"));
	else
		trayToolTip = QString("Vol %1%").arg(vol);
	trayIcon->setToolTip(trayToolTip);

	if (vol == 0) {
		trayIcon->setIcon(muteIcon);
		return;
	}
	switch (vol * 3 / 100) {
	case 0:
		trayIcon->setIcon(lVolIcon);
		break;
	case 1: trayIcon->setIcon(mVolIcon);
		break;
	case 2:
	case 3:
		trayIcon->setIcon(hVolIcon);
	}
}

void
MainWin::updateTrayIcon()
{
	int	vol;
	int	idx = tabs->currentIndex();
	QIcon	icon;
	QString trayToolTip;

	if (idx == -1)
		return;
	if (mixers.at(idx)->muted) {
		vol = 0;
		trayToolTip = QString(tr("Muted"));
		trayIcon->setIcon(muteIcon);
	} else {
		dsbmixer_t *dev = mixers.at(idx)->getDev();

		vol = dsbmixer_getvol(dev, DSBMIXER_MASTER);
		vol = (DSBMIXER_CHAN_RIGHT(vol) + DSBMIXER_CHAN_LEFT(vol)) >> 1;

		trayToolTip = QString("Vol %1%").arg(vol);

		if (vol == 0)
			icon = muteIcon;
		else {
			switch (vol * 3 / 100) {
			case 0:
				icon = lVolIcon;
				break;
			case 1:
				icon = mVolIcon;
				break;
			case 2:
			case 3:
			default:
				icon = hVolIcon;
			}
		}
		trayIcon->setIcon(icon);
	}
	trayIcon->setToolTip(trayToolTip);
}

int
MainWin::mixerUnitToTabIndex(int unit)
{
	for (int i = 0; i < mixers.count(); i++) {
		if (mixers.at(i)->getDev()->unit == unit)
			return (i);
	}
	return (-1);
}

