/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "iconloader.h"

#include "qt-helper/qt-helper.h"

IconLoader::IconLoader(QString &themeName, QObject *parent)
    : QObject(parent), themeName{themeName} {
  loadIcons();
}

void IconLoader::loadIcons() {
  loadTrayIcons();
  quitIcon = qh::loadIcon("application-exit");
  prefsIcon = qh::loadIcon("preferences-desktop-multimedia");
  mixerIcon = qh::loadIcon("audio-volume-high");
  appsMixerIcon = qh::loadIcon(QStringList() << "audio-headphones"
                                             << "audio-speakers"
                                             << "speaker");
  windowIcon = qh::loadIcon(QStringList() << "window"
                                          << "window-maximize"
                                          << "window_fullscreen"
                                          << "window-restore");
  helpIcon = qh::loadIcon(QStringList() << "help"
                                        << "stock_help");
  aboutIcon = qh::loadIcon(QStringList() << "help-about"
                                         << "stock_about");
  if (quitIcon.isNull())
    quitIcon = qh::loadStockIcon(QStyle::SP_DialogCloseButton);
  if (prefsIcon.isNull())
    prefsIcon = QIcon(":/icons/preferences-desktop-multimedia.png");
  if (mixerIcon.isNull()) mixerIcon = QIcon(":/icons/audio-volume-high.png");
  if (appsMixerIcon.isNull()) appsMixerIcon = mixerIcon;
}

void IconLoader::loadTrayIcons() {
  muteIcon = qh::loadStaticIconFromTheme(
      themeName, QStringList() << "audio-volume-muted-panel"
                               << "audio-volume-muted");
  lVolIcon = qh::loadStaticIconFromTheme(
      themeName, QStringList() << "audio-volume-low-panel"
                               << "audio-volume-low");
  mVolIcon = qh::loadStaticIconFromTheme(
      themeName, QStringList() << "audio-volume-medium-panel"
                               << "audio-volume-medium");
  hVolIcon = qh::loadStaticIconFromTheme(
      themeName, QStringList() << "audio-volume-high-panel"
                               << "audio-volume-high");
  if (muteIcon.isNull()) muteIcon = QIcon(":/icons/audio-volume-muted.png");
  if (lVolIcon.isNull()) lVolIcon = QIcon(":/icons/audio-volume-low.png");
  if (mVolIcon.isNull()) mVolIcon = QIcon(":/icons/audio-volume-medium.png");
  if (hVolIcon.isNull()) hVolIcon = QIcon(":/icons/audio-volume-high.png");
}

void IconLoader::setTheme(QString &themeName) {
  if (this->themeName == themeName) return;
  this->themeName = themeName;
  loadTrayIcons();
  emit trayIconThemeChanged();
}
