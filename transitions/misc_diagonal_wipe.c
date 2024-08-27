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
transition_render(cairo_t         *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress,
				   gint         type );

/* Plug-in API */
void
img_get_plugin_info( gchar  **group,
					 gchar ***trans )
{
	gint i = 0;
	*group = "Misc Diagonal Wipe";

	*trans = g_new( gchar *, 7 );
	(*trans)[i++] = "Double Barn Door";
	(*trans)[i++] = "img_barn";
	(*trans)[i++] = GINT_TO_POINTER( 38 );
	(*trans)[i++] = "Double Diamond";
	(*trans)[i++] = "img_diamond";
	(*trans)[i++] = GINT_TO_POINTER( 39 );
	(*trans)[i++] = NULL;
}

void
img_barn( cairo_t  *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 1 );
}

void
img_diamond( cairo_t *cr,
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
				   gint				type )
{
	gint width, height, w2, h2;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_surface( cr, image_to, 0, 0 );
	w2 = width / 2;
	h2 = height / 2;
	switch( type )
	{
		case 1:
			cairo_move_to( cr, 0, 0 );
			cairo_line_to( cr, 0, h2 * progress );
			cairo_line_to( cr, w2 * ( 1 - progress ), h2 );
			cairo_line_to( cr, 0, h2 * ( 2 - progress ) );
			cairo_line_to( cr, 0, height );
			cairo_line_to( cr, w2 * progress, height );
			cairo_line_to( cr, w2, h2 * ( 1 + progress ) );
			cairo_line_to( cr, w2 * ( 2 - progress ), height );
			cairo_line_to( cr, width, height );
			cairo_line_to( cr, width, h2 * ( 2 - progress ) );
			cairo_line_to( cr, w2 * ( 1 + progress ), h2 );
			cairo_line_to( cr, width, h2 * progress );
			cairo_line_to( cr, width, 0 );
			cairo_line_to( cr, w2 * ( 2 - progress ), 0 );
			cairo_line_to( cr, w2, h2 * ( 1 - progress ) );
			cairo_line_to( cr, w2 * progress, 0 );
			cairo_close_path( cr );
			break;

		case 2:
			cairo_set_fill_rule( cr, CAIRO_FILL_RULE_EVEN_ODD );

			cairo_move_to( cr, 0, h2 * ( 1 - progress ) );
			cairo_line_to( cr, 0, h2 * ( 1 + progress ) );
			cairo_line_to( cr, w2 * ( 1 - progress ), height );
			cairo_line_to( cr, w2 * ( 1 + progress ), height );
			cairo_line_to( cr, width, h2 * ( 1 + progress ) );
			cairo_line_to( cr, width, h2 * ( 1 - progress ) );
			cairo_line_to( cr, w2 * ( 1 + progress ), 0 );
			cairo_line_to( cr, w2 * ( 1 - progress ), 0 );
			cairo_close_path( cr );

			cairo_new_sub_path( cr );
			cairo_move_to( cr, w2 * progress, h2 );
			cairo_line_to( cr, w2, h2 * ( 2 - progress ) );
			cairo_line_to( cr, w2 * ( 2 - progress ), h2 );
			cairo_line_to( cr, w2, h2 * progress );
			cairo_close_path( cr );
			break;
	}
	cairo_fill( cr );
}
