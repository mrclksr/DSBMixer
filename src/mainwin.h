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

#ifndef MAINWIN_H
#define MAINWIN_H

#include <QWidget>
#include <QMainWindow>
#include <QCloseEvent>
#include <QMoveEvent>

#include "mixer.h"
#include "preferences.h"
#include "config.h"
#include "libdsbmixer.h"
#include "mixertrayicon.h"

class MainWin : public QMainWindow
{
	Q_OBJECT
public:
	MainWin(dsbcfg_t *cfg, QWidget *parent = 0);
	QMenu *menu();

public slots:
#ifndef WITHOUT_DEVD
	void addNewMixer();
	void removeMixer(dsbmixer_t *mixer);
#endif
	void updateMixers();
	void trayClicked(QSystemTrayIcon::ActivationReason reason);
	void closeEvent(QCloseEvent *event);
	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);
	void showConfigMenu();
	void toggleWin();
	void quit();
private slots:
	void checkForSysTray();
	void catchCurrentChanged();
	void catchMasterVolChanged(int vol);
	void scrGeomChanged(const QRect &);
private:
	int  mixerUnitToTabIndex(int unit);
	void loadIcons();
	void redrawMixers();
	void updateTrayIcon();
	void createMixerList();
	void createTabs();
	void setDefaultTab(int);
	void createMenuActions();
	void createMainMenu();
	void createTrayIcon();
	void saveGeometry();
private:
	int	      *posX;
	int	      *posY;
	int	      *wWidth;
	int	      *hHeight;
	int	      *chanMask;
	int	      *pollIval;
	char	      **playCmd;
	bool	      *lrView;
	bool	      *showTicks;
	bool	      trayAvailable;
	QIcon	      muteIcon;
	QIcon	      lVolIcon;
	QIcon	      mVolIcon;
	QIcon	      hVolIcon;
	QIcon	      quitIcon;
	QIcon	      prefsIcon;
	QMenu	      *mainMenu;
	QTimer	      *traytimer;
	QTimer	      *timer;
	QAction	      *quitAction;
	QAction	      *preferencesAction;
	QAction	      *toggleAction;
	dsbcfg_t      *cfg;
	QTabWidget    *tabs;
	MixerTrayIcon *trayIcon;
	QList<Mixer *>mixers;
};
#endif // MAINWIN_H
