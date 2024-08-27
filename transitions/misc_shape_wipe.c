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

#include <cairo.h>
#include <glib.h>

/* Local functions declarations go here */
static void
transition_render( cairo_t         *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress,
                   gint				type,
				   gboolean			direction );


/* Plug-in API */
void
img_get_plugin_info( gchar  **group,
                     gchar ***trans )
{
    gint i = 0;
    *group = "Misc Shape Wipe";

    *trans = g_new( gchar *, 13 );

    (*trans)[i++] = "Heart In";
    (*trans)[i++] = "img_heart_in";
    (*trans)[i++] = GINT_TO_POINTER( 56 );

    (*trans)[i++] = "Heart Out";
    (*trans)[i++] = "img_heart_out";
    (*trans)[i++] = GINT_TO_POINTER( 57 );

    (*trans)[i++] = "Keyhole In";
    (*trans)[i++] = "img_key_in";
    (*trans)[i++] = GINT_TO_POINTER( 58 );

    (*trans)[i++] = "Keyhole Out";
    (*trans)[i++] = "img_key_out";
    (*trans)[i++] = GINT_TO_POINTER( 59 );

    (*trans)[i++] = NULL;
}

void
img_heart_in( cairo_t         *cr,
			  cairo_surface_t *image_from,
			  cairo_surface_t *image_to,
			  gdouble          progress )
{
    transition_render( cr, image_from, image_to, progress, 1, TRUE );
}

void
img_heart_out( cairo_t         *cr,
			   cairo_surface_t *image_from,
			   cairo_surface_t *image_to,
			   gdouble          progress )
{
    transition_render( cr, image_from, image_to, progress, 1, FALSE );
}

void
img_key_in( cairo_t         *cr,
			cairo_surface_t *image_from,
			cairo_surface_t *image_to,
			gdouble          progress )
{
    transition_render( cr, image_from, image_to, progress, 2, TRUE );
}

void
img_key_out( cairo_t         *cr,
			 cairo_surface_t *image_from,
			 cairo_surface_t *image_to,
			 gdouble          progress )
{
    transition_render( cr, image_from, image_to, progress, 2, FALSE );
}

/* Local functions definitions */
static void
transition_render(	cairo_t         *cr,
					cairo_surface_t *image_from,
					cairo_surface_t *image_to,
					gdouble          progress,
					gint         type,
					gboolean     direction )
{
	gint width, height, w2, h2, offset;
	cairo_surface_t *layer1, *layer2;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

    /* Drawing code goes here */
	if( direction )
	{
		layer2 = image_from;
		layer1 = image_to;
		progress = 1 - progress;
	}
	else
	{
		layer2 = image_to;
		layer1 = image_from;
	}
	w2 = width / 2;
	h2 = height / 2;

	cairo_set_source_surface( cr, layer1, 0, 0 );
	cairo_paint( cr );

	if( type == 1 )
		offset = 100;
	else
		offset = 40;

	cairo_set_source_surface( cr, layer2, 0, 0 );
	cairo_translate( cr, w2, h2 - ( 1 - progress ) * offset );
	cairo_scale( cr, progress, progress );
	switch( type )
	{
		case 1: /* Heart */
			cairo_move_to( cr, 0, - h2 );

			/* Left hand side */
			cairo_curve_to( cr, - 275, - 355 - h2,
								- 930, - 5 - h2,
								- 450, 495 - h2 );
			cairo_curve_to( cr, - 155, 840 - h2,
								-65, 940 - h2,
								0, 1020 - h2 );
			
			/* Right hand side */
			cairo_curve_to( cr, 65, 940 - h2,
								155, 840 - h2,
								450, 495 - h2 );
			cairo_curve_to( cr, 930, -5 - h2,
								275, - 355 - h2,
								0, - h2 );
			break;

		case 2: /* Keyhole */
			cairo_move_to( cr, 0, - 725 );

			cairo_curve_to( cr, - 275, -725,
								- 500, -500,
								- 500, -230 );
			cairo_curve_to( cr, - 500, -100,
								- 450, 25,
								- 360, 110 );
			cairo_line_to( cr, - 480, 725 );
			cairo_line_to( cr, 480, 725 );
			cairo_line_to( cr, 360, 110 );
			cairo_curve_to( cr, 450, 25,
								500, - 100,
								500, - 230 );
			cairo_curve_to( cr, 500, - 500,
								275, - 725,
								0, - 725 );
			break;
	}
	cairo_fill( cr );
}
