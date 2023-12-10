/*-
 * Copyright (c) 2023 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "libdsbmixer.h"

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <paths.h>
#include <pthread.h>
#include <pwd.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define FATAL_SYSERR (DSBMIXER_ERR_SYS | DSBMIXER_ERR_FATAL)
#define PATH_SNDSTAT "/dev/sndstat"
#define PATH_DEVD_SOCKET "/var/run/devd.seqpacket.pipe"
#define PATH_DEFAULT_MIXER "/dev/mixer"
#define PATH_PS "/bin/ps"


#define RE_STR                                             \
  "([0-9]+)[[:space:]]+([^[:space:]]+)[[:space:]]+"        \
  "([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+" \
  "([^[:space:]]+).*"

#define ERROR(ret, error, prepend, fmt, ...)       \
  do {                                             \
    set_error(error, prepend, fmt, ##__VA_ARGS__); \
    return (ret);                                  \
  } while (0)

#define VALIDATE_CHAN(mx, ch) \
  ((mx) != NULL && !(mx)->removed && (ch) >= 0 && (ch) < SOUND_MIXER_NRDEVICES)

#define VALIDATE_APPS_CHAN(mx, ch) \
  ((mx) != NULL && (ch) >= 0 && (ch) < (mx)->nchans)

#define RETURN_ERROR_IF_CHAN_INVALID(errval, mx, ch) \
  do {                                               \
    if (!VALIDATE_CHAN((mx), (ch))) {                \
      errno = EINVAL;                                \
      return (errval);                               \
    }                                                \
  } while (0)

#define RETURN_ERROR_IF_APPS_CHAN_INVALID(errval, mx, ch) \
  do {                                                    \
    if (!VALIDATE_APPS_CHAN((mx), (ch))) {                \
      errno = EINVAL;                                     \
      return (errval);                                    \
    }                                                     \
  } while (0)

enum SOCK_ERR { SOCK_ERR_CONN_CLOSED = 1, SOCK_ERR_IO_ERROR };

struct dsbmixer_snd_settings_s dsbmixer_snd_settings = {0};

#ifndef WITHOUT_DEVD
/*
 * Struct to represent the fields of a devd notify event.
 */
static struct devdevent_s {
  char *ugen;      /* ugen device name. */
  char *system;    /* Bus or DEVFS */
  char *subsystem; /* INTERFACE, CDEV */
  char *type;      /* Event type: CREATE, DESTROY, DETACH. */
  char *cdev;      /* Device name. */
} devdevent;

typedef struct devqueue_item_s {
  int state; /* NEW, REMOVED */
  dsbmixer_t *mixer;
} devqueue_item_t;

typedef struct mqmsg_s {
  long mtype;
  char mdata[sizeof(devqueue_item_t)];
} mqmsg_t;
#endif

/*
 * Placeholder   Meaning
 * %u            Previous audio default unit number
 * %U            Current audio default unit number
 * %p            PID of the audio proc
 */
#define CMD_MOVE_PA_STREAMS PATH_DSBMIXER_PA " move-streams %u %U"
#define CMD_RESTART_PA PATH_DSBMIXER_PA " restart"
#define CMD_RESTART_SNDIO "pkill -HUP %p"

static struct _audio_proc_updater_s audio_proc_updaters[] = {
    {"pulseaudio", {.move = CMD_MOVE_PA_STREAMS, .restart = CMD_RESTART_PA}},
    {"sndiod", {.move = NULL, .restart = CMD_RESTART_SNDIO}}};

static int uconnect(const char *path);
static int get_unit(const char *mixer);
static int get_mixers(void);
static int add_mixer(const char *name);
static int read_appvol(const char *dev);
static int add_app_chans(dsbappsmixer_t *am);
static int pid_to_cmd(pid_t pid, char *cmd, size_t cmdsize);
static int get_oss_audioinfo(int fd, int devid, oss_audioinfo *info);
static int get_oss_sysinfo(int fd, oss_sysinfo *sysinfo);
static int set_recsrc(dsbmixer_t *mixer, int mask);
static int set_vol(dsbmixer_t *mixer, int dev, int vol);
static int read_vol(dsbmixer_t *mixer);
static int read_recsrc(dsbmixer_t *mixer);
static int run_audio_proc_updater(dsbmixer_audio_proc_t *, const char *);
static char *get_cardname(const char *mixer);
static char *get_ugen(int pcmunit);
static char *exec_backend(const char *cmd, int *ret);
#ifndef WITHOUT_DEVD
static void *devd_watcher(void *unused);
static char *read_devd_event(int s, int *error);
static void parse_devd_event(char *str);
static void devd_reconnect(int *);
static int devd_connect(void);
#endif
static void set_error(int, bool, const char *fmt, ...);
static void get_snd_settings(void);
static void destroy_appchannel(dsbappsmixer_channel_t *chan);
static dsbappsmixer_channel_t *create_appchannel(const char *dev, pid_t pid);

static int _error = 0, nmixers = 0;
#ifndef WITHOUT_DEVD
static int devdfd;
static int mqid;
static pthread_t thr;
#endif
static char errormsg[_POSIX2_LINE_MAX];
static dsbmixer_t **mixers = NULL;
static const char *channel_names[] = SOUND_DEVICE_LABELS;
static const struct _audio_proc_updater_s *lookup_audio_proc_updater(
    const char *);

int dsbmixer_init() {
  dsbmixer_snd_settings.prev_unit = -1;
  get_snd_settings();
#ifndef WITHOUT_DEVD
  int n;

  for (n = 0, devdfd = -1; devdfd == -1 && n < 10; n++) {
    if ((devdfd = uconnect(PATH_DEVD_SOCKET)) == -1) {
      warn("Connection to devd socket failed");
      if (n < 9) {
        (void)sleep(1);
        warnx("Retrying ...");
      }
    }
  }
  if (devdfd == -1) {
    ERROR(-1, DSBMIXER_ERR_FATAL, true, "Couldn't connect to devd");
  }
  if ((mqid = msgget(IPC_PRIVATE, S_IRWXU)) == -1)
    ERROR(-1, FATAL_SYSERR, false, "msgget()");
  if (pthread_create(&thr, NULL, &devd_watcher, NULL) == -1)
    ERROR(-1, FATAL_SYSERR, false, "pthread_create()");
#endif
  if (get_mixers() == -1) ERROR(-1, 0, true, "get_mixers()");
  return (0);
}

void dsbmixer_cleanup() {
  int i;
#ifndef WITHOUT_DEVD
  (void)pthread_cancel(thr);
  if (pthread_join(thr, NULL) == -1) warn("pthread_join() failed.");
  (void)close(devdfd);
  msgctl(mqid, IPC_RMID, NULL);
  mqid = -1;
#endif
  for (i = 0; i < dsbmixer_get_ndevs(); i++)
    dsbmixer_del_mixer(dsbmixer_get_mixer(i));
}

int dsbmixer_default_unit() { return (dsbmixer_snd_settings.default_unit); }

int dsbmixer_amplification() { return (dsbmixer_snd_settings.amplify); }

int dsbmixer_feeder_rate_quality() {
  return (dsbmixer_snd_settings.feeder_rate_quality);
}
int dsbmixer_get_unit(const dsbmixer_t *mixer) { return (mixer->unit); }

int dsbmixer_get_id(const dsbmixer_t *mixer) { return (mixer->id); }

const char *dsbmixer_get_card_name(const dsbmixer_t *mixer) {
  return (mixer->cardname);
}

const char *dsbmixer_get_name(const dsbmixer_t *mixer) { return (mixer->name); }

const char *dsbmixer_get_audio_proc_cmd(const dsbmixer_audio_proc_t *ap) {
  return (ap->updater->proc);
}

pid_t dsbmixer_get_audio_proc_pid(const dsbmixer_audio_proc_t *ap) {
  return (ap->pid);
}

int dsbmixer_maxautovchans() { return (dsbmixer_snd_settings.maxautovchans); }

int dsbmixer_latency() { return (dsbmixer_snd_settings.latency); }

bool dsbmixer_bypass_mixer() { return (dsbmixer_snd_settings.mixer_bypass); }

int dsbmixer_set_vol(dsbmixer_t *mixer, int chan, int vol) {
  return (set_vol(mixer, chan, vol));
}

int dsbmixer_set_lvol(dsbmixer_t *mixer, int chan, int lvol) {
  return (set_vol(
      mixer, chan,
      DSBMIXER_CHAN_CONCAT(lvol, DSBMIXER_CHAN_RIGHT(mixer->chan[chan].vol))));
}

int dsbmixer_set_rvol(dsbmixer_t *mixer, int chan, int rvol) {
  return (set_vol(
      mixer, chan,
      DSBMIXER_CHAN_CONCAT(DSBMIXER_CHAN_LEFT(mixer->chan[chan].vol), rvol)));
}

int dsbmixer_get_vol(dsbmixer_t *mixer, int chan) {
  RETURN_ERROR_IF_CHAN_INVALID(-1, mixer, chan);
  return (mixer->chan[chan].vol);
}

bool dsbmixer_can_rec(const dsbmixer_t *mixer, int chan) {
  RETURN_ERROR_IF_CHAN_INVALID(false, mixer, chan);
  errno = 0;
  return ((mixer->recmask & (1 << chan)) ? true : false);
}

int dsbmixer_set_rec(dsbmixer_t *mixer, int chan, bool on) {
  int mask;

  RETURN_ERROR_IF_CHAN_INVALID(-1, mixer, chan);
  mask = mixer->recsrc;
  if (on)
    mask |= (1 << chan);
  else
    mask &= ~(1 << chan);
  return (set_recsrc(mixer, mask));
}

bool dsbmixer_is_recsrc(const dsbmixer_t *mixer, int chan) {
  RETURN_ERROR_IF_CHAN_INVALID(false, mixer, chan);
  errno = 0;
  return (((1 << chan) & mixer->recsrc) ? true : false);
}

int dsbmixer_set_mute(dsbmixer_t *mixer, int chan, bool mute) {
  RETURN_ERROR_IF_CHAN_INVALID(-1, mixer, chan);
  if (!mute && mixer->chan[chan].muted) {
    if (set_vol(mixer, chan, mixer->chan[chan].saved_vol) == -1) return (-1);
    mixer->chan[chan].muted = false;
  } else if (mute && !mixer->chan[chan].muted) {
    mixer->chan[chan].saved_vol = mixer->chan[chan].vol;
    if (set_vol(mixer, chan, 0) == -1) return (-1);
    mixer->chan[chan].muted = true;
  }
  return (0);
}

bool dsbmixer_is_muted(dsbmixer_t *mixer, int chan) {
  RETURN_ERROR_IF_CHAN_INVALID(false, mixer, chan);
  if (read_vol(mixer) == -1) return (false);
  errno = 0;
  if (mixer->chan[chan].vol != 0)
    mixer->chan[chan].muted = false;
  else
    mixer->chan[chan].muted = true;
  return (mixer->chan[chan].muted);
}

int dsbmixer_get_err(char const **errmsg) {
  if (!_error) return (0);
  *errmsg = errormsg;
  return (_error);
}

int dsbmixer_get_ndevs() { return (nmixers); }

int dsbmixer_get_nchans(dsbmixer_t *mixer) { return (mixer->nchans); }

int dsbmixer_get_chan_id(dsbmixer_t *mixer, int index) {
  int i, n;

  for (i = n = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (((1 << i) & mixer->dmask) && n++ == index) return (i);
  }
  return (-1);
}

const char *dsbmixer_get_chan_name(const dsbmixer_t *mixer, int chan) {
  RETURN_ERROR_IF_CHAN_INVALID(NULL, mixer, chan);
  if ((1 << chan) & mixer->dmask) return (mixer->chan[chan].name);
  return (NULL);
}
const char **dsbmixer_get_chan_names() { return (channel_names); }

dsbmixer_t *dsbmixer_get_mixer_by_id(int id) {
  int i;

  for (i = 0; i < nmixers; i++) {
    if (mixers[i]->id == id) return (mixers[i]);
  }
  return (NULL);
}

dsbmixer_t *dsbmixer_get_mixer(int index) {
  if (index < 0 || index >= nmixers) return (NULL);
  return (mixers[index]);
}

dsbmixer_channel_t *dsbmixer_get_chan(dsbmixer_t *mixer, int chan) {
  if (chan < 0 || chan > SOUND_MIXER_NRDEVICES) return (NULL);
  if ((1 << chan) & mixer->dmask) return (&mixer->chan[chan]);
  return (NULL);
}

void dsbmixer_del_mixer(dsbmixer_t *mixer) {
  int i;

  for (i = 0; i < nmixers && mixer != mixers[i]; i++)
    ;
  if (i < nmixers) {
    if (mixers[i]->fd > 0) (void)close(mixers[i]->fd);
    free(mixers[i]->name);
    free(mixers[i]);
    for (; i < nmixers - 1; i++) mixers[i] = mixers[i + 1];
    nmixers--;
  }
}

dsbmixer_t *dsbmixer_poll_mixers() {
  static int i, next = 0;

  if (next >= nmixers) next = 0;
  for (i = next; i < nmixers; i++) {
    if (mixers[i]->removed) continue;
    read_vol(mixers[i]);
    read_recsrc(mixers[i]);
    if (mixers[i]->changemask || mixers[i]->rchangemask) {
      next = i + 1;
      return (mixers[i]);
    }
  }
  next = 0;
  return (NULL);
}

#ifndef WITHOUT_DEVD
/*
 * *state = { -1 if device was removed, > 0 if device was added. }
 * If 'block' is true block until a new mixer was added or a mixer
 * was removed from the system.
 */
dsbmixer_t *dsbmixer_query_devlist(int *state, bool block) {
  int ec;
  mqmsg_t msg;
  devqueue_item_t *dqi;

  if (mqid == -1) {
    /* Message queue was removed (dsbmixer_cleanup()) */
    if (!block) return (NULL);
    /* Just wait for process termination */
    for (;; sleep(1))
      ;
  }
  if (block)
    ec = msgrcv(mqid, &msg, sizeof(devqueue_item_t), 0, 0);
  else
    ec = msgrcv(mqid, &msg, sizeof(devqueue_item_t), 0, IPC_NOWAIT);
  if (ec == -1) return (NULL);
  dqi = (devqueue_item_t *)msg.mdata;

  *state = dqi->state;
  return (dqi->mixer);
}
#endif /* !WITHOUT_DEVD */

int dsbmixer_poll_default_unit(void) {
  int unit;
  size_t sz = sizeof(int);

  unit = 0;
  if (sysctlbyname("hw.snd.default_unit", &unit, &sz, NULL, 0))
    warn("sysctl(hw.snd.default_unit)");
  if (unit != dsbmixer_snd_settings.default_unit)
    dsbmixer_snd_settings.prev_unit = dsbmixer_snd_settings.default_unit;
  dsbmixer_snd_settings.default_unit = unit;

  return (unit);
}

int dsbmixer_set_default_unit(int unit) {
  if (sysctlbyname("hw.snd.default_unit", NULL, NULL, &unit, sizeof(unit)) !=
      0) {
    warn("Couldn't set hw.snd.default_unit");
    return (-1);
  }
  dsbmixer_snd_settings.prev_unit = dsbmixer_snd_settings.default_unit;
  dsbmixer_snd_settings.default_unit = unit;
  return (0);
}

int dsbmixer_change_settings(int dfltunit, int amp, int qual, int latency,
                             int max_auto_vchans, bool bypass_mixer) {
  int error = 0;
  char cmd[512], *output;
  (void)snprintf(cmd, sizeof(cmd), "%s -u %d -d -a %d -q %d -l %d -v %d -b %d",
                 PATH_BACKEND, dfltunit, amp, qual, latency, max_auto_vchans,
                 bypass_mixer);
  if ((output = exec_backend(cmd, &error)) == NULL) {
    set_error(0, true, "exec_backend(%s)", cmd);
    free(output);
    return (-1);
  } else if (error != 0) {
    set_error(DSBMIXER_ERR_FATAL, true, "Failed to execute %s: %s\n", cmd,
              output);
    free(output);
    return (-1);
  }
  get_snd_settings();

  return (0);
}

int dsbmixer_audio_proc_move(dsbmixer_audio_proc_t *proc) {
  return (run_audio_proc_updater(proc, proc->updater->update_cmds.move));
}

int dsbmixer_audio_proc_restart(dsbmixer_audio_proc_t *proc) {
  return (run_audio_proc_updater(proc, proc->updater->update_cmds.restart));
}

dsbmixer_audio_proc_t *dsbmixer_get_audio_procs(size_t *_nprocs) {
  FILE *fp;
  char buf[_POSIX2_LINE_MAX], pwdbuf[512], *cmd, *pid;
  size_t nap, nprocs;
  regex_t re;
  regmatch_t pmatch[6];
  const char *rcmd;
  struct passwd *pwdres, pwd;
  dsbmixer_audio_proc_t *ap;
  const struct _audio_proc_updater_s *updater;

  _error = 0;
  if (getpwuid_r(getuid(), &pwd, pwdbuf, sizeof(pwdbuf), &pwdres) != 0)
    ERROR(NULL, FATAL_SYSERR, false, "getpwuid_r()");
  endpwent();
  if (regcomp(&re, RE_STR, REG_EXTENDED) != 0) {
    warn("regcomp()");
    return (NULL);
  }
  (void)snprintf(buf, sizeof(buf), "%s -U %s -x", PATH_PS, pwd.pw_name);
  if ((fp = popen(buf, "r")) == NULL)
    ERROR(NULL, FATAL_SYSERR, false, "popen(%s)", buf);
  ap = NULL;
  nap = nprocs = 0;
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    if (regexec(&re, buf, 6, pmatch, 0) != 0) continue;
    buf[pmatch[1].rm_eo] = '\0';
    pid = buf + pmatch[1].rm_so;
    buf[pmatch[5].rm_eo] = '\0';
    cmd = buf + pmatch[5].rm_so;
    cmd = basename(cmd);
    if ((updater = lookup_audio_proc_updater(cmd)) == NULL) continue;
    if (++nprocs >= nap) {
      nap += 5;
      ap = realloc(ap, nap * sizeof(dsbmixer_audio_proc_t));
      if (ap == NULL) ERROR(NULL, FATAL_SYSERR, false, "realloc()");
    }
    ap[nprocs - 1].pid = strtol(pid, NULL, 10);
    ap[nprocs - 1].updater = updater;
  }
  *_nprocs = nprocs;
  regfree(&re);
  (void)pclose(fp);

  return (ap);
}

bool dsbmixer_audio_proc_can_move(const dsbmixer_audio_proc_t *proc) {
  return ((proc->updater->update_cmds.move != NULL));
}

bool dsbmixer_audio_proc_can_restart(const dsbmixer_audio_proc_t *proc) {
  return ((proc->updater->update_cmds.restart != NULL));
}

dsbappsmixer_t *dsbappsmixer_create_mixer() {
  dsbappsmixer_t *am;

  if ((am = malloc(sizeof(dsbappsmixer_t))) == NULL)
    ERROR(NULL, DSBMIXER_ERR_FATAL, false, "malloc()");
  (void)memset(am, 0, sizeof(dsbappsmixer_t));
  if (add_app_chans(am) == -1) {
    dsbappsmixer_destroy(am);
    return (NULL);
  }
  return (am);
}

int dsbappsmixer_get_nchans(const dsbappsmixer_t *am) { return (am->nchans); }

int dsbappsmixer_get_vol(const dsbappsmixer_t *am, int chan) {
  RETURN_ERROR_IF_APPS_CHAN_INVALID(-1, am, chan);
  errno = 0;
  return (am->chan[chan]->vol);
}

const char *dsbappsmixer_get_name(const dsbappsmixer_t *am, int chan) {
  RETURN_ERROR_IF_APPS_CHAN_INVALID(NULL, am, chan);
  errno = 0;
  return (am->chan[chan]->name);
}

const char *dsbappsmixer_get_dev(const dsbappsmixer_t *am, int chan) {
  RETURN_ERROR_IF_APPS_CHAN_INVALID(NULL, am, chan);
  errno = 0;
  return (am->chan[chan]->dev);
}

pid_t dsbappsmixer_get_pid(const dsbappsmixer_t *am, int chan) {
  RETURN_ERROR_IF_APPS_CHAN_INVALID((pid_t)-1, am, chan);
  errno = 0;
  return (am->chan[chan]->pid);
}

int dsbappsmixer_set_vol(dsbappsmixer_t *am, int chan, int vol) {
  int fd, ret;
  RETURN_ERROR_IF_APPS_CHAN_INVALID(-1, am, chan);
  if (DSBMIXER_CHAN_LEFT(vol) > 100 || DSBMIXER_CHAN_RIGHT(vol) > 100) {
    errno = EINVAL;
    return (-1);
  }
  if ((fd = open(am->chan[chan]->dev, O_RDONLY, 0)) == -1)
    ERROR(-1, DSBMIXER_ERR_SYS, false, "open(%s)", am->chan[chan]->dev);
  if ((ret = ioctl(fd, MIXER_WRITE(SOUND_MIXER_PCM), &vol)) == 0) {
    am->chan[chan]->vol = vol;
  }
  (void)close(fd);
  errno = 0;

  return (ret);
}

int dsbappsmixer_set_lvol(dsbappsmixer_t *am, int chan, int lvol) {
  RETURN_ERROR_IF_APPS_CHAN_INVALID(-1, am, chan);
  errno = 0;
  return (dsbappsmixer_set_vol(
      am, chan,
      DSBMIXER_CHAN_CONCAT(lvol, DSBMIXER_CHAN_RIGHT(am->chan[chan]->vol))));
}

int dsbappsmixer_set_rvol(dsbappsmixer_t *am, int chan, int rvol) {
  RETURN_ERROR_IF_APPS_CHAN_INVALID(-1, am, chan);
  errno = 0;
  return (dsbappsmixer_set_vol(
      am, chan,
      DSBMIXER_CHAN_CONCAT(DSBMIXER_CHAN_LEFT(am->chan[chan]->vol), rvol)));
}

int dsbappsmixer_set_mute(dsbappsmixer_t *mixer, int chan, bool mute) {
  RETURN_ERROR_IF_APPS_CHAN_INVALID(-1, mixer, chan);
  if (!mute && mixer->chan[chan]->muted) {
    if (dsbappsmixer_set_vol(mixer, chan, mixer->chan[chan]->saved_vol) == -1)
      return (-1);
    mixer->chan[chan]->muted = false;
  } else if (mute && !mixer->chan[chan]->muted) {
    mixer->chan[chan]->saved_vol = mixer->chan[chan]->vol;
    if (dsbappsmixer_set_vol(mixer, chan, 0) == -1) return (-1);
    mixer->chan[chan]->muted = true;
  }
  errno = 0;
  return (0);
}

bool dsbappsmixer_is_muted(const dsbappsmixer_t *mixer, int chan) {
  RETURN_ERROR_IF_APPS_CHAN_INVALID(false, mixer, chan);
  errno = 0;
  return (mixer->chan[chan]->muted);
}

/*
 * Reads the volume of all channels, and checks whether channels were
 * removed or added.
 * Returns -1 on error, DSBAPPSMIXER_CHANS_CHANGED if channels were added
 * or removed, 0 else.
 */
int dsbappsmixer_poll(dsbappsmixer_t *am) {
  int i, n, fd, vol;
  oss_sysinfo sysinfo;
  oss_audioinfo info;

  if ((fd = open(PATH_DEFAULT_MIXER, O_RDONLY)) == -1) {
    ERROR(-1, DSBMIXER_ERR_SYS, false, "open(%s)", PATH_DEFAULT_MIXER);
  }
  if (get_oss_sysinfo(fd, &sysinfo) == -1) {
    (void)close(fd);
    return (-1);
  }
  for (i = n = 0; i < sysinfo.numaudios; i++) {
    if (!((1 << i) & sysinfo.openedaudio[0])) continue;
    if (get_oss_audioinfo(fd, i, &info) == -1) continue;
    if (info.pid <= 0) continue;
    if (!(info.caps & DSP_CAP_OUTPUT)) continue;
    if (n >= am->nchans || am->chan[n]->pid != info.pid) {
      (void)close(fd);
      return (DSBAPPSMIXER_CHANS_CHANGED);
    } else if ((vol = read_appvol(am->chan[n]->dev)) != -1) {
      am->chan[n]->vol = vol;
      if (vol > 0) am->chan[n]->muted = false;
    }
    n++;
  }
  (void)close(fd);
  if (n < am->nchans) return (DSBAPPSMIXER_CHANS_CHANGED);
  return (0);
}

int dsbappsmixer_reinit(dsbappsmixer_t *am) {
  int i;

  for (i = 0; i < am->nchans; i++) destroy_appchannel(am->chan[i]);
  am->nchans = 0;
  return (add_app_chans(am));
}

void dsbappsmixer_destroy(dsbappsmixer_t *am) {
  int i;

  for (i = 0; i < am->nchans; i++) destroy_appchannel(am->chan[i]);
  free(am);
}

static dsbappsmixer_channel_t *create_appchannel(const char *dev, pid_t pid) {
  char buf[64];
  dsbappsmixer_channel_t *chan;

  if ((chan = malloc(sizeof(dsbappsmixer_channel_t))) == NULL)
    ERROR(NULL, DSBMIXER_ERR_FATAL, false, "malloc()");
  chan->name = NULL;
  if (pid_to_cmd(pid, buf, sizeof(buf)) == 0) {
    chan->name = strdup(basename(buf));
    if (chan->name == NULL) {
      free(chan);
      ERROR(NULL, DSBMIXER_ERR_FATAL, false, "strdup()");
    }
  }
  if ((chan->dev = strdup(dev)) == NULL) {
    free(chan);
    ERROR(NULL, DSBMIXER_ERR_FATAL, false, "strdup()");
  }
  chan->muted = false;
  chan->pid = pid;
  chan->vol = chan->saved_vol = read_appvol(dev);

  return (chan);
}

const char *dsbmixer_error() { return (errormsg); }

static void printerr() { (void)fprintf(stderr, "%s\n", errormsg); }

static void set_error(int error, bool prepend, const char *fmt, ...) {
  int _errno;
  char errbuf[sizeof(errormsg)];
  size_t len;
  va_list ap;

  _errno = errno;

  va_start(ap, fmt);
  if (prepend) {
    _error |= error;
    if (error & DSBMIXER_ERR_FATAL) {
      if (strncmp(errormsg, "Fatal: ", 7) == 0) {
        (void)memmove(errormsg, errormsg + 7, strlen(errormsg) - 6);
      }
      (void)strlcpy(errbuf, "Fatal: ", sizeof(errbuf));
      len = strlen(errbuf);
    } else
      len = 0;
    (void)vsnprintf(errbuf + len, sizeof(errbuf) - len, fmt, ap);

    len = strlen(errbuf);
    (void)snprintf(errbuf + len, sizeof(errbuf) - len, ":%s", errormsg);
    (void)strlcpy(errormsg, errbuf, sizeof(errormsg));
  } else {
    _error = error;
    (void)vsnprintf(errormsg, sizeof(errormsg), fmt, ap);
    if (error == DSBMIXER_ERR_FATAL) {
      (void)snprintf(errbuf, sizeof(errbuf), "Fatal: %s", errormsg);
      (void)strlcpy(errormsg, errbuf, sizeof(errormsg));
    }
  }
  if ((error & DSBMIXER_ERR_SYS) && _errno != 0) {
    len = strlen(errormsg);
    (void)snprintf(errormsg + len, sizeof(errormsg) - len, ":%s",
                   strerror(_errno));
    errno = 0;
  }
}

#define EXPAND(F) dsbmixer_snd_settings.F

static void get_snd_settings() {
  int prev_unit, curr_unit;
  size_t sz, i;
  struct oid_s {
    const char *oid;
    void *val;
  } oids[] = {{"hw.snd.default_unit", &EXPAND(default_unit)},
              {"hw.snd.vpc_0db", &EXPAND(amplify)},
              {"hw.snd.feeder_rate_quality", &EXPAND(feeder_rate_quality)},
              {"hw.snd.vpc_mixer_bypass", &EXPAND(mixer_bypass)},
              {"hw.snd.maxautovchans", &EXPAND(maxautovchans)},
              {"hw.snd.latency", &EXPAND(latency)}};
  prev_unit = EXPAND(prev_unit);
  curr_unit = EXPAND(default_unit);
  for (i = 0; i < sizeof(oids) / sizeof(struct oid_s); i++) {
    sz = sizeof(int);
    if (sysctlbyname(oids[i].oid, oids[i].val, &sz, NULL, 0))
      warn("sysctl(%s)", oids[i].oid);
  }
  if (prev_unit == -1)
    EXPAND(prev_unit) = EXPAND(default_unit);
  else
    EXPAND(prev_unit) = curr_unit;
}

#ifndef WITHOUT_DEVD
static void parse_devd_event(char *str) {
  char *p, *q;

  devdevent.cdev = "";
  devdevent.system = devdevent.subsystem = devdevent.type = "";

  for (p = str; (p = strtok(p, " \n")) != NULL; p = NULL) {
    if ((q = strchr(p, '=')) == NULL) continue;
    *q++ = '\0';
    if (strcmp(p, "ugen") == 0)
      devdevent.ugen = q;
    else if (strcmp(p, "system") == 0)
      devdevent.system = q;
    else if (strcmp(p, "subsystem") == 0)
      devdevent.subsystem = q;
    else if (strcmp(p, "type") == 0)
      devdevent.type = q;
    else if (strcmp(p, "cdev") == 0)
      devdevent.cdev = q;
  }
}

static void *devd_watcher(void *unused) {
  fd_set allset, rset;
  static int i, error;
  static char *lp;
  static char path[sizeof(_PATH_DEV) + sizeof("mixer") + 16];
  static mqmsg_t msg;
  static devqueue_item_t item;

  /* Silence warning about unused parameter */
  (void)unused;

  FD_ZERO(&allset);
  FD_SET(devdfd, &allset);

  for (;;) {
    rset = allset;
    if (select(devdfd + 1, &rset, NULL, NULL, NULL) < 0) continue;
    lp = read_devd_event(devdfd, &error);
    if (lp == NULL) {
      if (error == SOCK_ERR_CONN_CLOSED) {
        devd_reconnect(&devdfd);
        FD_ZERO(&allset);
        FD_SET(devdfd, &allset);
      }
      continue;
    }
    if (lp[0] != '!') continue;
    parse_devd_event(lp + 1);
    if (strcmp(devdevent.type, "CREATE") == 0 &&
        strcmp(devdevent.subsystem, "CDEV") == 0 &&
        strncmp(devdevent.cdev, "mixer", 5) == 0) {
      (void)snprintf(path, sizeof(path), "%s%s", _PATH_DEV, devdevent.cdev);
      if (add_mixer(path) == -1) continue;
      item.state = DSBMIXER_STATE_NEW;
      item.mixer = mixers[nmixers - 1];
      msg.mtype = 1;
      (void)memcpy(msg.mdata, &item, sizeof(item));
      msgsnd(mqid, &msg, sizeof(devqueue_item_t), 0);
    } else if (strcmp(devdevent.system, "USB") == 0 &&
               strcmp(devdevent.type, "DETACH") == 0) {
      for (i = 0; i < nmixers; i++) {
        if (mixers[i]->removed) continue;
        if (mixers[i]->ugen != NULL &&
            !strcmp(mixers[i]->ugen, devdevent.ugen)) {
          mixers[i]->removed = true;
          /*
           * Close mixer's fd so that the
           * system can remove the associated
           * devices.
           */
          (void)close(mixers[i]->fd);
          mixers[i]->fd = -1;
          item.state = DSBMIXER_STATE_REMOVED;
          item.mixer = mixers[i];
          msg.mtype = 1;
          (void)memcpy(msg.mdata, &item, sizeof(item));
          msgsnd(mqid, &msg, sizeof(devqueue_item_t), 0);
        }
      }
    }
  }
}

static char *read_devd_event(int s, int *error) {
  int n, rd;
  static int bufsz = 0;
  static char seq[1024], *lnbuf = NULL;
  struct iovec iov;
  struct msghdr msg;

  if (lnbuf == NULL) {
    if ((lnbuf = malloc(_POSIX2_LINE_MAX)) == NULL)
      err(EXIT_FAILURE, "malloc()");
    bufsz = _POSIX2_LINE_MAX;
  }
  iov.iov_len = sizeof(seq);
  iov.iov_base = seq;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  for (n = rd = *error = 0;;) {
    msg.msg_name = msg.msg_control = NULL;
    msg.msg_namelen = msg.msg_controllen = 0;

    if ((n = recvmsg(s, &msg, 0)) == (ssize_t)-1) {
      if (errno == EINTR) continue;
      if (errno == ECONNRESET) {
        *error = SOCK_ERR_CONN_CLOSED;
        return (NULL);
      }
      if (errno == EAGAIN) return (NULL);
      err(EXIT_FAILURE, "recvmsg()");
    } else if (n == 0 && rd == 0) {
      *error = SOCK_ERR_CONN_CLOSED;
      return (NULL);
    }
    if (rd + n + 1 > bufsz) {
      if ((lnbuf = realloc(lnbuf, rd + n + 65)) == NULL)
        err(EXIT_FAILURE, "realloc()");
      bufsz = rd + n + 65;
    }
    (void)memcpy(lnbuf + rd, seq, n);
    rd += n;
    lnbuf[rd] = '\0';
    if (msg.msg_flags & MSG_TRUNC) {
      warn("recvmsg(): Message truncated");
      return (rd > 0 ? lnbuf : NULL);
    } else if (msg.msg_flags & MSG_EOR)
      return (lnbuf);
  }
  return (NULL);
}

#endif /* !WITHOUT_DEVD */

static int get_unit(const char *mixer) {
  const char *p = strchr(mixer, '\0');

  while (isdigit(*--p))
    ;
  if (!isdigit(*++p))
    return (0);
  else
    return (strtol(p, NULL, 10));
}

static char *get_cardname(const char *mixer) {
  int unit;
  FILE *fp;
  char *name, ln[64], pattern[8];

  unit = get_unit(mixer);

  (void)snprintf(pattern, sizeof(pattern), "pcm%d:", unit);
  if ((fp = fopen(PATH_SNDSTAT, "r")) == NULL) {
    warn("get_cardname()->fopen(%s)", PATH_SNDSTAT);
    if ((name = malloc(7 * sizeof(char))) == NULL)
      ERROR(NULL, FATAL_SYSERR, false, "malloc()");
    (void)snprintf(name, 7, "pcm%d", unit);
    return (name);
  }
  for (name = NULL; fgets(ln, sizeof(ln), fp) != NULL;) {
    (void)strtok(ln, "\r\n");
    if (strlen(ln) > 8 && strcmp(&ln[strlen(ln) - 8], " default") == 0)
      ln[strlen(ln) - 8] = '\0';
    if (strncmp(ln, pattern, strlen(pattern)) == 0) {
      if ((name = strdup(ln)) == NULL)
        ERROR(NULL, FATAL_SYSERR, false, "strdup()");
      break;
    }
  }
  (void)fclose(fp);
  if (name == NULL) {
    if ((name = malloc(7 * sizeof(char))) == NULL)
      ERROR(NULL, FATAL_SYSERR, false, "malloc()");
    (void)snprintf(name, 7, "pcm%d", unit);
  }
  return (name);
}

static char *get_ugen(int pcmunit) {
  int i, unit;
  size_t sz;
  static char query[64], buf[128], *p;

  (void)snprintf(query, sizeof(query), "dev.pcm.%d.%%parent", pcmunit);
  sz = sizeof(buf);
  if (sysctlbyname(query, buf, &sz, NULL, 0) == -1) {
    warn("get_ugen()->sysctlbyname(%s) failed", query);
    return (NULL);
  }
  buf[sz] = '\0';

  for (i = strcspn(buf, "") - 1; i > 0 && isdigit(buf[i]); i--)
    ;
  if (i <= 0) return (NULL);
  unit = strtol(&buf[i + 1], NULL, 10);
  buf[i + 1] = '\0';
  (void)snprintf(query, sizeof(query), "dev.%s.%d.%%location", buf, unit);
  sz = sizeof(buf);

  if (sysctlbyname(query, buf, &sz, NULL, 0) == -1) {
    warn("get_ugen()->sysctlbyname(%s) failed", query);
    return (NULL);
  }
  buf[sz] = '\0';
  for (p = buf; (p = strtok(p, "\t ")) != NULL; p = NULL) {
    if (strncmp(p, "ugen=", 5) == 0) {
      if ((p = strdup(p + 5)) == NULL)
        ERROR(NULL, FATAL_SYSERR, false, "strdup()");
      return (p);
    }
  }
  return (NULL);
}

static int get_mixers() {
  int cwdfd;
  DIR *dirp;
  struct stat sb;
  struct dirent *dp;

  /* Save current working directory. */
  if ((cwdfd = open(".", O_EXEC)) == -1)
    ERROR(-1, FATAL_SYSERR, false, "open(.)");
  /* Scan /dev for mixer devices. */
  if (chdir(_PATH_DEV) == -1)
    ERROR(-1, FATAL_SYSERR, false, "chdir(%s)", _PATH_DEV);
  if ((dirp = opendir(".")) == NULL)
    ERROR(-1, FATAL_SYSERR, false, "opendir(%s)", _PATH_DEV);
  while ((dp = readdir(dirp)) != NULL) {
    if (strncmp(dp->d_name, "mixer", 5) != 0) continue;
    if (!isdigit(dp->d_name[5])) continue;
    if (lstat(dp->d_name, &sb) == -1) {
      warn("get_mixers()->stat(%s)", dp->d_name);
      continue;
    } else if (S_ISLNK(sb.st_mode)) {
      /* Skip symlinks */
      continue;
    }
    /*
     * Just skip mixers that produce an error. There could be
     * a permission problem or the device is damaged. We try
     * to find at least one mixer device.
     */
    if (add_mixer(dp->d_name) == -1) {
      if (_error & DSBMIXER_ERR_FATAL) {
        ERROR(-1, DSBMIXER_ERR_FATAL, true, "add_mixer(%s)", dp->d_name);
      } else if (_error & DSBMIXER_ERR_SYS) {
        set_error(0, true, "add_mixer(%s)", dp->d_name);
        printerr();
      }
    }
  }
  closedir(dirp);
  if (nmixers == 0 && add_mixer(PATH_DEFAULT_MIXER) == -1) {
    ERROR(-1, DSBMIXER_ERR_FATAL, true, "No mixer devices found: add_mixer(%s)",
          PATH_DEFAULT_MIXER);
  }
  (void)fchdir(cwdfd);
  (void)close(cwdfd);

  return (0);
}

static int add_mixer(const char *name) {
  int i, vol;
  static int id = 1;
  dsbmixer_t mixer;

  /*
   * If we immediately add a mixer after it was plugged in, it can
   * happend that we receive errors if try to open it or execute
   * ioctl()s on it. We must give the device some time and retry
   * if it's not ready yet.
   */
  for (i = 0; i < 3; i++) {
    if ((mixer.fd = open(name, O_RDWR)) == -1) {
      if (errno == EBADF || errno == EINTR)
        warn("add_mixer()->open(%s)", name);
      else {
        ERROR(-1, DSBMIXER_ERR_SYS, false, "Couldn't open mixer %s", name);
      }
      (void)sleep(1);
    } else
      break;
  }
  if (i == 3) {
    ERROR(-1, DSBMIXER_ERR_SYS, false, "Couldn't open mixer %s", name);
  }
  for (i = 0; i < 3; i++) {
    if (ioctl(mixer.fd, SOUND_MIXER_READ_DEVMASK, &mixer.dmask) == -1) {
      if (errno == EBADF) {
        warn(
            "add_mixer()->ioctl"
            "(SOUND_MIXER_READ_DEVMASK");
      } else {
        ERROR(-1, DSBMIXER_ERR_SYS, false, "ioctl(SOUND_MIXER_READ_DEVMASK)");
      }
      (void)sleep(1);
    } else
      break;
  }
  if (i == 3) {
    ERROR(-1, DSBMIXER_ERR_SYS, false, "ioctl(SOUND_MIXER_READ_DEVMASK)");
  }

  /* Count channels */
  mixer.nchans = 0;
  for (i = 0; i < DSBMIXER_MAX_CHANNELS; i++) {
    if ((1 << i) & mixer.dmask) mixer.nchans++;
  }

  for (i = 0; i < 3; i++) {
    if (ioctl(mixer.fd, SOUND_MIXER_READ_RECMASK, &mixer.recmask) == -1) {
      if (errno == EBADF) {
        warn(
            "add_mixer()->ioctl"
            "(SOUND_MIXER_READ_RECMASK");
      } else {
        ERROR(-1, DSBMIXER_ERR_SYS, false, "ioctl(SOUND_MIXER_READ_RECMASK)");
      }
      (void)sleep(1);
    } else
      break;
  }
  if (i == 3) {
    ERROR(-1, DSBMIXER_ERR_SYS, false, "ioctl(SOUND_MIXER_READ_RECMASK)");
  }

  for (i = 0; i < 3; i++) {
    if (ioctl(mixer.fd, SOUND_MIXER_READ_RECSRC, &mixer.recsrc) == -1) {
      if (errno == EBADF) {
        warn(
            "add_mixer()->ioctl"
            "(SOUND_MIXER_READ_RECSRC)");
      } else {
        ERROR(-1, DSBMIXER_ERR_SYS, false, "ioctl(SOUND_MIXER_READ_RECSRC)");
      }
      (void)sleep(1);
    } else
      break;
  }
  if (i == 3) {
    ERROR(-1, DSBMIXER_ERR_SYS, false, "ioctl(SOUND_MIXER_READ_RECSRC)");
  }

  /* Init all non-recording devices. */
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (!((1 << i) & mixer.dmask)) continue;
    mixer.chan[i].name = channel_names[i];
    if (ioctl(mixer.fd, MIXER_READ(i), &vol) != 0) {
      warn("ioctl()");
      continue;
    }
    mixer.chan[i].vol = vol;
    mixer.chan[i].saved_vol = vol;
    mixer.chan[i].muted = vol == 0 ? true : false;
  }
  mixer.id = id++;
  mixer.unit = get_unit(name);
  mixer.ugen = get_ugen(mixer.unit);
  if (mixer.ugen == NULL && (_error & DSBMIXER_ERR_FATAL))
    ERROR(-1, DSBMIXER_ERR_FATAL, true, "get_ugen(%s)", name);
  mixer.cardname = get_cardname(name);
  if (mixer.ugen == NULL && (_error & DSBMIXER_ERR_FATAL))
    ERROR(-1, DSBMIXER_ERR_FATAL, true, "get_cardname(%s)", name);
  mixer.removed = false;
  mixers = realloc(mixers, (nmixers + 1) * sizeof(dsbmixer_t **));
  if (mixers == NULL) ERROR(-1, FATAL_SYSERR, false, "realloc()");
  if ((mixers[nmixers] = malloc(sizeof(dsbmixer_t))) == NULL)
    ERROR(-1, FATAL_SYSERR, false, "malloc()");
  (void)memcpy(mixers[nmixers], &mixer, sizeof(mixer));
  if (strchr(name, '/') != NULL) {
    name = strchr(name, '\0');
    while (*name != '/') name--;
    ++name;
  }
  if ((mixers[nmixers]->name = strdup(name)) == NULL)
    ERROR(-1, FATAL_SYSERR, false, "strdup()");
  read_vol(mixers[nmixers]);
  nmixers++;

  return (0);
}

static int read_vol(dsbmixer_t *mixer) {
  int i, _errno = 0, ret = 0, vol = 0;

  if (mixer == NULL || mixer->removed) {
    errno = EINVAL;
    return (-1);
  }
  mixer->changemask = 0;
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (!(mixer->dmask & (1 << i))) continue;
    if (ioctl(mixer->fd, MIXER_READ(i), &vol) != 0) {
      ret = -1;
      _errno = errno;
      warn("ioctl()");
      continue;
    }
    if (mixer->chan[i].vol != vol) {
      mixer->changemask |= (1 << i);
      mixer->chan[i].saved_vol = mixer->chan[i].vol;
      mixer->chan[i].vol = vol;
      mixer->chan[i].muted = vol == 0 ? true : false;
    }
  }
  errno = _errno;

  return (ret);
}

static int read_recsrc(dsbmixer_t *mixer) {
  int recsrc = 0;

  if (mixer == NULL || mixer->removed) {
    errno = EINVAL;
    return (-1);
  }
  if (ioctl(mixer->fd, SOUND_MIXER_READ_RECSRC, &recsrc) == -1) return (-1);
  mixer->rchangemask = (recsrc ^ mixer->recsrc);
  mixer->recsrc = recsrc;
  return (0);
}

static int set_vol(dsbmixer_t *mixer, int chan, int vol) {
  errno = 0;
  if (!VALIDATE_CHAN(mixer, chan) || !((1 << chan) & mixer->dmask) ||
      (DSBMIXER_CHAN_LEFT(vol) > 100 || DSBMIXER_CHAN_RIGHT(vol) > 100)) {
    errno = EINVAL;
    return (-1);
  }
  if (ioctl(mixer->fd, MIXER_WRITE(chan), &vol) == 0) {
    mixer->chan[chan].vol = vol;
    return (0);
  }
  return (-1);
}

static int set_recsrc(dsbmixer_t *mixer, int mask) {
  int i;

  if (mixer->removed) return (0);
  /* Remove invalid recording sources */
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if ((mask & (1 << i)) && !(mixer->recmask & (1 << i))) mask &= ~(1 << i);
  }
  if (ioctl(mixer->fd, SOUND_MIXER_WRITE_RECSRC, &mask) == -1) {
    ERROR(-1, FATAL_SYSERR, false, "Couldn't add/delete recording source(s)");
  }
  if (ioctl(mixer->fd, SOUND_MIXER_READ_RECSRC, &mixer->recsrc) == -1) {
    ERROR(-1, FATAL_SYSERR, false, "ioctl(SOUND_MIXER_READ_RECSRC)");
  }
  return (0);
}

#ifndef WITHOUT_DEVD
static int uconnect(const char *path) {
  int s, opt = 1;
  struct sockaddr_un saddr = {};

  if ((s = socket(PF_LOCAL, SOCK_SEQPACKET, 0)) == -1) return (-1);
  (void)snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", path);
  saddr.sun_family = AF_LOCAL;
  if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) return (-1);
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
    warn("setsockopt()");
  return (s);
}

static void devd_reconnect(int *sock) {
  (void)close(*sock);
  warnx("Lost connection to devd. Reconnecting ...");
  if ((*sock = devd_connect()) == -1)
    errx(EXIT_FAILURE, "Connecting to devd failed. Giving up.");
  warnx("Connection to devd established");
}

static int devd_connect() {
  int i, s;

  for (i = 0, s = -1; i < 30 && s == -1; i++) {
    if ((s = uconnect(PATH_DEVD_SOCKET)) == -1) (void)sleep(1);
  }
  return (s);
}
#endif

static char *exec_backend(const char *cmd, int *ret) {
  int bufsz, rd, len, pfd[2];
  char *buf;
  pid_t pid;
  errno = 0;
  if ((pid = pipe(pfd)) == -1) return (NULL);
  switch (fork()) {
    case -1:
      return (NULL);
    case 0:
      (void)close(pfd[0]);
      (void)dup2(pfd[1], 1);
      (void)dup2(pfd[1], 2);
      execlp("/bin/sh", "/bin/sh", "-c", cmd, NULL);
      exit(-1);
    default:
      (void)close(pfd[1]);
  }
  bufsz = len = rd = 0;
  buf = NULL;
  do {
    len += rd;
    if (len >= bufsz - 1) {
      if ((buf = realloc(buf, bufsz + 64)) == NULL) return (NULL);
      bufsz += 64;
    }
  } while ((rd = read(pfd[0], buf + len, bufsz - len - 1)) > 0);
  buf[len] = '\0';
  (void)waitpid(pid, ret, WEXITED);
  *ret = WEXITSTATUS(*ret);
  if (*ret == -1) free(buf);
  return (*ret == -1 ? NULL : buf);
}

static const struct _audio_proc_updater_s *lookup_audio_proc_updater(
    const char *cmd) {
  size_t i;

  for (i = 0; i < sizeof(audio_proc_updaters) / sizeof(audio_proc_updaters[0]);
       i++) {
    if (strcmp(cmd, audio_proc_updaters[i].proc) == 0)
      return (&audio_proc_updaters[i]);
  }
  return (NULL);
}

static int run_audio_proc_updater(dsbmixer_audio_proc_t *proc,
                                  const char *update_cmd) {
  int error;
  char *cmd, *q;
  size_t n, len;
  const char *p;

  if (update_cmd == NULL || *update_cmd == '\0') {
    warnx("Empty update command passed");
    return (-1);
  }
  _error = 0;
  len = strlen(update_cmd);
  for (p = update_cmd; *p != '\0'; p++) {
    if (*p == '%') len += 16;
  }
  if ((cmd = malloc(len)) == NULL) ERROR(-1, FATAL_SYSERR, false, "malloc()");
  for (p = update_cmd, q = cmd; *p != '\0'; p++) {
    if (*p == '%') {
      if (p[1] == '%') {
        *q++ = '%';
      } else if (p[1] == 'p') {
        n = snprintf(q, len - (q - cmd), "%d", proc->pid);
        q += n;
      } else if (p[1] == 'u') {
        n = snprintf(q, len - (q - cmd), "%d", dsbmixer_snd_settings.prev_unit);
        q += n;
      } else if (p[1] == 'U') {
        n = snprintf(q, len - (q - cmd), "%d",
                     dsbmixer_snd_settings.default_unit);
        q += n;
      }
      p++;
    } else
      *q++ = *p;
  }
  *q = '\0';
  warnx("Running %s", cmd);
  error = system(cmd);
  free(cmd);

  return (error);
}

static int get_oss_sysinfo(int fd, oss_sysinfo *sysinfo) {
  if (ioctl(fd, SNDCTL_SYSINFO, sysinfo) == 0) return (0);
  if (errno == EINVAL) {
    ERROR(-1, DSBMIXER_ERR_SYS, false,
          "Error: OSS version 4.0 or later required");
  }
  ERROR(-1, DSBMIXER_ERR_SYS, false, "ioctl(SNDCTL_SYSINFO)");
}

static int get_oss_audioinfo(int fd, int devid, oss_audioinfo *info) {
  int ret;

  bzero(info, sizeof(oss_audioinfo));
  info->dev = devid;
  ret = ioctl(fd, SNDCTL_AUDIOINFO, info);
  if (ret != 0) warn("ioctl(SNDCTL_AUDIOINFO)");
  return (ret);
}

static int pid_to_cmd(pid_t pid, char *cmd, size_t cmdsize) {
  FILE *fp;
  bool found;
  char buf[128], fmt[64];

  (void)snprintf(buf, sizeof(buf), "%s -x", PATH_PS);
  if ((fp = popen(buf, "r")) == NULL)
    ERROR(-1, FATAL_SYSERR, false, "popen(%s)", buf);
  (void)snprintf(fmt, sizeof(fmt), "%d %%*s %%*s %%*s %%%lus", pid, cmdsize);
  found = false;
  while (!found && fgets(buf, sizeof(buf), fp) != NULL) {
    if (sscanf(buf, fmt, cmd) == 1) found = true;
  }
  (void)fclose(fp);

  return (found ? 0 : -1);
}

static int add_app_chans(dsbappsmixer_t *am) {
  int i, n, fd;
  oss_sysinfo sysinfo;
  oss_audioinfo info;

  (void)memset(&info, 0, sizeof(info));
  if ((fd = open(PATH_DEFAULT_MIXER, O_RDONLY)) == -1) {
    ERROR(-1, DSBMIXER_ERR_SYS, false, "open(%s)", PATH_DEFAULT_MIXER);
  }
  if (get_oss_sysinfo(fd, &sysinfo) == -1) goto error;
  am->nchans = 0;
  for (i = n = 0; i < sysinfo.numaudios; i++) {
    if (n >= DSBAPPSMIXER_MAX_CHANS) continue;
    if (!((1 << i) & sysinfo.openedaudio[0])) continue;
    if (get_oss_audioinfo(fd, i, &info) == -1) continue;
    if (info.pid <= 0) continue;
    if (!(info.caps & DSP_CAP_OUTPUT)) continue;
    am->chan[n] = create_appchannel(info.devnode, info.pid);
    if (am->chan[n] == NULL) goto error;
    am->nchans = ++n;
  }
  if (n >= DSBAPPSMIXER_MAX_CHANS) {
    warnx("# of max. app channels (%d) exceeded.", DSBAPPSMIXER_MAX_CHANS);
  }
  (void)close(fd);

  return (0);
error:
  (void)close(fd);
  return (-1);
}

static void destroy_appchannel(dsbappsmixer_channel_t *chan) {
  free(chan->name);
  free(chan->dev);
  free(chan);
}

static int read_appvol(const char *dev) {
  int fd, ret, vol;

  if ((fd = open(dev, O_RDONLY, 0)) == -1) {
    warn("open(%s)", dev);
    return (-1);
  }
  if ((ret = ioctl(fd, MIXER_READ(SOUND_MIXER_PCM), &vol)) == -1)
    warn("ioctl(%s, MIXER_READ(SOUND_MIXER_PCM))", dev);
  (void)close(fd);

  return (ret == -1 ? -1 : vol);
}
