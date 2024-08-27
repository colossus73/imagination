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

#include "imgcellrendererpixbuf.h"
#include "imagination.h"

#define IMG_PARAM_DEFAULTS \
	G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB
#define IMG_PARAM_READWRITE \
	G_PARAM_WRITABLE | G_PARAM_READABLE | IMG_PARAM_DEFAULTS

#define HORIZ_F 0.3
#define VERT_F  0.5
#define BORDER  2

/* Properties */
enum
{
	P_0,
	P_HAS_TEXT,   /* Does selected slide contains text? */
	P_TEXT_ICO,   /* Icon describing text */
	P_TRANSITION, /* Transition pixbuf */
	P_PIXBUF,     /* Main pixbuf to be rendered */
	P_ZOOM        /* Zoom factor */
};

#define IMG_CELL_RENDERER_PIXBUF_GET_PRIVATE( obj ) \
								   img_cell_renderer_pixbuf_get_instance_private(IMG_CELL_RENDERER_PIXBUF( obj))

typedef struct _ImgCellRendererPixbufPrivate ImgCellRendererPixbufPrivate;
struct _ImgCellRendererPixbufPrivate
{
	gboolean   has_text;
	GdkPixbuf *text_ico;
	GdkPixbuf *transition;
	GdkPixbuf *pixbuf;
	gdouble    zoom;
};

/* ****************************************************************************
 * Local declarations
 * ************************************************************************* */
static void
img_cell_renderer_pixbuf_set_property( GObject      *object,
									   guint         prop_id,
									   const GValue *value,
									   GParamSpec   *pspec );

static void
img_cell_renderer_pixbuf_get_property( GObject    *object,
									   guint       prop_id,
									   GValue     *value,
									   GParamSpec *pspec );

static void
img_cell_renderer_pixbuf_render( GtkCellRenderer      *cell,
								 cairo_t              *cr,
								 GtkWidget            *widget,
								 const GdkRectangle         *background_a,
								 const GdkRectangle         *cell_a,
								 GtkCellRendererState  state );

static void
img_cell_renderer_pixbuf_get_size( GtkCellRenderer *cell,
								   GtkWidget       *widget,
								   const GdkRectangle    *cell_area,
								   gint            *x_off,
								   gint            *y_off,
								   gint            *width,
								   gint            *height );


static void
img_cell_renderer_pixbuf_finalize( GObject *object );

/* ****************************************************************************
 * Initialization
 * ************************************************************************* */
G_DEFINE_TYPE_WITH_CODE( ImgCellRendererPixbuf,
			   img_cell_renderer_pixbuf,
			   GTK_TYPE_CELL_RENDERER,
			   G_ADD_PRIVATE(ImgCellRendererPixbuf));

static void
img_cell_renderer_pixbuf_class_init( ImgCellRendererPixbufClass *klass )
{
	GParamSpec           *spec;
	GObjectClass         *gobject_class = G_OBJECT_CLASS( klass );
	GtkCellRendererClass *renderer_class = GTK_CELL_RENDERER_CLASS( klass );

	gobject_class->set_property = img_cell_renderer_pixbuf_set_property;
	gobject_class->get_property = img_cell_renderer_pixbuf_get_property;
	gobject_class->finalize = img_cell_renderer_pixbuf_finalize;

	renderer_class->render = img_cell_renderer_pixbuf_render;
	renderer_class->get_size = img_cell_renderer_pixbuf_get_size;

	spec = g_param_spec_boolean( "has-text",
								 "Has text",
								 "Indicates whether text indicator should be "
								 "drawn or not.",
								 FALSE,
								 IMG_PARAM_READWRITE );
	g_object_class_install_property( gobject_class, P_HAS_TEXT, spec );

	spec = g_param_spec_object( "text-ico",
								"Text icon",
								"Icon for text indicator.",
								GDK_TYPE_PIXBUF,
								IMG_PARAM_READWRITE );
	g_object_class_install_property( gobject_class, P_TEXT_ICO, spec );

	spec = g_param_spec_object( "transition",
								"Transition",
								"Indicates what ind of transition is applied "
								"onto the slide.",
								GDK_TYPE_PIXBUF,
								IMG_PARAM_READWRITE );
	g_object_class_install_property( gobject_class, P_TRANSITION, spec );

	spec = g_param_spec_object( "pixbuf",
								"Pixbuf",
								"Main pixbuf to be drawn.",
								GDK_TYPE_PIXBUF,
								IMG_PARAM_READWRITE );
	g_object_class_install_property( gobject_class, P_PIXBUF, spec );

	spec = g_param_spec_double( "zoom",
								"Zoom",
								"Zoom factor at which to draw image.",
								0.1, 3.0, 1.0,
								IMG_PARAM_READWRITE );
	g_object_class_install_property( gobject_class, P_ZOOM, spec );
}

static void
img_cell_renderer_pixbuf_init( ImgCellRendererPixbuf *cell )
{
	ImgCellRendererPixbufPrivate *priv;

	priv = IMG_CELL_RENDERER_PIXBUF_GET_PRIVATE( cell );
	priv->has_text = FALSE;
	priv->text_ico = NULL;
	priv->transition = NULL;
	priv->pixbuf = NULL;
	priv->zoom = 1.0;
}

/* ****************************************************************************
 * Local Declarations
 * ************************************************************************* */
static void
img_cell_renderer_pixbuf_set_property( GObject      *object,
									   guint         prop_id,
									   const GValue *value,
									   GParamSpec   *pspec )
{
	ImgCellRendererPixbufPrivate *priv;

	g_return_if_fail( IMG_IS_CELL_RENDERER_PIXBUF( object ) );
	priv = IMG_CELL_RENDERER_PIXBUF_GET_PRIVATE( object );

	switch( prop_id )
	{
		case P_HAS_TEXT:
			priv->has_text = g_value_get_boolean( value );
			break;

		case P_TEXT_ICO:
			if( priv->text_ico )
				g_object_unref( G_OBJECT( priv->text_ico ) );
			priv->text_ico = g_value_dup_object( value );
			break;

		case P_TRANSITION:
			if( priv->transition )
				g_object_unref( G_OBJECT( priv->transition ) );
			priv->transition = g_value_dup_object( value );
			break;

		case P_PIXBUF:
			if( priv->pixbuf )
				g_object_unref( G_OBJECT( priv->pixbuf ) );
			priv->pixbuf = g_value_dup_object( value );
			break;

		case P_ZOOM:
			priv->zoom = g_value_get_double( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

static void
img_cell_renderer_pixbuf_get_property( GObject    *object,
									   guint       prop_id,
									   GValue     *value,
									   GParamSpec *pspec )
{
	ImgCellRendererPixbufPrivate *priv;

	g_return_if_fail( IMG_IS_CELL_RENDERER_PIXBUF( object ) );
	priv = IMG_CELL_RENDERER_PIXBUF_GET_PRIVATE( object );

	switch( prop_id )
	{
		case P_HAS_TEXT:
			g_value_set_boolean( value, priv->has_text );
			break;

		case P_TEXT_ICO:
			g_value_set_object( value, priv->text_ico );
			break;

		case P_TRANSITION:
			g_value_set_object( value, priv->transition );
			break;

		case P_PIXBUF:
			g_value_set_object( value, priv->pixbuf );
			break;

		case P_ZOOM:
			g_value_set_double( value, priv->zoom );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

static void
img_cell_renderer_pixbuf_render( GtkCellRenderer      *cell,
								 cairo_t            *cr,
								 GtkWidget            *widget,
								 const GdkRectangle         * UNUSED(background_a),
								 const GdkRectangle         *cell_a,
								 GtkCellRendererState  UNUSED(state) )
{
	ImgCellRendererPixbufPrivate *priv;

	/* Rectangles */
	GdkRectangle rect;//, draw_rect;
	gint xpad, ypad;

	priv = IMG_CELL_RENDERER_PIXBUF_GET_PRIVATE( cell );

	/* Get image size */
	img_cell_renderer_pixbuf_get_size( cell, widget, cell_a, &rect.x, &rect.y, &rect.width, &rect.height );
	gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
	rect.x += cell_a->x + xpad;
	rect.y += cell_a->y + ypad;
	rect.width  -= 2 * xpad;
	rect.height -= 2 * ypad;

	/* Draw base image */
	cairo_save( cr );
	cairo_translate( cr, rect.x, rect.y );
	cairo_scale( cr, priv->zoom, priv->zoom );
	gdk_cairo_set_source_pixbuf( cr, priv->pixbuf, 0, 0 );
	cairo_paint( cr );
	cairo_restore( cr );

	if( priv->has_text && priv->text_ico )
	{
		gint    w,
				h;
		gdouble wf,
				hf,
				cf;

		w = gdk_pixbuf_get_width( priv->text_ico );
		h = gdk_pixbuf_get_height( priv->text_ico );
		wf = (gdouble)w / ( rect.width - 2 * BORDER );
		hf = (gdouble)h / ( rect.height - 4 * BORDER  );
		cf = MIN( MIN( 1.0, HORIZ_F / wf ), MIN( 1.0, VERT_F / hf ) );

		cairo_save( cr );
		cairo_translate( cr, rect.x + rect.width - BORDER,
							 rect.y + BORDER );
		cairo_scale( cr, cf, cf );
		gdk_cairo_set_source_pixbuf( cr, priv->text_ico, -w, 0 );
		cairo_paint( cr );
		cairo_restore( cr );
	}

	if( priv->transition )
	{
		gint    w,
				h;
		gdouble wf,
				hf,
				cf;

		w = gdk_pixbuf_get_width( priv->transition );
		h = gdk_pixbuf_get_height( priv->transition );
		wf = (gdouble)w / ( rect.width - 2 * BORDER );
		hf = (gdouble)h / ( rect.height - 4 * BORDER );
		cf = MIN( MIN( 1.0, HORIZ_F / wf ), MIN( 1.0, VERT_F / hf ) );

		cairo_translate( cr, rect.x + rect.width - BORDER, 
							 rect.y + rect.height - BORDER );
		cairo_scale( cr, cf, cf );
		gdk_cairo_set_source_pixbuf( cr, priv->transition, - w, - h );
		cairo_paint( cr );
	}
}

static void
img_cell_renderer_pixbuf_get_size( GtkCellRenderer *cell,
								   GtkWidget       *widget,
								   const GdkRectangle    *cell_area,
								   gint            *x_off,
								   gint            *y_off,
								   gint            *width,
								   gint            *height )
{
	/* Private data */
	ImgCellRendererPixbufPrivate *priv;
	/* Calculated values */
	gboolean calc_off;
	gint     xpad, ypad, w = 0, h = 0;
	gfloat   xalign, yalign;

	priv = IMG_CELL_RENDERER_PIXBUF_GET_PRIVATE( cell );
	gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
	gtk_cell_renderer_get_alignment(cell, &xalign, &yalign);

	/* Get image size */
	if( priv->pixbuf )
	{
		w = gdk_pixbuf_get_width( priv->pixbuf )  * priv->zoom;
		h = gdk_pixbuf_get_height( priv->pixbuf ) * priv->zoom;
	}

	calc_off = ( w > 0 && h > 0 ? TRUE : FALSE );

	/* Add padding */
	w += xpad * 2;
	h += ypad * 2;

	/* Calculate offsets */
	if( cell_area && calc_off )
	{
		if( x_off )
		{
			gboolean dir;
			
			dir = ( gtk_widget_get_direction( widget ) == GTK_TEXT_DIR_LTR );

			*x_off = ( dir ? xalign : 1.0 - xalign ) *
					 ( cell_area->width - w );
			*x_off = MAX( *x_off, 0 );
		}

		if( y_off )
		{
			*y_off = yalign * ( cell_area->height - h );
			*y_off = MAX( *y_off, 0 );
		}
	}
	else
	{
		if( x_off )
			*y_off = 0;
		if( y_off )
			*y_off = 0;
	}

	/* Return dimensions */
	if( width )
		*width = w;
	if( height )
		*height = h;
}

static void
img_cell_renderer_pixbuf_finalize( GObject *object )
{
	ImgCellRendererPixbufPrivate *priv;

	priv = IMG_CELL_RENDERER_PIXBUF_GET_PRIVATE( object );

	if( priv->text_ico )
		g_object_unref( priv->text_ico );
	if( priv->transition )
		g_object_unref( priv->transition );
	if( priv->pixbuf )
		g_object_unref( priv->pixbuf );

	G_OBJECT_CLASS( img_cell_renderer_pixbuf_parent_class )->finalize( object );
}


/* ****************************************************************************
 * Public API
 * ************************************************************************* */
GtkCellRenderer *
img_cell_renderer_pixbuf_new( void )
{
	return( g_object_new( IMG_TYPE_CELL_RENDERER_PIXBUF, NULL ) );
}

