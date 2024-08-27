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
#include <math.h>

/* Local functions declarations */
static void
transition_render( cairo_t         *cr,
				   cairo_surface_t *image_from,
				   cairo_surface_t *image_to,
				   gdouble          progress,
				   gdouble     		initial_rotation,
				   gint				direction );

/* Plug-in API */
void
img_get_plugin_info( gchar  **group,
					 gchar ***trans )
{
	gint i = 0;
	*group = "Clock Wipe";

	*trans = g_new( gchar *, 25 );
	(*trans)[i++] = "Clockwise Twelve";
	(*trans)[i++] = "img_ctw";
	(*trans)[i++] = GINT_TO_POINTER( 40 );
	(*trans)[i++] = "Counter Clockwise Twelve";
	(*trans)[i++] = "img_cctw";
	(*trans)[i++] = GINT_TO_POINTER( 41 );
	(*trans)[i++] = "Clockwise Three";
	(*trans)[i++] = "img_cth";
	(*trans)[i++] = GINT_TO_POINTER( 42 );
	(*trans)[i++] = "Counter Clockwise Three";
	(*trans)[i++] = "img_ccth";
	(*trans)[i++] = GINT_TO_POINTER( 43 );
	(*trans)[i++] = "Clockwise Six";
	(*trans)[i++] = "img_csi";
	(*trans)[i++] = GINT_TO_POINTER( 44 );
	(*trans)[i++] = "Counter Clockwise Six";
	(*trans)[i++] = "img_ccsi";
	(*trans)[i++] = GINT_TO_POINTER( 45 );
	(*trans)[i++] = "Clockwise Nine";
	(*trans)[i++] = "img_cni";
	(*trans)[i++] = GINT_TO_POINTER( 46 );
	(*trans)[i++] = "Counter Clockwise Nine";
	(*trans)[i++] = "img_ccni";
	(*trans)[i++] = GINT_TO_POINTER( 47 );
	(*trans)[i++] = NULL;
}

void
img_ctw(cairo_t         *cr,
		cairo_surface_t *image_from,
		cairo_surface_t *image_to,
		gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 3, 1 );
}

void
img_cth(cairo_t         *cr,
		cairo_surface_t *image_from,
		cairo_surface_t *image_to,
		gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 0, 1 );
}

void
img_csi(cairo_t         *cr,
		cairo_surface_t *image_from,
		cairo_surface_t *image_to,
		gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 1, 1 );
}

void
img_cni(cairo_t         *cr,
		cairo_surface_t *image_from,
		cairo_surface_t *image_to,
		gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 2, 1 );
}

void
img_cctw(cairo_t         *cr,
		cairo_surface_t *image_from,
		cairo_surface_t *image_to,
		gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 3, -1 );
}

void
img_ccth(	cairo_t         *cr,
			cairo_surface_t *image_from,
			cairo_surface_t *image_to,
			gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 0, -1 );
}

void
img_ccsi(	cairo_t         *cr,
			cairo_surface_t *image_from,
			cairo_surface_t *image_to,
			gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 1, -1 );
}

void
img_ccni(	cairo_t         *cr,
			cairo_surface_t *image_from,
			cairo_surface_t *image_to,
			gdouble          progress)
{
	transition_render( cr, image_from, image_to, progress, 2, -1 );
}


/* Local functions definitions */
static void
transition_render(	cairo_t         *cr,
					cairo_surface_t *image_from,
					cairo_surface_t *image_to,
					gdouble          progress,
				   	gdouble			initial_rotation,
				   	gint			direction )
{
	gint width, height;
	gdouble          begin, end;
	gdouble          diag;

	width  = cairo_image_surface_get_width( image_from );
	height = cairo_image_surface_get_height( image_from );

	cairo_set_source_surface( cr, image_from, 0, 0 );
	cairo_paint( cr );

	diag = sqrt( ( width * width ) + ( height * height ) );
	cairo_set_source_surface( cr, image_to, 0, 0 );
	cairo_move_to( cr, width / 2 , height / 2 );

	begin = initial_rotation / 2 * G_PI;
	end   = begin + direction * progress * 2 * G_PI;
	if( begin < end )
		cairo_arc( cr, width / 2, height / 2, diag, begin, end );
	else
		cairo_arc( cr, width / 2, height / 2, diag, end, begin );

	cairo_close_path( cr );
	cairo_fill( cr );
}
