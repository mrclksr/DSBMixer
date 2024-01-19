/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include <signal.h>
#include <unistd.h>
#include <QApplication>
#include <QTranslator>

#include "defs.h"
#include "libdsbmixer.h"
#include "mainwin.h"
#include "qt-helper/qt-helper.h"

static dsbcfg_t *cfg;

static void usage() {
  (void)fprintf(stderr,
                "Usage: %s [-hi]\n"
                "   -i: Start %s as tray icon\n",
                PROGRAM, PROGRAM);
  exit(EXIT_FAILURE);
}

static void cleanup() {
  if (cfg != NULL) dsbcfg_write(PROGRAM, "config", cfg);
  dsbmixer_cleanup();
}

static void catchSigs(int /* unused */) {
  static sig_atomic_t block = 0;

  if (block == 1) return;
  block = 1;
  cleanup();
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  int ch;
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

  (void)signal(SIGINT, catchSigs);
  (void)signal(SIGTERM, catchSigs);
  (void)signal(SIGQUIT, catchSigs);
  (void)signal(SIGHUP, catchSigs);

  QApplication app(argc, argv);
  QTranslator translator;

  /* Set application name and RESOURCE_NAME env to set WM_CLASS */
  QApplication::setApplicationName(PROGRAM);
  (void)qputenv("RESOURCE_NAME", PROGRAM);

  if (translator.load(QLocale(), QLatin1String(PROGRAM), QLatin1String("_"),
                      QLatin1String(LOCALE_PATH)))
    app.installTranslator(&translator);

  cfg = dsbcfg_read(PROGRAM, "config", vardefs, CFG_NVARS);
  if (cfg == NULL && errno == ENOENT) {
    cfg = dsbcfg_new(NULL, vardefs, CFG_NVARS);
    if (cfg == NULL) qh::errx(nullptr, EXIT_FAILURE, QString(dsbcfg_strerror()));
  } else if (cfg == NULL)
    qh::errx(nullptr, EXIT_FAILURE, QString(dsbcfg_strerror()));

  if (dsbmixer_init() == -1) {
    char const *r;
    dsbmixer_get_err(&r);
    qh::err(nullptr, EXIT_FAILURE, r);
  }
  MainWin w(cfg);
  if (!iflag) w.show();
  int ret{app.exec()};
  cleanup();
  return (ret);
}
