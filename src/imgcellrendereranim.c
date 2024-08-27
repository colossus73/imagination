#include "imgcellrendereranim.h"
#include "imagination.h"

#define IMG_PARAM_DEFAULTS \
	G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB
#define IMG_PARAM_READWRITE \
	G_PARAM_WRITABLE | G_PARAM_READABLE | IMG_PARAM_DEFAULTS

enum
{
	P_0,
	P_ANIMATION
};

#define IMG_CELL_RENDERER_ANIM_GET_PRIVATE( obj ) \
	( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), \
								   IMG_TYPE_CELL_RENDERER_ANIM, \
								   ImgCellRendererAnimPrivate ) )

typedef struct _ImgCellRendererAnimPrivate ImgCellRendererAnimPrivate;
struct _ImgCellRendererAnimPrivate
{
	GdkPixbufAnimation *anim;
};


static void
img_cell_renderer_anim_set_property( GObject      *object,
									 guint         prop_id,
									 const GValue *value,
									 GParamSpec   *pspec );

static void
img_cell_renderer_anim_get_property( GObject    *object,
									 guint       prop_id,
									 GValue     *value,
									 GParamSpec *pspec );

static void
img_cell_renderer_anim_render( GtkCellRenderer      *cell,
							   cairo_t              *cr,
							   GtkWidget            *widget,
							   const GdkRectangle         *background_a,
							   const GdkRectangle         *cell_a,
							   GtkCellRendererState  state );

static void
img_cell_renderer_anim_get_size( GtkCellRenderer *cell,
								 GtkWidget       *widget,
								 const GdkRectangle    *cell_area,
								 gint            *x_off,
								 gint            *y_off,
								 gint            *width,
								 gint            *height );


static void
img_cell_renderer_anim_finalize( GObject *object );

/* ****************************************************************************
 * Initialization
 * ************************************************************************* */
G_DEFINE_TYPE_WITH_CODE( ImgCellRendererAnim,
			   img_cell_renderer_anim,
			   GTK_TYPE_CELL_RENDERER,
			   G_ADD_PRIVATE (ImgCellRendererAnim));

static void
img_cell_renderer_anim_class_init( ImgCellRendererAnimClass *klass )
{
	GParamSpec           *spec;
	GObjectClass         *gobject_class = G_OBJECT_CLASS( klass );
	GtkCellRendererClass *renderer_class = GTK_CELL_RENDERER_CLASS( klass );

	gobject_class->set_property = img_cell_renderer_anim_set_property;
	gobject_class->get_property = img_cell_renderer_anim_get_property;
	gobject_class->finalize 	= img_cell_renderer_anim_finalize;

	renderer_class->render 	 = img_cell_renderer_anim_render;
	renderer_class->get_size = img_cell_renderer_anim_get_size;

	spec = g_param_spec_object( "anim",
								"Pixbuf Animation Object",
								"The animated pixbuf to render",
								GDK_TYPE_PIXBUF_ANIMATION,
								IMG_PARAM_READWRITE );
	g_object_class_install_property (gobject_class, P_ANIMATION, spec);
}

static void
img_cell_renderer_anim_init( ImgCellRendererAnim *cell )
{
	ImgCellRendererAnimPrivate *priv;

	priv = img_cell_renderer_anim_get_instance_private( cell );
	priv->anim = NULL;
}

/* ****************************************************************************
 * Local Declarations
 * ************************************************************************* */
static gboolean
cb_timeout( GdkPixbufAnimationIter *iter )
{
	gint     delay;
	gboolean flag;

	flag = gdk_pixbuf_animation_iter_advance( iter, NULL );
	if( flag )
	{
		/* FIXME: This code will work, but is ugly as hell when it comes to
		 * non-ombo box cell views */
		GtkWidget *widget;

		widget = g_object_get_data( G_OBJECT( iter ), "widget" );
		if( ! gtk_widget_get_visible( widget ) )
			return( FALSE );

		gtk_widget_queue_draw( widget );
	}
	delay = gdk_pixbuf_animation_iter_get_delay_time( iter );
	gdk_threads_add_timeout( delay, (GSourceFunc)cb_timeout, iter );

	return( FALSE );
}

static void
img_cell_renderer_anim_set_property( GObject      *object,
									 guint         prop_id,
									 const GValue *value,
									 GParamSpec   *pspec )
{
	ImgCellRendererAnimPrivate *priv;

	g_return_if_fail( IMG_IS_CELL_RENDERER_ANIM( object ) );
	priv = img_cell_renderer_anim_get_instance_private( IMG_CELL_RENDERER_ANIM (object) );

	switch( prop_id )
	{
		case P_ANIMATION:
			{
				if( priv->anim )
					g_object_unref( G_OBJECT( priv->anim ) );
				priv->anim = g_value_dup_object( value );
			}
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

static void
img_cell_renderer_anim_get_property( GObject    *object,
									 guint       prop_id,
									 GValue     *value,
									 GParamSpec *pspec )
{
	ImgCellRendererAnimPrivate *priv;

	g_return_if_fail( IMG_IS_CELL_RENDERER_ANIM( object ) );
	priv = img_cell_renderer_anim_get_instance_private( IMG_CELL_RENDERER_ANIM (object) );

	switch( prop_id )
	{
		case P_ANIMATION:
			g_value_set_object( value, priv->anim );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

static void
img_cell_renderer_anim_render( GtkCellRenderer      *cell,
							   cairo_t            *cr,
							   GtkWidget            *widget,
							   const GdkRectangle         * UNUSED(background_a),
							   const GdkRectangle         *cell_a,
							   GtkCellRendererState UNUSED(state) )
{
	ImgCellRendererAnimPrivate *priv;
	GdkPixbufAnimationIter     *iter;
	GdkPixbuf                  *pixbuf;
	GdkRectangle                rect, draw_rect;
	gint xpad, ypad;

	g_return_if_fail( IMG_IS_CELL_RENDERER_ANIM( cell ) );
	priv = img_cell_renderer_anim_get_instance_private ( IMG_CELL_RENDERER_ANIM(cell) );
	
	if (!priv->anim) {
		return;
	}

	/* Get image size */
	img_cell_renderer_anim_get_size( cell, widget, cell_a, &rect.x,
									 &rect.y, &rect.width, &rect.height );
	gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
	rect.x += cell_a->x + xpad;
	rect.y += cell_a->y + ypad;
	rect.width  -= 2 * xpad;
	rect.height -= 2 * ypad;

	if( ! gdk_rectangle_intersect( cell_a, &rect, &draw_rect )) {
		return;
	}

	/* Draw the current frame of the GdkPixbufAnimation */
	iter = g_object_get_data( G_OBJECT( priv->anim ), "iter" );
	if( ! iter )
	{
		gint delay;

		/* Initialiize iter */
		iter = gdk_pixbuf_animation_get_iter( priv->anim, NULL );
		g_object_set_data_full( G_OBJECT( priv->anim ), "iter", iter,
								(GDestroyNotify)g_object_unref );

		/* Install timeout */
		delay = gdk_pixbuf_animation_iter_get_delay_time( iter );
		gdk_threads_add_timeout( delay, (GSourceFunc)cb_timeout, iter );
	}
	g_object_set_data( G_OBJECT( iter ), "widget", widget );
	pixbuf = gdk_pixbuf_animation_iter_get_pixbuf( iter );
	gdk_cairo_set_source_pixbuf( cr, pixbuf, rect.x, rect.y );
	gdk_cairo_rectangle( cr, &draw_rect );
	cairo_fill( cr );
}

static void
img_cell_renderer_anim_get_size( GtkCellRenderer *cell,
								 GtkWidget       *widget,
								 const GdkRectangle    *cell_area,
								 gint            *x_off,
								 gint            *y_off,
								 gint            *width,
								 gint            *height )
{
	/* Private data */
	ImgCellRendererAnimPrivate *priv;
	/* Calculated values */
	gboolean calc_off;
	gint     xpad, ypad, w = 0, h = 0;
	gfloat     xalign, yalign;

	g_return_if_fail(IMG_IS_CELL_RENDERER_ANIM(cell));
	priv = img_cell_renderer_anim_get_instance_private( IMG_CELL_RENDERER_ANIM (cell) );

	/* Get image size */
	if( priv->anim )
	{
		w = gdk_pixbuf_animation_get_width( priv->anim );
		h = gdk_pixbuf_animation_get_height( priv->anim );
	}

	calc_off = ( w > 0 && h > 0 ? TRUE : FALSE );

	gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
	gtk_cell_renderer_get_alignment(cell, &xalign, &yalign);

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
img_cell_renderer_anim_finalize( GObject *object )
{
	ImgCellRendererAnimPrivate *priv;

	g_return_if_fail(IMG_IS_CELL_RENDERER_ANIM(object));
	priv = img_cell_renderer_anim_get_instance_private( IMG_CELL_RENDERER_ANIM (object) );

	if( priv->anim )
		g_object_unref( priv->anim );

	G_OBJECT_CLASS( img_cell_renderer_anim_parent_class )->finalize( object );
}

/* ****************************************************************************
 * Public API
 * ************************************************************************* */
GtkCellRenderer *img_cell_renderer_anim_new( void )
{
	return( g_object_new( IMG_TYPE_CELL_RENDERER_ANIM, NULL ) );
}
