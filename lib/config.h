/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "dsbcfg/dsbcfg.h"

/*
 * Definition of config file variables and their default values.
 */
enum {
  CFG_POS_X = 0,
  CFG_POS_Y,
  CFG_WIDTH,
  CFG_HEIGHT,
  CFG_MASK,
  CFG_LRVIEW,
  CFG_TICKS,
  CFG_POLL_IVAL,
  CFG_PLAY_CMD,
  CFG_TRAY_THEME,
  CFG_UNIT_CHK_IVAL,
  CFG_VOL_INC,
  CFG_INVERSE_SCROLL,
  CFG_NVARS
};

#define PLAYCMD "sh -c \"cat /dev/random > /dev/dsp\""

extern dsbcfg_vardef_t vardefs[];
