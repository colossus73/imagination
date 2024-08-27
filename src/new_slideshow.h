/*
 *  Copyright (c) 2009-2020 Giuseppe Torelli <colossus73@gmail.com>
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

#ifndef __NEW_SLIDESHOW_H__
#define __NEW_SLIDESHOW_H__

#include <gtk/gtk.h>
#include "main-window.h"
#include "callbacks.h"

void img_new_slideshow_settings_dialog(img_window_struct *, gboolean);
void img_new_slideshow_button_clicked(GtkWidget *, gpointer data );
#endif
