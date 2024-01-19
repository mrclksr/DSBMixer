/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QIcon>
#include <QObject>
#include <QString>

class IconLoader : public QObject {
  Q_OBJECT
 public:
  IconLoader(QString &themeName, QObject *parent = nullptr);
  void setTrayIconTheme(QString &themeName);
  void loadIcons();
  void loadTrayIcons();

 signals:
  void trayIconThemeChanged();

 public:
  QIcon muteIcon;
  QIcon lVolIcon;
  QIcon mVolIcon;
  QIcon hVolIcon;
  QIcon quitIcon;
  QIcon prefsIcon;
  QIcon mixerIcon;
  QIcon aboutIcon;
  QIcon helpIcon;
  QIcon noMixerIcon;
  QIcon appsMixerIcon;
  QIcon windowIcon;
  QString themeName;
};
