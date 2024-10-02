/*
 *  Copyright (c) 2021-2024 Giuseppe Torelli <colossus73@gmail.com>
 *   *
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

#ifndef __IMG_TIMELINE_H__
#define __IMG_TIMELINE_H__

#include <gtk/gtk.h>
#include "imagination.h"

G_BEGIN_DECLS

struct _ImgTimeline
{
  /*< private >*/
  GtkLayout da;
};

enum
{
	PROP_0,
	VIDEO_BACKGROUND,
	AUDIO_BACKGROUND,
	TOTAL_TIME,
	TIME_MARKER_POS,
	N_PROPERTIES
};

enum
{
    SIGNAL_TIME_CHANGED,
    N_SIGNALS
};

#define IMG_TIMELINE_TYPE img_timeline_get_type()
G_DECLARE_FINAL_TYPE(ImgTimeline, img_timeline, IMG, TIMELINE, GtkLayout)

#define TIMELINE_TYPE_HANDLE (timeline_handle_get_type())
#define TIMELINE_HANDLE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), TIMELINE_TYPE_HANDLE, TimelineHandle))
#define TIMELINE_IS_HANDLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), TIMELINE_TYPE_HANDLE))
#define TIMELINE_HANDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), TIMELINE_TYPE_HANDLE, TimelineHandleClass))
#define TIMELINE_IS_HANDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), TIMELINE_TYPE_HANDLE))
#define TIMELINE_HANDLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), TIMELINE_TYPE_HANDLE, TimelineHandleClass))

//Public functions.
GtkWidget* img_timeline_new();

//Set and get colors.
void img_timeline_set_video_background	(ImgTimeline *, const gchar *);
void img_timeline_set_audio_background	(ImgTimeline *, const gchar *);
void img_timeline_adjust_zoom					(GtkWidget *, gdouble);
void img_timeline_adjust_marker_posx		(GtkWidget *, gint );
void img_timeline_set_total_time				(ImgTimeline *, gint );
void img_timeline_add_media						(GtkWidget *, gchar *filename, gint );
void img_timeline_draw_time_marker			(GtkWidget *, cairo_t *, gint );
void img_timeline_set_time_marker			(ImgTimeline *, gint );

gboolean img_timeline_scroll_event(GtkWidget *, GdkEventScroll *, GtkWidget *);
void img_timeline_drag_data_received (GtkWidget *, GdkDragContext *, gint , gint , GtkSelectionData *, guint , guint , img_window_struct * );
gboolean img_timeline_motion_notify(GtkWidget *, GdkEventMotion *event, ImgTimeline *);
gboolean img_timeline_mouse_button_press (GtkWidget *, GdkEvent *event, ImgTimeline *);
gboolean img_timeline_mouse_button_release (GtkWidget *, GdkEvent *event, ImgTimeline *);

//Media button events
gboolean img_timeline_media_button_press_event (GtkWidget *, GdkEventButton *event, ImgTimeline *);
gboolean img_timeline_media_button_release_event (GtkWidget *, GdkEventButton *event, ImgTimeline *);
gboolean img_timeline_media_motion(GtkWidget *, GdkEventCrossing *, ImgTimeline *);
GtkWidget *img_timeline_private_get_slide_selected(ImgTimeline *);

G_END_DECLS

#endif 
