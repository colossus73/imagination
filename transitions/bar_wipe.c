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
				   gint             direction );

/* Plug-in API */
void
img_get_plugin_info( gchar  **group,
					 gchar ***trans )
{
	gint i = 0;

	*group = "Bar Wipe";

	*trans = g_new( gchar *, 13 );
	(*trans)[i++] = "Left to Right";
	(*trans)[i++] = "img_left";
	(*trans)[i++] = GINT_TO_POINTER( 1 );
	(*trans)[i++] = "Top to Bottom";
	(*trans)[i++] = "img_top";
	(*trans)[i++] = GINT_TO_POINTER( 2 );
	(*trans)[i++] = "Right to Left";
	(*trans)[i++] = "img_right";
	(*trans)[i++] = GINT_TO_POINTER( 3 );
	(*trans)[i++] = "Bottom to Top";
	(*trans)[i++] = "img_bottom";
	(*trans)[i++] = GINT_TO_POINTER( 4 );
	(*trans)[i++] = NULL;
}

void
img_left( cairo_t         *cr,
		  cairo_surface_t *image_from,
		  cairo_surface_t *image_to,
		  gdouble          progress )
{
	transition_render( cr, image_from, image_to, progress, 1 );
}

void
img_top( cairo_t         *cr,
		 cairo_surface_t *image_from,
		 cairo_surface_t *image_to,
		 gdouble          progress )
{
	transition_render( cr, image_from, image_to, progress, 2 );
}

void
img_right( cairo_t         *cr,
		   cairo_surface_t *image_from,
		   cairo_surface_t *image_to,
		   gdouble          progress )
{
	transition_render( cr, image_from, image_to, progress, 3 );
}

void
img_bottom( cairo_t         *cr,
			cairo_surface_t *image_from,
			cairo_surface_t *image_to,
			gdouble          progress )
{
	transition_render( cr, image_from, image_to, progress, 4 );
}

/* Local functions definitions */
static void
transition_render( cairo_t         *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress,
				   gint             direction )
{
	gint width, height;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_surface( cr, image_to, 0, 0 );

	switch( direction )
	{
		case 1:	/* left */
			cairo_rectangle( cr, 0, 0, width * progress, height );
			break;
		case 2:	/* top */
			cairo_rectangle( cr, 0, 0, width, height * progress );
			break;
		case 3:	/* right */
			cairo_rectangle( cr, width * ( 1 - progress ), 0, width, height );
			break;
		case 4:	/* bottom */
			cairo_rectangle( cr, 0, height * ( 1 - progress ), width, height );
			break;
	}
	cairo_fill( cr );
}
