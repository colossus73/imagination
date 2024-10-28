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

#define BASE_SCALE 10

G_BEGIN_DECLS

struct _ImgTimeline
{
  /*< private >*/
  GtkLayout da;
};

typedef struct _ImgTimelinePrivate
{
	gdouble current_preview_time;
	gint total_time;
	gdouble time_marker_pos;
	gdouble zoom_scale;
	gboolean button_pressed_on_needle;
	gdouble pixels_per_second;
	GArray *tracks;
	
	gboolean rubber_band_active;
    gdouble rubber_band_start_x;
    gdouble rubber_band_start_y;
    gdouble rubber_band_end_x;
    gdouble rubber_band_end_y;
} ImgTimelinePrivate;

enum
{
	PROP_0,
	VIDEO_BACKGROUND,
	AUDIO_BACKGROUND,
	TOTAL_TIME,
	TIME_MARKER_POS,
	N_PROPERTIES
};

typedef struct _media_timeline
{
	gint					id;							//This has the same id value in the media_struct in imagination.h
	gint					media_type;
	gdouble		 	x;
	gdouble 			y;
	gdouble 			drag_x;
	gdouble			old_x;
	gdouble			right_edge_pos; 
	gdouble			initial_width;
	gdouble			width;
	gdouble			start_time;
    gdouble			duration;
    gboolean 		to_be_deleted;	//This is for multiple deletion when it occurs multiple times on the timeline
    gboolean 		is_playing;			//This is needed during the preview
	enum 				{	RESIZE_NONE, RESIZE_LEFT, RESIZE_RIGHT } resizing;
	gboolean 		button_pressed;
	GtkWidget 		*button;
	gchar    			*tree_path; 			/* Transition model path to transition */
	gint       			transition_id; 		/* Transition id */
	ImgRender  render;        			/* Transition render function */
} media_timeline;

typedef struct _Track
{
	GArray *items;
	gchar *background_color;
	gint type;
	gint order;
	gdouble	last_media_posX;
	gboolean is_selected;
	gboolean is_default;
} Track;

enum
{
    SIGNAL_TIME_CHANGED,
    N_SIGNALS
};

#define TRACK_HEIGHT 50
#define TRACK_GAP 5

#define IMG_TIMELINE_TYPE img_timeline_get_type()
G_DECLARE_FINAL_TYPE(ImgTimeline, img_timeline, IMG, TIMELINE, GtkLayout)

//Public functions.
GtkWidget* img_timeline_new();
ImgTimelinePrivate* img_timeline_get_private_struct(GtkWidget *);

void img_timeline_start_stop_preview(GtkWidget *, img_window_struct *);
gboolean img_timeline_preview_update(img_window_struct *);

void img_timeline_set_total_time				(ImgTimeline *, gint );
void img_timeline_add_media						(GtkWidget *, media_struct *, gint, gint, img_window_struct *);
void img_timeline_create_toggle_button		(media_timeline *, gint, gchar *, img_window_struct *);
void img_timeline_add_track						(GtkWidget *, gint, gchar *);
void img_timeline_draw_time_marker			(GtkWidget *, cairo_t *, gint, gint);
void img_timeline_set_time_marker			(ImgTimeline *, gdouble );
void img_timeline_delete_all_media			(ImgTimeline *);
void img_timeline_delete_additional_tracks(ImgTimeline *);
void img_timeline_center_button_image		(GtkWidget *);
gint img_timeline_get_final_time					(img_window_struct *);
GArray *img_timeline_get_active_picture_media		(GtkWidget *, double );
GArray *img_timeline_get_active_audio_media		(GtkWidget *, double );
void img_timeline_go_start_time					(GtkWidget *, img_window_struct *);
void img_timeline_go_final_time(				GtkWidget *, img_window_struct *);
void img_timeline_play_audio						(media_timeline *, gint);
void img_timeline_stop_audio						(media_timeline *);
void img_timeline_update_audio_states		(img_window_struct *, gdouble );

//Timeline events
gboolean img_timeline_scroll_event					(GtkWidget *, GdkEventScroll *, GtkWidget *);
gboolean	img_timeline_drag_motion				(GtkWidget *, GdkDragContext *, gint , gint , guint , img_window_struct *);
gboolean img_timeline_drag_leave					(GtkWidget *, GdkDragContext *context, guint , img_window_struct *);
gboolean img_timeline_motion_notify				(GtkWidget *, GdkEventMotion *event, img_window_struct *);
gboolean img_timeline_mouse_button_press 	(GtkWidget *, GdkEventButton *event, img_window_struct *);
gboolean img_timeline_mouse_button_release (GtkWidget *, GdkEvent *event, ImgTimeline *);
gboolean img_timeline_key_press					(GtkWidget *, GdkEventKey *, img_window_struct *);
void		img_timeline_drag_data_received 	(GtkWidget *, GdkDragContext *, gint , gint , GtkSelectionData *, guint , guint , img_window_struct *);
void 		img_timeline_drag_data_get				(GtkWidget *, GdkDragContext *, GtkSelectionData *, guint , guint , img_window_struct *);

//Media button events
gboolean img_timeline_media_button_press_event 	(GtkWidget *, GdkEventButton *event, img_window_struct *);
gboolean img_timeline_media_button_release_event (GtkWidget *, GdkEventButton *event, img_window_struct *);
gboolean img_timeline_media_motion_notify				(GtkWidget *, GdkEventMotion *, img_window_struct *);
gboolean img_timeline_media_leave_event 				(GtkWidget *, GdkEventCrossing *, ImgTimeline *);
gboolean img_timeline_media_button_tooltip				(GtkWidget *, gint , gint , gboolean , GtkTooltip *,  media_timeline *);
void 		img_timeline_media_drag_data_get				(GtkWidget *, GdkDragContext *, GtkSelectionData *, guint , guint, ImgTimeline *);

G_END_DECLS

#endif 
