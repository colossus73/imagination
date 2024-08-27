/*
 *  Copyright (c) 2009 Giuseppe Torelli <colossus73@gmail.com>
 *  Copyright (C) 2009 Tadej Borovšak <tadeboro@gmail.com>
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
#include <stdlib.h>

#define RAND_VALS 10 /* Number of random values to use */

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

	*group = "Misc";

	*trans = g_new( gchar *, 7 );
	(*trans)[i++] = "Cross Fade";
	(*trans)[i++] = "img_cross_fade";
	(*trans)[i++] = GINT_TO_POINTER( 19 );
	(*trans)[i++] = "Dissolve";
	(*trans)[i++] = "img_dissolve";
	(*trans)[i++] = GINT_TO_POINTER( 67 );
	(*trans)[i++] = NULL;
}

void
img_cross_fade( cairo_t         *cr,
				cairo_surface_t *image_from,
				cairo_surface_t *image_to,
				gdouble          progress )
{
	transition_render( cr, image_from, image_to, progress, 1 );
}

/* Local functions definitions */
static void
transition_render( cairo_t         *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress,
				   gint             direction )
{
	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_surface( cr, image_to, 0, 0 );
	cairo_paint_with_alpha( cr, progress );
}

void
img_dissolve( cairo_t         *cr,
			  cairo_surface_t *image_from,
			  cairo_surface_t *image_to,
			  gdouble          progress )
{
	gint    width,  /* Image width */
			height, /* Image height */
			size,   /* Size of the surface (width * height) */
			draw,   /* Number of pixels that need to be filled */
			count;  /* Number of pixels that are already drawn */
	guchar *data;   /* Mask surface data */
	gint    values[RAND_VALS]; /* Random values */

	/* Persistent data */
	static cairo_surface_t *mask = NULL;
	static gint             filled;
	static gint             stride;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_surface( cr, image_to, 0, 0 );

	/* Create new mask surface if we're starting effect */
	if( progress < 0.00001 )
	{
		if( mask )
			cairo_surface_destroy( mask );

		mask = cairo_image_surface_create( CAIRO_FORMAT_A1, width, height );
		stride = cairo_image_surface_get_stride( mask );
		filled = 0;

		return;
	}
	else if( progress > 0.9999 )
	{
		cairo_paint( cr );
		return;
	}

	/* Calculate number of pixels that need to be drawn in this round. */
	size = width * height;
	draw = size * progress - filled;
	filled += draw;

	/* Get random values */
	for( count = 0; count < RAND_VALS; count++ )
		values[count] = rand() % size;

	/* set fixel values on mask surface */
	cairo_surface_flush( mask );
	data = cairo_image_surface_get_data( mask );

	for( count = 0; count < draw; count++ )
	{
		gint        row,
					col,
					index,
					shift;
		static gint value = 0;

		value += values[count % RAND_VALS] % size;
		value %= size;
		row = value / width;
		col = value - width * row;

		do
		{
			col++;
			if( col == width )
			{
				col = 0;
				row++;
				row %= height;
			}

			index = row * stride + ( col / 8 );
			shift = col % 8;
		}
		while( data[index] & ( 1 << shift ) );

		data[index] |= ( 1 << shift );
	}
	cairo_surface_mark_dirty( mask );

	/* Paint second surface */
	cairo_mask_surface( cr, mask, 0, 0 );
}
