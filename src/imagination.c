/*
 *  Copyright (c) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License,or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not,write to the Free Software
 *  Foundation,Inc.,59 Temple Place - Suite 330,Boston,MA 02111-1307,USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "main-window.h"
#include "support.h"
#include "callbacks.h"
#include "export.h"

int main (int argc, char *argv[])
{
	img_window_struct *img_window;

	#ifdef ENABLE_NLS
  		bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  		textdomain (GETTEXT_PACKAGE);
	#endif
	
	gtk_init (&argc, &argv);

	#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,9,100)
		av_register_all();
		avcodec_register_all();
	#endif

	img_window = img_create_window();

	/* Load the transitions as plugins with GModule */
	img_load_available_transitions(img_window);
	g_print(_("Welcome to Imagination " VERSION " - %d transitions loaded\n"), img_window->nr_transitions_loaded);	
	g_print("%s\n", LIBAVCODEC_IDENT);
	g_print("%s\n", LIBAVFORMAT_IDENT);
	g_print("%s\n", LIBAVUTIL_IDENT);
	gtk_widget_show_all( img_window->imagination_window );

	/* Reads the arguments passed from the cmd line */
 	if (argc > 1)
		img_load_slideshow( img_window, NULL, argv[1] );

	gtk_main ();

	g_object_unref(img_window->textbox->layout);
	pango_font_description_free(img_window->textbox->font_desc);
	g_string_free(img_window->textbox->text, FALSE);
    g_free(img_window->textbox);
    
	g_free(img_window);
	return 0;
}

