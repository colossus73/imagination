/*
** Copyright (C) 2009 Tadej Borovšak <tadeboro@gmail.com>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __SUBTITLES_H__
#define __SUBTITLES_H__

#include "imagination.h"


typedef struct _TextAnimation TextAnimation;
struct _TextAnimation
{
	gchar             *name; /* Name of animation */
	TextAnimationFunc  func; /* Actual renderer */
	gint               id;   /* Unique id (for save and load operations) */
};


gint
img_get_text_animation_list( TextAnimation **animations );

void
img_free_text_animation_list( gint           no_animations,
							  TextAnimation *animations );

void
img_render_subtitle( img_window_struct	  *img,
					 cairo_t              *cr,
					 gdouble               zoom,
					 gint					posx,
					 gint					posy,
					 gint					angle,
					 gint					layout,
					 gdouble               factor,
					 gdouble               offx,
					 gdouble               offy,
					 gboolean				centerX,
					 gboolean				centerY,
					 gdouble               progress );

void
img_set_slide_text_info( slide_struct      *slide,
						 GtkListStore      *store,
						 GtkTreeIter       *iter,
						 guint8		       *subtitle,
						 gchar		       *pattern_filename,
						 gint	            anim_id,
						 gint               anim_duration,
						 gint               posx,
						 gint               posy,
						 gint               angle,
						 const gchar       *font_desc,
						 gdouble           *font_color,
                         gdouble           *font_brdr_color,
                         gdouble           *font_bgcolor,
                         gdouble           *border_color,
                         gboolean			top_border,
						 gboolean			bottom_border,
                         gint	           alignment,
                         gint	           border_width,
						 img_window_struct *img );

#endif
