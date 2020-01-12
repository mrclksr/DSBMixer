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

#include <unistd.h>
#include <signal.h>
#include <QTranslator>

#include "mainwin.h"
#include "libdsbmixer.h"
#include "qt-helper/qt-helper.h"

static dsbcfg_t *cfg;

static void
usage()
{
	(void)fprintf(stderr, "Usage: %s [-hi]\n" \
	    "   -i: Start %s as tray icon\n", PROGRAM, PROGRAM);
	exit(EXIT_FAILURE);
}

static void
save_config(int /* unused */)
{
	static sig_atomic_t block = 0;

	if (block == 1)
		return;
	block = 1;
	if (cfg != NULL)
		dsbcfg_write(PROGRAM, "config", cfg);
	exit(EXIT_SUCCESS);
}

int
main(int argc, char *argv[])
{
	int  ch;
	bool iflag;

	iflag = false;
	while ((ch = getopt(argc, argv, "ih")) != -1) {
		switch (ch) {
		case 'i':
			/* Start as tray icon. */
			iflag = true;
			break;
		case '?':
		case 'h':
			usage();
		}
	}

	(void)signal(SIGINT, save_config);
	(void)signal(SIGTERM, save_config);
	(void)signal(SIGQUIT, save_config);
	(void)signal(SIGHUP, save_config);

	QApplication app(argc, argv);
	QTranslator translator;

	/* Set application name and RESOURCE_NAME env to set WM_CLASS */
	QApplication::setApplicationName(PROGRAM);
	(void)qputenv("RESOURCE_NAME", PROGRAM);

	if (translator.load(QLocale(), QLatin1String(PROGRAM),
	    QLatin1String("_"), QLatin1String(LOCALE_PATH)))
		app.installTranslator(&translator);

	cfg = dsbcfg_read(PROGRAM, "config", vardefs, CFG_NVARS);
	if (cfg == NULL && errno == ENOENT) {
		cfg = dsbcfg_new(NULL, vardefs, CFG_NVARS);
		if (cfg == NULL)
			qh_errx(0, EXIT_FAILURE, "%s", dsbcfg_strerror());
	} else if (cfg == NULL)
		qh_errx(0, EXIT_FAILURE, "%s", dsbcfg_strerror());

	if (dsbmixer_init() == -1) {
		char const *r;
		dsbmixer_geterr(&r);
		qh_err(0, EXIT_FAILURE, r);
	}
	MainWin w(cfg);
	if (!iflag)
		w.show();
	return (app.exec());
}
