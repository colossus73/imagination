#ifndef __IMG_CELL_RENDERER_ANIM_H__
#define __IMG_CELL_RENDERER_ANIM_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IMG_TYPE_CELL_RENDERER_ANIM \
	( img_cell_renderer_anim_get_type() )

#define IMG_CELL_RENDERER_ANIM( obj ) \
	( G_TYPE_CHECK_INSTANCE_CAST( ( obj ), \
								  IMG_TYPE_CELL_RENDERER_ANIM, \
								  ImgCellRendererAnim ) )

#define IMG_CELL_RENDERER_ANIM_CLASS( klass ) \
	( G_TYPE_CHECK_CLASS_CAST( ( klass ), \
							   IMG_TYPE_CELL_RENDERER_ANIM, \
							   ImgCellRendererClass ) )

#define IMG_IS_CELL_RENDERER_ANIM( obj ) \
	( G_TYPE_CHECK_INSTANCE_TYPE( ( obj ), IMG_TYPE_CELL_RENDERER_ANIM ) )

#define IMG_IS_CELL_RENDERER_ANIM_CLASS( klass ) \
	( G_TYPE_CHECK_CLASS_TYPE( ( klass ), IMG_TYPE_CELL_RENDERER_ANIM ) )

typedef struct _ImgCellRendererAnim      ImgCellRendererAnim;
typedef struct _ImgCellRendererAnimClass ImgCellRendererAnimClass;

struct _ImgCellRendererAnim
{
	GtkCellRenderer parent;
};

struct _ImgCellRendererAnimClass
{
	GtkCellRendererClass parent_class;
};

GType
img_cell_renderer_anim_get_type( void ) G_GNUC_CONST;

GtkCellRenderer *
img_cell_renderer_anim_new( void );

G_END_DECLS

#endif /* __IMG_CELL_RENDERER_ANIM_H__ */
