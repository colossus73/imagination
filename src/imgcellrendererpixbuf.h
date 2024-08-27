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

#ifndef __IMG_CELL_RENDERER_PIXBUF_H__
#define __IMG_CELL_RENDERER_PIXBUF_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IMG_TYPE_CELL_RENDERER_PIXBUF \
	( img_cell_renderer_pixbuf_get_type() )

#define IMG_CELL_RENDERER_PIXBUF( obj ) \
	( G_TYPE_CHECK_INSTANCE_CAST( ( obj ), \
								  IMG_TYPE_CELL_RENDERER_PIXBUF, \
								  ImgCellRendererPixbuf ) )

#define IMG_CELL_RENDERER_PIXBUF_CLASS( klass ) \
	( G_TYPE_CHECK_CLASS_CAST( ( klass ), \
							   IMG_TYPE_CELL_RENDERER_PIXBUF, \
							   ImgCellRendererPixbufClass ) )

#define IMG_IS_CELL_RENDERER_PIXBUF( obj ) \
	( G_TYPE_CHECK_INSTANCE_TYPE( ( obj ), \
								  IMG_TYPE_CELL_RENDERER_PIXBUF ) )

#define IMG_IS_CELL_RENDERER_PIXBUF_CLASS( klass ) \
	( G_TYPE_CHECK_CLASS_TYPE( ( klass ), \
							   IMG_TYPE_CELL_RENDERER_PIXBUF_CLASS ) )

typedef struct _ImgCellRendererPixbuf ImgCellRendererPixbuf;
typedef struct _ImgCellRendererPixbufClass ImgCellRendererPixbufClass;

struct _ImgCellRendererPixbuf
{
	GtkCellRenderer parent;
};

struct _ImgCellRendererPixbufClass
{
	GtkCellRendererClass parent_class;
};

GType
img_cell_renderer_pixbuf_get_type( void ) G_GNUC_CONST;

GtkCellRenderer *
img_cell_renderer_pixbuf_new( void );

G_END_DECLS

#endif /* __IMG_CELL_RENDERER_PIXBUF_H__ */
