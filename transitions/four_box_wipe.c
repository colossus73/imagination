/*
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

#include <cairo.h>
#include <glib.h>

/* Local functions declarations */
static void
transition_render( cairo_t         *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress,
				   gint             type);

/* Plug-in API */
void
img_get_plugin_info( gchar  **group,
					 gchar ***trans )
{
	gint i = 0;
	*group = "Four Box Wipe";

	*trans = g_new( gchar *, 7 );
	(*trans)[i++] = "Corners In";
	(*trans)[i++] = "img_corners_in";
	(*trans)[i++] = GINT_TO_POINTER( 13 );
	(*trans)[i++] = "Corners Out";
	(*trans)[i++] = "img_corners_out";
	(*trans)[i++] = GINT_TO_POINTER( 14 );
	(*trans)[i++] = NULL;
}

void
img_corners_in( cairo_t         *cr,
				cairo_surface_t *image_from,
				cairo_surface_t *image_to,
				gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 1 );
}

void
img_corners_out(cairo_t         *cr,
				cairo_surface_t *image_from,
				cairo_surface_t *image_to,
				gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 2 );
}

/* Local functions definitions */
static void
transition_render( cairo_t         *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress,
				   gint         	direction )
{
	gint width, height, w, h, x, y;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_surface( cr, image_to, 0, 0 );
	w = width  * progress / 2;
	h = height * progress / 2;
	switch( direction )
	{
		case 1:
			cairo_rectangle( cr, 0, 0, w, h );
			cairo_rectangle( cr, width - w, 0, w, h );
			cairo_rectangle( cr, 0, height - h , w, h );
			cairo_rectangle( cr, width - w, height - h, w, h );
			break;
		case 2:
			x = ( ( width  / 2 ) - w ) / 2;
			y = ( ( height / 2 ) - h ) / 2;
			cairo_rectangle( cr, x, y, w, h );
			cairo_rectangle( cr, width - w - x, y, w, h );
			cairo_rectangle( cr, x, height - h - y, w, h );
			cairo_rectangle( cr, width - w - x, height - h - y, w, h );
			break;
	}
	cairo_clip(cr );
	cairo_paint(cr);
}
