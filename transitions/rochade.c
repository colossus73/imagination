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
#include <math.h>

#define POW2( val ) \
	( ( val ) * ( val ) )

static void
rochade_h( cairo_t         *cr,
		   cairo_surface_t *image_from,
		   cairo_surface_t *image_to,
		   gdouble          progress,
		   gboolean         right );

static void
rochade_v( cairo_t         *cr,
		   cairo_surface_t *image_from,
		   cairo_surface_t *image_to,
		   gdouble          progress,
		   gboolean         up );

/* Plug-in API */
void
img_get_plugin_info( gchar  **group,
					 gchar ***trans )
{
	gint i = 0;

	*group = "Rochade";

	*trans = g_new( gchar *, 13 );
	(*trans)[i++] = "Rochade Right";
	(*trans)[i++] = "right";
	(*trans)[i++] = GINT_TO_POINTER( 68 );
	(*trans)[i++] = "Rochade Left";
	(*trans)[i++] = "left";
	(*trans)[i++] = GINT_TO_POINTER( 69 );
	(*trans)[i++] = "Rochade Up";
	(*trans)[i++] = "up";
	(*trans)[i++] = GINT_TO_POINTER( 70 );
	(*trans)[i++] = "Rochade Down";
	(*trans)[i++] = "down";
	(*trans)[i++] = GINT_TO_POINTER( 71 );
	(*trans)[i++] = NULL;
}

void
left( cairo_t         *cr,
	  cairo_surface_t *image_from,
	  cairo_surface_t *image_to,
	  gdouble          progress )
{
	rochade_h( cr, image_from, image_to, progress, FALSE );
}

void
right( cairo_t         *cr,
	   cairo_surface_t *image_from,
	   cairo_surface_t *image_to,
	   gdouble          progress )
{
	rochade_h( cr, image_from, image_to, progress, TRUE );
}

void
up( cairo_t         *cr,
	cairo_surface_t *image_from,
	cairo_surface_t *image_to,
	gdouble          progress )
{
	rochade_v( cr, image_from, image_to, progress, FALSE );
}

void
down( cairo_t         *cr,
	  cairo_surface_t *image_from,
	  cairo_surface_t *image_to,
	  gdouble          progress )
{
	rochade_v( cr, image_from, image_to, progress, TRUE );
}

static void
rochade_h( cairo_t         *cr,
		   cairo_surface_t *image_from,
		   cairo_surface_t *image_to,
		   gdouble          progress,
		   gboolean         right )
{
	const gdouble b = 70;
	gint          width, height;
	gdouble       x, y, ff, ft, w2;
	gdouble       state;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );
	w2 = width  * 0.5;

	if( progress > 0.5 )
		state = progress - 1;
	else
		state = progress;

	x = width * state;
	y = sqrt( ( POW2( w2 ) - POW2( x ) ) / POW2( w2 ) ) * b;

	if( progress < 0.5 )
	{
		ff = 1 - 2 * ( b - y ) / height;
		ft = 1 - 2 * ( b + y ) / height;
	}
	else
	{
		ff = 1 - 2 * ( b + y ) / height;
		ft = 1 - 2 * ( b - y ) / height;
	}

	cairo_set_source_rgb( cr, 0, 0, 0 );
	cairo_paint( cr );

	if( progress < 0.5 )
	{
		/* image_to */
		cairo_save( cr );
		cairo_translate( cr, w2 * ( 1 - ft ) + ( right ? - x : x ), b + y );
		cairo_scale( cr, ft, ft );
		cairo_set_source_surface( cr, image_to, 0, 0 );
		cairo_paint( cr );
		cairo_restore( cr );

		/* image_from */
		cairo_translate( cr, w2 * ( 1 - ff ) + ( right ? x : - x ), b - y );
		cairo_scale( cr, ff, ff );
		cairo_set_source_surface( cr, image_from, 0, 0 );
		cairo_paint( cr );
	}
	else
	{
		/* image_from */
		cairo_save( cr );
		cairo_translate( cr, w2 * ( 1 - ff ) + ( right ? - x : x ), b + y );
		cairo_scale( cr, ff, ff );
		cairo_set_source_surface( cr, image_from, 0, 0 );
		cairo_paint( cr );
		cairo_restore( cr );

		/* image_to */
		cairo_translate( cr, w2 * ( 1 - ft ) + ( right ? x : - x ), b - y );
		cairo_scale( cr, ft, ft );
		cairo_set_source_surface( cr, image_to, 0, 0 );
		cairo_paint( cr );
	}
}

static void
rochade_v( cairo_t         *cr,
		   cairo_surface_t *image_from,
		   cairo_surface_t *image_to,
		   gdouble          progress,
		   gboolean         up )
{
	const gdouble a = 70;
	gint          width, height;
	gdouble       x, y, ff, ft, h2;
	gdouble       state;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );
	h2 = height * 0.5;

	if( progress > 0.5 )
		state = progress - 1;
	else
		state = progress;

	y = height * state;
	x = sqrt( ( POW2( h2 ) - POW2( y ) ) / POW2( h2 ) ) * a;

	if( progress < 0.5 )
	{
		ff = 1 - 2 * ( a - x ) / width;
		ft = 1 - 2 * ( a + x ) / width;
	}
	else
	{
		ff = 1 - 2 * ( a + x ) / width;
		ft = 1 - 2 * ( a - x ) / width;
	}

	cairo_set_source_rgb( cr, 0, 0, 0 );
	cairo_paint( cr );

	if( progress < 0.5 )
	{
		/* image_to */
		cairo_save( cr );
		cairo_translate( cr, a + x, h2 * ( 1 - ft ) + ( up ? - y : y ) );
		cairo_scale( cr, ft, ft );
		cairo_set_source_surface( cr, image_to, 0, 0 );
		cairo_paint( cr );
		cairo_restore( cr );

		/* image_from */
		cairo_translate( cr, a - x, h2 * ( 1 - ff ) + ( up ? y : - y ) );
		cairo_scale( cr, ff, ff );
		cairo_set_source_surface( cr, image_from, 0, 0 );
		cairo_paint( cr );
	}
	else
	{
		/* image_from */
		cairo_save( cr );
		cairo_translate( cr, a + x, h2 * ( 1 - ff ) + ( up ? - y : y ) );
		cairo_scale( cr, ff, ff );
		cairo_set_source_surface( cr, image_from, 0, 0 );
		cairo_paint( cr );
		cairo_restore( cr );

		/* image_to */
		cairo_translate( cr, a - x, h2 * ( 1 - ft ) + ( up ? y : - y ) );
		cairo_scale( cr, ft, ft );
		cairo_set_source_surface( cr, image_to, 0, 0 );
		cairo_paint( cr );
	}
}

