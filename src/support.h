/*
 *  Copyright (c) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
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

#ifndef __IMAGINATION_SUPPORT_H
#define __IMAGINATION_SUPPORT_H

#ifndef PLUGINS_INSTALLED
#define PLUGINS_INSTALLED 0
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include "imagination.h"
#include "main-window.h"
#include "imgcellrendereranim.h"

#ifdef ENABLE_NLS
#  include <glib/gi18n.h>
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define Q_(String) g_strip_context ((String), (String))
#  define N_(String) (String)
#endif

GtkWidget *img_load_icon(gchar *, GtkIconSize );
gchar *img_convert_seconds_to_time(gint );
GtkWidget *_gtk_combo_box_new_text(gint);
gint img_ask_user_confirmation(img_window_struct *, gchar *);
void img_message(img_window_struct *, gchar *);
void img_load_available_transitions(img_window_struct *);
void img_show_file_chooser(GtkWidget *, GtkEntryIconPosition, int, img_window_struct *);
void img_delete_subtitle_pattern(GtkButton *button, img_window_struct *img);
void img_update_zoom_variables(img_window_struct *img);
void img_get_audio_data(media_struct *);

media_struct *img_create_new_media();

void img_set_empty_slide_info( media_struct *slide,
							 gint          gradient,
							 gint          countdown,
							 gdouble      *start_color,
							 gdouble      *stop_color,
							 gdouble      *countdown_color,
							 gdouble      *start_point,
							 gdouble      *stop_point );

GdkPixbuf *img_set_fade_gradient(	img_window_struct *img,
							gint gradient,
							media_struct *slide_info);
void img_set_slide_still_info( media_struct      *slide,
						  gdouble           duration,
						  img_window_struct *img );

void img_set_slide_transition_info( media_struct      *slide,
							   GtkListStore      *store,
							   GtkTreeIter       *iter,
							   GdkPixbuf         *pix,
							   const gchar       *path,
							   gint               transition_id,
							   ImgRender          render,
							   img_window_struct *img );

void img_set_slide_ken_burns_info( media_struct *slide,
							  gint          cur_point,
							  gsize         length,
							  gdouble      *points );

void img_free_media_struct( media_struct * );
void img_save_relative_filenames(GtkCheckButton *togglebutton, img_window_struct *img);
gint img_calc_slide_duration_points( GList *list, gint   length );
GdkPixbuf *img_convert_surface_to_pixbuf( cairo_surface_t *surface );
void img_taint_project(img_window_struct *);
void img_sync_timings( media_struct  *, img_window_struct * );

gboolean img_scale_empty_slide( gint gradient, gint countdown,
					gdouble          *p_start,
					gdouble          *p_stop,
					gdouble          *c_start,
					gdouble          *c_stop,
					gdouble          *countdown_color,
					gboolean			preview,
					gdouble 			countdown_angle,
					gint              width,
					gint              height,
					GdkPixbuf       **pixbuf,
					cairo_surface_t **surface );

gboolean img_check_for_recent_file(img_window_struct *, const gchar *);
gboolean img_find_media_in_list(img_window_struct *, gchar *);
void rotate_point(double , double , double , double , double , double *, double *);
void transform_coords(img_textbox *, double , double , double *, double *);
void select_word_at_position(img_textbox *, int );
void to_upper(gchar **);
gint img_convert_time_string_to_seconds(gchar *);
gchar *img_convert_time_to_string(gint );
gint find_nearest_major_tick(gint, gint x);
const gchar *img_get_media_id_and_filename(img_window_struct *, gint, gint *);

#endif
