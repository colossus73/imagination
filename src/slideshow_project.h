/*
 *  Copyright (c) 2009 Giuseppe Torelli <colossus73@gmail.com>
 *  Copyright (c) 2009 Tadej Borovšak 	<tadeboro@gmail.com>
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
 
#ifndef __SLIDESHOW_PROJECT_H__
#define __SLIDESHOW_PROJECT_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include "imagination.h"
#include "support.h"
#include "callbacks.h"

void
img_save_slideshow( img_window_struct *img,
					const gchar       *output,
					gboolean		   relative );

void
img_load_slideshow( img_window_struct *img,
					GtkWidget			*menu,
					const gchar       *input );

gboolean
img_append_slides_from( img_window_struct *img, GtkWidget *, const gchar *input );
#endif
