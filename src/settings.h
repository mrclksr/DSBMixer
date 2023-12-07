/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QString>

class Settings {
 public:
  int chanMask;
  int amplify;
  int feederRateQuality;
  int defaultUnit;
  int maxAutoVchans;
  int latency;
  int pollIval;
  int unitChkIval;
  int volInc;
  bool bypassMixer;
  bool lrView;
  bool showTicks;
  bool inverseScroll;
  QString playCmd;
  QString themeName;
};
