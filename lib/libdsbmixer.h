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

#ifndef _DSBMIXER_H_
#define _DSBMIXER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <sys/types.h>
#include <sys/soundcard.h>

#define DSBMIXER_CHAN_RIGHT(vol)	 (((vol) >> 8) & 0x7f)
#define DSBMIXER_CHAN_LEFT(vol)		 ((vol) & 0x7f)
#define DSBMIXER_CHAN_CONCAT(lvol, rvol) ((lvol) + ((rvol) << 8))

#define DSBMIXER_ERR_SYS	 	 (1 << 0)
#define DSBMIXER_ERR_FATAL		 (1 << 1)
#define DSBMIXER_ERR_NOMIXER		 (1 << 2)
#define DSBMIXER_ERR_DEVDCONNLOST	 (1 << 3)

#define DSBMIXER_MAX_CHANNELS		 SOUND_MIXER_NRDEVICES
#define DSBMIXER_STATE_NEW	1
#define DSBMIXER_STATE_REMOVED -1

typedef struct dsbmixer_channel_s {
        int        vol;	   /* (rvol << 8) + vol */
        char const *name;  /* Vol, Pcm, Mic, etc. */
} dsbmixer_channel_t;

typedef struct mixer_s {
	int   id;	   /* Unique ID */
	int   fd;          /* File descriptor of mixer */
	int   nchans;	   /* # of channels */
	int   unit;	   /* pcm[unit], mixer[unit] */
	char  *ugen;	   /* ugen device in case of USB audio. */
	char  *name;       /* Device name of mixer. */
	char  *cardname;   /* Name of soundcard. */
	bool  removed;	   /* If mixer was removed don't try to access it */
	int recsrc;	   /* Bitmask of current recording sources. */
	int recmask;	   /* Bitmask of all rec. devices/channels. */
	int dmask;	   /* Bitmask of all channels. */
	int changemask;    /* Mask for volume changes since last poll. */
	int rchangemask;   /* Mask for record source changes "      "  */
	dsbmixer_channel_t chan[SOUND_MIXER_NRDEVICES];
} dsbmixer_t;

/*
 * General sound settings. man sound(9)
 */
struct dsbmixer_snd_settings_s {
	int default_unit;
	int amplify;
	int feeder_rate_quality;
};
extern struct dsbmixer_snd_settings_s dsbmixer_snd_settings;

extern int	  dsbmixer_geterr(char const **errmsg);
extern int	  dsbmixer_setrec(dsbmixer_t *mixer, int chan, bool on);
extern int	  dsbmixer_init(void);
extern int	  dsbmixer_getndevs(void);
extern int	  dsbmixer_getnchans(dsbmixer_t *mixer);
extern int	  dsbmixer_getchanid(dsbmixer_t *mixer, int index);
extern int	  dsbmixer_default_unit(void);
extern int	  dsbmixer_getvol(dsbmixer_t *mixer, int chan);
extern int	  dsbmixer_apply_settings(void);
extern int	  dsbmixer_change_settings(int dfltunit, int amp, int qual);
extern bool	  dsbmixer_canrec(dsbmixer_t *mixer, int chan);
extern bool	  dsbmixer_getrec(dsbmixer_t *mixer, int chan);
extern void	  dsbmixer_cleanup(void);
extern void	  dsbmixer_delmixer(dsbmixer_t *);
extern void	  dsbmixer_setvol(dsbmixer_t *, int chan, int vol);
extern void	  dsbmixer_setlvol(dsbmixer_t *, int chan, int lvol);
extern void	  dsbmixer_setrvol(dsbmixer_t *, int chan, int rvol);
extern const char *dsbmixer_error(void);
extern const char *dsbmixer_getchaname(dsbmixer_t *mixer, int chan);
extern const char **dsbmixer_getchanames(void);
extern dsbmixer_t *dsbmixer_getmixer_byid(int);
#ifndef WITHOUT_DEVD
extern dsbmixer_t *dsbmixer_querydevlist(int *, bool);
#endif
extern dsbmixer_t *dsbmixer_pollmixers(void);
extern dsbmixer_t *dsbmixer_getmixer(int mixer);
extern dsbmixer_channel_t *dsbmixer_getchan(dsbmixer_t *, int);
extern int dsbmixer_change_settings1(char *name, ...);
#ifdef __cplusplus
}
#endif	/* __cplusplus */
#endif	/* !_DSBMIXER_H_ */

