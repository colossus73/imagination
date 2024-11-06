/*
 *  Copyright (c) 2009-2018 Giuseppe Torelli <colossus73@gmail.com>
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

#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <gtk/gtk.h>
#include "main-window.h"
#include "new_project.h"
#include "file.h"
#include "text.h"

gboolean img_can_discard_unsaved_project(img_window_struct *);
void img_project_properties(GtkMenuItem *item, img_window_struct *);
void img_refresh_window_title(img_window_struct *);
void img_new_project(GtkMenuItem *,img_window_struct *);
gboolean img_create_media_struct(gchar *, img_window_struct *);
void img_add_media(gchar *, media_struct *, img_window_struct *);
void img_add_media_items(GtkMenuItem *,img_window_struct *);
//TODO to remove?
void img_detect_media_orientation_from_pixbuf(GdkPixbuf *, gboolean *, ImgAngle *);

void img_swap_preview_button_images( img_window_struct *, gboolean);
void img_select_all_media(GtkWidget *, img_window_struct *);
void img_unselect_all_media(GtkWidget *, img_window_struct *);
void img_rotate_slides_left( GtkWidget  *, img_window_struct * );
void img_rotate_slides_right( GtkWidget  *,  img_window_struct * );
void img_show_about_dialog (GtkMenuItem *,img_window_struct *);
void img_go_fullscreen(GtkMenuItem *, img_window_struct *);
void img_media_library_drag_begin(GtkWidget *,GdkDragContext *, img_window_struct *);
void img_media_library_drag_data_received(GtkWidget *,GdkDragContext *, int, int, GtkSelectionData *, unsigned int, unsigned int, img_window_struct *);
void img_media_library_drag_data_get(GtkWidget *, GdkDragContext *, GtkSelectionData *, guint , guint , img_window_struct * );
void img_media_library_drag_end(GtkWidget *,GdkDragContext *, img_window_struct *);
void img_start_stop_export(GtkWidget *, img_window_struct *);
void img_open_recent_slideshow(GtkWidget *, img_window_struct *);
void img_add_any_media_callback( GtkButton * ,  img_window_struct *);
void img_zoom_fit( GtkWidget *, img_window_struct *);
void img_media_duration_value_changed (GtkSpinButton *, img_window_struct *);
void img_choose_project_filename(GtkWidget *,img_window_struct *);
void img_close_project(GtkWidget *,img_window_struct *);
void img_increase_progressbar(img_window_struct *, gint);
GSList *img_import_slides_file_chooser(img_window_struct *);
void img_free_allocated_memory(img_window_struct *);
gboolean img_window_key_pressed(GtkWidget *, GdkEventKey *, img_window_struct *);
void img_exit_fullscreen(img_window_struct *img);
gboolean img_quit_application(GtkWidget *, GdkEvent *, img_window_struct *);
gboolean img_on_draw_event(GtkWidget *,cairo_t *,img_window_struct *);
void img_ken_burns_zoom_changed( GtkRange *, img_window_struct * );
gboolean img_image_area_scroll( GtkWidget *, GdkEvent *, img_window_struct * );
gboolean img_image_area_button_press( GtkWidget *, GdkEventButton *, img_window_struct * );
gboolean img_image_area_button_release( GtkWidget *, GdkEventButton *, img_window_struct * );
gboolean img_image_area_motion( GtkWidget *, GdkEventMotion *, img_window_struct * );
gboolean img_image_area_key_press(GtkWidget *widget, GdkEventKey *, img_window_struct *);

void img_draw_image_on_surface( cairo_t           *cr,
						   gint               width,
						   cairo_surface_t   *surface,
						   ImgStopPoint      *point,
						   img_window_struct *img );
void img_add_stop_point( GtkButton         *button,
					img_window_struct *img );
void img_update_stop_point( GtkSpinButton *button, img_window_struct *img );
void img_delete_stop_point( GtkButton *button, img_window_struct *img );

void img_goto_prev_point( GtkButton  *button,  img_window_struct *img );
void img_goto_next_point( GtkButton  *button,  img_window_struct *img );
void img_goto_point ( GtkEntry          *entry,  img_window_struct *img );
void img_calc_current_ken_point( ImgStopPoint *res,
							ImgStopPoint *from,
							ImgStopPoint *to,
							gdouble       progress,
							gint          mode );

void img_add_empty_slide( GtkMenuItem  *item, img_window_struct *img );
gboolean  img_save_window_settings( img_window_struct *img );
gboolean img_load_window_settings( img_window_struct *img );
void img_set_window_default_settings( img_window_struct *img );
void img_rotate_flip_slide( media_struct   *slide, ImgAngle        angle,  gboolean        flipped);
void img_pattern_clicked(GtkMenuItem *item, img_window_struct *img);

void img_fadeout_duration_changed (GtkSpinButton *spinbutton, img_window_struct *img);

void img_flip_horizontally(GtkMenuItem *item, img_window_struct *img);
gboolean img_transition_timeout(img_window_struct *);
#endif
