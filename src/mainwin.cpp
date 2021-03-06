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
#include <QSignalMapper>
#include <QTextCodec>
#include <unistd.h>
#include <stdlib.h>
#include <QRect>
#include <QScreen>
#include <QDebug>

#include "mainwin.h"
#include "thread.h"
#include "mixer.h"
#include "restartapps.h"
#include "preferences.h"
#include "settings.h"
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
	trayTheme = &dsbcfg_getval(cfg, CFG_TRAY_THEME).string;

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

	createMainMenu();
	setWindowIcon(winIcon);
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
	muteIcon  = qh_loadStaticIconFromTheme(*trayTheme,
				"audio-volume-muted-panel",
				"audio-volume-muted", NULL);
	lVolIcon  = qh_loadStaticIconFromTheme(*trayTheme,
				"audio-volume-low-panel",
				"audio-volume-low", NULL);
	mVolIcon  = qh_loadStaticIconFromTheme(*trayTheme,
				"audio-volume-medium-panel",
				"audio-volume-medium", NULL);
	hVolIcon  = qh_loadStaticIconFromTheme(*trayTheme,
				"audio-volume-high-panel",
				"audio-volume-high", NULL);
	quitIcon  = qh_loadIcon("application-exit", NULL); 
	prefsIcon = qh_loadIcon("preferences-desktop-multimedia", NULL);
	winIcon   = qh_loadIcon("audio-volume-high", NULL);
	
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
	if (winIcon.isNull())
		winIcon = QIcon(":/icons/audio-volume-high.png");
}

void
MainWin::createMixerList()
{
	for (int i = 0; i < dsbmixer_getndevs(); i++) {
		dsbmixer_t *dev = dsbmixer_getmixer(i);
		Mixer *mixer = new Mixer(dev, *chanMask, *lrView, this);
		mixers.append(mixer);
		mixer->setTicks(*showTicks);
		connect(mixer, SIGNAL(masterVolChanged(int, int, int)), this,
		    SLOT(catchMasterVolChanged(int, int, int)));
	}
}

void
MainWin::createTabs()
{
	int didx = mixerUnitToTabIndex(dsbmixer_default_unit());
	QWidget	    *container = new QWidget(this);
	QVBoxLayout *vbox      = new QVBoxLayout(container);

	for (int i = 0; i < mixers.count(); i++) {
		dsbmixer_t *dev = mixers.at(i)->getDev();
		QString label(mixers.at(i)->cardname);

		if (i == didx)
			label.append("*");
		tabs->addTab(mixers.at(i), label);
		tabs->setTabToolTip(i, QString(dev->name));
	}
	vbox->addWidget(tabs);
	vbox->setContentsMargins(15, 5, 15, 0);
	setCentralWidget(container);
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
		QString label(mixers.at(i)->cardname);
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
	if (trayAvailable)
		updateTrayIcon();
}

#ifndef WITHOUT_DEVD
void
MainWin::addNewMixer()
{
	redrawMixers();
	addTrayMenuActions();
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
			addTrayMenuActions();
			return;
		}
  	}	
}
#endif

void
MainWin::updateMixers()
{
	int	   unit;
	size_t	   nap = 0;
	static int prev_unit = -1;
	static int cnt = 0;
	dsbmixer_t *mixer;
	dsbmixer_audio_proc_t *ap;

	if (cnt++ % 5 == 0) {
		unit = dsbmixer_poll_default_unit();
		if (prev_unit != unit) {
			setDefaultTab(unit);
			if (prev_unit != -1) {
				ap = dsbmixer_get_audio_procs(&nap);
				if (ap != NULL) {
					RestartApps raWin(ap, nap, this);
					(void)raWin.exec();
				}
			}
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
MainWin::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
	case Qt::Key_0:
	case Qt::Key_1:
	case Qt::Key_2:
	case Qt::Key_3:
	case Qt::Key_4:
	case Qt::Key_5:
	case Qt::Key_6:
	case Qt::Key_7:
	case Qt::Key_8:
	case Qt::Key_9:
		int tabno = e->text().toInt();
		if (e->modifiers() == Qt::AltModifier) {
			if (tabno == 0)
				tabno = tabs->count();
			tabs->setCurrentIndex(tabno - 1);
		}
	}
}

void
MainWin::showConfigMenu()
{
	Settings settings;
	
	settings.chanMask	   = *chanMask;
	settings.amplify	   = dsbmixer_amplification();
	settings.feederRateQuality = dsbmixer_feeder_rate_quality(),
	settings.defaultUnit	   = dsbmixer_default_unit();
	settings.maxAutoVchans     = dsbmixer_maxautovchans();
	settings.latency	   = dsbmixer_latency();
	settings.bypassMixer	   = dsbmixer_bypass_mixer();
	settings.lrView	           = *lrView;
	settings.showTicks	   = *showTicks;
	settings.pollIval	   = *pollIval;
	if (*playCmd != NULL)
		settings.playCmd = QString(*playCmd);
	else
		settings.playCmd = QString();
	if (*trayTheme != NULL)
		settings.themeName = QString(*trayTheme);
	else
		settings.themeName = QString();
	Preferences prefs(settings, this);

	/* No need to poll the mixers while the config menu is showing */
	timer->stop();
	if (prefs.exec() != QDialog::Accepted) {
		timer->start(*pollIval);
		return;
	}
	if (settings.defaultUnit != prefs.settings.defaultUnit)
		setDefaultTab(prefs.settings.defaultUnit);
	if (settings.lrView    != prefs.settings.lrView     ||
	    settings.chanMask  != prefs.settings.chanMask   ||
	    settings.showTicks != prefs.settings.showTicks) {
		*lrView    = prefs.settings.lrView;
		*chanMask  = prefs.settings.chanMask;
		*showTicks = prefs.settings.showTicks;
		redrawMixers();
	}
	if (settings.pollIval != prefs.settings.pollIval) {
		timer->start(prefs.settings.pollIval);
		*pollIval = prefs.settings.pollIval;
	}
	QTextCodec *codec = QTextCodec::codecForLocale();
	QByteArray encstr = codec->fromUnicode(prefs.settings.playCmd);
	if (settings.playCmd != prefs.settings.playCmd) {
		free(*playCmd);
		if (prefs.settings.playCmd.isNull())
			*playCmd = NULL;
		else {
			*playCmd = strdup(encstr.data());
			if (*playCmd == NULL)
				qh_err(this, EXIT_FAILURE, "strdup()");
		}
	}
	encstr = codec->fromUnicode(prefs.settings.themeName);
	if (prefs.settings.themeName != settings.themeName) {
		free(*trayTheme);
		if (prefs.settings.themeName.isNull())
			*trayTheme = NULL;
		else
			*trayTheme = strdup(encstr.data());
		loadIcons();
		if (trayAvailable)
			updateTrayIcon();
	}
	dsbcfg_write(PROGRAM, "config", cfg);
	if (prefs.settings.amplify	     != settings.amplify	   ||
	    prefs.settings.defaultUnit	     != settings.defaultUnit	   ||
	    prefs.settings.feederRateQuality != settings.feederRateQuality ||
	    prefs.settings.latency	     != settings.latency	   ||
	    prefs.settings.maxAutoVchans     != settings.maxAutoVchans	   ||
	    prefs.settings.bypassMixer	     != settings.bypassMixer) {
		if (dsbmixer_change_settings(prefs.settings.defaultUnit,
		    prefs.settings.amplify, prefs.settings.feederRateQuality,
		    prefs.settings.latency, prefs.settings.maxAutoVchans,
		    prefs.settings.bypassMixer) == -1)
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

QAction *
MainWin::createQuitAction()
{
	QAction *action = new QAction(quitIcon, tr("&Quit"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(quit()));

	return (action);
}

QAction *
MainWin::createPrefsAction()
{
	QAction *action = new QAction(prefsIcon, tr("&Preferences"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(showConfigMenu()));

	return (action);
}

void
MainWin::createMainMenu()
{
	mainMenu = menuBar()->addMenu(tr("&File"));

	mainMenu->addAction(createPrefsAction());
	mainMenu->addAction(createQuitAction());
}

void
MainWin::addTrayMenuActions()
{
	if (!trayAvailable)
		return;
	int didx = mixerUnitToTabIndex(dsbmixer_default_unit());
	QAction	      *toggle = new QAction(winIcon, tr("Show/hide window"), this);
	QSignalMapper *mapper = new QSignalMapper(this);

	trayMenu->clear();
	trayMenu->addAction(toggle);
	trayMenu->addSection(tr("Sound devices"));

	for (int i = 0; i < mixers.count(); i++) {
		QString label(mixers.at(i)->cardname);
		if (i == didx)
			label.append("*");
		QAction *action = new QAction(winIcon, label, this);
		if (i == tabs->currentIndex())
			action->setEnabled(false);
		connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
		mapper->setMapping(action, i);
		trayMenu->addAction(action);
	}
	trayMenu->addSeparator();
	connect(mapper, SIGNAL(mapped(int)), this, SLOT(setTabIndex(int)));
	connect(toggle, SIGNAL(triggered()), this, SLOT(toggleWin()));

	trayMenu->addAction(createPrefsAction());
	trayMenu->addAction(createQuitAction());
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
		trayAvailable = true;
		tries = 60;
		createTrayIcon();
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
	int idx  = tabs->currentIndex();
	trayMenu = new QMenu(this);
	trayIcon = new MixerTrayIcon(idx == -1 ? 0 : mixers.at(idx),
					hVolIcon, this);
	updateTrayIcon();
	trayIcon->setContextMenu(trayMenu);
	trayIcon->show();

	connect(trayIcon,
	    SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	    this,
	    SLOT(trayClicked(QSystemTrayIcon::ActivationReason)));
}

void
MainWin::setTabIndex(int index)
{
	tabs->setCurrentIndex(index);
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
MainWin::catchMasterVolChanged(int unit, int lvol, int rvol)
{
	int	idx = tabs->currentIndex();
	QString trayToolTip;

	if (!trayAvailable)
		return;
	if (tabs->currentIndex() != mixerUnitToTabIndex(unit))
		return;
	if (mixers.at(idx)->muted)
		trayToolTip = QString(tr("Muted"));
	else {
		if (*lrView)
			trayToolTip = QString("Vol %1:%2").arg(lvol).arg(rvol);
		else
			trayToolTip = QString("Vol %1%").arg((lvol + rvol) >> 1);
	}
	trayIcon->setToolTip(trayToolTip);

	if (lvol + rvol == 0) {
		trayIcon->setIcon(muteIcon);
		return;
	}
	switch (((lvol + rvol) >> 1) * 3 / 100) {
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
	int	lvol, rvol, vol;
	int	idx = tabs->currentIndex();
	QIcon	icon;
	QString trayToolTip;

	if (idx == -1)
		return;
	addTrayMenuActions();
	trayIcon->setMixer(mixers.at(idx));
	if (mixers.at(idx)->muted) {
		trayToolTip = QString(tr("Muted"));
		trayIcon->setIcon(muteIcon);
	} else {
		mixers.at(idx)->getMasterVol(&lvol, &rvol);
		vol = (lvol + rvol) >> 1;
		if (*lrView)
			trayToolTip = QString("Vol %1:%2").arg(lvol).arg(rvol);
		else
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
