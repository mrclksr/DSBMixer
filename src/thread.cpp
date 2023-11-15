/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "thread.h"
#include "libdsbmixer.h"

void Thread::run() {
  int state;
  dsbmixer_t *mixer;

  for (;;) {
    mixer = dsbmixer_query_devlist(&state, true);
    if (mixer) {
      if (state == DSBMIXER_STATE_REMOVED)
        emit sendRemoveMixer(mixer);
      else
        emit sendNewMixer(mixer);
    }
  }
}

Thread::Thread(QObject *parent) : QThread(parent) {}
