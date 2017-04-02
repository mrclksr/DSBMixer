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
#include <signal.h>
#include <dirent.h>
#ifdef WITH_GETTEXT
# include <libintl.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include "gtk-helper/gtk-helper.h"
#include "dsbcfg/dsbcfg.h"

#define INTCLASS_AUDIO	    0x01
#define PATH_DEVDSOCKET	    "/var/run/devd.pipe"
#define PATH_DEFAULT_MIXER  "/dev/mixer"
#define PATH_SNDSTAT	    "/dev/sndstat"
#define TITLE		    "DSBMixer"
#define PATH_CONFIG	    "config"

#ifdef WITH_DEVD
/*
 * Struct to represent the fields of a devd notify event.
 */
struct devdevent_s {
	int  intclass;	 /* Interface class. */
	char *ugen;	 /* ugen device name. */
	char *system;    /* Bus or DEVFS */
	char *subsystem; /* INTERFACE, CDEV */
	char *type;      /* Event type: CREATE, DESTROY, DETACH. */
	char *cdev;	 /* Device name. */
} devdevent;
#endif

static struct snd_settings {
	int default_unit;
	int amplify;
	int feeder_rate_quality;
} snd_settings;

typedef struct channel_s {
	int	   vol;
	char const *name;	 /* Vol, Pcm, Mic, etc. */
} channel_t;

typedef struct mixer_s {
	int	  fd;		 /* File descriptor of mixer */
	int	  recsrc;	 /* Bitmask of current recording sources. */
	int	  recmask;	 /* Bitmask of all rec. devices/channels. */
	int	  dmask;	 /* Bitmask of all channels. */
	char	  *name;	 /* Device name of mixer. */
	char	  *cardname;	 /* Name of soundcard. */
	channel_t chan[SOUND_MIXER_NRDEVICES];
} mixer_t;

/*
 * Struct for the settings menu.
 */
struct settings_s {
	GtkWidget *show_chan_cb[SOUND_MIXER_NRDEVICES];
} settings;

/*
 * Struct to represent and control a mixer in the GUI.
 */
typedef struct mixer_tab_s {
	mixer_t	  *mixer;
	GtkVScale *chan[SOUND_MIXER_NRDEVICES];
	GtkWidget *recsrc_cb[SOUND_MIXER_NRDEVICES];
} mixer_tab_t;

struct mainwin_s {
	int	       *posx;
	int	       *posy;
	int	       *width;
	int	       *height;
	int	       nmixer_tabs;
	GtkWindow      *win;
	GtkWidget      *notebook;
	mixer_tab_t    **mixer_tabs;
	GtkStatusIcon  *tray_icon;
	GdkWindowState win_state;
} mainwin;

static int	  nmixers = 0, all_chan_mask = 0;
#ifdef WITH_DEVD
static FILE	  *devdfp;
#endif
static mixer_t	  **mixers = NULL;
static dsbcfg_t	  *cfg = NULL;
static const char *channel_names[] = SOUND_DEVICE_LABELS;

/*
 * Definition of config file variables and their default values.
 */
enum { CFG_POS_X = 0, CFG_POS_Y, CFG_WIDTH, CFG_HEIGHT, CFG_MASK, CFG_NVARS };

static dsbcfg_vardef_t vardefs[] = {
        { "win_pos_x",  DSBCFG_VAR_INTEGER, CFG_POS_X,  DSBCFG_VAL(0)   },
        { "win_pos_y",  DSBCFG_VAR_INTEGER, CFG_POS_Y,  DSBCFG_VAL(0)   },
        { "win_width",  DSBCFG_VAR_INTEGER, CFG_WIDTH,  DSBCFG_VAL(100) },
        { "win_height", DSBCFG_VAR_INTEGER, CFG_HEIGHT, DSBCFG_VAL(200) },
        { "chan_mask",  DSBCFG_VAR_INTEGER, CFG_MASK,   DSBCFG_VAL(-1)  }
};

static int	add_mixer(const char *);
static void	usage(void);
static void	cleanup(int);
static void	get_snd_settings(void);
static void	get_mixers(void);
static void	get_vol(mixer_t *);
static void	settings_menu(void);
static void	create_mainwin(void);
static void 	hide_win(GtkWidget *);
static void	add_mixer_tab(mixer_t *);
static void	add_mixer_tabs(void);
static void	set_vol(mixer_t *, int, int);
static void	change_recsrc(mixer_tab_t *);
static void	change_volume(mixer_tab_t *);
static void	tray_click(GtkStatusIcon *, gpointer);
static void	popup_context_menu(GtkStatusIcon *, guint, guint, gpointer);
static char	*exec_backend(const char *, int *);
#ifdef WITH_DEVD
static void	del_mixers(void);
static void	del_mixer_tabs(void);
static void	parse_devd_event(char *);
static FILE	*uconnect(const char *);
static gboolean	readevent(GIOChannel *, GIOCondition, gpointer);
#endif
static gboolean	update_mixer(gpointer);
static gboolean window_state_event(GtkWidget *, GdkEvent *, gpointer);

int
main(int argc, char *argv[])
{
	int	   ch;
	sigset_t   sigmask;
#ifdef WITH_DEVD
	GIOChannel *ioc;
#endif

#ifdef WITH_GETTEXT
	(void)setlocale(LC_ALL, "");
	(void)bindtextdomain(PROGRAM, PATH_LOCALE);
	(void)textdomain(PROGRAM);
#endif
	gtk_init(&argc, &argv);

	mainwin.win_state = GDK_WINDOW_STATE_ABOVE;
	while ((ch = getopt(argc, argv, "ih")) != -1) {
                switch (ch) {
		case 'i':
			/* Start as tray icon. */
			mainwin.win_state = GDK_WINDOW_STATE_WITHDRAWN;
			break;
		case '?':
		case 'h':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	cfg = dsbcfg_read(PROGRAM, PATH_CONFIG, vardefs, CFG_NVARS);
	if (cfg == NULL && errno == ENOENT) {
		cfg = dsbcfg_new(NULL, vardefs, CFG_NVARS);
		if (cfg == NULL)
			xerrx(NULL, EXIT_FAILURE, "%s", dsbcfg_strerror());
	} else if (cfg == NULL)
		xerrx(NULL, EXIT_FAILURE, "%s", dsbcfg_strerror());

	mainwin.win	    = NULL;
	mainwin.posx	    = &dsbcfg_getval(cfg, CFG_POS_X).integer;
	mainwin.posy	    = &dsbcfg_getval(cfg, CFG_POS_Y).integer;
	mainwin.width	    = &dsbcfg_getval(cfg, CFG_WIDTH).integer;
	mainwin.height	    = &dsbcfg_getval(cfg, CFG_HEIGHT).integer;
	mainwin.mixer_tabs  = NULL;
	mainwin.nmixer_tabs = 0;
	
	get_snd_settings();
	create_mainwin();

#ifdef WITH_DEVD
	/* Connect to devd */
	if ((devdfp = uconnect(PATH_DEVDSOCKET)) != NULL) {
		ioc = g_io_channel_unix_new(fileno(devdfp));
		(void)g_io_add_watch(ioc, G_IO_IN, readevent, NULL);
	} else
		xwarn(mainwin.win, _("Couldn't connect to devd"));
#endif
	/* 
	 * Install the signal handler for SIGHUP, SIGINT, SIGQUIT, SIGTERM,
	 * and block all other signals.
	 */
	(void)signal(SIGHUP, cleanup);
	(void)signal(SIGINT, cleanup);
	(void)signal(SIGQUIT, cleanup);
	(void)signal(SIGTERM, cleanup);

	(void)sigfillset(&sigmask);
	(void)sigdelset(&sigmask, SIGHUP);
	(void)sigdelset(&sigmask, SIGINT);
	(void)sigdelset(&sigmask, SIGTERM);
	(void)sigdelset(&sigmask, SIGQUIT);
	(void)sigprocmask(SIG_BLOCK, &sigmask, NULL);

	gtk_main();

	return (0);
}

static void
usage()
{
	(void)printf("Usage: %s [-ih]\n", PROGRAM);
	exit(EXIT_FAILURE);
}

static void
get_snd_settings()
{
	size_t sz;

	sz = sizeof(int);
	(void)sysctlbyname("hw.snd.default_unit", &snd_settings.default_unit,
	    &sz, NULL, 0);

	sz = sizeof(int);
	(void)sysctlbyname("hw.snd.vpc_0db", &snd_settings.amplify,
	    &sz, NULL, 0);

	sz = sizeof(int);
	(void)sysctlbyname("hw.snd.feeder_rate_quality",
	    &snd_settings.feeder_rate_quality, &sz, NULL, 0);
}

#ifdef WITH_DEVD
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
		if (strcmp(p, "system") == 0)
			devdevent.system = q;
		else if (strcmp(p, "subsystem") == 0)
			devdevent.subsystem = q;
		else if (strcmp(p, "type") == 0)
			devdevent.type = q;
		else if (strcmp(p, "cdev") == 0)
			devdevent.cdev = q;
		else if (strcmp(p, "intclass") == 0) {
			if (strncmp(q, "0x", 2) == 0)
				q += 2;
			devdevent.intclass = strtol(q, NULL, 16);
		}
	}
}

static gboolean
readevent(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	static char ugen[12], ln[_POSIX2_LINE_MAX];
	static char path[sizeof(_PATH_DEV) + sizeof("mixer") + 2];

	ugen[0] = '\0';
	while (fgets(ln, sizeof(ln), devdfp) != NULL) {
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
			add_mixer_tab(mixers[nmixers - 1]);
			if (*mainwin.posx >= 0 && *mainwin.posy >= 0) {
				gtk_window_move(GTK_WINDOW(mainwin.win),
				    *mainwin.posx, *mainwin.posy);
			}
			gtk_widget_show_all(GTK_WIDGET(mainwin.win));
		} else if (strcmp(devdevent.system, "USB") == 0 &&
		    strcmp(devdevent.type, "DETACH") == 0	&&
		    devdevent.intclass == INTCLASS_AUDIO) {
			if (strcmp(devdevent.ugen, ugen) == 0)
				continue;
			(void)strncpy(ugen, devdevent.ugen, sizeof(ugen));
			del_mixer_tabs();
			del_mixers();
			get_mixers();
			add_mixer_tabs();
			gtk_window_resize(mainwin.win, 1, *mainwin.height);
			if ((mainwin.win_state & GDK_WINDOW_STATE_WITHDRAWN))
				continue;
			if (*mainwin.posx >= 0 && *mainwin.posy >= 0) {
				gtk_window_move(GTK_WINDOW(mainwin.win),
				    *mainwin.posx, *mainwin.posy);
			}
			gtk_widget_show_all(GTK_WIDGET(mainwin.win));
		}
	}
	if (feof(devdfp)) {
		/* Connection to devd lost. */
		(void)fclose(devdfp); devdfp = NULL;
		return (FALSE);
	}
	return (TRUE);
}
#endif

static char *
get_cardname(const char *mixer)
{
	int	   unit;
	FILE	   *fp;
	char	   *name, ln[_POSIX2_LINE_MAX], pattern[8];
	const char *p;

	p = strchr(mixer, '\0');
	while (isdigit(*--p))
		;
	if (!isdigit(*++p))
		unit = 0;
	else
		unit = strtol(p, NULL, 10);
	(void)snprintf(pattern, sizeof(pattern), "pcm%d:", unit);
	if ((fp = fopen(PATH_SNDSTAT, "r")) == NULL) {
		warn("fopen(%s)", PATH_SNDSTAT);
		if ((name = malloc(7)) == NULL)
			xerr(NULL, EXIT_FAILURE, "malloc()");
		(void)snprintf(name, 7, "pcm%d", unit);
		return (name);
	}
	for (name = NULL; fgets(ln, sizeof(ln), fp) != NULL;) {
		(void)strtok(ln, "\r\n");
		if (strncmp(ln, pattern, strlen(pattern)) == 0) {
			if ((name = strdup(ln)) == NULL)
				xerr(NULL, EXIT_FAILURE, "strdup()");
			break;
		}
	}
	(void)fclose(fp);
	if (name == NULL) {
		if ((name = malloc(7)) == NULL)
			xerr(NULL, EXIT_FAILURE, "malloc()");
		(void)snprintf(name, 7, "pcm%d", unit);
	}
	return (name);
}

static void
get_mixers()
{
	int	       cwdfd;
	DIR           *dirp;
	struct stat    sb;
        struct dirent *dp;

	/* Save current working directory. */
	if ((cwdfd = open(".", O_EXEC)) == -1)
		xerr(mainwin.win, EXIT_FAILURE, "open(\".\")");

	/* Scan /dev for mixer devices. */
	if (chdir(_PATH_DEV) == -1)
		xerr(mainwin.win, EXIT_FAILURE, "chdir(%s)", _PATH_DEV);
	if ((dirp = opendir(".")) == NULL)
		xerr(mainwin.win, EXIT_FAILURE, "opendir(%s)", _PATH_DEV);
	while ((dp = readdir(dirp)) != NULL) {
		if (strncmp(dp->d_name, "mixer", 5) != 0)
			continue;
		if (!isdigit(dp->d_name[5]))
			continue;
		if (lstat(dp->d_name, &sb) == -1) {
			warn("stat(%s)", dp->d_name);
			continue;
		} else if (S_ISLNK(sb.st_mode)) {
			/* Skip symlinks */
			continue;
		}
		add_mixer(dp->d_name);
	}
	closedir(dirp);
	if (nmixers == 0) {
		add_mixer(PATH_DEFAULT_MIXER);
		if (nmixers == 0) {
			xerrx(mainwin.win, EXIT_FAILURE,
			    _("No mixer devices found"));
		}
	}
	(void)fchdir(cwdfd);
	(void)close(cwdfd);
}

static int
add_mixer(const char *name)
{
	int	i;
	mixer_t mixer;

	for (i = 0; i < 3; i++) {
		if ((mixer.fd = open(name, O_RDWR)) == -1) {
			if (errno == EBADF)
				warn("open(%s)", name);
			else {
				xwarn(mainwin.win, _("Couldn't open mixer %s"),
				    name);
				return (-1);
			}
			(void)sleep(1);
		} else
			break;
	}
	if (i == 3) {
		xwarn(mainwin.win, _("Couldn't open mixer %s"), name);
		return (-1);
	}
	for (i = 0; i < 3; i++) {
		if (ioctl(mixer.fd, SOUND_MIXER_READ_DEVMASK,
		    &mixer.dmask) == -1) {
			if (errno == EBADF)
				warn("ioctl(SOUND_MIXER_READ_DEVMASK");
			else {
				xwarn(mainwin.win,
				    "ioctl(SOUND_MIXER_READ_DEVMASK)");
				return (-1);
			}
			(void)sleep(1);
		} else
			break;
	}
	if (i == 3) {
		xwarn(mainwin.win, "ioctl(SOUND_MIXER_READ_DEVMASK)");
		return (-1);
	}

	for (i = 0; i < 3; i++) {
		if (ioctl(mixer.fd, SOUND_MIXER_READ_RECMASK,
		    &mixer.recmask) == -1) {
			if (errno == EBADF)
				warn("ioctl(SOUND_MIXER_READ_RECMASK");
			else {
				xwarn(mainwin.win,
				    "ioctl(SOUND_MIXER_READ_RECMASK)");
				return (-1);		
			}
			(void)sleep(1);
		} else
			break;
	}
	if (i == 3) {
		xwarn(mainwin.win, "ioctl(SOUND_MIXER_READ_RECMASK)");
		return (-1);
	}

	for (i = 0; i < 3; i++) {
		if (ioctl(mixer.fd, SOUND_MIXER_READ_RECSRC,
		    &mixer.recsrc) == -1) {
			if (errno == EBADF)
				warn("ioctl(SOUND_MIXER_READ_RECSRC)");
			else {
	                	xwarn(mainwin.win,
				    "ioctl(SOUND_MIXER_READ_RECSRC)");
				return (-1);
			}
			(void)sleep(1);
		} else
			break;
	}
	if (i == 3) {
	 	xwarn(mainwin.win, "ioctl(SOUND_MIXER_READ_RECSRC)");
		return (-1);
	}

	/* Get all non-recording devices. */
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (((1 << i) & mixer.dmask)) {
			all_chan_mask |= (1 << i);
			mixer.chan[i].name = channel_names[i];
		}
	}
	get_vol(&mixer);
	mixer.cardname = get_cardname(name);
	mixers = realloc(mixers, (nmixers + 1) * sizeof(mixer_t **));
	if (mixers == NULL)
		xerr(mainwin.win, EXIT_FAILURE, "realloc()");
	if ((mixers[nmixers] = malloc(sizeof(mixer_t))) == NULL)
		xerr(mainwin.win, EXIT_FAILURE, "malloc()");
	(void)memcpy(mixers[nmixers], &mixer, sizeof(mixer_t));
	if ((mixers[nmixers]->name = strdup(name)) == NULL)
		xerr(mainwin.win, EXIT_FAILURE, "strdup()");
	nmixers++;

	return (0);
}

#ifdef WITH_DEVD
static void
del_mixers()
{
	int i;

	if (nmixers > 0) {
		for (i = 0; i < nmixers; i++) {
			(void)close(mixers[i]->fd);
			free(mixers[i]->name);
			free(mixers[i]);
		}
		free(mixers);
		mixers = NULL; nmixers = 0;
		/*
		 * After removing a USB soundcard, it takes the system some
		 * time to remove the device node from /dev.
		 */
		(void)sleep(2);
	}
}
#endif

static gboolean
update_mixer(gpointer unused)
{
	static int	     i, j;
	static GtkAdjustment *adj;

#ifdef WITH_DEVD
	if (devdfp == NULL) {
		/* Reconnect to devd */
		GIOChannel *ioc;

		if ((devdfp = uconnect(PATH_DEVDSOCKET)) != NULL) {
			ioc = g_io_channel_unix_new(fileno(devdfp));
			(void)g_io_add_watch(ioc, G_IO_IN, readevent, NULL);
		}
	}
#endif
	for (i = 0; i < mainwin.nmixer_tabs; i++) {
		get_vol(mainwin.mixer_tabs[i]->mixer);

		(void)ioctl(mainwin.mixer_tabs[i]->mixer->fd,
		    SOUND_MIXER_READ_RECSRC,
		    &mainwin.mixer_tabs[i]->mixer->recsrc);
		for (j = 0; j < SOUND_MIXER_NRDEVICES; j++) {
			if (!(mainwin.mixer_tabs[i]->mixer->dmask & (1 << j)) ||
			    !(dsbcfg_getval(cfg, CFG_MASK).integer & (1 << j)))
				continue;
			adj = gtk_range_get_adjustment(
			    GTK_RANGE(mainwin.mixer_tabs[i]->chan[j]));

			gtk_adjustment_set_value(adj,
			    (gdouble)mainwin.mixer_tabs[i]->mixer->chan[j].vol);
			gtk_widget_show(GTK_WIDGET(
			    mainwin.mixer_tabs[i]->chan[j]));
			if (!(mainwin.mixer_tabs[i]->mixer->recmask & (1 << j)))
				continue;
			gtk_toggle_button_set_active(
			    GTK_TOGGLE_BUTTON(
				mainwin.mixer_tabs[i]->recsrc_cb[j]),
			    (mainwin.mixer_tabs[i]->mixer->recsrc & \
				(1 << j)) ? TRUE : FALSE);
			gtk_widget_show(GTK_WIDGET(
			    mainwin.mixer_tabs[i]->recsrc_cb[j])
			    );
		}
	}
	return (TRUE);
}

static void
get_vol(mixer_t *mixer)
{
	static int i;

	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (!(mixer->dmask & (1 << i)))
			continue;
		if (ioctl(mixer->fd, MIXER_READ(i), &mixer->chan[i].vol) != 0)
			warn("ioctl()");
		mixer->chan[i].vol &= 0x7f;
	}
}

static void
set_vol(mixer_t *mixer, int dev, int vol)
{
	int buf = ((vol << 8) + vol);

	(void)ioctl(mixer->fd, MIXER_WRITE(dev), &buf);
}

static void
change_volume(mixer_tab_t *mixer)
{
	int	      i;
	static char   tt[32];
	GtkAdjustment *adj;

	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (!(mixer->mixer->dmask & (1 << i)) ||
		    !(dsbcfg_getval(cfg, CFG_MASK).integer & (1 << i)))
			continue;
		adj = gtk_range_get_adjustment(GTK_RANGE(mixer->chan[i]));
		if ((int)gtk_adjustment_get_value(adj) !=
		    mixer->mixer->chan[i].vol) {
			set_vol(mixer->mixer, i,
			    (int)gtk_adjustment_get_value(adj));
		}
		(void)snprintf(tt, sizeof(tt), "%d%%",
		    (int)gtk_adjustment_get_value(adj));
		gtk_widget_set_tooltip_text(GTK_WIDGET(mixer->chan[i]), tt);
	}
}

static void
change_recsrc(mixer_tab_t *mixer)
{
	int i;

	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (!(mixer->mixer->dmask & (1 << i)) ||
		    !(mixer->mixer->recmask & (1 << i)) ||
		    !(dsbcfg_getval(cfg, CFG_MASK).integer & (1 << i)))
			continue;
		if (gtk_toggle_button_get_active(
		    GTK_TOGGLE_BUTTON(mixer->recsrc_cb[i])))
			mixer->mixer->recsrc |= (1 << i);
		else
			mixer->mixer->recsrc &= ~(1 << i);
	}
	if (ioctl(mixer->mixer->fd, SOUND_MIXER_WRITE_RECSRC,
	    &mixer->mixer->recsrc) == -1)
		xwarn(mainwin.win, _("Couldn't add/delete recording source"));
	if (ioctl(mixer->mixer->fd, SOUND_MIXER_READ_RECSRC,
	    &mixer->mixer->recsrc) == -1)
                xwarn(mainwin.win, "ioctl(SOUND_MIXER_READ_RECSRC)");
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (!(mixer->mixer->dmask & (1 << i)) ||
		    !(mixer->mixer->recmask & (1 << i)) ||
		    !(dsbcfg_getval(cfg, CFG_MASK).integer & (1 << i)))
			continue;
		gtk_toggle_button_set_active(
		    GTK_TOGGLE_BUTTON(mixer->recsrc_cb[i]),
		    (mixer->mixer->recsrc & (1 << i)) ? TRUE : FALSE);
	}
}

static void
popup_context_menu(GtkStatusIcon *icon, guint bt, guint tm, gpointer menu)
{
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, bt, tm);
}

static void
tray_click(GtkStatusIcon *status_icon, gpointer win)
{
	if ((mainwin.win_state & GDK_WINDOW_STATE_ICONIFIED) ||
	    (mainwin.win_state & GDK_WINDOW_STATE_WITHDRAWN) ||
	    (mainwin.win_state & GDK_WINDOW_STATE_BELOW)) {
		if ((mainwin.win_state & GDK_WINDOW_STATE_ICONIFIED))
			gtk_window_deiconify(win);
		if (*mainwin.posx >= 0 && *mainwin.posy >= 0) {
			gtk_window_move(GTK_WINDOW(win),
			    *mainwin.posx, *mainwin.posy);
		}
		/* Update the mixer */
		update_mixer(NULL);
		gtk_widget_show_all(GTK_WIDGET(win));
	} else {
		gtk_window_get_position(GTK_WINDOW(win),
		    mainwin.posx, mainwin.posy);
		if (*mainwin.posx < 0 || *mainwin.posy < 0 ||
		    gdk_screen_width() - *mainwin.posx < 0 ||
		    gdk_screen_height() - *mainwin.posy < 0)
			*mainwin.posx = *mainwin.posy = 0;
		gtk_widget_hide(GTK_WIDGET(win));
	}	
}

static gboolean
window_state_event(GtkWidget *wdg, GdkEvent *event, gpointer unused)
{
	switch ((int)event->type) {
	case GDK_CONFIGURE:
		*mainwin.posx   = ((GdkEventConfigure *)event)->x;
		*mainwin.posy   = ((GdkEventConfigure *)event)->y;
		*mainwin.width  = ((GdkEventConfigure *)event)->width;
		*mainwin.height = ((GdkEventConfigure *)event)->height;
		dsbcfg_write(PROGRAM, PATH_CONFIG, cfg);
		return (FALSE);
		break;
	case GDK_WINDOW_STATE:
		mainwin.win_state =
		    ((GdkEventWindowState *)event)->new_window_state;
		return (FALSE);
		break;
	}
	return (FALSE);
}

static void
hide_win(GtkWidget *win)
{
	gtk_window_get_position(GTK_WINDOW(win), mainwin.posx, mainwin.posy);
	if (*mainwin.posx < 0 || *mainwin.posy < 0 ||
	    gdk_screen_width() - *mainwin.posx < 0 ||
	    gdk_screen_height() - *mainwin.posy < 0)
		*mainwin.posx = *mainwin.posy = 0;
	gtk_window_get_size(GTK_WINDOW(win), mainwin.width, mainwin.height);
	gtk_widget_hide(win);
}


static void
add_mixer_tab(mixer_t *mixer)
{
	int	      i, ncols, col;
	char	      *str, *p, tt[32];
	gsize	      wrt;
	GtkWidget     *table, *label;
	mixer_tab_t   *mt;
	GtkAdjustment *adj;

	mainwin.mixer_tabs = realloc(mainwin.mixer_tabs,
	    sizeof(mixer_tab_t *) * (mainwin.nmixer_tabs + 1));
	if (mainwin.mixer_tabs == NULL)
		xerr(mainwin.win, EXIT_FAILURE, "realloc()");
	mainwin.mixer_tabs[mainwin.nmixer_tabs] = malloc(sizeof(mixer_tab_t));
	if (mainwin.mixer_tabs[mainwin.nmixer_tabs] == NULL)
		xerr(mainwin.win, EXIT_FAILURE, "malloc()");
	mainwin.mixer_tabs[mainwin.nmixer_tabs]->mixer = mixer;
	mt = mainwin.mixer_tabs[mainwin.nmixer_tabs++];

	for (i = 0, ncols = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if ((mt->mixer->dmask & (1 << i)) &&
		    (dsbcfg_getval(cfg, CFG_MASK).integer & (1 << i)))
			ncols++;
	}
	table = gtk_table_new(3, ncols, FALSE);
	for (i = 0, col = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (!(mt->mixer->dmask & (1 << i)) ||
		    !(dsbcfg_getval(cfg, CFG_MASK).integer & (1 << i)))
			continue;
		adj = GTK_ADJUSTMENT(
			  gtk_adjustment_new((float)mt->mixer->chan[i].vol,
			      0.0, 100.0, 1.0, 10.0, 0.0));
		mt->chan[i] = GTK_VSCALE(gtk_vscale_new(adj));
		(void)snprintf(tt, sizeof(tt), "%d%%", mixer->chan[i].vol);
		gtk_widget_set_tooltip_text(GTK_WIDGET(mt->chan[i]), tt);
		gtk_scale_set_draw_value(GTK_SCALE(mt->chan[i]), FALSE);
		gtk_range_set_inverted(GTK_RANGE(mt->chan[i]), TRUE);
		gtk_table_attach(GTK_TABLE(table),
		    GTK_WIDGET(mt->chan[i]), col, col + 1, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
		g_signal_connect_swapped(G_OBJECT(adj), "value-changed",
		    G_CALLBACK(change_volume), mt);
		label = new_label(ALIGN_CENTER, ALIGN_CENTER,
		    mt->mixer->chan[i].name);
		gtk_table_attach(GTK_TABLE(table),
		    GTK_WIDGET(label), col, col + 1, 2, 3,
		    GTK_EXPAND | GTK_FILL, 0, 0, 0);

		if ((mt->mixer->recmask & (1 << i))) {
			mt->recsrc_cb[i] = gtk_check_button_new();
			gtk_toggle_button_set_active(
			    GTK_TOGGLE_BUTTON(mt->recsrc_cb[i]),
			    (mt->mixer->recsrc & (1 << i)) ? TRUE : FALSE);
			gtk_table_attach(GTK_TABLE(table),
			    GTK_WIDGET(mt->recsrc_cb[i]), col, col + 1, 0, 1,
		    	    0, 0, 0, 0);
			str = g_locale_to_utf8(_("Add to recording sources"),
			    -1, NULL, &wrt, NULL);
 			gtk_widget_set_tooltip_text(
			    GTK_WIDGET(mt->recsrc_cb[i]), str);
			g_signal_connect_swapped(mt->recsrc_cb[i], "toggled",
			    G_CALLBACK(change_recsrc), mt);
		}
		col++;
	}
	if ((p = strrchr(mt->mixer->name, '/')) != NULL)
		p++;
	else
		p = mt->mixer->name;
	label = new_label(ALIGN_CENTER, ALIGN_CENTER, p);
	gtk_widget_set_tooltip_text(label, mixer->cardname);
	gtk_notebook_append_page(GTK_NOTEBOOK(mainwin.notebook), table,
	    GTK_WIDGET(label));
	gtk_widget_show_all(GTK_WIDGET(mainwin.notebook));
}

static void
add_mixer_tabs()
{
	int i;

	for (i = 0; i < nmixers; i++)
		add_mixer_tab(mixers[i]);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(mainwin.notebook),
	    snd_settings.default_unit);
}

static void
del_mixer_tabs()
{
	int i;

	for (i = 0; i < mainwin.nmixer_tabs; i++) {
		gtk_notebook_remove_page(GTK_NOTEBOOK(mainwin.notebook), 0);
		free(mainwin.mixer_tabs[i]);
	}
	free(mainwin.mixer_tabs);
	mainwin.mixer_tabs = NULL; mainwin.nmixer_tabs = 0;
}

static void
settings_menu()
{
	int	      i, j, mask, row, col, error, amplify, quality;
	char	      cmd[256], str[12], *output;
	GSList	      *grp;
	GtkWidget     *win, *cb, *bt, *nb, *vbox, *label, *ca, *table, *menu;
	GtkWidget     *optmenu, *spin;
	GtkWidget     *rb[32];
	GdkPixbuf     *icon;
	GtkIconTheme  *icon_theme;
	GtkAdjustment *adj;

	get_snd_settings();
	icon_theme = gtk_icon_theme_get_default();
	icon = gtk_icon_theme_load_icon(icon_theme, GTK_STOCK_PREFERENCES,
	    32, 0, NULL);
	win = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(mainwin.win));
	gtk_window_set_title(GTK_WINDOW(win), _("Settings"));
	gtk_window_set_icon(GTK_WINDOW(win), icon);
	gtk_window_set_resizable(GTK_WINDOW(win), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(win), 10);

	/*
	 * View settings tab.
	 */
	for (i = j = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (all_chan_mask & (1 << i))
			j++;
	}
	table = gtk_table_new(j / 3 + 1, 3, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        
	vbox = gtk_vbox_new(FALSE, 5);
	label = new_pango_label(ALIGN_CENTER, ALIGN_CENTER,
	    _("<b>Visible channels</b>"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(label), FALSE, FALSE, 0);
	label = new_label(ALIGN_CENTER, ALIGN_CENTER,
	    _("Select the mixer channels to be visible.\n"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(label), FALSE, FALSE, 0);

	mask = all_chan_mask;
	for (i = row = col = 0; i < nmixers; i++) {
		for (j = 0; j < SOUND_MIXER_NRDEVICES; j++) {
			if (!(mixers[i]->dmask & (1 << j)) ||
			    !(mask & (1 << j)))
				continue;
			else
				mask &= ~(1 << j);
			cb = gtk_check_button_new_with_label(
			    mixers[i]->chan[j].name);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
			    ((1 << j) & dsbcfg_getval(cfg, CFG_MASK).integer) \
			    ? TRUE : FALSE);
			if (col == 3) {
				col = 0; row++;
			}
			gtk_table_attach(GTK_TABLE(table), cb,
			    col, col + 1, row, row + 1,
			    GTK_EXPAND | GTK_FILL,
			    GTK_EXPAND | GTK_FILL, 0, 0);
			col++;
			settings.show_chan_cb[j] = cb;
		}
	}
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	nb = gtk_notebook_new();
	gtk_container_set_border_width(GTK_CONTAINER(table), 10);

	label = new_label(ALIGN_CENTER, ALIGN_CENTER, _("View"));
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), vbox, label);

	/*
	 * Default audio device settings tab.
	 */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

	label = new_pango_label(ALIGN_CENTER, ALIGN_CENTER,
	    _("<b>Default audio device</b>"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	label = new_label(ALIGN_CENTER, ALIGN_CENTER,
	    _("Choose the default audio " \
	    "device to be used by applications.\n"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	for (i = 0, grp = NULL; i < nmixers &&
	    i < sizeof(rb) / sizeof(GtkWidget *); i++) {
		rb[i] = gtk_radio_button_new_with_label(grp,
		    mixers[i]->cardname);
		grp = gtk_radio_button_get_group(GTK_RADIO_BUTTON(rb[i]));
		if (i == snd_settings.default_unit) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb[i]),
			    TRUE);
		}
		gtk_box_pack_start(GTK_BOX(vbox), rb[i], FALSE, FALSE, 0);
	}
	label = new_label(ALIGN_CENTER, ALIGN_CENTER, _("Default device"));

	gtk_notebook_append_page(GTK_NOTEBOOK(nb), vbox, label);

	/*
	 * Advanced settings tab 
	 */
	table = gtk_table_new(3, 3, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 15);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

	label = new_pango_label(ALIGN_CENTER, ALIGN_CENTER,
	    _("<b>Advanced settings</b>"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
	menu = gtk_menu_new();
	for (i = 0; i <= 4; i++) {
		(void)snprintf(str, sizeof(str), "%d", i);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		    gtk_menu_item_new_with_label(str));
	}
	optmenu = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(optmenu), menu);
	gtk_option_menu_set_history(GTK_OPTION_MENU(optmenu),
	    snd_settings.feeder_rate_quality);
	gtk_table_attach(GTK_TABLE(table), optmenu, 1, 2, 0, 1,
			    GTK_EXPAND | GTK_FILL,
			    GTK_EXPAND | GTK_FILL, 0, 0);
	
	label = new_label(ALIGN_CENTER, ALIGN_CENTER,
	    _("Feeder rate quality:"));
	gtk_misc_set_alignment(GTK_MISC(label), ALIGN_LEFT, ALIGN_CENTER);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			    GTK_EXPAND | GTK_FILL,
			    GTK_EXPAND | GTK_FILL, 0, 0);

	/* Amplify */
	adj = GTK_ADJUSTMENT(gtk_adjustment_new((float)snd_settings.amplify,
	    0, 999.0, 1.0, 1.0, 0.0));
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	label = new_label(ALIGN_CENTER, ALIGN_CENTER, _("Amplify:"));
	gtk_misc_set_alignment(GTK_MISC(label), ALIGN_LEFT, ALIGN_CENTER);

	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			    GTK_EXPAND | GTK_FILL,
			    GTK_EXPAND | GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(table), spin, 1, 2, 1, 2,
			    GTK_EXPAND | GTK_FILL,
			    GTK_EXPAND | GTK_FILL, 0, 0);
	
	label = new_label(ALIGN_LEFT, ALIGN_CENTER, "dB");
	gtk_misc_set_alignment(GTK_MISC(label), ALIGN_LEFT, ALIGN_CENTER);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2,
			    GTK_EXPAND | GTK_FILL,
			    GTK_EXPAND | GTK_FILL, 0, 0);

	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	
	label = new_label(ALIGN_CENTER, ALIGN_CENTER, _("Advanced settings"));
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), vbox, label);

	ca = gtk_dialog_get_content_area(GTK_DIALOG(win));
	gtk_container_add(GTK_CONTAINER(ca), nb);

	bt = new_button(_("_Apply"), GTK_STOCK_APPLY);
	gtk_dialog_add_action_widget(GTK_DIALOG(win), bt, GTK_RESPONSE_OK);
	bt = new_button(_("_Close"), GTK_STOCK_CLOSE);
	gtk_dialog_add_action_widget(GTK_DIALOG(win), bt, GTK_RESPONSE_CANCEL);

	gtk_widget_show_all(win);

	if (gtk_dialog_run(GTK_DIALOG(win)) != GTK_RESPONSE_OK) {
		gtk_widget_destroy(win);
		return;
	}
	/* "Apply" clicked */
	mask = all_chan_mask;
	for (i = 0; i < mainwin.nmixer_tabs; i++) {
		for (j = 0; j < SOUND_MIXER_NRDEVICES; j++) {
			if (!(mainwin.mixer_tabs[i]->mixer->dmask & (1 << j)) ||
			    !(mask & (1 << j)))
				continue;
			else
				mask &= ~(1 << j);
			if (gtk_toggle_button_get_active(
			    GTK_TOGGLE_BUTTON(settings.show_chan_cb[j]))) {
				dsbcfg_getval(cfg, CFG_MASK).integer |= \
				    (1 << j);
			} else {
				dsbcfg_getval(cfg, CFG_MASK).integer &= \
				    ~(1 << j);
			}
		}
	}
	quality = gtk_option_menu_get_history(GTK_OPTION_MENU(optmenu));
	amplify = (int)gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

	for (i = 0; i < nmixers && i < sizeof(rb) / sizeof(GtkWidget *); i++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb[i])))
			break;
	}
	if (snd_settings.default_unit != i  ||
	    snd_settings.amplify != amplify ||
	    snd_settings.feeder_rate_quality != quality) {
		(void)snprintf(cmd, sizeof(cmd), "%s -u %d -d -a %d -q %d",
		    PATH_BACKEND, i, amplify, quality);
		if ((output = exec_backend(cmd, &error)) == NULL) {
			xerr(GTK_WINDOW(win), EXIT_FAILURE,
			    _("Failed to execute %s"), cmd);
		} else if (error != 0) {
			xwarnx(GTK_WINDOW(win), _("Failed to execute %s:\n"),
			    cmd, output);
			free(output);
		} else {
			snd_settings.default_unit = i;
			gtk_notebook_set_current_page(
			    GTK_NOTEBOOK(mainwin.notebook),
			    snd_settings.default_unit);
			free(output);
		}
	}
	dsbcfg_write(PROGRAM, PATH_CONFIG, cfg);
	gtk_widget_destroy(win);

	del_mixer_tabs();
	add_mixer_tabs();
	gtk_window_resize(mainwin.win, 1, *mainwin.height);
	gtk_widget_show_all(GTK_WIDGET(mainwin.win));
}

static void
create_mainwin()
{
	GdkPixbuf     *icon;
	GtkWidget     *menu, *root_menu, *menu_bar, *item, *vbox;
	GtkIconTheme  *icon_theme;

	icon_theme = gtk_icon_theme_get_default();
	if ((icon = gtk_icon_theme_load_icon(icon_theme,
	    "audio-volume-high", 32, 0, NULL)) == NULL &&
	    (icon = gtk_icon_theme_load_icon(icon_theme,
            "sound", 32, 0, NULL)) == NULL &&
	    (icon = gtk_icon_theme_load_icon(icon_theme,
            "audio-card", 32, 0, NULL)) == NULL) {
		icon = gtk_icon_theme_load_icon(icon_theme,
		    GTK_STOCK_MISSING_IMAGE, 32, 0, NULL);
	}
	mainwin.win = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_default_size(mainwin.win, *mainwin.width,
	    *mainwin.height);
	gtk_window_set_title(mainwin.win, TITLE);
	gtk_window_set_resizable(mainwin.win, TRUE);
	gtk_window_set_icon(mainwin.win, icon);
	
	/* Create the menu */
	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	gtk_menu_item_set_label(GTK_MENU_ITEM(item), _("Preferences"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate",
	    G_CALLBACK(settings_menu), NULL);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	gtk_menu_item_set_label(GTK_MENU_ITEM(item), _("Exit"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(cleanup), NULL);

	root_menu = gtk_menu_item_new_with_mnemonic(_("_File"));

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(root_menu), menu);
	menu_bar = gtk_menu_bar_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), root_menu);

	mainwin.notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(mainwin.notebook), GTK_POS_TOP);
	
	get_mixers();
	add_mixer_tabs();

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 2);

	gtk_container_add(GTK_CONTAINER(vbox), mainwin.notebook);
	gtk_container_add(GTK_CONTAINER(mainwin.win), GTK_WIDGET(vbox));

	mainwin.tray_icon = gtk_status_icon_new_from_pixbuf(icon);
	g_signal_connect(G_OBJECT(mainwin.tray_icon), "activate",
	    G_CALLBACK(tray_click), mainwin.win);

	g_signal_connect(G_OBJECT(mainwin.tray_icon), "popup-menu",
	    G_CALLBACK(popup_context_menu), G_OBJECT(menu));

	gtk_status_icon_set_tooltip(mainwin.tray_icon, "Mixer");
	gtk_status_icon_set_visible(mainwin.tray_icon, TRUE);

	g_signal_connect(mainwin.win, "delete-event",
	    G_CALLBACK(hide_win), NULL);
	g_signal_connect(G_OBJECT(mainwin.win), "window-state-event",
	    G_CALLBACK(window_state_event), mainwin.tray_icon);
	g_signal_connect(G_OBJECT(mainwin.win), "configure-event",
	    G_CALLBACK(window_state_event), mainwin.win);
	g_timeout_add(200, update_mixer, NULL);

	if (*mainwin.posx >= 0 && *mainwin.posy >= 0) {
		gtk_window_move(GTK_WINDOW(mainwin.win),
		    *mainwin.posx, *mainwin.posy);
	}
	if (mainwin.win_state != GDK_WINDOW_STATE_WITHDRAWN)
		gtk_widget_show_all(GTK_WIDGET(mainwin.win));
}

static void
cleanup(int unused)
{

	dsbcfg_write(PROGRAM, PATH_CONFIG, cfg);
	gtk_main_quit();
}

#ifdef WITH_DEVD
static FILE *
uconnect(const char *path)
{
	int  s;
	FILE *sp;
	struct sockaddr_un saddr;

	if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
		return (NULL);
	(void)memset(&saddr, (unsigned char)0, sizeof(saddr));
	(void)snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", path);
	saddr.sun_family = AF_LOCAL;
	if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
		return (NULL);
	if ((sp = fdopen(s, "r+")) == NULL)
		return (NULL);
	/* Make the stream line buffered, and the socket non-blocking. */
	if (setvbuf(sp, NULL, _IOLBF, 0) == -1 ||
	    fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK) == -1)
		return (NULL);
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

