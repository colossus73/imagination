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

typedef struct _ImgTimelinePrivate
{
	gint last_media_posX;
	gint zoom;
	gint seconds;
	gint minutes;
	gint hours;
	gint total_time;
	gint time_marker_pos;
	gdouble zoom_scale;
	gboolean button_pressed_on_needle;
	gdouble pixels_per_second;
	GArray *tracks;
} ImgTimelinePrivate;

typedef struct _TimelineHandle TimelineHandle;
typedef struct _TimelineHandleClass TimelineHandleClass;

struct _TimelineHandle
{
    GtkDrawingArea parent;
    gboolean is_left_handle;
};

struct _TimelineHandleClass
{
    GtkDrawingAreaClass parent_class;
};

G_DEFINE_TYPE(TimelineHandle, timeline_handle, GTK_TYPE_DRAWING_AREA)

static guint timeline_signals[N_SIGNALS] = { 0 };
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
	g_object_class_install_property(object_class, TIME_MARKER_POS, 	g_param_spec_int		("time_marker_pos", "Time Marker Position", "Position of the time marker", 0, G_MAXINT, 0, G_PARAM_READWRITE));

	// Install signals
    timeline_signals[SIGNAL_TIME_CHANGED] = g_signal_new("time-changed",
        G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT);
}

static void img_timeline_init(ImgTimeline *timeline)
{
    ImgTimelinePrivate *priv = img_timeline_get_instance_private(timeline);

    priv->last_media_posX = 0;
    priv->zoom_scale = 6.12;
    priv->pixels_per_second = 61.2;
    priv->total_time = 0;
    priv->time_marker_pos = 0;
	priv->tracks = g_array_new(FALSE, TRUE, sizeof(Track));

	// Add default image and audio tracks
	Track image_track;
	image_track.type = 0;
	image_track.is_selected = FALSE;
	image_track.order = next_order++;
	image_track.items = g_array_new(FALSE, TRUE, sizeof(media_timeline));
	image_track.background_color = "#CCCCFF";
    g_array_append_val(priv->tracks, image_track);
    
    Track audio_track;
	audio_track.type = 1;
	audio_track.is_selected = FALSE;
	audio_track.order = next_order++;
	audio_track.items = g_array_new(FALSE, TRUE, sizeof(media_timeline));
	audio_track.background_color = "#d6d1cd";
    g_array_append_val(priv->tracks, audio_track);

    // Set up CSS styling
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        ".timeline-button { border-width: 0; outline-width: 0; background: #404040; padding: 0; margin: 0; }"
        ".timeline-button:focus { outline-width: 0; }"
        ".timeline-button .timeline-image-container { border-top: 2px solid transparent; border-bottom: 2px solid transparent; }"
        ".timeline-button:checked .timeline-image-container { border-top-color: #FFD700; border-bottom-color: #FFD700; }"
        ".timeline-image-container { padding: 0; margin: 0; }", -1, NULL);
    //".timeline-button:hover { opacity: 0.8; }"
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);
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
			img_timeline_set_time_marker(da, g_value_get_int(value));
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

void img_timeline_set_time_marker(ImgTimeline *timeline, gint posx)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private(timeline);

	priv->time_marker_pos = posx;
	gtk_widget_queue_draw(GTK_WIDGET(timeline));
    
	g_signal_emit(timeline, timeline_signals[SIGNAL_TIME_CHANGED], 0, posx);
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

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

GtkWidget* img_timeline_new()
{
	return GTK_WIDGET(g_object_new(img_timeline_get_type(), NULL));
}

static void timeline_handle_init(TimelineHandle *handle)
{
	gtk_widget_set_size_request(GTK_WIDGET(handle), 10, -1);
}

static gboolean timeline_handle_draw(GtkWidget *widget, cairo_t *cr)
{
    TimelineHandle *handle = TIMELINE_HANDLE(widget);
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    double radius = 8.0;  // Radius for rounded corners
    double degrees = G_PI / 180.0;

    // Draw handle background with rounded corners on one side
    cairo_new_sub_path(cr);
    
    if (handle->is_left_handle)
    {
        cairo_arc(cr, radius, radius, radius, 180 * degrees, 270 * degrees);
        cairo_line_to(cr, allocation.width, 0);
        cairo_line_to(cr, allocation.width, allocation.height);
        cairo_line_to(cr, radius, allocation.height);
        cairo_arc(cr, radius, allocation.height - radius, radius, 90 * degrees, 180 * degrees);
    }
    else
    {
        cairo_move_to(cr, 0, 0);
        cairo_line_to(cr, allocation.width - radius, 0);
        cairo_arc(cr, allocation.width - radius, radius, radius, -90 * degrees, 0 * degrees);
        cairo_arc(cr, allocation.width - radius, allocation.height - radius, radius, 0 * degrees, 90 * degrees);
        cairo_line_to(cr, 0, allocation.height);
    }
    
    cairo_close_path(cr);

    cairo_set_source_rgb(cr, 1, 1, 0);
    cairo_fill_preserve(cr);
    
    // Draw a subtle border
    cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 1);  // Slightly darker gray for border
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);

    // Draw circles
    cairo_set_source_rgb(cr, 0, 0, 0);  // Black circles
    for (int i = 0; i < 3; i++) {
        cairo_arc(cr, allocation.width / 2, 5 + allocation.height * (i + 1) / 5, 2, 0, 2 * G_PI);
        cairo_fill(cr);
    }

    return TRUE;
}

static void timeline_handle_class_init(TimelineHandleClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    widget_class->draw = timeline_handle_draw;
}

static GtkWidget *timeline_handle_new(gboolean is_left_handle)
{
    TimelineHandle *handle = g_object_new(TIMELINE_TYPE_HANDLE, NULL);
    handle->is_left_handle = is_left_handle;
    gtk_widget_set_size_request(GTK_WIDGET(handle), 10, -1);
    return GTK_WIDGET(handle);
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

void img_timeline_add_media(GtkWidget *da, media_struct *entry, gint x, gint y)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);

    GdkPixbuf *pix = NULL;
    GtkWidget *img, *box, *image_container;
    media_timeline *item;
    Track *track;
    gint pos, track_nr, new_y, width;

	//Calculate the track to move the media to later according to the dropped y coord
	track_nr = img_timeline_get_track_at_position(da, y, &new_y);
	track = &g_array_index(priv->tracks, Track, track_nr);

	if (track->type != entry->media_type)
		return;

    item = g_new0(media_timeline, 1);
	item->id = entry->id;
	if (entry->media_type == 1)
		item->duration = img_convert_time_string_to_seconds(entry->audio_duration);
	else
		item->duration = 2;

	g_array_append_val(track->items, item);

    // Create a box to hold the handles and the image
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    item->button = gtk_toggle_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(item->button), "timeline-button");
    gtk_container_add(GTK_CONTAINER(item->button), box);

    // Add left handle
    GtkWidget *left_handle = timeline_handle_new(TRUE);
    gtk_box_pack_start(GTK_BOX(box), left_handle, FALSE, FALSE, 0);

	// Create image container
	image_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_style_context_add_class(gtk_widget_get_style_context(image_container), "timeline-image-container");
	gtk_box_pack_start(GTK_BOX(box), image_container, TRUE, TRUE, 0);
    
	if (entry->media_type == 0)
	{
		pix = gdk_pixbuf_new_from_file_at_scale(entry->full_path, -1, 45, TRUE, NULL);
		img = gtk_image_new_from_pixbuf(pix);
	}
	else if(entry->media_type == 1)
	{
		//Get the waveform into the image and resize the button according to the duration of the audio
		img = gtk_image_new_from_icon_name("audio-x-generic", GTK_ICON_SIZE_DIALOG);
	}
	// Add the image to the button
	gtk_widget_set_valign(img, GTK_ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(image_container), img);
	
    // Add right handle
    GtkWidget *right_handle = timeline_handle_new(FALSE);
    gtk_box_pack_end(GTK_BOX(box), right_handle, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(item->button), "mem_address", item);
    gtk_widget_add_events(item->button, GDK_POINTER_MOTION_MASK
                                      | GDK_LEAVE_NOTIFY_MASK
                                      | GDK_BUTTON1_MASK 
                                      | GDK_BUTTON1_MOTION_MASK
                                      | GDK_BUTTON_PRESS_MASK
                                      | GDK_BUTTON_RELEASE_MASK);
                                      
	g_signal_connect(item->button, "drag-data-get", 				G_CALLBACK(img_timeline_media_drag_data_get), NULL);
	g_signal_connect(item->button, "motion-notify-event",		G_CALLBACK(img_timeline_media_motion_notify), da);
	g_signal_connect(item->button, "leave-notify-event", 		G_CALLBACK(img_timeline_media_leave_event), da);
	g_signal_connect(item->button, "button-press-event",		G_CALLBACK(img_timeline_media_button_press_event), da);
	g_signal_connect(item->button, "button-release-event", 	G_CALLBACK(img_timeline_media_button_release_event), da);
	
	gtk_container_add(GTK_CONTAINER(da), item->button);
	gtk_widget_show_all(item->button);
	
	if (x > 0)
		pos = x - 47.5;
	else
        pos = priv->last_media_posX;
    
	//Position the button inside the dropped track and set its size and duration if it's an audio media 
	if (entry->media_type == 1)
		width = img_convert_time_string_to_seconds(entry->audio_duration);
	else
		width = item->duration;

	width *= BASE_SCALE * priv->zoom;
	gtk_widget_set_size_request(item->button, width * priv->zoom_scale, -1);


	gtk_layout_move(GTK_LAYOUT(da), item->button, pos, new_y);
	
	item->old_x = pos;
	item->y = new_y;
    priv->last_media_posX += 95;

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
	new_track.order = next_order++;
	new_track.items = g_array_new(FALSE, TRUE, sizeof(media_timeline));
	new_track.background_color = hexcode;
	g_array_append_val(priv->tracks, new_track);
	
	// Sort by track type so they can be easily drawn in the draw event
	g_array_sort(priv->tracks,  img_sort_image_track_first);
}

void img_timeline_draw_time_marker(GtkWidget *widget, cairo_t *cr, gint posx, gint length)
{
	cairo_save(cr);
	cairo_set_source_rgb(cr, 1,0,0);
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
    cairo_set_source_rgb(cr, 1, 0, 0);  // Red color
    cairo_set_line_width(cr, 2);
    cairo_fill(cr);

	//Draw the needle
	cairo_move_to(cr, 5, 15);
	cairo_line_to(cr, 5, img_timeline_calculate_total_tracks_height(widget));
	cairo_stroke(cr);
	cairo_restore(cr);
}

void img_timeline_media_drag_data_get(GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	//This callback is for the timeline itself when data is being dragged OUT of it
	gtk_selection_data_set (data, gdk_atom_intern_static_string ("IMG_TIMELINE_MEDIA"), 32, (const guchar *)&widget, sizeof (gpointer));
	g_print("Drag data get\n");
}

void img_timeline_adjust_marker_posx(GtkWidget *da, gint posx)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);

	priv->time_marker_pos = posx;
	gtk_widget_queue_draw((GtkWidget*)da);
}

gboolean img_timeline_mouse_button_press (GtkWidget *timeline, GdkEventButton *event, ImgTimeline *da)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private(da);
	if (event->y < 12.0 || event->y > 32.0)
		return FALSE;

	if (event->button == 1)
	{
		priv->button_pressed_on_needle = TRUE;
		img_timeline_adjust_marker_posx(timeline, event->x - 5);
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

gboolean img_timeline_motion_notify(GtkWidget *timeline, GdkEventMotion *event, ImgTimeline *da)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private(da);

	if (priv->button_pressed_on_needle)
			img_timeline_adjust_marker_posx(timeline, event->x - 5);

   return TRUE;
}

static void img_timeline_update_button_image(GtkWidget *button)
{
    GtkWidget *box, *image_container, *image;
    GdkPixbuf *original_pixbuf, *tiled_pixbuf;
    gint button_width, button_height, image_width, image_height;

    // Get the box that contains the image container
    box = gtk_bin_get_child(GTK_BIN(button));
    if (!GTK_IS_BOX(box))
        return;

    // Find the image container within the box
    GList *children = gtk_container_get_children(GTK_CONTAINER(box));
    image_container = NULL;
    for (GList *l = children; l != NULL; l = l->next) {
        if (GTK_IS_BOX(l->data) && 
            gtk_style_context_has_class(gtk_widget_get_style_context(l->data), "timeline-image-container")) {
            image_container = GTK_WIDGET(l->data);
            break;
        }
    }
    g_list_free(children);

    if (!image_container)
        return;

    // Find the image widget within the image container
    children = gtk_container_get_children(GTK_CONTAINER(image_container));
    image = NULL;
    for (GList *l = children; l != NULL; l = l->next) {
        if (GTK_IS_IMAGE(l->data)) {
            image = GTK_WIDGET(l->data);
            break;
        }
    }
    g_list_free(children);

    original_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));

    gtk_widget_get_size_request(button, &button_width, &button_height);
    image_width = gdk_pixbuf_get_width(original_pixbuf);
    image_height = gdk_pixbuf_get_height(original_pixbuf);

    // Create a new pixbuf to hold the tiled image
    // Subtract width of handles (assumed to be 20px each)
    tiled_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 
                                  gdk_pixbuf_get_has_alpha(original_pixbuf), 
                                  8, 
                                  button_width - 20, 
                                  button_height);

    // Tile the original image across the button
    for (int y = 0; y < button_height; y += image_height)
    {
        for (int x = 0; x < button_width - 20; x += image_width)
        {
            gdk_pixbuf_copy_area(original_pixbuf, 
                                 0, 0, 
                                 MIN(image_width, button_width - 20 - x), 
                                 MAX(image_height, button_height - y), 
                                 tiled_pixbuf, 
                                 x, y);
        }
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(image), tiled_pixbuf);
    g_object_unref(tiled_pixbuf);
}

gboolean img_timeline_media_button_press_event(GtkWidget *button, GdkEventButton *event, ImgTimeline *timeline)
{
	//ImgTimelinePrivate *priv = img_timeline_get_instance_private(timeline);
	struct _media *item;
	  
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

gboolean img_timeline_media_button_release_event(GtkWidget *button, GdkEventButton *event, ImgTimeline *timeline)
{
	 struct _media *item;
	 item = g_object_get_data(G_OBJECT(button), "mem_address");
	 item->button_pressed = FALSE;
	 item->resizing = RESIZE_NONE;

	return FALSE;
}

gboolean img_timeline_media_motion_notify(GtkWidget *button, GdkEventMotion *event, ImgTimeline *timeline)
{
	gint x, new_width, button_width;
	GdkCursor *cursor;
	GdkWindow *window;
	media_timeline *item;
	  
	//ImgTimelinePrivate *priv = img_timeline_get_instance_private(timeline);
	item = g_object_get_data(G_OBJECT(button), "mem_address");
	  
	window = gtk_widget_get_window(button);
	button_width = gtk_widget_get_allocated_width(button);

	 if (item->button_pressed)
	{
		switch (item->resizing)
		{
			case RESIZE_LEFT:
				new_width = item->initial_width + (item->drag_x - event->x);
				new_width = MAX(new_width, 50);
				x = item->old_x + (item->initial_width - new_width);
				gtk_widget_set_size_request(button, new_width, 45);
				gtk_layout_move(GTK_LAYOUT(timeline), button, x, item->y);
				item->old_x = x;
				item->initial_width = new_width;
				//img_timeline_update_button_image(button);
			break;

			case RESIZE_RIGHT:
				new_width = MAX(event->x, 50);  // Ensure a minimum width
				gtk_widget_set_size_request(button, new_width, 45);
				//img_timeline_update_button_image(button);
			break;

			case RESIZE_NONE:
				x = item->old_x + (event->x - item->drag_x);
				gint timeline_width = gtk_widget_get_allocated_width(GTK_WIDGET(timeline));
				x = CLAMP(x, 0, timeline_width - button_width);
				gtk_layout_move(GTK_LAYOUT(timeline), button, x, item->y);
				
				// Update old_x for the next movement
				item->old_x = x;
			break;
		}
	}

	// Update cursor based on mouse position
	if (event->x <= 10 || (event->x >= button_width - 10 && event->x <= button_width))
		cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_SB_H_DOUBLE_ARROW);
	else
		cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_ARROW);
	
	gdk_window_set_cursor(window, cursor);
	g_object_unref(cursor);

	return FALSE;
}

gboolean img_timeline_media_leave_event(GtkWidget *button, GdkEventCrossing *event, gpointer timeline)
{
	 GdkCursor *cursor;
	 GdkWindow *window;

	window = gtk_widget_get_window(button);
	cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_ARROW);
	gdk_window_set_cursor(window, cursor);
	return FALSE;
}

gboolean img_timeline_scroll_event(GtkWidget *timeline, GdkEventScroll *event, img_window_struct *img) 
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track *track;
	media_timeline *item;
	gdouble dx, dy;
	gint width;
	
	GdkModifierType accel_mask = gtk_accelerator_get_default_mod_mask();

	if ( (event->state & accel_mask) == GDK_CONTROL_MASK)
	{
		gdk_event_get_scroll_deltas((GdkEvent *)event, &dx, &dy);
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
					item = g_array_index(track->items, media_timeline *, q);
					if (item)
					{
						width = (gint)(item->duration * priv->pixels_per_second);
						gtk_widget_set_size_request(item->button, width, -1);
					}
				}
			}
		}
		gtk_widget_queue_draw(timeline);
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
					img_timeline_add_media(timeline, img->current_media, x, y);
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
			img_timeline_add_media(timeline, media_structs[i], x, y);
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
