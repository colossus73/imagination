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
	*group = "Box Wipe";

	*trans = g_new( gchar *, 25 );
	(*trans)[i++] = "Top Left";
	(*trans)[i++] = "img_top_left";
	(*trans)[i++] = GINT_TO_POINTER( 5 );
	(*trans)[i++] = "Top Right";
	(*trans)[i++] = "img_top_right";
	(*trans)[i++] = GINT_TO_POINTER( 6 );
	(*trans)[i++] = "Bottom Right";
	(*trans)[i++] = "img_bottom_right";
	(*trans)[i++] = GINT_TO_POINTER( 7 );
	(*trans)[i++] = "Bottom Left";
	(*trans)[i++] = "img_bottom_left";
	(*trans)[i++] = GINT_TO_POINTER( 8 );
	(*trans)[i++] = "Top Center";
	(*trans)[i++] = "img_top_center";
	(*trans)[i++] = GINT_TO_POINTER( 9 );
	(*trans)[i++] = "Right Center";
	(*trans)[i++] = "img_right_center";
	(*trans)[i++] = GINT_TO_POINTER( 10 );
	(*trans)[i++] = "Bottom Center";
	(*trans)[i++] = "img_bottom_center";
	(*trans)[i++] = GINT_TO_POINTER( 11 );
	(*trans)[i++] = "Left Center";
	(*trans)[i++] = "img_left_center";
	(*trans)[i++] = GINT_TO_POINTER( 12 );
	(*trans)[i++] = NULL;
}

void
img_top_left( cairo_t         *cr,
			  cairo_surface_t *image_from,
			  cairo_surface_t *image_to,
			  gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 1 );
}

void
img_top_right( cairo_t         *cr,
			   cairo_surface_t *image_from,
			   cairo_surface_t *image_to,
			   gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 2 );
}

void
img_bottom_right(cairo_t         *cr,
			  	 cairo_surface_t *image_from,
			  	 cairo_surface_t *image_to,
			  	 gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 3 );
}

void
img_bottom_left(	cairo_t         *cr,
			   		cairo_surface_t *image_from,
			   		cairo_surface_t *image_to,
			   		gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 4 );
}

void
img_top_center( cairo_t         *cr,
			  	cairo_surface_t *image_from,
			   	cairo_surface_t *image_to,
			   	gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 5 );
}

void
img_right_center(	cairo_t         *cr,
			 		cairo_surface_t *image_from,
			   		cairo_surface_t *image_to,
			   		gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 6 );
}

void
img_bottom_center(	cairo_t         *cr,
			   		cairo_surface_t *image_from,
			   		cairo_surface_t *image_to,
			   		gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 7 );
}

void
img_left_center(cairo_t         *cr,
			   	cairo_surface_t *image_from,
			   	cairo_surface_t *image_to,
			   	gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 8 );
}

/* Local functions definitions */
static void
transition_render(	cairo_t         *cr,
			   		cairo_surface_t *image_from,
			   		cairo_surface_t *image_to,
			   		gdouble          progress,
					gint             direction )
{
	gint             x, y;
	gint width, height;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_surface( cr, image_to, 0, 0 );
	switch( direction )
	{
		case 1:
			cairo_rectangle( cr, 0, 0, width * progress, height * progress );
			break;
		case 2:
			cairo_rectangle( cr, width * ( 1 - progress ), 0,
								 width * progress, height * progress );
			break;
		case 3:
			cairo_rectangle( cr, width  * ( 1 - progress ),
								 height * ( 1 - progress ),
								 width  * progress, height * progress );
			break;
		case 4:
			cairo_rectangle( cr, 0, height * ( 1 - progress ),
								 width * progress, height * progress );
			break;
		case 5:
			x = ( width * ( 1 - progress ) ) / 2;
			cairo_rectangle( cr, x, 0, width * progress, height * progress );
			break;
		case 6:
			y = ( height * ( 1 - progress ) ) / 2;
			cairo_rectangle( cr, width * ( 1 - progress ), y, width * progress,
								 height * progress );
			break;
		case 7:
			x = ( width * ( 1 - progress ) ) / 2;
			cairo_rectangle( cr, x, height * ( 1 - progress ),
								 width * progress, height * progress );
			break;
		case 8:
			y = ( height * ( 1 - progress ) ) / 2;
			cairo_rectangle( cr, 0, y, width * progress,
								 height * progress );
			break;
	}

	cairo_clip(cr);
	cairo_paint(cr);
}
