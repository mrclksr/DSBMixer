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

#ifndef _GTK_HELPER_H_
#define _GTK_HELPER_H_

#include <gtk/gtk.h>

#ifdef WITH_GETTEXT
# include <libintl.h>
# define _(STRING) gettext_wrapper(STRING)
#else
# define _(STRING) STRING
#endif

#define ALIGN_LEFT   0.0
#define ALIGN_RIGHT  1.0
#define ALIGN_CENTER 0.5
#define ALIGN_TOP    0.0
#define ALIGN_BOTTOM 1.0

extern int	 yesnobox(GtkWindow *, const char *, ...);
extern void	 xerr(GtkWindow *, int, const char *, ...);
extern void	 xwarn(GtkWindow *, const char *, ...);
extern void	 xerrx(GtkWindow *, int, const char *, ...);
extern void	 xwarnx(GtkWindow *, const char *, ...);
extern void	 infobox(GtkWindow *, const char *, const char *);
extern char	 *gettext_wrapper(const char *);
extern GtkWidget *new_label(float, float, const char *, ...);
extern GtkWidget *new_button(const char *, const char *);
extern GtkWidget *new_pango_label(float, float, const char *, ...);
extern GdkPixbuf *load_icon(int, const char *, ...);

#endif	/* ! _GTK_HELPER_H_ */

