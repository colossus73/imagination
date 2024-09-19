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

#ifndef __IMAGINATION_INTERFACE_H__
#define __IMAGINATION_INTERFACE_H__

#include <gtk/gtk.h>
#include "support.h"
#include "imagination.h"

img_window_struct *img_create_window(void);
void img_iconview_selection_changed (GtkIconView *, img_window_struct *);

void
img_queue_subtitle_update( GtkTextBuffer     *buffer,
						   img_window_struct *img );

void
img_text_font_set( GtkFontChooser     *button,
				   img_window_struct *img );

void
img_font_color_changed( GtkColorChooser   *button,
						img_window_struct *img );

void
img_font_brdr_color_changed( GtkColorChooser    *button,
                          img_window_struct *img );

void
img_font_bg_color_changed( GtkColorChooser    *button,
                          img_window_struct *img );
                          
void
img_disable_videotab (img_window_struct *img);

void
img_update_sub_properties( img_window_struct *,
						   TextAnimationFunc  anim,
						   gint               anim_id,
						   gdouble            anim_duration,
						   const gchar       *,
						   gdouble           *,
                           gdouble           *,
                           gdouble           *,
                           gdouble           *,
                           gint		         alignment );

void img_combo_box_transition_type_changed (GtkComboBox *,
											img_window_struct *);

void
img_combo_box_anim_speed_changed( GtkSpinButton       *spinbutton,
								  img_window_struct *img );
								  
void
img_text_anim_set( GtkComboBox       *combo,
				   img_window_struct *img );

#endif
