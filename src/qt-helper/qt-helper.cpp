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

#include <QFile>
#include <QTextStream>
#include <QTranslator>
#include <QApplication>
#include <QWidget>
#include <pwd.h>
#include <stdarg.h>
#include <unistd.h>
#include "qt-helper.h"

static char msgbuf[1024];
static bool themeNameSet = false;
static QString themeFiles[2] = { ".gtkrc.mine", ".gtkrc-2.0" };

enum Icon { WarningIcon, ErrorIcon };

static void
setThemeName()
{
	QRegExp rx("^gtk-icon-theme-name\\s*=\\s*\"{1,1}(\\S+)\"{1,1}");
	struct passwd *pw;

	themeNameSet = true;
	pw = getpwuid(getuid()); endpwent();
	if (pw == NULL)
		return;
	for (int i = 0; i < 2; i++) {
		QString line;
		QString path = QString("%1/%2").arg(pw->pw_dir, themeFiles[i]);
		QFile file(path);
	
		if (!file.open(QFile::ReadOnly))
			return;
		QTextStream in(&file);
		in.setCodec("UTF-8");
		do {
			line = in.readLine();
			if (rx.indexIn(line) != -1) {
				QIcon::setThemeName(rx.cap(1));
				return;
			}
		} while (!line.isNull());
	}
}

QIcon
qh_loadStockIcon(QStyle::StandardPixmap pm, QWidget *parent)
{
	return (QApplication::style()->standardIcon(pm, 0, parent));
}

QIcon
qh_loadThemeIcon(const QString &name)
{
	if (!themeNameSet)
		setThemeName();
	return (QIcon::fromTheme(name));
}

QIcon
qh_loadIcon(const char *name, ...)
{
	QIcon	   icon;
	va_list	   ap;
	const char *s;

	if (!themeNameSet)
		setThemeName();
	va_start(ap, name);
	for (s = name; s != NULL || (s = va_arg(ap, char *)); s = NULL) {
		icon = QIcon::fromTheme(s);
		if (!icon.isNull())
			return (icon);
	}
	return (QIcon());
}

static void
msgwin(const char *title, const char *msg, Icon _icon, QWidget *parent)
{
	QMessageBox msgBox(parent);
	msgBox.setWindowModality(Qt::WindowModal);
        msgBox.setWindowTitle(QObject::tr(title));
	msgBox.setText(QString(msg));

	msgBox.setIcon(_icon == ErrorIcon ? QMessageBox::Critical \
		: QMessageBox::Warning);
	QIcon icon = qh_loadStockIcon(_icon == ErrorIcon ? \
		QStyle::SP_MessageBoxCritical : QStyle::SP_MessageBoxWarning);
	msgBox.setWindowIcon(icon);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();
}

void
qh_warn(QWidget *parent, const char *fmt, ...)
{
	int     _errno = errno;
	size_t  len, rem;
	va_list ap;

	msgbuf[0] = '\0';

	va_start(ap, fmt);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	len = strlen(msgbuf);
	rem = sizeof(msgbuf) - len - 1;
	(void)snprintf(msgbuf + len, rem, ":\n%s\n", strerror(_errno));

	msgwin("Warning", msgbuf, WarningIcon, parent);
}

void
qh_warnx(QWidget *parent, const char *fmt, ...)
{
	va_list ap;
        
	msgbuf[0] = '\0';

	va_start(ap, fmt);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	msgwin("Warning", msgbuf, WarningIcon, parent);
}

void
qh_err(QWidget *parent, int eval, const char *fmt, ...)
{
	int     _errno = errno;
	size_t  len, rem;
	va_list ap;

	msgbuf[0] = '\0';

	va_start(ap, fmt);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	len = strlen(msgbuf);
	rem = sizeof(msgbuf) - len - 1;
	(void)snprintf(msgbuf + len, rem, ":\n%s\n", strerror(_errno));
	msgwin("Error", msgbuf, ErrorIcon, parent);
	exit(eval);
}

void
qh_errx(QWidget *parent, int eval, const char *fmt, ...)
{
	va_list ap;
        
	msgbuf[0] = '\0';

	va_start(ap, fmt);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	msgwin("Error", msgbuf, ErrorIcon, parent);
	exit(eval);
}

