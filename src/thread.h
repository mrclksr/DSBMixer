/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QThread>
#include "libdsbmixer.h"

class Thread : public QThread
{
  Q_OBJECT
public:
  Thread(QObject *parent = 0);

signals:
  void sendNewMixer(dsbmixer_t *mixer);
  void sendRemoveMixer(dsbmixer_t *mixer);

protected:
  void run();
};
