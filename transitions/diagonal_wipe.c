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
transition_render( cairo_t		*cr,
				cairo_surface_t *image_from,
				cairo_surface_t *image_to,
				gdouble 		progress,
				gint   			direction );

/* Plug-in API */
void
img_get_plugin_info( gchar  **group,
					 gchar ***trans )
{
	gint i = 0;

	*group = "Diagonal Wipe";

	*trans = g_new( gchar *, 13 );
	(*trans)[i++] = "Top Left";
	(*trans)[i++] = "img_top_left";
	(*trans)[i++] = GINT_TO_POINTER( 32 );
	(*trans)[i++] = "Top Right";
	(*trans)[i++] = "img_top_right";
	(*trans)[i++] = GINT_TO_POINTER( 33 );
	(*trans)[i++] = "Bottom Right";
	(*trans)[i++] = "img_bottom_right";
	(*trans)[i++] = GINT_TO_POINTER( 34 );
	(*trans)[i++] = "Bottom Left";
	(*trans)[i++] = "img_bottom_left";
	(*trans)[i++] = GINT_TO_POINTER( 35 );
	(*trans)[i++] = NULL;
}

void
img_top_left(	cairo_t         *cr,
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
img_bottom_right( cairo_t         *cr,
				cairo_surface_t *image_from,
				cairo_surface_t *image_to,
				gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 3 );
}

void
img_bottom_left( cairo_t         *cr,
				cairo_surface_t *image_from,
				cairo_surface_t *image_to,
				gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 4 );
}

/* Local functions definitions */
static void
transition_render( cairo_t         *cr,
				cairo_surface_t *image_from,
				cairo_surface_t *image_to,
				gdouble          progress,
				gint   			direction)
{
	gint width, height;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_surface( cr, image_to, 0, 0 );

	progress *= 2;
	switch( direction )
	{
		case 1:	/* top left */
			cairo_move_to( cr, 0, 0 );
			if( progress < 1 )
			{
				cairo_line_to( cr, 0, progress * height );
				cairo_line_to( cr, progress * width, 0 );
			}
			else
			{
				progress -= 1;
				cairo_line_to( cr, 0, height );
				cairo_line_to( cr, progress * width, height );
				cairo_line_to( cr, width, progress * height );
				cairo_line_to( cr, width, 0 );
			}
			break;
		case 2:	/* top right */
			cairo_move_to( cr, width, 0 );
			if( progress < 1 )
			{
				cairo_line_to( cr, width, progress * height );
				cairo_line_to( cr, ( 1 - progress ) * width, 0 );
			}
			else
			{
				progress -= 1;
				cairo_line_to( cr, width, height );
				cairo_line_to( cr, ( 1 - progress ) * width, height );
				cairo_line_to( cr, 0, progress * height );
				cairo_line_to( cr, 0, 0 );
			}
			break;
		case 3:	/* bottom right */
			cairo_move_to( cr, width, height );
			if( progress < 1 )
			{
				cairo_line_to( cr, width, ( 1 - progress ) * height );
				cairo_line_to( cr, ( 1 - progress ) * width, height );
			}
			else
			{
				progress -= 1;
				cairo_line_to( cr, width, 0 );
				cairo_line_to( cr, ( 1 - progress ) * width, 0 );
				cairo_line_to( cr, 0, ( 1 - progress ) * height );
				cairo_line_to( cr, 0, height );
			}
			break;
		case 4:	/* bottom left */
			cairo_move_to( cr, 0, height );
			if( progress < 1 )
			{
				cairo_line_to( cr, 0, ( 1 - progress ) * height );
				cairo_line_to( cr, progress * width, height );
			}
			else
			{
				progress -= 1;
				cairo_line_to( cr, 0, 0 );
				cairo_line_to( cr, progress * width, 0 );
				cairo_line_to( cr, width, ( 1 - progress ) * height );
				cairo_line_to( cr, width, height );
			}
			break;
	}

	cairo_close_path( cr );
	cairo_clip(cr);
	cairo_paint(cr);
}
