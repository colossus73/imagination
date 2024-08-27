/*
 *  Copyright (c) 2009 Giuseppe Torelli <colossus73@gmail.com>
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

	*group = "Push Wipe";

	*trans = g_new( gchar *, 13 );
	(*trans)[i++] = "From Left";
	(*trans)[i++] = "img_from_left";
	(*trans)[i++] = GINT_TO_POINTER( 26 );
	(*trans)[i++] = "From Right";
	(*trans)[i++] = "img_from_right";
	(*trans)[i++] = GINT_TO_POINTER( 27 );
	(*trans)[i++] = "From Top";
	(*trans)[i++] = "img_from_top";
	(*trans)[i++] = GINT_TO_POINTER( 28 );
	(*trans)[i++] = "From Bottom";
	(*trans)[i++] = "img_from_bottom";
	(*trans)[i++] = GINT_TO_POINTER( 29 );
	(*trans)[i++] = NULL;
}

void img_from_left( cairo_t *cr, cairo_surface_t *image_from, cairo_surface_t *image_to, gdouble progress )
{
	transition_render( cr, image_from, image_to, progress, 1 );
}

void img_from_right( cairo_t *cr, cairo_surface_t *image_from, cairo_surface_t *image_to, gdouble progress )
{
	transition_render( cr, image_from, image_to, progress, 2 );
}

void img_from_top( cairo_t *cr, cairo_surface_t *image_from, cairo_surface_t *image_to, gdouble progress )
{
	transition_render( cr, image_from, image_to, progress, 3 );
}

void img_from_bottom( cairo_t *cr, cairo_surface_t *image_from, cairo_surface_t *image_to, gdouble progress )
{
	transition_render( cr, image_from, image_to, progress, 4 );
}

/* Local functions definitions */
static void
transition_render( cairo_t         *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress,
				   gint				direction )
{
	gint width, height;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	switch (direction)
	{
		case 1:
		cairo_set_source_surface(cr,image_to,-width * (1- progress),0);
		break;
		
		case 2:
		cairo_set_source_surface(cr,image_from,-width * progress,0);
		break;
		
		case 3:
		cairo_set_source_surface(cr,image_from,0,height * progress);
		break;
		
		case 4:
		cairo_set_source_surface(cr,image_from,0,-height * progress);
		break;
	}
	cairo_paint( cr );

	switch (direction)
	{
		case 1:
		cairo_set_source_surface(cr,image_from,width * progress,0);
		break;
		
		case 2:
		cairo_set_source_surface(cr,image_to,width * (1 - progress),0);
		break;
		
		case 3:
		cairo_set_source_surface(cr,image_to,0,-height * (1 - progress));
		break;
		
		case 4:
		cairo_set_source_surface(cr,image_to,0,height * (1 - progress));
		break;
	}
   	cairo_paint(cr);
}
