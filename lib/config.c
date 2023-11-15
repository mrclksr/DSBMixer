/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "config.h"

dsbcfg_vardef_t vardefs[] = {
    {"win_pos_x", DSBCFG_VAR_INTEGER, CFG_POS_X, {.integer = 0}},
    {"win_pos_y", DSBCFG_VAR_INTEGER, CFG_POS_Y, {.integer = 0}},
    {"win_width", DSBCFG_VAR_INTEGER, CFG_WIDTH, {.integer = 100}},
    {"win_height", DSBCFG_VAR_INTEGER, CFG_HEIGHT, {.integer = 300}},
    {"chan_mask", DSBCFG_VAR_INTEGER, CFG_MASK, {.integer = -1}},
    {"poll_ival", DSBCFG_VAR_INTEGER, CFG_POLL_IVAL, {.integer = 600}},
    {"vol_inc", DSBCFG_VAR_INTEGER, CFG_VOL_INC, {.integer = 3}},
    {"unit_chk_ival", DSBCFG_VAR_INTEGER, CFG_UNIT_CHK_IVAL, {.integer = 3000}},
    {"lrview", DSBCFG_VAR_BOOLEAN, CFG_LRVIEW, {.boolean = false}},
    {"ticks", DSBCFG_VAR_BOOLEAN, CFG_TICKS, {.boolean = true}},
    {"apps_mixer", DSBCFG_VAR_BOOLEAN, CFG_APPSMIXER, {.boolean = true}},
    {"play_cmd", DSBCFG_VAR_STRING, CFG_PLAY_CMD, {.string = PLAYCMD}},
    {"tray_theme", DSBCFG_VAR_STRING, CFG_TRAY_THEME, {.string = (char *)0}}};
