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

#include "audio.h"
#include "img_timeline.h"
#include "callbacks.h"

static int next_order = 0;

static void img_timeline_class_init(ImgTimelineClass *);
static void img_timeline_set_property(GObject *, guint , const GValue *, GParamSpec *);
static void img_timeline_get_property(GObject *, guint , GValue *, GParamSpec *);
static void img_timeline_init(ImgTimeline *);
static void img_timeline_draw_ticks(GtkWidget *widget, cairo_t *);
static void img_timeline_draw_rubber_band(cairo_t *, ImgTimelinePrivate *);
static void img_timeline_select_items_in_rubber_band(GtkWidget *);
static void img_timeline_finalize(GObject *);
static gboolean img_timeline_draw(GtkWidget *, cairo_t *);
static gint img_timeline_calculate_total_tracks_height(GtkWidget *);
static gint img_sort_image_track_first(gconstpointer , gconstpointer );
static gint img_timeline_get_track_at_position(GtkWidget *, gint, gint *);
static void img_timeline_unhighlight_track(GArray *);

G_DEFINE_TYPE_WITH_PRIVATE(ImgTimeline, img_timeline, GTK_TYPE_LAYOUT);

void img_timeline_start_stop_preview(GtkWidget *item, img_window_struct *img)
{
    ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)img->timeline);
	GArray *active_elements;

	if (img->total_time == 0.0)
		return;

	// If the preview is running, stop it
	if (img->preview_is_running)
	{
		 if (img->source_id > 0)
		{
			g_source_remove(img->source_id);
			img->source_id = 0;
		}
		img->preview_is_running = FALSE;
		img_swap_preview_button_images(img, TRUE);
		img_timeline_update_audio_states(img, priv->current_preview_time);
		cairo_surface_destroy(img->exported_image);
		img->exported_image = NULL;
		g_slist_free(img->media_playing);
    }
    // Start the preview
    else
	{
		g_print("\e[1;1H\e[2J");
		img->preview_is_running = TRUE;
		img->transition_progress = 0.0;
		if (priv->current_preview_time >= img->total_time)
			img_timeline_go_start_time(NULL, img);

		active_elements = img_timeline_get_active_media_at_given_time(img->timeline, priv->current_preview_time);
		if (active_elements)
			img->source_id = g_timeout_add(1000 / priv->pixels_per_second, (GSourceFunc) img_timeline_preview_timeout, img);

		img->preview_is_running = TRUE;
		img_swap_preview_button_images(img, FALSE);
		priv->current_preview_time = priv->time_marker_pos / priv->pixels_per_second;

		img_timeline_update_audio_states(img, priv->current_preview_time);
		g_array_free(active_elements, FALSE);
	}
}

gboolean img_timeline_preview_timeout(img_window_struct *img)
{
    ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)img->timeline);
    media_timeline *current_media = NULL,  *media = NULL;
    GArray *current_media_array = NULL;
    GArray *next_media_array = NULL;
    cairo_surface_t *surface = NULL, *composite_surface = NULL;
    cairo_surface_t *next_composite = NULL;
    cairo_t *cr;
    gint max_width = 0, max_height = 0;
    gdouble next_time, current_end_time, transition_duration;
    gboolean is_transitioning = FALSE;
    
    // Get current active media
    current_media_array = img_timeline_get_active_picture_media(img->timeline, priv->current_preview_time);
    if (current_media_array->len == 0)
        goto update_red_needle;

	current_media = g_array_index(current_media_array, media_timeline *, 0);

	if (current_media->transition_id > -1)
		is_transitioning = TRUE;
	else
		is_transitioning = FALSE;
  
    // Find max dimensions for all current media
    for (gint i = 0; i < current_media_array->len; i++)
    {
        media = g_array_index(current_media_array, media_timeline *, i);
        surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(media->id));
        if (surface)
        {
            max_width = MAX(max_width, cairo_image_surface_get_width(surface));
            max_height = MAX(max_height, cairo_image_surface_get_height(surface));
        }
    }
	if (is_transitioning)
	{
		// Get the first media to check for transition timing
		current_media = g_array_index(current_media_array, media_timeline *, 0);

		// Calculate when current media ends
		current_end_time = current_media->start_time + current_media->duration;	

		// Check if we're near the end of current media (transition zone)
		transition_duration = 1.5;

		// Calculate transition progress
		img->transition_progress = 1.0 - ((current_end_time - priv->current_preview_time) / transition_duration);
		
		// Get the next set of media items
		next_time = current_end_time + 0.01; // Look slightly after current end
		next_media_array = img_timeline_get_active_picture_media(img->timeline, next_time);
		
		// Update max dimensions considering next media items
		if (next_media_array)
		{
			for (gint i = 0; i < next_media_array->len; i++)
			{
				media = g_array_index(next_media_array, media_timeline *, i);
				surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(media->id));
				if (surface)
				{
					max_width = MAX(max_width, cairo_image_surface_get_width(surface));
					max_height = MAX(max_height, cairo_image_surface_get_height(surface));
				}
			}
		}
	}
	// Create composite surface for final output
	if (max_width > 0 && max_height > 0)
	{
		composite_surface = cairo_surface_create_similar(g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(current_media->id)),
			CAIRO_CONTENT_COLOR_ALPHA, max_width, max_height);

		// Compose current media items
		cr = cairo_create(composite_surface);
		for (gint i = current_media_array->len - 1; i >= 0; i--)
		{
			media = g_array_index(current_media_array, media_timeline *, i);
			surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(media->id));
			if (surface)
			{
				//gint x = (max_width - cairo_image_surface_get_width(surface)) / 2;
				//gint y = (max_height - cairo_image_surface_get_height(surface)) / 2;
				cairo_set_source_surface(cr, surface, media->x, media->y);
				cairo_paint(cr);
			}
		}
		cairo_destroy(cr);
	
		// If transitioning, compose next media items
		if (is_transitioning && next_media_array && next_media_array->len > 0)
		{
			next_composite = cairo_surface_create_similar(composite_surface, CAIRO_CONTENT_COLOR_ALPHA, max_width, max_height);
			cr = cairo_create(next_composite);
			for (gint i = next_media_array->len - 1; i >= 0; i--)
			{
				media = g_array_index(next_media_array, media_timeline *, i);
				surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(media->id));
				if (surface)
				{
					gint x = (max_width - cairo_image_surface_get_width(surface)) / 2;
					gint y = (max_height - cairo_image_surface_get_height(surface)) / 2;
					cairo_set_source_surface(cr, surface, x, y);
					cairo_paint(cr);
				}
			}
			cairo_destroy(cr);

			// Apply transition effect using current media's render function
			cr = cairo_create(composite_surface);
			current_media->render(cr, composite_surface, next_composite, img->transition_progress);
			cairo_destroy(cr);
			cairo_surface_destroy(next_composite);
		}
		else
		{
			cr = cairo_create(composite_surface);
			cairo_set_source_surface(cr, composite_surface, 0, 0);
			cairo_paint(cr);
			cairo_destroy(cr);
			
			// Reset transition progress when not transitioning
			img->transition_progress = 0.0;
		}
	}
	
	if (next_media_array)
		g_array_free(next_media_array, FALSE);

update_red_needle:
	g_array_free(current_media_array, FALSE);
	
	if (img->exported_image)
	{
		cairo_surface_destroy(img->exported_image);
		img->exported_image = NULL;
	}
	img->exported_image = composite_surface;
	img_timeline_preview_update(img);
	gtk_widget_queue_draw(img->image_area);

	return G_SOURCE_CONTINUE;
}

void img_timeline_preview_update(img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)img->timeline);
	
	// Update timeline position
	priv->current_preview_time += (1.0 / priv->pixels_per_second);
	img_timeline_update_audio_states(img, priv->current_preview_time);
	
	// Update UI
	gdouble new_marker_pos = priv->current_preview_time * priv->pixels_per_second;
	img_timeline_set_time_marker((ImgTimeline*)img->timeline, new_marker_pos);
	
	gchar *time_str = img_convert_time_to_string(priv->current_preview_time);
	gtk_label_set_text(GTK_LABEL(img->current_time), time_str);
	g_free(time_str);
	
	// Handle auto-scroll
	GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(img->timeline_scrolled_window));
	gdouble current_scroll = gtk_adjustment_get_value(hadj);
	gdouble page_size = gtk_adjustment_get_page_size(hadj);
	
	if (new_marker_pos > (current_scroll + page_size - 100))
		gtk_adjustment_set_value(hadj, new_marker_pos - page_size + 100);
	
	// Check for end of timeline
	if (priv->current_preview_time >= img->total_time)
	{
		g_source_remove(img->source_id);
		img->source_id = 0;
		img->preview_is_running = FALSE;		
		img_swap_preview_button_images(img, TRUE);
		img_timeline_update_audio_states(img, priv->current_preview_time);
		cairo_surface_destroy(img->exported_image);
		img->exported_image = NULL;
	}
	//g_print("\r%2.2f",priv->current_preview_time);
}

static void img_timeline_class_init(ImgTimelineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	object_class->set_property = img_timeline_set_property;
	object_class->get_property = img_timeline_get_property;
	object_class->finalize = img_timeline_finalize;
	
	widget_class->draw = img_timeline_draw;

	g_object_class_install_property(object_class, TOTAL_TIME,       			g_param_spec_int		("total_time", "total_time", "total_time", 0, G_MAXINT, 60, G_PARAM_READWRITE));
	g_object_class_install_property(object_class, TIME_MARKER_POS, 	g_param_spec_double("time_marker_pos", "Time Marker Position", "Position of the time marker", 0, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
}

static void img_timeline_init(ImgTimeline *timeline)
{
    ImgTimelinePrivate *priv = img_timeline_get_instance_private(timeline);

	priv->current_preview_time = 0.0;
	priv->zoom_scale = 6.12;
	priv->pixels_per_second = 61.2;
	priv->total_time = 0;
	priv->time_marker_pos = 0.0;
	priv->tracks = g_array_new(FALSE, TRUE, sizeof(Track *));
	
	priv->rubber_band_active = FALSE;
	priv->rubber_band_start_x = 0;
	priv->rubber_band_start_y = 0;
	priv->rubber_band_end_x = 0;
	priv->rubber_band_end_y = 0;
	
	// Add default image and audio tracks
	Track *image_track = g_new0(Track, 1);
	image_track->type = 0;
	image_track->is_selected = FALSE;
	image_track->is_default = TRUE;
	image_track->order = next_order++;
	image_track->items = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	image_track->background_color = g_strdup("#CCCCFF");
	g_array_append_val(priv->tracks, image_track);
	
	Track *audio_track = g_new0(Track, 1);
	audio_track->type = 1;
	audio_track->is_selected = FALSE;
	audio_track->is_default = TRUE;
	audio_track->order = next_order++;
	audio_track->items = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	audio_track->background_color = g_strdup("#d6d1cd");
	g_array_append_val(priv->tracks, audio_track);	
}

//Needed for g_object_set()
static void img_timeline_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ImgTimeline *da = IMG_TIMELINE(object);
	switch(prop_id)
	{
		case TOTAL_TIME:
			img_timeline_set_total_time(da, g_value_get_int(value));
		break; 
		case TIME_MARKER_POS:
			img_timeline_set_time_marker(da, g_value_get_double(value));
		break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
  }
}

void img_timeline_set_total_time(ImgTimeline *da, gint total_time)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private(da);

	priv->total_time = total_time;
}

void img_timeline_set_time_marker(ImgTimeline *timeline, gdouble posx)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private(timeline);

	priv->time_marker_pos = posx;
	gtk_widget_queue_draw(GTK_WIDGET(timeline));
}

ImgTimelinePrivate *img_timeline_get_private_struct(GtkWidget *timeline)
{
	return img_timeline_get_instance_private((ImgTimeline*)timeline);	
}

static void img_timeline_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ImgTimeline *da = IMG_TIMELINE(object);
	ImgTimelinePrivate *priv = img_timeline_get_instance_private(da);
	  
	switch(prop_id)
	{
		case TOTAL_TIME:
			g_value_set_int(value, priv->total_time);
		break;

		case TIME_MARKER_POS:
			g_value_set_double(value, (priv->time_marker_pos / priv->pixels_per_second));
		break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

GtkWidget* img_timeline_new()
{
	return GTK_WIDGET(g_object_new(img_timeline_get_type(), NULL));
}

static gboolean img_timeline_draw(GtkWidget *timeline, cairo_t *cr)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	GdkRGBA rgba;
	Track *track;

	gint width = gtk_widget_get_allocated_width(timeline);
	img_timeline_draw_ticks(timeline, cr);

	// Draw the tracks
	int y = 0;
	for (int i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		gdk_rgba_parse(&rgba, track->background_color);
		cairo_set_line_width(cr, 1.0);
		if (priv->dark_theme)
			cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
		else
			cairo_set_source_rgb(cr, rgba.red, rgba.green, rgba.blue);
		cairo_rectangle(cr, 0, y+32 , width, TRACK_HEIGHT);
		cairo_fill(cr);
	
		if (track->is_selected)
		{
			cairo_set_source_rgb(cr,  96/255.0, 96/255.0, 96/255.0);
			cairo_set_line_width(cr, 2);
			cairo_rectangle(cr, 0, y+32 , width, TRACK_HEIGHT);
			cairo_stroke(cr);
		}
		y += TRACK_HEIGHT + TRACK_GAP;
	}

	  //This is necessary to draw the media represented by the GtkToggleButtons
	  GTK_WIDGET_CLASS (img_timeline_parent_class)->draw (timeline, cr);

	//Draw the red time marker 
	img_timeline_draw_time_marker(timeline, cr, priv->time_marker_pos, img_timeline_calculate_total_tracks_height(timeline));
	
	img_timeline_draw_rubber_band(cr, priv);
	return TRUE;
}

static void img_timeline_draw_rubber_band(cairo_t *cr, ImgTimelinePrivate *priv)
{
	if (!priv->rubber_band_active)
		return;

	// Set semi-transparent blue for the rubber band
	cairo_set_source_rgba(cr, 0.2, 0.4, 0.9, 0.2);
	
	// Calculate rectangle dimensions
	gdouble x = MIN(priv->rubber_band_start_x, priv->rubber_band_end_x);
	gdouble y = MIN(priv->rubber_band_start_y, priv->rubber_band_end_y);
	gdouble width = ABS(priv->rubber_band_end_x - priv->rubber_band_start_x);
	gdouble height = ABS(priv->rubber_band_end_y - priv->rubber_band_start_y);
	
	// Draw filled rectangle
	cairo_rectangle(cr, x, y, width, height);
	cairo_fill_preserve(cr);
	
	// Draw border
	cairo_set_source_rgba(cr, 0.2, 0.4, 0.9, 0.8);
	cairo_set_line_width(cr, 1.0);
	cairo_stroke(cr);
}

static void img_timeline_finalize(GObject *object)
{ 
	ImgTimeline *timeline = IMG_TIMELINE(object);
	ImgTimelinePrivate *priv =img_timeline_get_instance_private(timeline);
	  
	if (priv->tracks)
	{
		for (gint i = 0; i < priv->tracks->len; i++)
		{
			Track *track = g_array_index(priv->tracks, Track *, i);
			if (track->items)
				g_array_free(track->items, TRUE);
			
			g_free(track->background_color);
			g_free(track);
		}
        g_array_free(priv->tracks, TRUE);
    }

  G_OBJECT_CLASS(img_timeline_parent_class)->finalize(object);
}

void img_timeline_draw_ticks(GtkWidget *da, cairo_t *cr)
{
    ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);
    cairo_text_extents_t extents;
    gint width, total_time = 0;
    
    width  = gtk_widget_get_allocated_width(da);
    g_object_get(G_OBJECT(da), "total_time", &total_time, NULL);

	if (priv->dark_theme)
    cairo_set_source_rgb(cr, 0.33, 0.33, 0.33);
    else
		cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    
    if (priv->dark_theme)
		cairo_set_source_rgb(cr, 1, 1, 1);
	else	
		cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    cairo_select_font_face(cr, "Dejavu", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);

    // Calculate appropriate tick intervals based on zoom level
    gdouble tick_interval = 1.0;
    if (priv->pixels_per_second < 0.05) tick_interval = 1800.0;       		// 30 minutes
    else if (priv->pixels_per_second < 0.1) tick_interval = 900.0;		// 15 minutes
    else if (priv->pixels_per_second < 0.2) tick_interval = 600.0; 		// 10 minutes
    else if (priv->pixels_per_second < 0.5) tick_interval = 300.0; 		// 5 minutes
    else if (priv->pixels_per_second < 1) tick_interval = 120.0; 			// 2 minutes
    else if (priv->pixels_per_second < 2) tick_interval = 60.0;			// 1 minute
    else if (priv->pixels_per_second < 5) tick_interval = 30.0;			// 30 seconds
    else if (priv->pixels_per_second < 10) tick_interval = 15.0;			// 15 seconds
    else if (priv->pixels_per_second < 20) tick_interval = 5.0;			// 5 seconds
    else if (priv->pixels_per_second < 40) tick_interval = 2.0;			// 2 seconds
    else tick_interval = 1.0;                                         							// 1 second

    gdouble label_interval = tick_interval * 2;
    gdouble max_time = width / priv->pixels_per_second;

    // Calculate the minimum width needed for a label
    char min_time_str[20];
    snprintf(min_time_str, sizeof(min_time_str), "00:00:00");
    cairo_text_extents(cr, min_time_str, &extents);
    gdouble min_label_width = extents.width * 1.0;  // Add padding

    for (gdouble t = 0; t <= max_time; t += tick_interval)
    {
        gdouble x = t * priv->pixels_per_second;
        if (x > width)
			break;
        if (fmod(t, label_interval) < 0.05)
        {
            // Draw major tick and label
            cairo_set_line_width(cr, 2.0);
            cairo_move_to(cr, x, 4 + 12);
            cairo_line_to(cr, x, 14 + 12);
            cairo_stroke(cr);

            int hours = (int)t / 3600;
            int minutes = ((int)t % 3600) / 60;
            int seconds = (int)t % 60;
    
            char time_str[20];
            if (hours > 0)
                snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hours, minutes, seconds);
            else
                snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);

            cairo_text_extents(cr, time_str, &extents);
            
            // Check if there's enough space for the label
            if (x - extents.width / 2 >= 0 && x + extents.width / 2 <= width &&
                (x - min_label_width / 2 >= 0 || x + min_label_width / 2 <= width))
            {
                cairo_move_to(cr, x - extents.width / 2, 0 + 12);
                cairo_show_text(cr, time_str);
            }
        }
        else
        {
            // Draw minor tick
            cairo_set_line_width(cr, 1.0);
            cairo_move_to(cr, x, 10 + 12);
            cairo_line_to(cr, x, 14 + 12);
            cairo_stroke(cr);
        }
    }
}

void img_timeline_add_media(GtkWidget *timeline, media_struct *entry, gint x, gint y, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);

    Track *track;
	media_timeline *item;
	GtkWidget *audio_button;
    gint track_nr, new_y;
    gdouble width, position_x;

	//Calculate the track to move the media to later according to the dropped y coord
	track_nr = img_timeline_get_track_at_position(timeline, y, &new_y);
	track = g_array_index(priv->tracks, Track *, track_nr);

	if (track->type != entry->media_type)
		return;

	item = g_new0(media_timeline, 1);
	item->id = entry->id;
	item->media_type = entry->media_type;
	item->timeline_y = new_y;

	if (entry->media_type == 0)
	{
		item->opacity = 1.0;
		item->duration = gtk_spin_button_get_value(GTK_SPIN_BUTTON(img->media_duration));
		item->tree_path = g_strdup("0");

		//This is needed to speed up the preview
		img_create_cached_cairo_surface(img, item, entry->full_path);
		width = item->duration;
		item->transition_id = -1;
	}
	else if (entry->media_type == 1)
	{
		item->duration = img_convert_time_string_to_seconds(entry->audio_duration);
		width = img_convert_time_string_to_seconds(entry->audio_duration);
		item->trans_group = NULL;
		item->tree_path = NULL;
	}
	img_timeline_create_toggle_button(item, entry->media_type, entry->full_path, img);
	width *= priv->pixels_per_second;

	// If x is -1, use track->last_media_posX, otherwise use the provided x position
	if (x < 0)
		position_x = track->last_media_posX;
	else
	{
		position_x = x;
		// Update track->last_media_posX to the drop position for subsequent items
		track->last_media_posX = x;
	}
	// Set start time based on the actual position
	item->old_x = position_x;
    item->start_time = position_x / (BASE_SCALE * priv->zoom_scale);
   	gtk_widget_set_size_request(item->button, width, 50);
	gtk_layout_move(GTK_LAYOUT(timeline), item->button, position_x, new_y);
	track->last_media_posX += width;
	
	g_array_append_val(track->items, item);
	img_taint_project(img);
	
	gint test = img_timeline_get_final_time(img);
}

void img_timeline_create_toggle_button(media_timeline *item, gint media_type, gchar *filename, img_window_struct *img)
{
	GdkPixbuf *pix = NULL;
    GtkWidget *image, *layout;

	if (item->media_type == 0)
	{
		item->button = gtk_toggle_button_new();
		gtk_widget_set_has_tooltip(item->button, TRUE);
	
		layout = gtk_layout_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(item->button), layout);
		gtk_style_context_add_class(gtk_widget_get_style_context(item->button), "timeline-button");
		pix = gdk_pixbuf_new_from_file_at_scale(filename, -1, 45, TRUE, NULL);
		image = gtk_image_new_from_pixbuf(pix);
		g_object_unref(pix);

		// Add the image to the layout
		gtk_layout_put(GTK_LAYOUT(layout), image, 0, 0);
		img_timeline_center_button_image(item->button);
	}
	else if (item->media_type == 1)
	{
		item->button = img_media_audio_button_new();
		gtk_widget_set_has_tooltip(item->button, TRUE);
		img_load_audio_file(IMG_MEDIA_AUDIO_BUTTON(item->button), filename);
	}
	else if (item->media_type == 3)
	{
		item->button = gtk_toggle_button_new();
		gtk_widget_set_has_tooltip(item->button, TRUE);
		gtk_style_context_add_class(gtk_widget_get_style_context(item->button), "timeline-button");
	}
	g_object_set_data(G_OBJECT(item->button), "mem_address", item);
    gtk_widget_add_events(item->button, GDK_POINTER_MOTION_MASK
                                      | GDK_LEAVE_NOTIFY_MASK
                                      | GDK_BUTTON1_MASK 
                                      | GDK_BUTTON1_MOTION_MASK
                                      | GDK_BUTTON_PRESS_MASK
                                      | GDK_BUTTON_RELEASE_MASK);
                                      
	g_signal_connect(item->button, "drag-data-get", 				G_CALLBACK(img_timeline_media_drag_data_get), NULL);
	g_signal_connect(item->button, "motion-notify-event",		G_CALLBACK(img_timeline_media_motion_notify), img);
	g_signal_connect(item->button, "leave-notify-event", 		G_CALLBACK(img_timeline_media_leave_event), img->timeline);
	g_signal_connect(item->button, "button-press-event",		G_CALLBACK(img_timeline_media_button_press_event), img);
	g_signal_connect(item->button, "button-release-event", 	G_CALLBACK(img_timeline_media_button_release_event), img);
	g_signal_connect(item->button, "query-tooltip",					G_CALLBACK(img_timeline_media_button_tooltip), item);

	gtk_container_add(GTK_CONTAINER(img->timeline), item->button);
	gtk_widget_show_all(item->button);
}

void img_timeline_add_track(GtkWidget *timeline, gint type, gchar *hexcode)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *new_track;
	gint total_height;
	
	new_track = g_new0(Track, 1);
	new_track->type = type;
	new_track->is_selected = FALSE;
	new_track->is_default = FALSE;
	new_track->order = next_order++;
	new_track->items = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	new_track->background_color = g_strdup(hexcode);
	g_array_append_val(priv->tracks, new_track);
	
	// Sort by image track first so they can be easily drawn in the draw event
	g_array_sort(priv->tracks,  img_sort_image_track_first);
}

void img_timeline_draw_time_marker(GtkWidget *widget, cairo_t *cr, gint posx, gint length)
{
    // Start the path
    cairo_move_to(cr, posx - 5, 16);
    // Top horizontal line
    cairo_line_to(cr, posx + 5, 16);
    // Right vertical line
    cairo_line_to(cr, posx + 5, 16);
    // Bottom-right curve
    cairo_curve_to(cr,
        posx + 5, 32,
        posx + 2.5, 32,
        posx, 32);
    // Bottom-left curve
    cairo_curve_to(cr,
        posx - 2.5, 32,
        posx - 5, 32,
        posx - 5, 16);
    cairo_close_path(cr);
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_set_line_width(cr, 2);
    cairo_fill(cr);

    //Draw the needle
    cairo_move_to(cr, posx, 32);
    cairo_line_to(cr, posx, img_timeline_calculate_total_tracks_height(widget) + 6);
    cairo_stroke(cr);
}

void img_timeline_media_drag_data_get(GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *data, guint info, guint time, ImgTimeline *timeline)
{
	//This callback is for the timeline itself when data is being dragged OUT of it
	gtk_selection_data_set (data, gdk_atom_intern_static_string ("IMG_TIMELINE_MEDIA"), 32, (const guchar *)&widget, sizeof (gpointer));
	g_print("Drag data get\n");
}

gboolean img_timeline_mouse_button_press (GtkWidget *timeline, GdkEventButton *event, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	gchar *time;
	
	if (event->y < 12.0 || event->y > 32.0)
	{
		if (event->button == 1)
        {
			priv->rubber_band_active = TRUE;
			priv->rubber_band_start_x = event->x;
			priv->rubber_band_start_y = event->y;
			priv->rubber_band_end_x = event->x;
			priv->rubber_band_end_y = event->y;
			gtk_widget_queue_draw(timeline);
			return TRUE;
        }
	}

	if (event->button == 1)
	{
		priv->current_preview_time = event->x  / priv->pixels_per_second;
		priv->button_pressed_on_needle = TRUE;
		img_timeline_set_time_marker((ImgTimeline*)timeline, event->x);
		time = img_convert_time_to_string(event->x  / priv->pixels_per_second);
		gtk_label_set_text(GTK_LABEL(img->current_time), time);
		g_free(time);
		gtk_widget_queue_draw(img->image_area);
	}
	return TRUE;
}

gboolean img_timeline_mouse_button_release (GtkWidget *timeline, GdkEvent *event, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)img->timeline);
	Track *track = NULL;

	if (priv->rubber_band_active)
    {
        priv->rubber_band_active = FALSE;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(img->toggle_button_text)))
		{
			gdouble y;
			gint new_y;
			gdk_event_get_coords(event, NULL, &y);
			
			gint track_index = img_timeline_get_track_at_position(timeline, y, &new_y);
			track = g_array_index(priv->tracks, Track *, track_index);
			if (track)
			{
				track->type == 0;
				img_create_text_item(img, track_index, new_y);
			}
		}
        gtk_widget_queue_draw(img->timeline);
        gtk_widget_queue_draw(img->image_area);
    }
  
	if (priv->button_pressed_on_needle)
		priv->button_pressed_on_needle = FALSE;

	return TRUE;
}

gboolean img_timeline_key_press(GtkWidget *widget, GdkEventKey *event, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)widget);
	Track *track = NULL;
	Track *track2 = NULL;
	media_timeline *item = NULL;
	gint y = 0;
	
	gboolean shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;

	switch (event->keyval)
	{
		case GDK_KEY_Home:
			img_timeline_go_start_time(NULL, img);
		break;

		case GDK_KEY_End:
			img_timeline_go_final_time(NULL, img);
		break;

		case GDK_KEY_A:
		case GDK_KEY_a:
			if (shift_pressed)
			{
				for (gint i = 0; i < priv->tracks->len; i++)
				{
					track = g_array_index(priv->tracks, Track *, i);
					if (track->type == 0)
					{
						for (gint q = track->items->len - 1; q >= 0; q--)
						{
							item = g_array_index(track->items, media_timeline  *, q);
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->button), TRUE);
						}
					}
				}
			}
		break;

		case GDK_KEY_Delete:
			// First pass - delete the selected items
			for (gint i = 0; i < priv->tracks->len; i++)
			{
				track = g_array_index(priv->tracks, Track *, i);
				if (track->items) {
					for (gint q = track->items->len - 1; q >= 0; q--)
					{
						item = g_array_index(track->items, media_timeline  *, q);
						if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)))
						{
							gtk_widget_destroy(item->button);
							if (item->tree_path)
								g_free(item->tree_path);
							if (item->trans_group)
								g_free(item->trans_group);
							if (item->media_type == 3)
							{
								if(item->text->cursor_source_id > 0)
								{
									g_source_remove(item->text->cursor_source_id);
									item->text->cursor_source_id = 0;
								}
								img_free_media_text_struct(item->text);
							}
							g_array_remove_index(track->items, q);
							g_free(item);
							img_taint_project(img);
						}
					}
				}
			}
			// Second pass - move items up
			if (priv->tracks->len > 2)
			{
				for (gint i = 0; i < priv->tracks->len; i++)
				{
					track = g_array_index(priv->tracks, Track *, i);
					if (track->items->len == 0)
					{
						// When we find an empty track, shift up ALL elements from ALL tracks below it
						for (gint j = i + 1; j < priv->tracks->len; j++)
						{
							track2 = g_array_index(priv->tracks, Track *, j);
							 if (track2->items->len > 0 && ! track->is_default) 
							{
								for (gint q = 0; q < track2->items->len; q++)
								{
									item = g_array_index(track2->items, media_timeline  *, q);
									GtkAllocation allocation;
									gtk_widget_get_allocation(item->button, &allocation);
									item->timeline_y = allocation.y - (TRACK_HEIGHT + TRACK_GAP);
									gtk_layout_move(GTK_LAYOUT(img->timeline), item->button, allocation.x, item->timeline_y);
								}
							}
						}
					}
				}
			}
			// Third pass - delete non defaults empty tracks
			for (gint i = priv->tracks->len - 1; i >= 0; i--)
			{
				track = g_array_index(priv->tracks, Track *, i);
				if (track->items->len == 0 && track->is_default == FALSE)
				{
					g_array_remove_index(priv->tracks, i);
					g_free(track->background_color);
					g_free(track);
				}
			}
			break;
	}
	gtk_widget_queue_draw(img->timeline);
	gtk_widget_queue_draw(img->image_area);
	gint unused =	img_timeline_get_final_time(img);
	return TRUE;
}

gboolean img_timeline_motion_notify(GtkWidget *timeline, GdkEventMotion *event, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline *)timeline);
	gchar *time;

	if (priv->rubber_band_active)
    {
        priv->rubber_band_end_x = event->x;
        priv->rubber_band_end_y = event->y;
        
        // Select media items within the rubber band
        img_timeline_select_items_in_rubber_band(timeline);
        
        gtk_widget_queue_draw(timeline);
        return TRUE;
    }
    
	if (event->x < 0)
		return FALSE;

	if (priv->button_pressed_on_needle)
	{
		img_timeline_set_time_marker((ImgTimeline*)timeline, event->x);
		
		time = img_convert_time_to_string(event->x / priv->pixels_per_second);
		gtk_label_set_text(GTK_LABEL(img->current_time), time);
		g_free(time);
		
		gtk_widget_queue_draw(img->image_area);
	}

   return TRUE;
}

static void img_timeline_select_items_in_rubber_band(GtkWidget *timeline)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track =NULL;
	media_timeline *item = NULL;
	GtkAllocation button_allocation;

	gdouble band_left = MIN(priv->rubber_band_start_x, priv->rubber_band_end_x);
	gdouble band_right = MAX(priv->rubber_band_start_x, priv->rubber_band_end_x);
	gdouble band_top = MIN(priv->rubber_band_start_y, priv->rubber_band_end_y);
	gdouble band_bottom = MAX(priv->rubber_band_start_y, priv->rubber_band_end_y);

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		for (gint j = 0; j < track->items->len; j++)
		{
			item = g_array_index(track->items, media_timeline *, j);
			gtk_widget_get_allocation(item->button, &button_allocation);
			
			// Check if the media item intersects with the rubber band
			gboolean intersects = !(button_allocation.x + button_allocation.width < band_left ||
								button_allocation.x > band_right ||
								button_allocation.y + button_allocation.height < band_top ||
								button_allocation.y > band_bottom);
		
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->button), intersects);
			item->is_selected = intersects;
		}
	}
}

gboolean img_timeline_media_button_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, media_timeline *item)
{
	gchar *string, *string2, *string3;

	string 	= img_convert_time_to_string(item->start_time);
	string2 = img_convert_time_to_string(item->duration);
	if (item->media_type == 0)
		string3 = g_strdup_printf(_("Start time: %s\nDuration: %s\nTransition group: %s"), string, string2, item->trans_group);
	else
		string3 = g_strdup_printf(_("Start time: %s\nDuration: %s"), string, string2);
	gtk_tooltip_set_text(tooltip, string3);
	
	g_free(string);
	g_free(string2);
	g_free(string3);
	return TRUE;
}

gboolean img_timeline_media_button_press_event(GtkWidget *button, GdkEventButton *event, img_window_struct *img)
{
	media_timeline *item;
	
	if (img->current_item->text)
		img->current_item->text->visible = FALSE;
	
	item = g_object_get_data(G_OBJECT(button), "mem_address");
	item->button_pressed = TRUE;
	gtk_notebook_set_current_page (GTK_NOTEBOOK(img->side_notebook), 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(img->toggle_button_image_options), TRUE);

	// Store the initial mouse position and button width
	item->timeline_drag_x = event->x;
	item->initial_width = gtk_widget_get_allocated_width(button);
	
	if (event->x <= 10)
		item->resizing = RESIZE_LEFT;
	else if (event->x >= item->initial_width - 10)
		item->resizing = RESIZE_RIGHT;
	else
		item->resizing = RESIZE_NONE;

	img_timeline_set_media_properties(img, item);
	img->current_item = item;
	
	if (img->current_item->text)
		img->current_item->text->visible = TRUE;
	return FALSE;
}

gboolean img_timeline_media_button_release_event(GtkWidget *button, GdkEventButton *event, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline *)img->timeline);
	Track *track;
	media_timeline *item;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	gboolean shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;

	item = g_object_get_data(G_OBJECT(button), "mem_address");
	item->button_pressed = FALSE;
	item->resizing = RESIZE_NONE;
	item->right_edge_pos = 0;
	
	if (!shift_pressed)
		img_deselect_all_surfaces(img);

	gtk_widget_queue_draw(img->image_area);
	return FALSE;
}

gboolean img_timeline_media_motion_notify(GtkWidget *button, GdkEventMotion *event, img_window_struct *img)
{
    ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline *)img->timeline);
    gdouble x, new_width, button_width, timeline_width, nearest_tick;
    GdkCursor *cursor;
    GdkWindow *window;
    media_timeline *item;
	gint posx;

    item = g_object_get_data(G_OBJECT(button), "mem_address");
    window = gtk_widget_get_window(button);
    button_width = gtk_widget_get_allocated_width(button);

    if (item->button_pressed)
    {
        switch (item->resizing)
        {
			case RESIZE_LEFT:
				if (item->media_type == 1)
					return FALSE;

				// Store the initial right edge position when resize starts
				if (!item->right_edge_pos)
					item->right_edge_pos = item->old_x + item->initial_width;
				
				// Calculate how much the mouse has moved from the drag start point
				gdouble delta = event->x - item->timeline_drag_x;
				
				// Calculate new left edge position
				x = item->old_x + delta;
				
				// Find nearest tick for snapping
				nearest_tick = find_nearest_major_tick(priv->pixels_per_second, x);
				if (abs(x - nearest_tick) < 10) {
					x = nearest_tick;
				}
				
				// Ensure x doesn't go beyond bounds
				x = MAX(x, 0);
				x = MIN(x, item->right_edge_pos - 1); // Ensure minimum width of 1
				
				// Calculate new width while maintaining right edge
				new_width = item->right_edge_pos - x;
				
				// Apply changes
				gtk_widget_set_size_request(button, new_width, 50);
				gtk_layout_move(GTK_LAYOUT(img->timeline), button, x, item->timeline_y);
				
				// Update item properties
				item->old_x = x;
				item->initial_width = new_width;
				item->start_time = x / priv->pixels_per_second;
				item->duration = new_width / priv->pixels_per_second;
				img_timeline_center_button_image(item->button);
			break;
			
			case RESIZE_RIGHT:
				if (item->media_type == 1)
					return FALSE;
				new_width = MAX(event->x, 1);
				nearest_tick = find_nearest_major_tick(priv->pixels_per_second, item->old_x + new_width);
				if (abs((item->old_x + new_width) - nearest_tick) < 10)
					new_width = nearest_tick - item->old_x;
				
				gtk_widget_set_size_request(button, new_width, 50);
				item->duration = new_width / priv->pixels_per_second;
				img_timeline_center_button_image(item->button);
			break;


			case RESIZE_NONE:
			{
				GArray *selected_items = img_timeline_get_selected_items(img->timeline);
				timeline_width = gtk_widget_get_allocated_width(GTK_WIDGET(img->timeline));
				
				// Find the minimum and maximum x-coordinates of the selected items
				gint min_x = G_MAXINT, max_x = 0;
				for (gint j = 0; j < selected_items->len; j++)
				{
					media_timeline *item = g_array_index(selected_items, media_timeline *, j);
					min_x = MIN(min_x, item->old_x);
					max_x = MAX(max_x, item->old_x + gtk_widget_get_allocated_width(item->button));
				}
				// Calculate the offset based on the drag event
				gint offset = event->x - item->timeline_drag_x;
			
				// Iterate through the selected items and update their positions
				for (gint j = 0; j < selected_items->len; j++)
				{
					media_timeline *item = g_array_index(selected_items, media_timeline *, j);
					x = item->old_x + offset;
					button_width = gtk_widget_get_allocated_width(item->button);

					// Clamp the x-coordinate based on the combined width of the selected items
					x = CLAMP(x, 0, timeline_width - button_width);
				
					nearest_tick = find_nearest_major_tick(priv->pixels_per_second, x);
					if (abs(x - nearest_tick) < 10)
						x = nearest_tick;
				
					// Move the button to the new position, maintaining the relative alignment
					gtk_layout_move(GTK_LAYOUT(img->timeline), item->button, x, item->timeline_y);
					item->start_time = x / priv->pixels_per_second;
					item->old_x = x;
				}
				g_array_free(selected_items, FALSE);
			break;
			}
        }
        // Update the final time
		posx = img_timeline_get_final_time(img);
        gtk_widget_trigger_tooltip_query(GTK_WIDGET(item->button));
    }

    // Update cursor based on mouse position
    if (event->x <= 10 || (event->x >= button_width - 10 && event->x <= button_width))
        cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_SB_H_DOUBLE_ARROW);
    else
        cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_ARROW);
    
    gdk_window_set_cursor(window, cursor);
    g_object_unref(cursor);

    return TRUE;
}

gboolean img_timeline_media_leave_event(GtkWidget *button, GdkEventCrossing *event, ImgTimeline *timeline)
{
	 GdkCursor *cursor;
	 GdkWindow *window;

	window = gtk_widget_get_window(button);
	cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_ARROW);
	gdk_window_set_cursor(window, cursor);
	return FALSE;
}

gboolean img_timeline_scroll_event(GtkWidget *timeline, GdkEventScroll *event, GtkWidget *viewport) 
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	media_timeline *item;
	gdouble step, position, dx, dy, width, new_x;

	gboolean ctrl_pressed = (event->state & GDK_CONTROL_MASK) != 0;
	gdk_event_get_scroll_deltas((GdkEvent *)event, &dx, &dy);

	GtkAdjustment *scroll_x = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(viewport));
	step 								 = gtk_adjustment_get_step_increment(scroll_x);
	position							 = gtk_adjustment_get_value(scroll_x);

	if (ctrl_pressed)
	{
		if (dy > 0)
			priv->zoom_scale *= 1.1;
		else if (dy < 0)
			priv->zoom_scale /= 1.1;

		priv->zoom_scale = CLAMP(priv->zoom_scale, 0.01, 10.0);
        priv->pixels_per_second = BASE_SCALE * priv->zoom_scale;

		// Calculate the new width of all media items on all tracks
		for (gint i = 0; i < priv->tracks->len; i++)
		{
			track = g_array_index(priv->tracks, Track *, i);
			if (track->items)
			{
				for (gint q = 0; q < track->items->len; q++)
				{
					item = g_array_index(track->items, media_timeline  *, q);
					if (item)
					{
						width = item->duration * priv->pixels_per_second;
						gtk_widget_set_size_request(GTK_WIDGET(item->button), width, 50);
						new_x = item->start_time * priv->pixels_per_second;
						gtk_layout_move(GTK_LAYOUT(timeline), item->button, new_x, item->timeline_y);
						if (item->media_type == 0)
							img_timeline_center_button_image(item->button);
					}
				}
			}
		}
		gtk_widget_queue_draw(timeline);
	}
	else
	{
		 if (dy > 0)
			gtk_adjustment_set_value(scroll_x,  position + step);
		else
			gtk_adjustment_set_value(scroll_x, position - step);
	}
	return TRUE;
}

void img_timeline_drag_data_received (GtkWidget *timeline, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint time, img_window_struct *img)
{
	gchar **images = NULL;
	gchar *filename;
	gint i = 0, length, n_structs;
	const guchar *data;
	media_struct **media_structs;
	gboolean flag;

	if (gtk_selection_data_get_target(selection_data) != gdk_atom_intern( "application/x-media-item", FALSE))
	{
		images = gtk_selection_data_get_uris(selection_data);
		if (images)
		{
			while(images[i])
			{
				filename = g_filename_from_uri (images[i], NULL, NULL);
				if (img_create_media_struct(filename, img))
				{
					img_timeline_add_media(timeline, img->current_media, x, y, img);
					img_taint_project(img);
				}
				g_free(filename);
				i++;
			}
		}
		g_strfreev (images);	
	}
	// The timeline is receiving media items from the top media widget within Imagination itself
	else
	{
		// Get the received data
		data = gtk_selection_data_get_data(selection_data);
		length = gtk_selection_data_get_length(selection_data);

		if (length < 0)
		{
			g_print("No data received\n");
			return;
		}

		// Calculate how many media struct pointers we received
		n_structs = length / sizeof(media_struct*);
		media_structs = (media_struct**)data;

		// Process each received media_struct
		for (int i = n_structs-1 ; i >= 0; i--)
			img_timeline_add_media(timeline, media_structs[i], (i == n_structs-1) ? x : -1, y, img);
	}

	gtk_drag_finish (context, TRUE, FALSE, time);
}

void img_timeline_drag_data_get (GtkWidget *timeline, GdkDragContext *context, GtkSelectionData *selection_data, guint target_type, guint time, img_window_struct *img)
{
	g_print("Cucu\n");
}

gboolean img_timeline_drag_motion(GtkWidget *timeline, GdkDragContext *context, gint x, gint y, guint time, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);

	Track *track;
	gint track_index;
	static int last_highlighted_track = -1;
	
	track_index = img_timeline_get_track_at_position(timeline, y, NULL);

	if (track_index >=0 )
	{
		if (track_index != last_highlighted_track)
		{
            // Clear previous highlight and set new one
            img_timeline_unhighlight_track(priv->tracks);
            track = g_array_index(priv->tracks, Track *, track_index);
			track->is_selected = TRUE;
			gtk_widget_queue_draw(timeline);
			last_highlighted_track = track_index;
        }
		gdk_drag_status(context, gdk_drag_context_get_suggested_action(context), time);
		return TRUE;
	}
	else
	{
		if (last_highlighted_track != -1)
		{
			img_timeline_unhighlight_track(priv->tracks);
			last_highlighted_track = -1;
		}
		gdk_drag_status(context, 0, time);
		return FALSE;
	}
}

gboolean img_timeline_drag_leave(GtkWidget *timeline, GdkDragContext *context,  guint time, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	
	img_timeline_unhighlight_track(priv->tracks);
	return TRUE;
}

static gint img_timeline_calculate_total_tracks_height(GtkWidget *da)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);
	gint total = 0; //Start from the height of the first two default tracks
	
	for (gint i = 0; i < priv->tracks->len; i++)
		total += (TRACK_HEIGHT + TRACK_GAP);

    return total + 20;
}

static gint img_sort_image_track_first(gconstpointer a, gconstpointer b)
{
	const Track *track_a = *(const Track **)a;
	const Track *track_b = *(const Track **)b;;
	
	if (track_a->type != track_b->type)
        return track_a->type - track_b->type;

    // If both are image tracks, sort by FILO order
    if (track_a->type == 0)
        return track_b->order - track_a->order;
    
    // If both are audio tracks, sort by FIFO order
    return track_a->order - track_b->order;
}

static gint img_timeline_get_track_at_position(GtkWidget *timeline, gint pos, gint *y_out)
{
	Track *track;
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
		
	gint y = 32;
	 for (int i = 0; i < priv->tracks->len; i++)
	{
		if (pos > y && pos < y + TRACK_HEIGHT + TRACK_GAP)
		{
			if (y_out)
				*y_out = y;
			return i;
		} 
		y += TRACK_HEIGHT + TRACK_GAP;
	}
	return -1;
}

static void img_timeline_unhighlight_track(GArray *tracks)
{
	Track *track;

	for (gint i = 0; i < tracks->len; i++)
	{
		track = g_array_index(tracks, Track *, i);
		track->is_selected = FALSE;
	}
}

void img_timeline_delete_all_media(ImgTimeline *timeline)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	media_timeline *item;

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->items->len > 0)
		{
			for (gint q = track->items->len - 1; q >= 0; q--)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				gtk_widget_destroy(item->button);
				if (item->tree_path)
					g_free(item->tree_path);
				if (item->trans_group)
					g_free(item->trans_group);
				if (item->media_type == 3)
					img_free_media_text_struct(item->text);
				g_array_remove_index(track->items, q);
				g_free(item);
			}
		}
	}
}

void img_timeline_delete_additional_tracks(ImgTimeline *timeline)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	
	for (gint i = priv->tracks->len -1; i>=0; i--)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (! track->is_default)
		{
			g_free(track->background_color);
			g_array_remove_index(priv->tracks, i);
			g_free(track);
		}
	}
	next_order = 2;
	gtk_widget_queue_draw(GTK_WIDGET(timeline));
}

GArray *img_timeline_get_active_picture_media(GtkWidget *timeline, gdouble current_time)
{
    ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
    Track *track;
    GArray* active_elements;
    media_timeline *item;
    double item_end;

    active_elements = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
    for (gint i = 0; i < priv->tracks->len; i++)
    {
        track = g_array_index(priv->tracks, Track *, i);
        for (gint j = 0; j < track->items->len; j++)
        {
            item = g_array_index(track->items, media_timeline *, j);
            if (item->media_type == 0 || item->media_type == 2 || item->media_type == 3)  
           {
				item_end = item->start_time + item->duration;
				if (current_time >= item->start_time && current_time < item_end)
					g_array_append_val(active_elements, item);
			}
        }
    }
    return active_elements;
}

GArray *img_timeline_get_active_audio_media(GtkWidget *timeline, gdouble current_time)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	GArray* active_elements;
	media_timeline *item;
	double item_end;

	active_elements = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		for (gint j = 0; j < track->items->len; j++)
		{
			item = g_array_index(track->items, media_timeline *, j);
			if (item->media_type == 1 && ! item->is_playing)
			{
				item_end = item->start_time + item->duration;
				if (current_time >= item->start_time && current_time <= item_end)
				{
						g_print("Aggiungo %d %2.2f - %2.2f\n",item->id,item->start_time, current_time);
						g_array_append_val(active_elements, item);
				}
			}
		}
	}
	return active_elements;
}

GArray *img_timeline_get_active_text_media(GtkWidget *timeline, gdouble current_time)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	GArray* active_elements;
	media_timeline *item;

	active_elements = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		for (gint j = 0; j < track->items->len; j++)
		{
			item = g_array_index(track->items, media_timeline *, j);
			if (item->media_type == 3)
				g_array_append_val(active_elements, item);
		}
	}
	return active_elements;
}

void img_timeline_center_button_image(GtkWidget *button)
{
	GtkWidget *image;
	GtkWidget *layout;
	GtkAllocation button_allocation;
	GtkAllocation image_allocation;
    gint x, y;
	GList *children;

	layout = gtk_bin_get_child(GTK_BIN(button));
	if (layout)
	{
		children = gtk_container_get_children(GTK_CONTAINER(layout));
 		image = GTK_WIDGET(children->data);
		g_list_free(children);

		gtk_widget_get_allocation(GTK_WIDGET(layout), &button_allocation);
		gtk_widget_get_allocation(image, &image_allocation);

		x = (button_allocation.width - image_allocation.width) / 2;
		y = (button_allocation.height - image_allocation.height) / 2;
	
		gtk_layout_move(GTK_LAYOUT(layout), image, x, y);
	}
}

gint img_timeline_get_final_time(img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)img->timeline);
	Track *track;
	media_timeline *item;
	gint total_time = 0;
	gint last_duration = 0;
	gchar* total_time_string;

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->items)
		{
			for (gint q = track->items->len - 1; q >= 0; q--)
			{
				item = g_array_index(track->items, media_timeline *, q);
				last_duration = item->start_time + item->duration;
				if (last_duration > total_time)
					total_time = last_duration;
			}
		}
	}
	total_time_string = img_convert_time_to_string(total_time);
	gtk_label_set_text(GTK_LABEL(img->total_time_label), total_time_string);
	g_free(total_time_string);
	
	img->total_time = total_time;
	return total_time;
}

void img_timeline_go_start_time(GtkWidget *button, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)img->timeline);

	priv->current_preview_time = 0.0;
	GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(img->timeline_scrolled_window));
	gdouble lower = gtk_adjustment_get_lower(hadj);
	gtk_adjustment_set_value(hadj, lower);

	gtk_label_set_text(GTK_LABEL(img->current_time), "00:00:00");
	img_timeline_set_time_marker((ImgTimeline*)img->timeline, 0.0);
	gtk_widget_queue_draw(img->image_area);
}

void img_timeline_go_final_time(GtkWidget *button, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)img->timeline);
	gchar *total_time_string;
	gint time;
	GtkAllocation allocation;
	GtkAdjustment *hadj;
	GtkWidget *viewport;
	gdouble pixel_position, viewport_width, upper, page_size, max_scroll, scroll_position;

	time = img_timeline_get_final_time(img);
	priv->current_preview_time = time;
	total_time_string = img_convert_time_to_string(time);
	gtk_label_set_text(GTK_LABEL(img->current_time), total_time_string);
	g_free(total_time_string);
	
	hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(img->timeline_scrolled_window));
	pixel_position = time * priv->pixels_per_second;
	
	viewport = gtk_bin_get_child(GTK_BIN(img->timeline_scrolled_window));
	gtk_widget_get_allocation(viewport, &allocation);
	viewport_width = allocation.width;
	
	// Calculate the scroll position to center the scroll in the middle of the timeline
	scroll_position = pixel_position - (viewport_width / 2);
    
	// Ensure we don't scroll beyond bounds
	upper = gtk_adjustment_get_upper(hadj);
	page_size = gtk_adjustment_get_page_size(hadj);
	max_scroll = upper - page_size;
    scroll_position = CLAMP(scroll_position, 0, max_scroll);
	gtk_adjustment_set_value(hadj, scroll_position);
    
	img_timeline_set_time_marker((ImgTimeline*)img->timeline, (gdouble)time * priv->pixels_per_second);
	gtk_widget_queue_draw(img->image_area);
}

void img_timeline_play_audio(media_timeline *media, img_window_struct *img, gdouble current_time)
{
	ImgMediaAudioButtonPrivate *priv = img_media_audio_button_get_private_struct((ImgMediaAudioButton*)media->button);
	GThread *playback_thread;
	
	g_mutex_lock(&priv->audio_data->play_mutex);
	g_atomic_int_set(&priv->audio_data->is_playing, TRUE);
	g_atomic_int_set(&media->is_playing , TRUE);
	g_mutex_unlock(&priv->audio_data->play_mutex);

	priv->audio_data->current_time = current_time;
	img->current_item = media;
	playback_thread = g_thread_new("Imagination_audio_thread",  (GThreadFunc)img_play_audio_alsa, img);
	g_thread_unref(playback_thread);
}

void img_timeline_stop_audio(media_timeline *media, img_window_struct *img)
{
	ImgMediaAudioButtonPrivate *priv = img_media_audio_button_get_private_struct((ImgMediaAudioButton*)media->button);
	if (media->is_playing)
	{
		//g_print("img_timeline_stop_audio metto a FALSE per %d\n",media->id);
		g_mutex_lock(&priv->audio_data->play_mutex);
		g_atomic_int_set(&priv->audio_data->is_playing, FALSE);
		g_atomic_int_set(&media->is_playing, FALSE);
		g_mutex_unlock(&priv->audio_data->play_mutex);

		img->media_playing = g_slist_remove(img->media_playing, media);
	}
}

void img_timeline_update_audio_states(img_window_struct *img, gdouble current_time)
{
    if (img->preview_is_running) 
	{
		GArray* active_media = img_timeline_get_active_audio_media(img->timeline, current_time);
        for (gint i = 0; i < active_media->len; i++) 
		{
			media_timeline *media = g_array_index(active_media, media_timeline *, i);
			if (!media->is_playing)
			{
				//g_print("Aggiungo %d - %s\n",media->id,img_get_media_filename(img,media->id));
				img->media_playing = g_slist_append(img->media_playing, media);
				//g_print("Lunghezza %d\n",g_slist_length(img->media_playing));
				g_print("Riproduco %d\n",media->id);
				img_timeline_play_audio(media, img, current_time);
			}
		}
	    g_array_free(active_media, FALSE);
    }
    else
		g_slist_foreach(img->media_playing, (GFunc)  img_timeline_stop_audio, img);
}

GArray *img_timeline_get_active_media_at_given_time(GtkWidget *timeline, gdouble current_time)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	media_timeline *item;
	GArray* active_elements;
	gint item_end;
	
	active_elements =  g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		for (gint j = 0; j < track->items->len; j++)
		{
			item = g_array_index(track->items, media_timeline *, j);
			item_end = item->start_time + item->duration;
			if (current_time >= item->start_time && current_time < item_end)
				g_array_append_val(active_elements, item);
		}
	}
	return  active_elements;;
}

GArray *img_timeline_get_selected_items(GtkWidget *timeline)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	media_timeline *item;
	GArray* selected_items;

	selected_items = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		for (gint j = 0; j < track->items->len; j++)
		{
			item = g_array_index(track->items, media_timeline *, j);
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)))
				g_array_append_val(selected_items, item);
		}
	}
	return selected_items;
}


gboolean img_timeline_check_for_media_audio(GtkWidget *timeline)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	media_timeline *item;
	
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		for (gint j = 0; j < track->items->len; j++)
		{
			item = g_array_index(track->items, media_timeline *, j);
			if (item->media_type == 1)
				return TRUE;
		}
	}
	return FALSE;	
}

void img_timeline_set_media_properties(img_window_struct *img, media_timeline *item)
{
	GtkTreeIter   iter;
	GtkTreeIter   parent;
	GtkTreeModel *model;

	if (item->media_type == 0)
	{
		gtk_widget_show(img->image_hbox);
		gtk_widget_show(img->transition_hbox);
		gtk_widget_show(img->opacity_hbox);
		gtk_widget_show(img->effect_combobox);
		gtk_widget_hide(img->volume_hbox);
		
		// Update the transition combo box with the transition set on the media item
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(img->transition_type));
		g_signal_handlers_block_by_func(img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(img->transition_type));
		gtk_tree_model_get_iter_from_string( model, &iter, item->tree_path);
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(img->transition_type), &iter);
		g_signal_handlers_unblock_by_func(img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);

		// Update the spin button with the duration set on the media item
		g_signal_handlers_block_by_func(img->media_duration, (gpointer)img_media_duration_value_changed, img);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(img->media_duration), item->duration);
		g_signal_handlers_unblock_by_func(img->media_duration, (gpointer)img_media_duration_value_changed, img);
		
		// Update the scale slider with the opacity set on the media item
		g_signal_handlers_block_by_func(img->media_opacity, (gpointer)img_opacity_value_changed, img);
		gtk_range_set_value(GTK_RANGE(img->media_opacity), item->opacity * 100);
		g_signal_handlers_unblock_by_func(img->media_opacity, (gpointer)img_opacity_value_changed, img);
		
		// Update the filter combo box slider with the effect set on the item
		g_signal_handlers_block_by_func(img->effect_combobox, (gpointer)img_surface_effect_changed, img);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->effect_combobox), item->color_filter);
		g_signal_handlers_unblock_by_func(img->effect_combobox, (gpointer)img_surface_effect_changed, img);

		item->is_selected = TRUE;
		img->current_item = item;
		gtk_widget_queue_draw(img->image_area);
	}
	else if (item->media_type == 1)
	{
		gtk_widget_hide(img->image_hbox);
		gtk_widget_hide(img->transition_hbox);
		gtk_widget_hide(img->opacity_hbox);
		gtk_widget_hide(img->effect_combobox);
		gtk_widget_show(img->volume_hbox);
		
		// Update the scale slider with the volume set on the media item
		g_signal_handlers_block_by_func(img->media_volume, (gpointer)img_volume_value_changed, img);
		gtk_range_set_value(GTK_RANGE(img->media_volume), item->volume * 100);
		g_signal_handlers_unblock_by_func(img->media_volume, (gpointer)img_volume_value_changed, img);
	}
}
