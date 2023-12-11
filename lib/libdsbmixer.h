/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#ifndef _DSBMIXER_H_
#define _DSBMIXER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <sys/soundcard.h>
#include <sys/types.h>

#define DSBMIXER_CHAN_RIGHT(vol) (((vol) >> 8) & 0x7f)
#define DSBMIXER_CHAN_LEFT(vol) ((vol)&0x7f)
#define DSBMIXER_CHAN_CONCAT(lvol, rvol) ((lvol) + ((rvol) << 8))

#define DSBMIXER_ERR_SYS (1 << 0)
#define DSBMIXER_ERR_FATAL (1 << 1)
#define DSBMIXER_ERR_NOMIXER (1 << 2)
#define DSBMIXER_ERR_DEVDCONNLOST (1 << 3)

#define DSBMIXER_MASTER SOUND_MIXER_VOLUME
#define DSBMIXER_MAX_CHANNELS SOUND_MIXER_NRDEVICES
#define DSBMIXER_STATE_NEW 1
#define DSBMIXER_STATE_REMOVED -1
#define DSBAPPSMIXER_CHANS_CHANGED 1
#define DSBAPPSMIXER_MAX_CHANS 24

typedef struct dsbmixer_channel_s dsbmixer_channel_t;
typedef struct mixer_s dsbmixer_t;
typedef struct dsbappsmixer_channel_s dsbappsmixer_channel_t;
typedef struct appsmixer_s dsbappsmixer_t;

/*
 * General sound settings. man sound(4)
 */
struct dsbmixer_snd_settings_s {
  int default_unit;
  int prev_unit;
  int amplify;
  int feeder_rate_quality;
  int maxautovchans;
  int latency;
  bool mixer_bypass;
};

struct dsbmixer_channel_s {
  int vol;          /* (rvol << 8) + vol */
  char const *name; /* Vol, Pcm, Mic, etc. */
};

struct mixer_s {
  int id;          /* Unique ID */
  int fd;          /* File descriptor of mixer */
  int nchans;      /* # of channels */
  int unit;        /* pcm[unit], mixer[unit] */
  char *ugen;      /* ugen device in case of USB audio. */
  char *name;      /* Device name of mixer. */
  char *cardname;  /* Name of soundcard. */
  bool removed;    /* If mixer was removed don't try to access it */
  int recsrc;      /* Bitmask of current recording sources. */
  int recmask;     /* Bitmask of all rec. devices/channels. */
  int dmask;       /* Bitmask of all channels. */
  int mutemask;    /* Bitmask for mute states of all channels. */
  int changemask;  /* Mask for volume changes since last poll. */
  int rchangemask; /* Mask for record source changes "      "  */
  int mchangemask; /* Mask for mute state changes "      " */
  dsbmixer_channel_t chan[SOUND_MIXER_NRDEVICES];
};

struct dsbappsmixer_channel_s {
  int vol; /* (rvol << 8) + lvol */
  int saved_vol;
  bool muted;
  char *dev;  /* dsp device */
  char *name; /* Name of audio application. */
  pid_t pid;  /* Audio application PID */
};

struct appsmixer_s {
  int fd; /* Filedescriptor for /dev/mixer */
  int nchans;
  dsbappsmixer_channel_t *chan[DSBAPPSMIXER_MAX_CHANS];
};

struct _audio_proc_update_cmds_s {
  const char *move;
  const char *restart;
};

struct _audio_proc_updater_s {
  const char *proc;
  const struct _audio_proc_update_cmds_s update_cmds;
};

typedef struct dsbmixer_audio_proc_s {
  pid_t pid;
  struct _audio_proc_updater_s const *updater;
} dsbmixer_audio_proc_t;

extern struct dsbmixer_snd_settings_s dsbmixer_snd_settings;

extern int dsbmixer_get_err(char const **errmsg);
extern int dsbmixer_set_default_unit(int unit);
extern int dsbmixer_set_rec(dsbmixer_t *mixer, int chan, bool on);
extern int dsbmixer_init(void);
extern int dsbmixer_get_ndevs(void);
extern int dsbmixer_get_nchans(dsbmixer_t *mixer);
extern int dsbmixer_get_chan_id(dsbmixer_t *mixer, int index);
extern int dsbmixer_default_unit(void);
extern int dsbmixer_amplification(void);
extern int dsbmixer_feeder_rate_quality(void);
extern int dsbmixer_maxautovchans(void);
extern int dsbmixer_latency(void);
extern int dsbmixer_get_unit(const dsbmixer_t *mixer);
extern int dsbmixer_get_id(const dsbmixer_t *mixer);
extern int dsbmixer_get_vol(dsbmixer_t *mixer, int chan);
extern int dsbmixer_set_vol(dsbmixer_t *, int chan, int vol);
extern int dsbmixer_set_lvol(dsbmixer_t *, int chan, int lvol);
extern int dsbmixer_set_rvol(dsbmixer_t *, int chan, int rvol);
extern int dsbmixer_set_mute(dsbmixer_t *mixer, int chan, bool mute);
extern int dsbmixer_apply_settings(void);
extern int dsbmixer_change_settings(int, int, int, int, int, bool);
extern int dsbmixer_poll_default_unit(void);
extern int dsbmixer_audio_proc_move(dsbmixer_audio_proc_t *);
extern int dsbmixer_audio_proc_restart(dsbmixer_audio_proc_t *);
extern int dsbappsmixer_reinit(dsbappsmixer_t *);
extern int dsbappsmixer_get_nchans(const dsbappsmixer_t *);
extern int dsbappsmixer_get_vol(const dsbappsmixer_t *, int);
extern int dsbappsmixer_poll(dsbappsmixer_t *);
extern int dsbappsmixer_set_rvol(dsbappsmixer_t *, int, int);
extern int dsbappsmixer_set_lvol(dsbappsmixer_t *, int, int);
extern int dsbappsmixer_set_vol(dsbappsmixer_t *, int, int);
extern int dsbappsmixer_set_mute(dsbappsmixer_t *, int, bool);
extern bool dsbappsmixer_is_muted(const dsbappsmixer_t *, int);
extern void dsbappsmixer_destroy(dsbappsmixer_t *);
extern bool dsbmixer_bypass_mixer(void);
extern bool dsbmixer_is_muted(dsbmixer_t *mixer, int chan);
extern bool dsbmixer_can_rec(const dsbmixer_t *mixer, int chan);
extern bool dsbmixer_is_recsrc(const dsbmixer_t *mixer, int chan);
extern bool dsbmixer_audio_proc_can_move(const dsbmixer_audio_proc_t *proc);
extern bool dsbmixer_audio_proc_can_restart(const dsbmixer_audio_proc_t *proc);
extern void dsbmixer_cleanup(void);
extern void dsbmixer_del_mixer(dsbmixer_t *);
extern pid_t dsbappsmixer_get_pid(const dsbappsmixer_t *, int);
extern pid_t dsbmixer_get_audio_proc_pid(const dsbmixer_audio_proc_t *);
extern const char *dsbmixer_get_card_name(const dsbmixer_t *mixer);
extern const char *dsbmixer_get_name(const dsbmixer_t *mixer);
extern const char *dsbmixer_get_audio_proc_cmd(const dsbmixer_audio_proc_t *);
extern const char *dsbmixer_error(void);
extern const char *dsbmixer_get_chan_name(const dsbmixer_t *mixer, int chan);
extern const char **dsbmixer_get_chan_names(void);
extern const char *dsbappsmixer_get_name(const dsbappsmixer_t *, int);
extern const char *dsbappsmixer_get_dev(const dsbappsmixer_t *, int);
extern dsbmixer_t *dsbmixer_get_mixer_by_id(int);
extern dsbappsmixer_t *dsbappsmixer_create_mixer(void);
#ifndef WITHOUT_DEVD
extern dsbmixer_t *dsbmixer_query_devlist(int *, bool);
#endif
extern dsbmixer_t *dsbmixer_poll_mixers(void);
extern dsbmixer_t *dsbmixer_get_mixer(int mixer);
extern dsbmixer_channel_t *dsbmixer_get_chan(dsbmixer_t *, int);
extern dsbmixer_audio_proc_t *dsbmixer_get_audio_procs(size_t *_nprocs);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !_DSBMIXER_H_ */
