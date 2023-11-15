/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "qt-helper.h"

#include <stdarg.h>
#include <unistd.h>

#include <QApplication>
#include <QMessageBox>
#include <QTranslator>

namespace qh {
enum Icon { WarningIcon, ErrorIcon };

QIcon loadStockIcon(QStyle::StandardPixmap pm, QWidget *parent) {
  return (QApplication::style()->standardIcon(pm, 0, parent));
}

QIcon loadIcon(const QStringList iconNames) {
  for (auto &n : iconNames) {
    if (!QIcon::hasThemeIcon(n)) continue;
    QIcon icon = QIcon::fromTheme(n);
    if (icon.isNull()) continue;
    if (icon.name().isEmpty() || icon.name().length() < 1) continue;
    return (icon);
  }
  return (QIcon());
}

QIcon loadIcon(const QString &iconName) {
  if (!QIcon::hasThemeIcon(iconName)) return (QIcon());
  return (QIcon::fromTheme(iconName));
}

QIcon loadStaticIconFromTheme(const QString theme,
                              const QStringList iconNames) {
  QIcon sIcon;
  QString curTheme = QIcon::themeName();

  if (!theme.isEmpty()) QIcon::setThemeName(theme);
  for (auto &n : iconNames) {
    if (!QIcon::hasThemeIcon(n)) continue;
    QIcon icon = QIcon::fromTheme(n);
    if (icon.isNull()) continue;
    if (icon.name().isEmpty() || icon.name().length() < 1) continue;
    QList<QSize> sizes = icon.availableSizes();
    for (auto &s : sizes) {
      QPixmap pix = icon.pixmap(s);
      sIcon.addPixmap(pix);
    }
    break;
  }
  QIcon::setThemeName(curTheme);
  return (sIcon);
}

static void msgwin(const char *title, QString &msg, Icon _icon,
                   QWidget *parent) {
  QMessageBox msgBox(parent);
  msgBox.setWindowModality(Qt::WindowModal);
  msgBox.setWindowTitle(QObject::tr(title));
  msgBox.setText(msg);

  msgBox.setIcon(_icon == ErrorIcon ? QMessageBox::Critical
                                    : QMessageBox::Warning);
  QIcon icon = loadStockIcon(_icon == ErrorIcon ? QStyle::SP_MessageBoxCritical
                                                : QStyle::SP_MessageBoxWarning);
  msgBox.setWindowIcon(icon);
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}

void warn(QWidget *parent, QString msg) {
  QString msgBuf = QString("%1: %2").arg(msg).arg(strerror(errno));
  msgwin("Warning", msgBuf, WarningIcon, parent);
}

void warnx(QWidget *parent, QString msg) {
  msgwin("Warning", msg, WarningIcon, parent);
}

void err(QWidget *parent, int eval, QString msg) {
  QString msgBuf = QString("%1: %2").arg(msg).arg(strerror(errno));
  msgwin("Error", msgBuf, ErrorIcon, parent);
  exit(eval);
}

void errx(QWidget *parent, int eval, QString msg) {
  msgwin("Error", msg, ErrorIcon, parent);
  exit(eval);
}

}  // namespace qh
