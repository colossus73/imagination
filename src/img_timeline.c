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

typedef struct _media media;
struct _media
{
	gdouble x;
	gdouble drag_x;
	gdouble old_x;
	gdouble initial_width;
	gdouble width;
	enum {	RESIZE_NONE, RESIZE_LEFT, RESIZE_RIGHT } resizing;
	gboolean button_pressed;
	GtkWidget *button;
};

static guint timeline_signals[N_SIGNALS] = { 0 };

//Private functions.
static void img_timeline_class_init(ImgTimelineClass *);
static void img_timeline_set_property(GObject *, guint , const GValue *, GParamSpec *);
static void img_timeline_get_property(GObject *, guint , GValue *, GParamSpec *);
static void img_timeline_init(ImgTimeline *);
static void img_timeline_draw_time_ticks(GtkWidget *widget, cairo_t *, gint );
static void img_timeline_finalize(GObject *);
static gboolean img_timeline_draw(GtkWidget *, cairo_t *);
static gint img_timeline_calculate_total_tracks_height(GtkWidget *);

static gint img_sort_image_track_first(gconstpointer a, gconstpointer b);

// Media button events
static void img_timeline_media_drag_data_get(GtkWidget *, GdkDragContext *, GtkSelectionData *, guint , guint, gpointer );
static gboolean img_timeline_media_motion_notify(GtkWidget *, GdkEventMotion *, ImgTimeline *);
static gboolean img_timeline_media_leave_event (GtkWidget *, GdkEventCrossing *, gpointer );

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
    priv->zoom = 1;
    priv->zoom_scale = 1.0;
    priv->seconds = priv->minutes = priv->hours = 0;
    priv->total_time = 60;
    priv->time_marker_pos = 0;
	priv->tracks = g_array_new(FALSE, FALSE, sizeof(Track));

	// Add default image and audio tracks
	Track image_track;
	image_track.type = 0;
	image_track.items = NULL;
	image_track.background_color = "#d6d1cd";
    g_array_append_val(priv->tracks, image_track);
    
    Track audio_track;
	audio_track.type = 1;
	audio_track.items = NULL;
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

//Needed for g_object_set().
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
	cairo_save(cr);
		cairo_translate (cr, 0 , 12);
		img_timeline_draw_time_ticks(da, cr, width);
		cairo_translate (cr, 0 , 20);
		
		int y = 0;
		for (guint i = 0; i < priv->tracks->len; i++)
		{
			track = &g_array_index(priv->tracks, Track, i);
			gdk_rgba_parse(&rgba, track->background_color);
			cairo_set_source_rgb(cr, rgba.red, rgba.green, rgba.blue);
			cairo_rectangle(cr, 0, y , width, TRACK_HEIGHT);
			y += TRACK_HEIGHT + TRACK_GAP;
			cairo_fill(cr);
		}
	  cairo_restore(cr);

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
        for (guint i = 0; i < priv->tracks->len; i++)
        {
            Track *track = &g_array_index(priv->tracks, Track, i);
            if (track->items)
                g_array_free(track->items, TRUE);
        }
        g_array_free(priv->tracks, TRUE);
    }

  G_OBJECT_CLASS(img_timeline_parent_class)->finalize(object);
}

void img_timeline_draw_time_ticks(GtkWidget *da, cairo_t *cr, gint width)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);
	cairo_text_extents_t extents;
	gchar *time;
	gint i, factor;
	gdouble distanceBetweenTicks, cairo_factor;

	distanceBetweenTicks = 48.5 * priv->zoom_scale;
	factor = 2;

	if (distanceBetweenTicks <= 12)
		distanceBetweenTicks = 12;    
	  
	gtk_widget_set_size_request(da, (int)(priv->total_time * distanceBetweenTicks), img_timeline_calculate_total_tracks_height(da));
	cairo_set_source_rgb(cr, 0,0,0);

	// Calculate the number of time ticks to draw based on zoom
	int numTicks = (int)(priv->total_time / priv->zoom_scale);
	for (i = 0; i < numTicks; i++)
	{
		if (i % 2 == 0)
			cairo_factor = 0;
		else
			cairo_factor = 0.5;
		
		//Draw the line markers
		cairo_move_to( cr, i * distanceBetweenTicks + cairo_factor, 4);
		cairo_line_to( cr, i * distanceBetweenTicks + cairo_factor, 14);

		if (priv->zoom_scale <= 0.25) // Adjust factor based on zoom scale
			factor = 4;
		else
			factor = 1;

		if (i % factor == 0)
		{  
		  time = g_strdup_printf("%02d:%02d:%02d", priv->hours, priv->minutes, priv->seconds);
		  cairo_select_font_face(cr, "Dejavu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		  cairo_set_font_size(cr, 10);
		  cairo_text_extents(cr, time, &extents);
		  cairo_move_to(cr, (-extents.width/2) + i*distanceBetweenTicks, 0);  
		  cairo_show_text(cr, time);
		  g_free(time);
		}
		
		if (priv->seconds >= 59)
		{
			priv->seconds = -1;
			priv->minutes++;
		}   
		if (priv->minutes >= 59)
		{
			priv->minutes = 0;
			priv->hours++;
		}
		priv->seconds += priv->zoom;
		if (priv->seconds > 59)
		  priv->seconds = 59;

		cairo_stroke(cr);
	}
	priv->seconds = priv->minutes = priv->hours = 0;
}

void img_timeline_adjust_zoom(GtkWidget *da, gdouble zoom_delta)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);

	// Adjust zoom scale based on delta 
	priv->zoom_scale += zoom_delta;

	// Clamp zoom scale within reasonable bounds
	if (priv->zoom_scale < 1) {
		priv->zoom_scale = 1;
	} else if (priv->zoom_scale > 4.0) {
		priv->zoom_scale = 4.0;
	}
	g_print("%2.5f\n",priv->zoom_scale );

	// Update the width of the toggle buttons (media items)
	GList *children = gtk_container_get_children(GTK_CONTAINER(da));
	for (GList *iter = children; iter != NULL; iter = iter->next) {
		gtk_widget_set_size_request(GTK_WIDGET(iter->data), (int)(95 * priv->zoom_scale), -1);
	}
	g_list_free(children);

 	gtk_widget_queue_draw(GTK_WIDGET(da));
 
 }

void img_timeline_add_media(GtkWidget *da, gchar *filename, gint x)
{
    GdkPixbuf *pix;
    GtkWidget *img, *box, *image_container;
    struct _media *item;
    gint pos;

    ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);

	pix = gdk_pixbuf_new_from_file_at_scale(filename, -1, 45, TRUE, NULL);
    if (pix == NULL)
		return;

	img = gtk_image_new_from_pixbuf(pix);
    item = g_new0(media, 1);

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
    
    // Add the image
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

    if (pix != NULL)
    {
        g_signal_connect(G_OBJECT(item->button), "drag-data-get", G_CALLBACK(img_timeline_media_drag_data_get), NULL);
        g_signal_connect(G_OBJECT(item->button), "motion-notify-event", G_CALLBACK(img_timeline_media_motion_notify), da);
        g_signal_connect(G_OBJECT(item->button), "leave-notify-event", G_CALLBACK(img_timeline_media_leave_event), da);
        g_signal_connect(G_OBJECT(item->button), "button-press-event", G_CALLBACK(img_timeline_media_button_press_event), da);
        g_signal_connect(G_OBJECT(item->button), "button-release-event", G_CALLBACK(img_timeline_media_button_release_event), da);
    }

    gtk_container_add(GTK_CONTAINER(da), item->button);
    gtk_widget_show_all(item->button);

    if (x > 0)
        pos = x - 47.5;
    else
        pos = priv->last_media_posX;
    
    gtk_layout_move(GTK_LAYOUT(da), item->button, pos, 35);
    item->old_x = pos;

    priv->last_media_posX += 95;
    gtk_widget_set_size_request(item->button, 95, (pix ? -1 : 35));

    if (pix)
        g_object_unref(pix);
}

void img_timeline_add_track(GtkWidget *timeline, gint type, gchar *hexcode)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)timeline);
	Track new_track;
	gint total_height;
	
	new_track.type = type;
	new_track.background_color = hexcode;
	new_track.items = g_array_new(FALSE, FALSE, sizeof(media_item));
	g_array_append_val(priv->tracks, new_track);
	
	// Sort by track type so we can easily draw them in the draw event
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

gboolean img_timeline_mouse_button_press (GtkWidget *timeline, GdkEvent *event, ImgTimeline *da)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private(da);
	if (event->button.y < 12.0 || event->button.y > 32.0)
		return FALSE;

	priv->button_pressed_on_needle = TRUE;
	img_timeline_adjust_marker_posx(timeline, event->button.x - 5);
	
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
	  
	  // Check if we're on the resize areas (left or right 10 pixels of the button)
	  if (event->x <= 10) {
		item->resizing = RESIZE_LEFT;
	  } else if (event->x >= item->initial_width - 10) {
		item->resizing = RESIZE_RIGHT;
	  } else {
		item->resizing = RESIZE_NONE;
	  }
	  
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

static gboolean img_timeline_media_motion_notify(GtkWidget *button, GdkEventMotion *event, ImgTimeline *timeline)
{
	gint x, new_width, button_width;
	GdkCursor *cursor;
	GdkWindow *window;
	struct _media *item;
	  
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
			gtk_layout_move(GTK_LAYOUT(timeline), button, x, 35);
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
		   gtk_layout_move(GTK_LAYOUT(timeline), button, x, 35);
			
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

static gboolean img_timeline_media_leave_event(GtkWidget *button, GdkEventCrossing *event, gpointer timeline)
{
	 GdkCursor *cursor;
	 GdkWindow *window;

	window = gtk_widget_get_window(button);
	cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_ARROW);
	gdk_window_set_cursor(window, cursor);
	return FALSE;
}

gboolean img_timeline_scroll_event(GtkWidget *timeline, GdkEventScroll *event, GtkWidget *scrolledwindow)
{
	gdouble deltax, deltay, zoom;

	GdkModifierType accel_mask = gtk_accelerator_get_default_mod_mask();
	GtkAdjustment *scroll_x    	= gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(scrolledwindow));
	gdouble step 		= gtk_adjustment_get_step_increment(scroll_x);
	gdouble position = gtk_adjustment_get_value(scroll_x);
    gdk_event_get_scroll_deltas((GdkEvent *)event, &deltax, &deltay);

    if (deltay > 0)
		gtk_adjustment_set_value(scroll_x,  position + step);
	else
		gtk_adjustment_set_value(scroll_x, position - step);

	if ( (event->state & accel_mask) == GDK_CONTROL_MASK)
	{
		gdk_event_get_scroll_deltas((GdkEvent *)event, &deltax, &deltay);
		if (deltay > 0)
			zoom = -0.25;
		else if (deltay < 0)
			zoom = 0.25;

		img_timeline_adjust_zoom(timeline, zoom);
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
					img_timeline_add_media(timeline, filename, x);
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
			img_timeline_add_media(timeline, media_structs[i]->full_path, x);
	}
	
	gtk_drag_finish (context, TRUE, FALSE, time);
}

static gint img_timeline_calculate_total_tracks_height(GtkWidget *da)
{
	ImgTimelinePrivate *priv = img_timeline_get_instance_private((ImgTimeline*)da);
	gint total = 0; //Start from the height of the first two default tracks
	
	for (guint i = 0; i < priv->tracks->len; i++)
		total += (TRACK_HEIGHT + TRACK_GAP);

    return total + 20;
}

static gint img_sort_image_track_first(gconstpointer a, gconstpointer b)
{
    const Track *track_a = a;
    const Track *track_b = b;

	// Sort image tracks (0) before audio tracks (1)
	return track_a->type - track_b->type;
}
