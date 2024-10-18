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
#include "img_timeline.h"
#include "callbacks.h"

static int next_order = 0;

//Private functions.
static void img_timeline_class_init(ImgTimelineClass *);
static void img_timeline_set_property(GObject *, guint , const GValue *, GParamSpec *);
static void img_timeline_get_property(GObject *, guint , GValue *, GParamSpec *);
static void img_timeline_init(ImgTimeline *);
static void img_timeline_draw_ticks(GtkWidget *widget, cairo_t *);
static void img_timeline_finalize(GObject *);
static gboolean img_timeline_draw(GtkWidget *, cairo_t *);
static gint img_timeline_calculate_total_tracks_height(GtkWidget *);
static gint img_sort_image_track_first(gconstpointer , gconstpointer );
static gint img_timeline_get_track_at_position(GtkWidget *, gint, gint *);
static void img_timeline_unhighlight_track(GArray *);

G_DEFINE_TYPE_WITH_PRIVATE(ImgTimeline, img_timeline, GTK_TYPE_LAYOUT)

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

    priv->last_media_posX = 0;
    priv->zoom_scale = 6.12;
    priv->pixels_per_second = 61.2;
    priv->total_time = 0;
    priv->time_marker_pos = 0.0;
	priv->tracks = g_array_new(FALSE, TRUE, sizeof(Track));

	// Add default image and audio tracks
	Track image_track;
	image_track.type = 0;
	image_track.is_selected = FALSE;
	image_track.is_default = TRUE;
	image_track.order = next_order++;
	image_track.items = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	image_track.background_color = "#CCCCFF";
    g_array_append_val(priv->tracks, image_track);
    
    Track audio_track;
	audio_track.type = 1;
	audio_track.is_selected = FALSE;
	audio_track.is_default = TRUE;
	audio_track.order = next_order++;
	audio_track.items = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	audio_track.background_color = "#d6d1cd";
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
	ImgTimelinePrivate *priv =  img_timeline_get_instance_private((ImgTimeline*)timeline);
	
	return priv;
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
			g_value_set_double(value, (priv->time_marker_pos / priv->pixels_per_second) + 0.1);
		break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

GtkWidget* img_timeline_new()
{
	return GTK_WIDGET(g_object_new(img_timeline_get_type(), NULL));
}

static gboolean img_timeline_draw(GtkWidget *da, cairo_t *cr)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);
	GdkRGBA rgba;
	Track *track;
	
	gint width = gtk_widget_get_allocated_width(da);
	img_timeline_draw_ticks(da, cr);

	// Draw the tracks
	int y = 0;
	for (int i = 0; i < priv->tracks->len; i++)
	{
		track = &g_array_index(priv->tracks, Track, i);
		gdk_rgba_parse(&rgba, track->background_color);
		cairo_set_line_width(cr, 1.0);
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

	  //This is necessary to draw the media represented by the GtkButtons
	  GTK_WIDGET_CLASS (img_timeline_parent_class)->draw (da, cr);

	  //Draw the red time marker 
	  img_timeline_draw_time_marker(da, cr, priv->time_marker_pos, img_timeline_calculate_total_tracks_height(da));

	  return TRUE;
}

static void img_timeline_finalize(GObject *object)
{ 
	ImgTimeline *timeline = IMG_TIMELINE(object);
	ImgTimelinePrivate *priv =img_timeline_get_instance_private(timeline);
	  
	if (priv->tracks)
	{
		for (gint i = 0; i < priv->tracks->len; i++)
		{
			Track *track = &g_array_index(priv->tracks, Track, i);
			if (track->items)
				g_array_free(track->items, TRUE);
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

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    
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

    // Calculate the minimum width needed for a label
    char min_time_str[20];
    snprintf(min_time_str, sizeof(min_time_str), "00:00:00");
    cairo_text_extents(cr, min_time_str, &extents);
    gdouble min_label_width = extents.width * 1.0;  // Add padding

    for (gdouble t = 0; t <= total_time; t += tick_interval)
    {
        gdouble x = t * priv->pixels_per_second;
        if (x > width)
			break;
        if (fmod(t, label_interval) < 0.001)
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

void img_timeline_add_media(GtkWidget *da, media_struct *entry, gint x, gint y, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);

    Track *track;
	media_timeline *item;
    gint pos, track_nr, new_y;
    double width;

	//Calculate the track to move the media to later according to the dropped y coord
	track_nr = img_timeline_get_track_at_position(da, y, &new_y);
	track = &g_array_index(priv->tracks, Track, track_nr);

	if (track->type != entry->media_type)
		return;

	item = g_new0(media_timeline, 1);
	item->id = entry->id;
	
	item->start_time = x / (BASE_SCALE * priv->zoom_scale);
	item->duration = (entry->media_type == 1) ? img_convert_time_string_to_seconds(entry->audio_duration) : 2; 	// Default to 2 seconds
	
	item->tree_path = g_strdup("0");
	item->transition_id = -1;

	img_timeline_create_toggle_button( item, entry->media_type, entry->full_path, img);
    g_array_append_val(track->items, item);
	
	if (x > 0)
		pos = x - 47.5;
	else
        pos = priv->last_media_posX;
    
	//Position the button inside the dropped track and set its size and duration if it's an audio media 
	if (entry->media_type == 1)
		width = img_convert_time_string_to_seconds(entry->audio_duration);
	else
		width = item->duration;

	width *= priv->pixels_per_second;
	gtk_widget_set_size_request(item->button, width, 50);
	gtk_layout_move(GTK_LAYOUT(da), item->button, pos, new_y);

	item->old_x = pos;
	item->y = new_y;
    priv->last_media_posX += 95;

    img_timeline_center_button_image(item->button);
	img_taint_project(img);
	
	gint test = img_timeline_get_final_time(img);
}

void img_timeline_create_toggle_button(media_timeline *item, gint media_type, gchar *filename, img_window_struct *img)
{
	GdkPixbuf *pix = NULL;
    GtkWidget *image, *layout;

	item->button = gtk_toggle_button_new();
	gtk_widget_set_has_tooltip(item->button, TRUE);
	
	layout = gtk_layout_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(item->button), layout);
	g_object_set_data(G_OBJECT(item->button), "mem_address", item);
    gtk_style_context_add_class(gtk_widget_get_style_context(item->button), "timeline-button");
	
	if (media_type == 0)
	{
		pix = gdk_pixbuf_new_from_file_at_scale(filename, -1, 45, TRUE, NULL);
		image = gtk_image_new_from_pixbuf(pix);
	}
	else if(media_type == 1)
	{
		//Get the waveform into the image and resize the button according to the duration of the audio
		image = gtk_image_new_from_icon_name("audio-x-generic", GTK_ICON_SIZE_DIALOG);
	}
	// Add the image to the layout
	gtk_layout_put(GTK_LAYOUT(layout), image, 0, 0);
	img_timeline_center_button_image(item->button);
		
    gtk_widget_add_events(item->button, GDK_POINTER_MOTION_MASK
                                      | GDK_LEAVE_NOTIFY_MASK
                                      | GDK_BUTTON1_MASK 
                                      | GDK_BUTTON1_MOTION_MASK
                                      | GDK_BUTTON_PRESS_MASK
                                      | GDK_BUTTON_RELEASE_MASK);
                                      
	g_signal_connect(item->button, "drag-data-get", 				G_CALLBACK(img_timeline_media_drag_data_get), NULL);
	g_signal_connect(item->button, "motion-notify-event",		G_CALLBACK(img_timeline_media_motion_notify), img->timeline);
	g_signal_connect(item->button, "leave-notify-event", 		G_CALLBACK(img_timeline_media_leave_event), img->timeline);
	g_signal_connect(item->button, "button-press-event",		G_CALLBACK(img_timeline_media_button_press_event), img->timeline);
	g_signal_connect(item->button, "button-release-event", 	G_CALLBACK(img_timeline_media_button_release_event), img);
	g_signal_connect(item->button, "query-tooltip",					G_CALLBACK(img_timeline_media_button_tooltip), item);
	//g_signal_connect(item->button, "size-allocate",					G_CALLBACK(img_timeline_center_button_image), NULL);

	gtk_container_add(GTK_CONTAINER(img->timeline), item->button);
	gtk_widget_show_all(item->button);

	if (pix)
		g_object_unref(pix);
}

void img_timeline_add_track(GtkWidget *timeline, gint type, gchar *hexcode)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track new_track;
	gint total_height;
	
	new_track.type = type;
	new_track.is_selected = FALSE;
	new_track.is_default = FALSE;
	new_track.order = next_order++;
	new_track.items = g_array_new(FALSE, TRUE, sizeof(media_timeline *));
	new_track.background_color = hexcode;
	g_array_append_val(priv->tracks, new_track);
	
	// Sort by image track first so they can be easily drawn in the draw event
	g_array_sort(priv->tracks,  img_sort_image_track_first);
}

void img_timeline_draw_time_marker(GtkWidget *widget, cairo_t *cr, gint posx, gint length)
{
	cairo_save(cr);
	cairo_translate(cr, posx,6);

	// Top horizontal line
    cairo_line_to(cr, 10, 10);

    // Right vertical line
    cairo_line_to(cr, 10, 10 );

    // Bottom-right curve
    cairo_curve_to(cr,
        10, 26,
        5 + 10/4, 26,
        5, 26);

    // Bottom-left curve
    cairo_curve_to(cr,
        5 - 10/4, 26,
        0, 26,
        0, 10 );

    // Left vertical line (closing the path)
    cairo_close_path(cr);

    // Set color and stroke
    cairo_set_source_rgba(cr, 1, 0, 0, 0.5);
    cairo_set_line_width(cr, 2);
    cairo_fill(cr);

	//Draw the needle
	cairo_move_to(cr, 5, 15);
	cairo_line_to(cr, 5, img_timeline_calculate_total_tracks_height(widget));
	cairo_stroke(cr);
	cairo_restore(cr);
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
		return FALSE;

	if (event->button == 1)
	{
		priv->button_pressed_on_needle = TRUE;
		img_timeline_set_time_marker((ImgTimeline*)timeline, event->x);
		
		time = img_convert_time_to_string((event->x + 10) / priv->pixels_per_second);
		gtk_label_set_text(GTK_LABEL(img->current_time), time);
		g_free(time);
		
		gtk_widget_queue_draw(img->image_area);
	}
	return TRUE;
}

gboolean img_timeline_mouse_button_release (GtkWidget *timeline, GdkEvent *event, ImgTimeline *da)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);

	if (priv->button_pressed_on_needle)
		priv->button_pressed_on_needle = FALSE;

	return TRUE;
}

gboolean img_timeline_key_press(GtkWidget *widget, GdkEventKey *event, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)widget);
	Track *track;
	media_timeline *item;

	if (event->keyval == GDK_KEY_Delete)
	{
		// Let's iterate through all the tracks and delete the selected toggle buttons representing the media items
		for (gint i = 0; i < priv->tracks->len; i++)
		{
			track = &g_array_index(priv->tracks, Track, i);
			if (track->items)
			{
				for (gint q = track->items->len - 1; q >= 0; q--)
				{
					item = g_array_index(track->items, media_timeline  *, q);
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)) )
					{
						gtk_widget_destroy(item->button);
						g_array_remove_index(track->items, q);
						g_free(item);
					}
				}
			}
		}
		img_taint_project(img);
		gint unused =	img_timeline_get_final_time(img);
	}
	return TRUE;
}

gboolean img_timeline_motion_notify(GtkWidget *timeline, GdkEventMotion *event, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline *)timeline);
	gchar *time;

	if (event->x < 0)
		return FALSE;

	if (priv->button_pressed_on_needle)
	{
		img_timeline_set_time_marker((ImgTimeline*)timeline, event->x);
		
		time = img_convert_time_to_string((event->x + 10) / priv->pixels_per_second);
		gtk_label_set_text(GTK_LABEL(img->current_time), time);
		g_free(time);
		
		gtk_widget_queue_draw(img->image_area);
	}
   return TRUE;
}

gboolean img_timeline_media_button_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, media_timeline *item)
{
	gchar *string, *string2, *string3;

	string 	= img_convert_time_to_string(item->start_time);
	string2 = img_convert_time_to_string(item->duration);
	string3 = g_strdup_printf(_("Start time: %s\nDuration: %s"), string, string2);
	gtk_tooltip_set_text(tooltip, string3);
	
	g_free(string);
	g_free(string2);
	g_free(string3);
	return TRUE;
}

gboolean img_timeline_media_button_press_event(GtkWidget *button, GdkEventButton *event, ImgTimeline *timeline)
{
	//ImgTimelinePrivate *priv = img_timeline_get_instance_private(timeline);
	media_timeline *item;
	  
	item = g_object_get_data(G_OBJECT(button), "mem_address");
	item->button_pressed = TRUE;
	  
	// Store the initial mouse position and button width
	item->drag_x = event->x;
	item->initial_width = gtk_widget_get_allocated_width(button);
	
	if (event->x <= 10)
		item->resizing = RESIZE_LEFT;
	else if (event->x >= item->initial_width - 10)
		item->resizing = RESIZE_RIGHT;
	else
		item->resizing = RESIZE_NONE;
	  
	  return FALSE;
}

gboolean img_timeline_media_button_release_event(GtkWidget *button, GdkEventButton *event, img_window_struct *img)
{
	 media_timeline *item;
	 item = g_object_get_data(G_OBJECT(button), "mem_address");
	 item->button_pressed = FALSE;
	 item->resizing = RESIZE_NONE;
	gint posx;
	
	posx = img_timeline_get_final_time(img);
	img_timeline_set_time_marker((ImgTimeline *)img->timeline, posx);
	
	return FALSE;
}

gboolean img_timeline_media_motion_notify(GtkWidget *button, GdkEventMotion *event, ImgTimeline *timeline)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private(timeline);

	gchar *string, *string2;
	gint x, new_width, button_width, timeline_width, nearest_tick;
	GdkCursor *cursor;
	GdkWindow *window;
	media_timeline *item;

	item = g_object_get_data(G_OBJECT(button), "mem_address");
	  
	window = gtk_widget_get_window(button);
	button_width = gtk_widget_get_allocated_width(button);

	 if (item->button_pressed)
	{
		switch (item->resizing)
		{
			case RESIZE_LEFT:
				new_width = item->initial_width + (item->drag_x - event->x);
				new_width = MAX(new_width, 1); // Ensure a minimum width of 1 pixel
				x = item->old_x + (item->initial_width - new_width);
				// Snap to nearest tick
				nearest_tick = find_nearest_major_tick(priv->pixels_per_second, x);
                if (abs(x - nearest_tick) < 10)
                {
                    x = nearest_tick;
                    new_width = item->old_x + item->initial_width - x;
                }
				gtk_widget_set_size_request(button, new_width, 50);
				gtk_layout_move(GTK_LAYOUT(timeline), button, x, item->y);
				item->old_x = x;
				item->initial_width = new_width;
				item->start_time = (x + 10)/ priv->pixels_per_second;
				item->duration = (new_width + 20) / (BASE_SCALE * priv->zoom_scale);
				 img_timeline_center_button_image(item->button);
			break;

			case RESIZE_RIGHT:
				new_width = MAX(event->x,1); 
				 nearest_tick = find_nearest_major_tick(priv->pixels_per_second, item->old_x + new_width);
                if (abs((item->old_x + new_width) - nearest_tick) < 10)
                    new_width = nearest_tick - item->old_x;
				gtk_widget_set_size_request(button, new_width, 50);
				item->duration = (new_width +10) / (BASE_SCALE * priv->zoom_scale);
				 img_timeline_center_button_image(item->button);
			break;

			case RESIZE_NONE:
				x = item->old_x + (event->x - item->drag_x);
				timeline_width = gtk_widget_get_allocated_width(GTK_WIDGET(timeline));
				x = CLAMP(x, 0, timeline_width - button_width);
				nearest_tick = find_nearest_major_tick(priv->pixels_per_second,  x);
				if (abs(x - nearest_tick) < 10)
					x = nearest_tick;
				gtk_layout_move(GTK_LAYOUT(timeline), button, x, item->y);
				item->start_time = (x + 10)/ priv->pixels_per_second;

				// Update old_x for the next movement
				item->old_x = x;
			break;
		}
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
	gdouble step, position, dx, dy;
	gint width, new_x;
	
	GdkModifierType accel_mask = gtk_accelerator_get_default_mod_mask();
	gdk_event_get_scroll_deltas((GdkEvent *)event, &dx, &dy);

	GtkAdjustment *scroll_x = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(viewport));
	step 								 = gtk_adjustment_get_step_increment(scroll_x);
	position							 = gtk_adjustment_get_value(scroll_x);

	if ( (event->state & accel_mask) == GDK_CONTROL_MASK)
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
			track = &g_array_index(priv->tracks, Track, i);
			if (track->items)
			{
				for (gint q = 0; q < track->items->len; q++)
				{
					item = g_array_index(track->items, media_timeline  *, q);
					if (item)
					{
						width = (gint)(item->duration * priv->pixels_per_second);
						gtk_widget_set_size_request(GTK_WIDGET(item->button), width, 50);
						new_x = (gint)(item->start_time * priv->pixels_per_second);
						gtk_layout_move(GTK_LAYOUT(timeline), item->button, new_x, item->y);
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
	gint i = 0;
	const guchar *data;
	gint length;
	media_struct **media_structs;
	gint n_structs;

	if (gtk_selection_data_get_target(selection_data) != gdk_atom_intern( "application/x-media-item", FALSE))
	{
		images = gtk_selection_data_get_uris(selection_data);
		if (images)
		{
			while(images[i])
			{
				filename = g_filename_from_uri (images[i], NULL, NULL);
				if (img_add_media(filename, img))
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
		for (int i = 0; i < n_structs; i++)
			img_timeline_add_media(timeline, media_structs[i], x, y, img);
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
            track = &g_array_index(priv->tracks, Track, track_index);
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
	const Track *track_a = a;
	const Track *track_b = b;
	
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
		track = &g_array_index(tracks, Track, i);
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
		track = &g_array_index(priv->tracks, Track, i);
		if (track->items->len > 0)
		{
			for (gint q = track->items->len - 1; q >= 0; q--)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				gtk_widget_destroy(item->button);
				if (item->tree_path)
					g_free(item->tree_path);
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
		track = &g_array_index(priv->tracks, Track, i);
		if (! track->is_default)
			g_array_remove_index(priv->tracks, i);
	}
	next_order = 2;
	gtk_widget_queue_draw(GTK_WIDGET(timeline));
}

GArray *img_timeline_get_active_media(GtkWidget *timeline, gdouble current_time)
{
    ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
    Track *track;
    GArray* active_elements;
    media_timeline *item;
    double item_end;

    active_elements = g_array_new(FALSE, FALSE, sizeof(media_timeline *));
    for (gint i = 0; i < priv->tracks->len; i++)
    {
        track = &g_array_index(priv->tracks, Track, i);
        for (gint j = 0; j < track->items->len; j++)
        {
            item = g_array_index(track->items, media_timeline *, j);
            item_end = item->start_time + item->duration;
            if (current_time >= item->start_time && current_time < item_end)
                g_array_append_val(active_elements, item);
        }
    }
    return active_elements;
}

void img_timeline_center_button_image(GtkWidget *button)
{
	GtkWidget *image;
	GtkWidget *layout;
	GtkAllocation button_allocation, image_allocation;
    gint x, y;
	GList *children;

	layout = gtk_bin_get_child(GTK_BIN(button));
	children = gtk_container_get_children(GTK_CONTAINER(layout));
 
	image = GTK_WIDGET(children->data);
    g_list_free(children);
    
    gtk_widget_get_allocation(GTK_WIDGET(layout), &button_allocation);
    gtk_widget_get_allocation(image, &image_allocation);

    x = (button_allocation.width - image_allocation.width) / 2;
    y = (button_allocation.height - image_allocation.height) / 2;

    gtk_layout_move(GTK_LAYOUT(layout), image, x, y);
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
		track = &g_array_index(priv->tracks, Track, i);
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
	gtk_label_set_text(GTK_LABEL(img->total_time), total_time_string);
	g_free(total_time_string);

	return total_time;
}
