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

#include "config.h"

dsbcfg_vardef_t vardefs[] = {
	{ "win_pos_x",  DSBCFG_VAR_INTEGER, CFG_POS_X,	   DSBCFG_VAL(0)       },
        { "win_pos_y",  DSBCFG_VAR_INTEGER, CFG_POS_Y,	   DSBCFG_VAL(0)       },
        { "win_width",  DSBCFG_VAR_INTEGER, CFG_WIDTH,	   DSBCFG_VAL(100)     },
        { "win_height", DSBCFG_VAR_INTEGER, CFG_HEIGHT,	   DSBCFG_VAL(300)     },
        { "chan_mask",  DSBCFG_VAR_INTEGER, CFG_MASK,      DSBCFG_VAL(-1)      },
	{ "poll_ival",  DSBCFG_VAR_INTEGER, CFG_POLL_IVAL, DSBCFG_VAL(600)     },
	{ "lrview",	DSBCFG_VAR_BOOLEAN, CFG_LRVIEW,	   DSBCFG_VAL(false)   },
	{ "ticks",	DSBCFG_VAR_BOOLEAN, CFG_TICKS,	   DSBCFG_VAL(true)    },
	{ "play_cmd",	DSBCFG_VAR_STRING,  CFG_PLAY_CMD,  DSBCFG_VAL(PLAYCMD) },
	{ "tray_theme",	DSBCFG_VAR_STRING,  CFG_TRAY_THEME,DSBCFG_VAL((char *)0) }

};

