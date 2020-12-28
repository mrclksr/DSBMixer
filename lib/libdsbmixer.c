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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <err.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/soundcard.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <regex.h>

#include "libdsbmixer.h"

#define FATAL_SYSERR	   (DSBMIXER_ERR_SYS|DSBMIXER_ERR_FATAL)
#define PATH_SNDSTAT	   "/dev/sndstat"
#define PATH_DEVDSOCKET	   "/var/run/devd.pipe"
#define PATH_DEFAULT_MIXER "/dev/mixer"
#define PATH_FSTAT	   "/usr/bin/fstat"

#define RE_STR \
	"([^[:blank:]]+)[[:space:]]+([^[:space:]]+)" \
	"[[:space:]]+([0-9]+).*dsp[0-9].*"

#define ERROR(ret, error, prepend, fmt, ...) \
	do { \
		set_error(error, prepend, fmt, ##__VA_ARGS__); \
		return (ret); \
	} while (0)

struct dsbmixer_snd_settings_s dsbmixer_snd_settings;

#ifndef WITHOUT_DEVD
/*
 * Struct to represent the fields of a devd notify event.
 */
static struct devdevent_s {
	char *ugen;	   /* ugen device name. */
	char *system;      /* Bus or DEVFS */
	char *subsystem;   /* INTERFACE, CDEV */
	char *type;        /* Event type: CREATE, DESTROY, DETACH. */
	char *cdev;	   /* Device name. */
} devdevent;

typedef struct devqueue_item_s {
	int	   state;  /* NEW, REMOVED */
	dsbmixer_t *mixer;
} devqueue_item_t;

typedef struct mqmsg_s {
	long mtype;
	char mdata[sizeof(devqueue_item_t)];
} mqmsg_t;
#endif

static struct restart_cmd_s {
	const char *cmd;
	const char *restart;
} restart_cmds[] = {
	{ "pulseaudio", PATH_RESTART_PA },
	{ "sndiod",     "kill -HUP %p"  }
};

static int  get_unit(const char *mixer);
static int  get_mixers(void);
static int  add_mixer(const char *name);
static int  set_recsrc(dsbmixer_t *mixer, int mask);
static char *get_cardname(const char *mixer);
static char *get_ugen(int pcmunit);
static char *exec_backend(const char *cmd, int *ret);
#ifndef WITHOUT_DEVD
static FILE *uconnect(const char *path);
static void *devd_watcher(void *unused);
static void parse_devd_event(char *str);
#endif
static void set_error(int, bool, const char *fmt, ...);
static void get_snd_settings(void);
static void read_vol(dsbmixer_t *mixer);
static void read_recsrc(dsbmixer_t *mixer);
static void set_vol(dsbmixer_t *mixer, int dev, int vol);

static int	  _error = 0, nmixers = 0;
#ifndef WITHOUT_DEVD
static int	  mqid;
static FILE	  *devdfp;
#endif
static char 	  errormsg[_POSIX2_LINE_MAX];
static dsbmixer_t **mixers = NULL;
static const char *channel_names[] = SOUND_DEVICE_LABELS;
static const char *lookup_restart_cmd(const char *);

int
dsbmixer_init()
{
	get_snd_settings();
#ifndef WITHOUT_DEVD
	int	  n;
	pthread_t thr;

	for (n = 0, devdfp = NULL; devdfp == NULL && n < 10; n++) {
		if ((devdfp = uconnect(PATH_DEVDSOCKET)) == NULL)
			(void)sleep(1);
	}
	if (devdfp == NULL) {
		ERROR(-1, DSBMIXER_ERR_FATAL, true,
		    "Couldn't connect to devd");
	}
	if ((mqid = msgget(IPC_PRIVATE, S_IRWXU)) == -1)
		ERROR(-1, FATAL_SYSERR, false, "msgget()");
	if (pthread_create(&thr, NULL, &devd_watcher, NULL) == -1)
		ERROR(-1, FATAL_SYSERR, false, "pthread_create()");
#endif
	if (get_mixers() == -1)
		ERROR(-1, 0, true, "get_mixers()");
	return (0);
}

void
dsbmixer_cleanup()
{
#ifndef WITHOUT_DEVD
	msgctl(mqid, IPC_RMID, NULL);
	mqid = -1;
#endif
}

int
dsbmixer_default_unit()
{
	return (dsbmixer_snd_settings.default_unit);
}

int
dsbmixer_amplification()
{
	return (dsbmixer_snd_settings.amplify);
}


int
dsbmixer_feeder_rate_quality()
{
	return (dsbmixer_snd_settings.feeder_rate_quality);
}

int
dsbmixer_maxautovchans()
{
	return (dsbmixer_snd_settings.maxautovchans);
}

int
dsbmixer_latency()
{
	return (dsbmixer_snd_settings.latency);
}

bool
dsbmixer_bypass_mixer()
{
	return (dsbmixer_snd_settings.mixer_bypass);
}

void
dsbmixer_setvol(dsbmixer_t *mixer, int chan, int vol)
{
	if (mixer == NULL || mixer->removed)
		return;
	set_vol(mixer, chan, vol);
}

void
dsbmixer_setlvol(dsbmixer_t *mixer, int chan, int lvol)
{
	if (mixer == NULL || mixer->removed)
		return;
	set_vol(mixer, chan,
	    DSBMIXER_CHAN_CONCAT(lvol,
	    DSBMIXER_CHAN_RIGHT(mixer->chan[chan].vol)));
}

void
dsbmixer_setrvol(dsbmixer_t *mixer, int chan, int rvol)
{
	if (mixer == NULL || mixer->removed)
		return;
	set_vol(mixer, chan,
	    DSBMIXER_CHAN_CONCAT(DSBMIXER_CHAN_LEFT(mixer->chan[chan].vol),
	    rvol));
}

int
dsbmixer_getvol(dsbmixer_t *mixer, int chan)
{
	if (mixer == NULL || chan < 0 || chan > SOUND_MIXER_NRDEVICES)
		return (0);
	return (mixer->chan[chan].vol);
}

bool
dsbmixer_canrec(dsbmixer_t *mixer, int chan)
{
	return ((mixer->recmask & (1 << chan)) ? true : false);
}

int
dsbmixer_setrec(dsbmixer_t *mixer, int chan, bool on)
{
	int mask;

	if (mixer == NULL || mixer->removed)
		return (-1);
	if (on)
		mask = (1 << chan);
	else
		mask =~(1 << chan);
	return (set_recsrc(mixer, mask));
}

bool
dsbmixer_getrec(dsbmixer_t *mixer, int chan)
{
	if (mixer == NULL || chan < 0 || chan > SOUND_MIXER_NRDEVICES)
		return (false);
	return (((1 << chan) & mixer->recsrc) ? true : false);
}

void
dsbmixer_setmute(dsbmixer_t *mixer, bool mute)
{
	if (!mute && mixer->mute) {
		set_vol(mixer, DSBMIXER_MASTER, mixer->saved_vol);
		mixer->mute = false;
	} else if (mute && !mixer->mute) {
		mixer->saved_vol = mixer->chan[DSBMIXER_MASTER].vol;
		set_vol(mixer, DSBMIXER_MASTER, 0);
		mixer->mute = true;	
	}
}

bool
dsbmixer_getmute(dsbmixer_t *mixer)
{
	read_vol(mixer);
	if (mixer->chan[DSBMIXER_MASTER].vol != 0)
		mixer->mute = false;
	else
		mixer->mute = true;
	return (mixer->mute);
}

int
dsbmixer_geterr(char const **errmsg)
{
	if (!_error)
		return (0);
	*errmsg = errormsg;
	return (_error);
}

int
dsbmixer_getndevs()
{
	return (nmixers);
}

int
dsbmixer_getnchans(dsbmixer_t *mixer)
{
	return (mixer->nchans);
}

int
dsbmixer_getchanid(dsbmixer_t *mixer, int index)
{
	int i, n;

	for (i = n = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (((1 << i) & mixer->dmask) && n++ == index)
			return (i);
	}
	return (-1);
}

const char *
dsbmixer_getchaname(dsbmixer_t *mixer, int chan)
{
	if (chan < 0 || chan > SOUND_MIXER_NRDEVICES)
		return (NULL);
	if ((1 << chan) & mixer->dmask)
		return (mixer->chan[chan].name);
	return (NULL);
}
const char **
dsbmixer_getchanames()
{
	return (channel_names);
}


dsbmixer_t *
dsbmixer_getmixerbyid(int id)
{
	int i;
	
	for (i = 0; i < nmixers; i++) {
		if (mixers[i]->id == id)
			return (mixers[i]);
	}
	return (NULL);
}

dsbmixer_t *
dsbmixer_getmixer(int index)
{
	if (index < 0 || index >= nmixers)
		return (NULL);
	return (mixers[index]);
}

dsbmixer_channel_t *
dsbmixer_getchan(dsbmixer_t *mixer, int chan)
{
	if (chan < 0 || chan > SOUND_MIXER_NRDEVICES)
		return (NULL);
	if ((1 << chan) & mixer->dmask)
		return (&mixer->chan[chan]);
	return (NULL);
}

void
dsbmixer_delmixer(dsbmixer_t *mixer)
{
	int i;

	for (i = 0; i < nmixers && mixer != mixers[i]; i++)
		;
	if (i < nmixers) {
		if (mixers[i]->fd > 0)
			(void)close(mixers[i]->fd);
		free(mixers[i]->name);
		free(mixers[i]);
		for (; i < nmixers - 1; i++)
			mixers[i] = mixers[i + 1];
		nmixers--;
	}
}

dsbmixer_t *
dsbmixer_pollmixers()
{
	static int i, next = 0;

	if (next >= nmixers)
		next = 0;
	for (i = next; i < nmixers; i++) {
		if (mixers[i]->removed)
			continue;
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
dsbmixer_t *
dsbmixer_querydevlist(int *state, bool block)
{
	int		ec;
	mqmsg_t		msg;
	devqueue_item_t *dqi;

	if (mqid == -1) {
		/* Message queue was removed (dsbmixer_cleanup()) */
		if (!block)
			return (NULL);
		/* Just wait for process termination */
		for (;; sleep(1))
			;
	}
	if (block)
		ec = msgrcv(mqid, &msg, sizeof(devqueue_item_t), 0, 0);
	else
		ec = msgrcv(mqid, &msg, sizeof(devqueue_item_t), 0, IPC_NOWAIT);
	if (ec == -1)
		return (NULL);
	dqi = (devqueue_item_t *)msg.mdata;

	*state = dqi->state;
	return (dqi->mixer);
	
}
#endif /* !WITHOUT_DEVD */

int
dsbmixer_poll_default_unit(void)
{
	int    unit;
	size_t sz = sizeof(int);

	unit = 0;
	if (sysctlbyname("hw.snd.default_unit", &unit, &sz, NULL, 0))
		warn("sysctl(hw.snd.default_unit)");
	dsbmixer_snd_settings.default_unit = unit;

	return (unit);
}

int
dsbmixer_set_default_unit(int unit)
{
	if (sysctlbyname("hw.snd.default_unit", NULL, NULL,
	    &unit, sizeof(unit)) != 0) {
		warn("Couldn't set hw.snd.default_unit");
		return (-1);
	}
	dsbmixer_snd_settings.default_unit = unit;

	return (0);
}

int
dsbmixer_change_settings(int dfltunit, int amp, int qual, int latency,
	int max_auto_vchans, bool bypass_mixer)
{
	int  error = 0;
	char cmd[512], *output;

	(void)snprintf(cmd, sizeof(cmd),
	    "%s -u %d -d -a %d -q %d -l %d -v %d -b %d",
	    PATH_BACKEND, dfltunit, amp, qual, latency, max_auto_vchans,
	    bypass_mixer);
	if ((output = exec_backend(cmd, &error)) == NULL) {
		set_error(0, true, "exec_backend(%s)", cmd);
		free(output);
		return (-1);
	} else if (error != 0) {
		set_error(DSBMIXER_ERR_FATAL, true,
		    "Failed to execute %s: %s\n", cmd, output);
		free(output);
		return (-1);
	}
	get_snd_settings();

	return (0);
}

int
dsbmixer_restart_audio_proc(dsbmixer_audio_proc_t *proc)
{
	int	   error;
	char	   *cmd, *q;
	size_t	   n, len;
	const char *p;

	_error = 0;
	len = strlen(proc->restart);
	for (p = proc->restart; *p != '\0'; p++) {
		if (*p == '%')
			len += 16;
	}
	if ((cmd = malloc(len)) == NULL)
		ERROR(-1, FATAL_SYSERR, false, "malloc()");
	for (p = proc->restart, q = cmd; *p != '\0'; p++) {
		if (*p == '%') {
			if (p[1] == '%') {
				*q++ = '%';
			} else if (p[1] == 'p') {
				n = snprintf(q, len - (q - cmd), "%d",
				    proc->pid);
				q += n;
			}
			p++;
		} else
			*q++ = *p;
	}
	*q = '\0';
	error = system(cmd);
	free(cmd);

	return (error);
}

dsbmixer_audio_proc_t *
dsbmixer_get_audio_procs(size_t *_nprocs)
{
	FILE	      *fp;
	char	      buf[_POSIX2_LINE_MAX], pwdbuf[512], *cmd, *pid, *usr;
	size_t	      nap, nprocs;
	regex_t	      re;
	regmatch_t    pmatch[4];
	const char    *rcmd;
	struct passwd *pwdres, pwd;
	dsbmixer_audio_proc_t *ap;

	_error = 0;
	if (getpwuid_r(getuid(), &pwd, pwdbuf, sizeof(pwdbuf), &pwdres) != 0)
		ERROR(NULL, FATAL_SYSERR, false, "getpwuid_r()");
	endpwent();
	if (regcomp(&re, RE_STR, REG_EXTENDED) != 0) {
		warn("regcomp()");
		return (NULL);
	}
	if ((fp = popen(PATH_FSTAT, "r")) == NULL)
		ERROR(NULL, FATAL_SYSERR, false, "popen(%s)", PATH_FSTAT);
	ap = NULL;
	nap = nprocs = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (regexec(&re, buf, 4, pmatch, 0) != 0)
			continue;
		buf[pmatch[1].rm_eo] = '\0'; usr = buf + pmatch[1].rm_so;
		buf[pmatch[2].rm_eo] = '\0'; cmd = buf + pmatch[2].rm_so;
		buf[pmatch[3].rm_eo] = '\0'; pid = buf + pmatch[3].rm_so;
		if (strcmp(usr, pwd.pw_name) != 0)
			continue;
		if ((rcmd = lookup_restart_cmd(cmd)) == NULL)
			continue;
		if (++nprocs >= nap) {
			nap += 5;
			ap = realloc(ap, nap * sizeof(dsbmixer_audio_proc_t));
			if (ap == NULL)
				ERROR(NULL, FATAL_SYSERR, false, "realloc()");
		}
		(void)strncpy(ap[nprocs - 1].cmd, cmd,
		    sizeof(ap[nprocs - 1].cmd));
		ap[nprocs - 1].pid = strtol(pid, NULL, 10);
		ap[nprocs - 1].restart = rcmd;
	}
	*_nprocs = nprocs;
	regfree(&re);
	(void)pclose(fp);

	return (ap);
}

static void
printerr()
{
	(void)fprintf(stderr, "%s\n", errormsg);
}

static void
set_error(int error, bool prepend, const char *fmt, ...)
{
	int	_errno;
	char	errbuf[sizeof(errormsg)];
	size_t  len;
	va_list ap;

	_errno = errno;

	va_start(ap, fmt);
	if (prepend) {
		_error |= error;
		if (error & DSBMIXER_ERR_FATAL) {
			if (strncmp(errormsg, "Fatal: ", 7) == 0) {
				memmove(errormsg, errormsg + 7,
				    strlen(errormsg) - 6);
			}
			(void)strncpy(errbuf, "Fatal: ", sizeof(errbuf) - 1);
			len = strlen(errbuf);
		} else
			len = 0;
		(void)vsnprintf(errbuf + len, sizeof(errbuf) - len, fmt, ap);

		len = strlen(errbuf);
		(void)snprintf(errbuf + len, sizeof(errbuf) - len, ":%s",
		    errormsg);
		(void)strcpy(errormsg, errbuf);
	} else {
		_error = error;
		(void)vsnprintf(errormsg, sizeof(errormsg), fmt, ap);
		if (error == DSBMIXER_ERR_FATAL) {
			(void)snprintf(errbuf, sizeof(errbuf), "Fatal: %s",
			    errormsg);
			(void)strcpy(errormsg, errbuf);
		}
		
	}
	if ((error & DSBMIXER_ERR_SYS) && _errno != 0) {
		len = strlen(errormsg);
		(void)snprintf(errormsg + len, sizeof(errormsg) - len, ":%s",
		    strerror(_errno));
		errno = 0;
	}
}

const char *
dsbmixer_error()
{
	return (errormsg);
}

#define EXPAND(F) dsbmixer_snd_settings.F

static void
get_snd_settings()
{
	size_t sz, i;
	struct oid_s {
		const char *oid;
		void *val;
	} oids[] = {
	    { "hw.snd.default_unit",	    &EXPAND(default_unit)	 },
	    { "hw.snd.vpc_0db",		    &EXPAND(amplify)		 },
	    { "hw.snd.feeder_rate_quality", &EXPAND(feeder_rate_quality) },
	    { "hw.snd.vpc_mixer_bypass",    &EXPAND(mixer_bypass)	 },
	    { "hw.snd.maxautovchans",	    &EXPAND(maxautovchans)	 },
	    { "hw.snd.latency",		    &EXPAND(latency)		 }
	};
	for (i = 0; i < sizeof(oids) / sizeof(struct oid_s); i++) {
		sz = sizeof(int);
		if (sysctlbyname(oids[i].oid, oids[i].val, &sz, NULL, 0))
			warn("sysctl(%s)", oids[i].oid);
	}
}

#ifndef WITHOUT_DEVD
static void
parse_devd_event(char *str)
{
	char *p, *q;

	devdevent.cdev	 = "";
	devdevent.system = devdevent.subsystem = devdevent.type = "";

	for (p = str; (p = strtok(p, " \n")) != NULL; p = NULL) {
		if ((q = strchr(p, '=')) == NULL)
			continue;
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

static void *
devd_watcher(void *unused)
{
	fd_set allset, rset;
	static int     i;
	static char    ln[_POSIX2_LINE_MAX];
	static char    path[sizeof(_PATH_DEV) + sizeof("mixer") + 2];
	static mqmsg_t msg;
	static devqueue_item_t item;

	/* Silence warning about unused parameter */
	(void)unused;

	FD_ZERO(&allset);
       	FD_SET(fileno(devdfp), &allset);

	for (;;) {
		rset = allset;
		if (select(fileno(devdfp) + 1, &rset, NULL, NULL, NULL) < 0)
			continue;
		if (fgets(ln, sizeof(ln), devdfp) == NULL) {
			if (!feof(devdfp))
				continue;
			/* Lost connection */
			(void)fclose(devdfp);
			while ((devdfp = uconnect(PATH_DEVDSOCKET)) == NULL)
				(void)sleep(1);
			FD_ZERO(&allset);
			FD_SET(fileno(devdfp), &allset);
			continue;
		}
		if (ln[0] != '!')
			continue;
		parse_devd_event(ln + 1);
		if (strcmp(devdevent.type, "CREATE") == 0    &&
		    strcmp(devdevent.subsystem, "CDEV") == 0 &&
		    strncmp(devdevent.cdev, "mixer", 5) == 0) {
			(void)snprintf(path, sizeof(path), "%s%s", _PATH_DEV,
			    devdevent.cdev);
			if (add_mixer(path) == -1)
				continue;
			item.state = DSBMIXER_STATE_NEW;
			item.mixer = mixers[nmixers - 1];
			msg.mtype = 1;
			memcpy(msg.mdata, &item, sizeof(item));
			msgsnd(mqid, &msg, sizeof(devqueue_item_t), 0);
		} else if (strcmp(devdevent.system, "USB") == 0 &&
		    strcmp(devdevent.type, "DETACH") == 0) {
			for (i = 0; i < nmixers; i++) {
				if (mixers[i]->removed)
					continue;
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
					item.mixer = mixers[nmixers - 1];
					msg.mtype = 1;
					memcpy(msg.mdata, &item, sizeof(item));
					msgsnd(mqid, &msg,
					    sizeof(devqueue_item_t), 0);
				}
			}
		}
	}
}
#endif	/* !WITHOUT_DEVD */

static int
get_unit(const char *mixer)
{
	const char *p = strchr(mixer, '\0');

	while (isdigit(*--p))
		;
	if (!isdigit(*++p))
		return (0);
	else
		return (strtol(p, NULL, 10));
}

static char *
get_cardname(const char *mixer)
{
	int  unit;
	FILE *fp;
	char *name, ln[64], pattern[8];

	unit = get_unit(mixer);

	(void)snprintf(pattern, sizeof(pattern), "pcm%d:", unit);
	if ((fp = fopen(PATH_SNDSTAT, "r")) == NULL) {
		warn("get_cardname()->fopen(%s)", PATH_SNDSTAT);
		if ((name = malloc(7)) == NULL)
			ERROR(NULL, FATAL_SYSERR, false, "malloc()");
		(void)snprintf(name, 7, "pcm%d", unit);
		return (name);
	}
	for (name = NULL; fgets(ln, sizeof(ln), fp) != NULL;) {
		(void)strtok(ln, "\r\n");
		if (strlen(ln) > 8 &&
		    strcmp(&ln[strlen(ln) - 8], " default") == 0)
			ln[strlen(ln) - 8] = '\0';
		if (strncmp(ln, pattern, strlen(pattern)) == 0) {
			if ((name = strdup(ln)) == NULL)
				ERROR(NULL, FATAL_SYSERR, false, "strdup()");
			break;
		}
	}
	(void)fclose(fp);
	if (name == NULL) {
		if ((name = malloc(7)) == NULL)
			ERROR(NULL, FATAL_SYSERR, false, "malloc()");
		(void)snprintf(name, 7, "pcm%d", unit);
	}
	return (name);
}

static char *
get_ugen(int pcmunit)
{
	int	    i, unit;
	size_t	    sz;
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
	if (i <= 0)
		return (NULL);
	unit = strtol(&buf[i + 1], NULL, 10); buf[i + 1] = '\0';
	(void)snprintf(query, sizeof(query), "dev.%s.%d.%%location",
	    buf, unit);
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

static int
get_mixers()
{
	int	      cwdfd;
	DIR           *dirp;
	struct stat    sb;
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
		if (strncmp(dp->d_name, "mixer", 5) != 0)
			continue;
		if (!isdigit(dp->d_name[5]))
			continue;
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
				ERROR(-1, DSBMIXER_ERR_FATAL, true,
				    "add_mixer(%s)", dp->d_name);
			} else if (_error & DSBMIXER_ERR_SYS) {
				set_error(0, true, "add_mixer(%s)",
				    dp->d_name); 
				printerr();
			}
		}
	}
	closedir(dirp);
	if (nmixers == 0 && add_mixer(PATH_DEFAULT_MIXER) == -1) {
		ERROR(-1, DSBMIXER_ERR_FATAL, true,
		    "No mixer devices found: add_mixer(%s)",
		    PATH_DEFAULT_MIXER);
	}
	(void)fchdir(cwdfd);
	(void)close(cwdfd);

	return (0);
}

static int
add_mixer(const char *name)
{
	int	   i;
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
				ERROR(-1, DSBMIXER_ERR_SYS, false,
				    "Couldn't open mixer %s", name);
			}
			(void)sleep(1);
		} else
			break;
	}
	if (i == 3) {
		ERROR(-1, DSBMIXER_ERR_SYS, false, "Couldn't open mixer %s",
		    name);
	}
	for (i = 0; i < 3; i++) {
		if (ioctl(mixer.fd, SOUND_MIXER_READ_DEVMASK,
		    &mixer.dmask) == -1) {
			if (errno == EBADF) {
				warn("add_mixer()->ioctl" \
				    "(SOUND_MIXER_READ_DEVMASK");
			} else {
				ERROR(-1, DSBMIXER_ERR_SYS, false,
				    "ioctl(SOUND_MIXER_READ_DEVMASK)");
			}
			(void)sleep(1);
		} else
			break;
	}
	if (i == 3) {
		ERROR(-1, DSBMIXER_ERR_SYS, false,
		    "ioctl(SOUND_MIXER_READ_DEVMASK)");
	}

	/* Count channels */
	mixer.nchans = 0;
	for (i = 0; i < DSBMIXER_MAX_CHANNELS; i++) {
		if ((1 << i) & mixer.dmask)
			mixer.nchans++;
	}

	for (i = 0; i < 3; i++) {
		if (ioctl(mixer.fd, SOUND_MIXER_READ_RECMASK,
		    &mixer.recmask) == -1) {
			if (errno == EBADF) {
				warn("add_mixer()->ioctl" \
				    "(SOUND_MIXER_READ_RECMASK");
			} else {
				ERROR(-1, DSBMIXER_ERR_SYS, false,
				    "ioctl(SOUND_MIXER_READ_RECMASK)");
			}
			(void)sleep(1);
		} else
			break;
	}
	if (i == 3) {
		ERROR(-1, DSBMIXER_ERR_SYS, false,
		    "ioctl(SOUND_MIXER_READ_RECMASK)");
	}

	for (i = 0; i < 3; i++) {
		if (ioctl(mixer.fd, SOUND_MIXER_READ_RECSRC,
		    &mixer.recsrc) == -1) {
			if (errno == EBADF) {
				warn("add_mixer()->ioctl" \
				    "(SOUND_MIXER_READ_RECSRC)");
			} else {
				ERROR(-1, DSBMIXER_ERR_SYS, false,
				    "ioctl(SOUND_MIXER_READ_RECSRC)");
			}
			(void)sleep(1);
		} else
			break;
	}
	if (i == 3) {
		ERROR(-1, DSBMIXER_ERR_SYS, false,
		    "ioctl(SOUND_MIXER_READ_RECSRC)");
	}

	/* Get all non-recording devices. */
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (((1 << i) & mixer.dmask))
			mixer.chan[i].name = channel_names[i];
	}
	mixer.id       = id++;
	mixer.unit     = get_unit(name);
	mixer.ugen     = get_ugen(mixer.unit);
	if (mixer.ugen == NULL && (_error & DSBMIXER_ERR_FATAL))
		ERROR(-1, DSBMIXER_ERR_FATAL, true, "get_ugen(%s)", name);
	mixer.cardname = get_cardname(name);
	if (mixer.ugen == NULL && (_error & DSBMIXER_ERR_FATAL))
		ERROR(-1, DSBMIXER_ERR_FATAL, true, "get_cardname(%s)", name);
	mixer.removed  = false;
	mixer.mute     = mixer.chan[DSBMIXER_MASTER].vol == 0 ? true : false;
	mixer.saved_vol = mixer.chan[DSBMIXER_MASTER].vol;
	mixers = realloc(mixers, (nmixers + 1) * sizeof(dsbmixer_t **));
	if (mixers == NULL)
		ERROR(-1, FATAL_SYSERR, false, "realloc()");
	if ((mixers[nmixers] = malloc(sizeof(dsbmixer_t))) == NULL)
		ERROR(-1, FATAL_SYSERR, false, "malloc()");
	(void)memcpy(mixers[nmixers], &mixer, sizeof(mixer));
	if (strchr(name, '/') != NULL) {
		name = strchr(name, '\0');
		while (*name != '/')
			name--;
		++name;
	}
	if ((mixers[nmixers]->name = strdup(name)) == NULL)
		ERROR(-1, FATAL_SYSERR, false, "strdup()");
	read_vol(mixers[nmixers]);
	nmixers++;

	return (0);
}


static void
read_vol(dsbmixer_t *mixer)
{
	int i, vol = 0;

	if (mixer->removed)
		return;
	mixer->changemask = 0;
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (mixer->removed)
			continue;
		if (!(mixer->dmask & (1 << i)))
			continue;

		if (ioctl(mixer->fd, MIXER_READ(i), &vol) != 0) {
			warn("ioctl()");
			continue;
		}
		if (mixer->chan[i].vol != vol) {
			mixer->changemask |= (1 << i);
			mixer->chan[i].vol = vol;
			if (i == DSBMIXER_MASTER && vol > 0)
				mixer->mute = false;
		}
	}
}

static void
read_recsrc(dsbmixer_t *mixer)
{
	int recsrc = 0;
	
	if (mixer->removed)
		return;
	(void)ioctl(mixer->fd, SOUND_MIXER_READ_RECSRC, &recsrc);
	mixer->rchangemask = (recsrc ^ mixer->recsrc);
	mixer->recsrc = recsrc;
}

static void
set_vol(dsbmixer_t *mixer, int chan, int vol)
{

	if (mixer->removed || chan < 0 || chan > SOUND_MIXER_NRDEVICES ||
	    !((1 << chan) & mixer->dmask))
		return;
	if (DSBMIXER_CHAN_LEFT(vol) > 100 || DSBMIXER_CHAN_RIGHT(vol) > 100)
		return;
	if (ioctl(mixer->fd, MIXER_WRITE(chan), &vol) == 0) {
		mixer->chan[chan].vol = vol;
	}
}

static int
set_recsrc(dsbmixer_t *mixer, int mask)
{
	int i;

	if (mixer->removed)
		return (0);
	/* Remove invalid recording sources */
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if ((mask & (1 << i)) && !(mixer->recmask & (1 << i)))
			mask &= ~(1 << i);
	}
	if (ioctl(mixer->fd, SOUND_MIXER_WRITE_RECSRC, &mask) == -1) {
		ERROR(-1, FATAL_SYSERR, false,
		    "Couldn't add/delete recording source(s)");
	}
	if (ioctl(mixer->fd, SOUND_MIXER_READ_RECSRC, &mixer->recsrc) == -1) {
		ERROR(-1, FATAL_SYSERR, false,
		    "ioctl(SOUND_MIXER_READ_RECSRC)");
	}
	return (0);
}

#ifndef WITHOUT_DEVD
static FILE *
uconnect(const char *path)
{
	int  s;
	FILE *sp;
	struct sockaddr_un saddr;

	if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
		ERROR(NULL, DSBMIXER_ERR_SYS, false, "socket()");
	(void)memset(&saddr, (unsigned char)0, sizeof(saddr));
	(void)snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", path);
	saddr.sun_family = AF_LOCAL;
	if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
		ERROR(NULL, DSBMIXER_ERR_SYS, false, "connect(%s)", path);
	if ((sp = fdopen(s, "r+")) == NULL)
		ERROR(NULL, DSBMIXER_ERR_SYS, false, "fdopen()");
	/* Make the stream line buffered, and the socket non-blocking. */
	if (setvbuf(sp, NULL, _IOLBF, 0) == -1 ||
	    fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK) == -1)
		ERROR(NULL, DSBMIXER_ERR_SYS, false, "setvbuf()/fcntl()");
	return (sp);
}
#endif

static char *
exec_backend(const char *cmd, int *ret)
{
	int    bufsz, rd, len, pfd[2];
	char  *buf;
	pid_t  pid;

	errno = 0;
	if ((pid = pipe(pfd)) == -1)
		return (NULL);
	switch (vfork()) {
	case -1:
		return (NULL);
	case  0:
		(void)close(pfd[0]);
		(void)dup2(pfd[1], 1);
		(void)dup2(pfd[1], 2);
		execlp("/bin/sh", "/bin/sh", "-c", cmd, NULL);
		exit(-1);
	default:
		(void)close(pfd[1]);
	}
	bufsz = len = rd = 0; buf = NULL;
	do {
		len += rd;
		if (len >= bufsz - 1) {
			if ((buf = realloc(buf, bufsz + 64)) == NULL)
				return (NULL);
			bufsz += 64;
		}
	} while ((rd = read(pfd[0], buf + len, bufsz - len - 1)) > 0);
	buf[len] = '\0';
	(void)waitpid(pid, ret, WEXITED);
	*ret = WEXITSTATUS(*ret);
	if (*ret == -1)
		free(buf);
	return (*ret == -1 ? NULL : buf);
}

static const char *
lookup_restart_cmd(const char *cmd)
{
	size_t i;

	for (i = 0; i < sizeof(restart_cmds) / sizeof(restart_cmds[0]); i++) {
		if (strcmp(cmd, restart_cmds[i].cmd) == 0)
			return (restart_cmds[i].restart);
	}
	return (NULL);
}
