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
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include "gtk-helper.h"

extern int errno;

/* Message buffer for xwarn* and xerr* */
static char msgbuf[1024];

#ifdef WITH_GETTEXT
char *
gettext_wrapper(const char *str)
{
	int   _errno;
	gchar *p;
	gsize wrt;
	
	_errno = errno;
	p = g_locale_to_utf8(gettext(str), -1, NULL, &wrt, NULL);
	if (p == NULL)
		return ((char *)str);
	errno = _errno;
	return (p);
}
#endif

GdkPixbuf *
load_icon(int size, const char *name, ...)
{
	va_list	     ap;
	GdkPixbuf    *icon;
	const char   *s;
	GtkIconTheme *icon_theme;

	va_start(ap, name);
	icon_theme = gtk_icon_theme_get_default();

	for (s = name; s != NULL || (s = va_arg(ap, char *)); s = NULL) {
		if ((icon = gtk_icon_theme_load_icon(icon_theme, s, size, 0,
		    NULL)) != NULL)
			return (icon);
	}
	return (NULL);
}

void
infobox(GtkWindow *parent, const char *str, const char *icon)
{
	GdkPixbuf    *ip;
	GtkWidget    *dialog, *image, *content, *ok, *hbox, *label;
	GtkIconTheme *icon_theme;

	gdk_threads_enter();
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

	ok = new_button("_Ok", GTK_STOCK_OK);
	image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_DIALOG);
	label = new_label(ALIGN_CENTER, ALIGN_CENTER, str);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	dialog = gtk_dialog_new();

	if (parent != NULL)
		gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	else
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	icon_theme = gtk_icon_theme_get_default();
	ip = gtk_icon_theme_load_icon(icon_theme, icon, 32, 0, NULL);
	if (ip != NULL)
		gtk_window_set_icon(GTK_WINDOW(dialog), ip);
	g_object_unref(ip);
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);

	gtk_misc_set_alignment(GTK_MISC(image), 0, ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(image), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), TRUE, TRUE, 5);

	gtk_container_add(GTK_CONTAINER(content), GTK_WIDGET(hbox));
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), ok,
	    GTK_RESPONSE_ACCEPT);
	g_signal_connect_swapped(dialog, "response",
	    G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_widget_show_all(dialog);
	(void)gtk_dialog_run(GTK_DIALOG(dialog));
	gdk_threads_leave();
}

void
xerr(GtkWindow *parent, int eval, const char *fmt, ...)
{
	int	_errno;
	gchar	*p;
	gsize	wrt;
	size_t	len, rem;
	va_list	ap;

	_errno = errno;
	
	msgbuf[0] = '\0';

	va_start(ap, fmt);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	len = strlen(msgbuf);
	rem = sizeof(msgbuf) - len - 1;

	p = g_locale_to_utf8(strerror(_errno), -1, NULL, &wrt, NULL);
	if (p == NULL)
		p = strerror(_errno);
	(void)snprintf(msgbuf + len, rem, ":\n%s\n", p);
	infobox(parent, msgbuf, GTK_STOCK_DIALOG_ERROR);
	exit(eval);
}

void
xerrx(GtkWindow *parent, int eval, const char *fmt, ...)
{
	va_list	ap;

	msgbuf[0] = '\0';
	va_start(ap, fmt);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	infobox(parent, msgbuf, GTK_STOCK_DIALOG_ERROR);
	exit(eval);
}

void
xwarn(GtkWindow *parent, const char *fmt, ...)
{
	int	_errno;
	gchar	*p;
	gsize	wrt;
	size_t	len, rem;
	va_list	ap;

	_errno = errno;
	msgbuf[0] = '\0';

	va_start(ap, fmt);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	len = strlen(msgbuf);
	rem = sizeof(msgbuf) - len;
	p = g_locale_to_utf8(strerror(_errno), -1, NULL, &wrt, NULL);
	if (p == NULL)
		p = strerror(_errno);
	(void)snprintf(msgbuf + len, rem, ":\n%s\n", p);
	infobox(parent, msgbuf, GTK_STOCK_DIALOG_ERROR);
}

void
xwarnx(GtkWindow *parent, const char *fmt, ...)
{
	va_list ap;

	msgbuf[0] = '\0';
	va_start(ap, fmt);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	infobox(parent, msgbuf, GTK_STOCK_DIALOG_WARNING);
}

GtkWidget *
new_label(float xalign, float yalign, const char *str, ...)
{
	char	  *p;
	va_list	  ap;
	GtkWidget *label;

	va_start(ap, str);
	p = g_strdup_vprintf(str, ap);
	label = gtk_label_new(p);
	gtk_misc_set_alignment(GTK_MISC(label), xalign, yalign);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_FILL);
	//gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	g_free(p);

	return (label);
}

GtkWidget *
new_button(const char *label, const char *icon)
{
	GtkWidget *bt, *image;

	image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_BUTTON);
	bt = gtk_button_new_with_mnemonic(label);
	gtk_button_set_image(GTK_BUTTON(bt), image);

	return (bt);
}

GtkWidget *
new_pango_label(float xalign, float yalign, const char *markup, ...)
{
	char	  *p;
	va_list	  ap;
	GtkWidget *label;

	va_start(ap, markup);
	p = g_strdup_vprintf(markup, ap);
	label = new_label(xalign, yalign, "");
	gtk_label_set_markup(GTK_LABEL(label), p);
	g_free(p);

	return (label);
}

int
yesnobox(GtkWindow *parent, const char *fmt, ...)
{
	char      *str;
	va_list   ap;
	GtkWidget *hbox, *dialog, *content, *ok, *cancel, *image, *label;
	
	gdk_threads_enter();
	va_start(ap, fmt);
	if ((str = g_strdup_vprintf(fmt, ap)) == NULL)
		xerr(NULL, EXIT_FAILURE, "g_strdup_vprintf()");
	(void)vsprintf(str, fmt, ap);
	ok	= new_button(_("_Yes"), GTK_STOCK_YES);
	hbox	= gtk_hbox_new(FALSE, 5);
	image	= gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
		      GTK_ICON_SIZE_DIALOG);
	label	= new_pango_label(ALIGN_CENTER, ALIGN_CENTER, str);
	dialog	= gtk_dialog_new();
	cancel	= new_button(_("_No"), GTK_STOCK_NO);
	content	= gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	g_free(str);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	if (parent != NULL)
		gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	else
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);

	gtk_misc_set_alignment(GTK_MISC(image), 0, ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(hbox), image);

	gtk_container_add(GTK_CONTAINER(hbox), label);

	gtk_container_add(GTK_CONTAINER(content), hbox);
	
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), ok,
	    GTK_RESPONSE_ACCEPT);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), cancel,
	    GTK_RESPONSE_REJECT);
	gtk_widget_show_all(dialog);
	
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_ACCEPT:
		gtk_widget_destroy(dialog);
		gdk_threads_leave();
		return (1);
	case GTK_RESPONSE_REJECT:
		gtk_widget_destroy(dialog);
		gdk_threads_leave();
		return (0);
	}
	gtk_widget_destroy(dialog);
	gdk_threads_leave();

	return (-1);
}

