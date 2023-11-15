/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QIcon>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QWidget>

namespace qh {
extern void warn(QWidget *parent, QString msg);
extern void warnx(QWidget *parent, QString msg);
extern void err(QWidget *parent, int eval, QString msg);
extern void errx(QWidget *parent, int eval, QString msg);

extern QIcon loadStockIcon(QStyle::StandardPixmap pm, QWidget *parent = 0);
extern QIcon loadIcon(const QStringList iconNames);
extern QIcon loadIcon(const QString &iconName);
extern QIcon loadStaticIconFromTheme(const QString theme,
                                     const QStringList iconNames);
}  // namespace qh
