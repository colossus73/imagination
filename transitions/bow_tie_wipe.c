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
				   gint				direction );

/* Plug-in API */
void
img_get_plugin_info( gchar  **group,
					 gchar ***trans )
{
	gint i = 0;

	*group = "Bow Tie Wipe";

	*trans = g_new( gchar *, 7 );
	(*trans)[i++] = "Vertical";
	(*trans)[i++] = "img_vertical";
	(*trans)[i++] = GINT_TO_POINTER( 36 );
	(*trans)[i++] = "Horizontal";
	(*trans)[i++] = "img_horizontal";
	(*trans)[i++] = GINT_TO_POINTER( 37 );
	(*trans)[i++] = NULL;
}

void
img_vertical( 	cairo_t         *cr,
				cairo_surface_t *image_from,
				cairo_surface_t *image_to,
				gdouble          progress,
			 	gint         file_desc )
{
	transition_render( cr, image_from, image_to, progress, 1 );
}

void
img_horizontal( cairo_t         *cr,
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
				   gint      		direction )
{
	gint tmp;
	gint width, height;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_surface( cr, image_to, 0, 0 );

	switch( direction )
	{
		case 1:	/* vertical */
			tmp = width / 2;
			cairo_move_to( cr, tmp * ( 1 - 2 * progress ), 0 );
			cairo_line_to( cr, tmp, height * progress );
			cairo_line_to( cr, tmp * ( 1 + 2 * progress ), 0 );
			cairo_close_path( cr );
			cairo_fill( cr );

			cairo_move_to( cr, tmp * ( 1 - 2 * progress ), height );
			cairo_line_to( cr, tmp, height * ( 1 - progress ) );
			cairo_line_to( cr, tmp * ( 1 + 2 * progress ), height );
			cairo_close_path( cr );
			cairo_fill( cr );
			break;

		case 2:	/* horizontal */
			tmp = height / 2;
			cairo_move_to( cr, 0, tmp * ( 1 - 2 * progress ) );
			cairo_line_to( cr, width * progress, tmp );
			cairo_line_to( cr, 0, tmp * ( 1 + 2 * progress ) );
			cairo_close_path( cr );
			cairo_fill( cr );

			cairo_move_to( cr, width, tmp * ( 1 - 2 * progress ) );
			cairo_line_to( cr, width * ( 1 - progress ), tmp );
			cairo_line_to( cr, width, tmp * ( 1 + 2 * progress ) );
			cairo_close_path( cr );
			cairo_fill( cr );
			break;
	}
}
