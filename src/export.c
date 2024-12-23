/*
** Copyright (c) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
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

#include "export.h"
#include "empty_slide.h"
#include "support.h"
#include "callbacks.h"

static void img_start_export( img_window_struct *);
static gint img_initialize_av_parameters(img_window_struct *, gint , gint , enum AVCodecID);
static gboolean img_export_still(img_window_struct *);
static void img_export_pause_unpause( GtkToggleButton  *, img_window_struct *);
static gboolean img_convert_cairo_frame_to_avframe(img_window_struct *, cairo_surface_t *);
static gboolean img_export_encode_av_frame(AVFrame *frame, AVFormatContext *fmt, AVCodecContext *ctx, AVPacket *pkt, AVStream *stream);
static gboolean img_export_project(img_window_struct *);
static cairo_surface_t *img_get_next_composed_media(img_window_struct *);

static void img_start_export( img_window_struct *img)
{
	GtkWidget 	*dialog, *image, *vbox, *label, *hbox,*progress;
	gchar 			*string;

	img->export_is_running = 1;

	dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL);
	img->export_dialog = dialog;
	gtk_window_set_title( GTK_WINDOW( img->export_dialog ), _("Exporting the sequence") );
	g_signal_connect (G_OBJECT(img->export_dialog), "delete_event", G_CALLBACK (on_close_export_dialog), img);
	gtk_container_set_border_width( GTK_CONTAINER(dialog), 10);
	gtk_window_set_default_size( GTK_WINDOW(dialog), 400, -1);
	gtk_window_set_type_hint( GTK_WINDOW(dialog ), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_modal( GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for( GTK_WINDOW(dialog),  GTK_WINDOW( img->imagination_window));

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_add( GTK_CONTAINER( dialog ), vbox);

	img->export_label = gtk_label_new(NULL);
	string = g_strdup_printf( _("Media %d export progress:"), 1 );
	gtk_label_set_label( GTK_LABEL(img->export_label), string);
	g_free(string);
	gtk_label_set_xalign(GTK_LABEL(img->export_label), 0);
	gtk_label_set_yalign(GTK_LABEL(img->export_label), 0.5);
	gtk_box_pack_start( GTK_BOX(vbox), img->export_label , FALSE, FALSE, 0);

	progress = gtk_progress_bar_new();
	img->export_pbar1 = progress;
	string = g_strdup_printf( "%.2f", .0 );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( progress ), string);
	gtk_box_pack_start( GTK_BOX( vbox ), progress, FALSE, FALSE, 0);

	label = gtk_label_new( _("Overall progress:"));
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_box_pack_start( GTK_BOX( vbox ), label, FALSE, FALSE, 0 );

	progress = gtk_progress_bar_new();
	img->export_pbar2 = progress;
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( progress ), string );
	gtk_box_pack_start( GTK_BOX( vbox ), progress, FALSE, FALSE, 0 );
	g_free( string );
	
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 0 );
	label = gtk_label_new( _("Elapsed time:") );
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 0 );

	img->elapsed_time_label = gtk_label_new(NULL);
	gtk_label_set_xalign(GTK_LABEL(img->elapsed_time_label), 0);
	gtk_label_set_yalign(GTK_LABEL(img->elapsed_time_label), 0.5);
	gtk_box_pack_start( GTK_BOX( hbox ), img->elapsed_time_label, FALSE, FALSE, 0 );

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
	gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 0 );

	image = gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON );
	img->export_cancel_button = gtk_button_new_with_label(_("Cancel"));
	gtk_button_set_always_show_image(GTK_BUTTON (img->export_cancel_button), TRUE);
	gtk_button_set_image (GTK_BUTTON(img->export_cancel_button), image);
	
	g_signal_connect_swapped(G_OBJECT(img->export_cancel_button), "clicked",  G_CALLBACK(img_close_export_dialog), img);
	gtk_box_pack_end( GTK_BOX(hbox), img->export_cancel_button, FALSE, FALSE, 0 );

	image = gtk_image_new_from_icon_name( "media-playback-pause", GTK_ICON_SIZE_BUTTON);
	img->export_pause_button = gtk_toggle_button_new_with_label(_("Pause"));
	gtk_button_set_always_show_image(GTK_BUTTON (img->export_pause_button), TRUE);
	gtk_button_set_image (GTK_BUTTON(img->export_pause_button), image);
	g_signal_connect( G_OBJECT(img->export_pause_button), "toggled",  G_CALLBACK( img_export_pause_unpause ), img );
	gtk_box_pack_end( GTK_BOX(hbox), img->export_pause_button, FALSE, FALSE, 0);
	gtk_widget_show_all(dialog);

	// Create the surface to be passed to the encoder
	img->exported_image = cairo_image_surface_create(CAIRO_FORMAT_RGB24, img->video_size[0], img->video_size[1]);
	img->total_nr_frames = img->total_time * img->export_fps;

	img->elapsed_timer = g_timer_new();
	img->source_id = g_idle_add((GSourceFunc) img_export_project, img);
}

static gboolean img_export_project(img_window_struct *img)
{
	media_timeline *media = NULL, *current_media = NULL;
	GArray *current_media_array = NULL;
	GArray *next_media_array = NULL;
    cairo_surface_t *surface = NULL, *composite_surface = NULL;
    cairo_surface_t *next_composite = NULL;
    cairo_t *cr;
    GdkPixbuf *pix = NULL;
    gint max_width = 0, max_height = 0;
    gdouble next_time, current_end_time, transition_duration;
    gboolean is_transitioning = FALSE;
    
    // Get export dimensions
    gint export_width = cairo_image_surface_get_width(img->exported_image);
    gint export_height = cairo_image_surface_get_height(img->exported_image);

	current_media_array = img_timeline_get_active_media_at_given_time(img->timeline, img->current_timeline_index);
	if (current_media_array->len == 0)
	{
		img_post_export(img);
		return G_SOURCE_REMOVE;
	}
	current_media = g_array_index(current_media_array, media_timeline *, 0);

	if (current_media->transition_id > -1)
		is_transitioning = TRUE;
	else
		is_transitioning = FALSE;
  
	// Find max dimensions for all current media
	for (gint i = 0; i < current_media_array->len; i++)
	{
		media = g_array_index(current_media_array, media_timeline *, i);
		pix = gdk_pixbuf_new_from_file_at_scale(img_get_media_filename(img, media->id), 1, 1, TRUE, NULL);
		if (pix)
		{
			max_width = MAX(max_width, gdk_pixbuf_get_width(pix));
			max_height = MAX(max_height, gdk_pixbuf_get_height(pix));
			g_object_unref(pix);
		}
	}

	if (is_transitioning)
	{
		current_media = g_array_index(current_media_array, media_timeline *, 0);
		current_end_time = current_media->start_time + current_media->duration;	
		transition_duration = 1.5;
		img->transition_progress = 1.0 - ((current_end_time - img->current_timeline_index) / transition_duration);
		
		next_time = current_end_time + 0.01;
		next_media_array = img_timeline_get_active_picture_media(img->timeline, next_time);
		
		if (next_media_array)
		{
			for (gint i = 0; i < next_media_array->len; i++)
			{
				media = g_array_index(next_media_array, media_timeline *, i);
				pix = gdk_pixbuf_new_from_file_at_scale(img_get_media_filename(img, media->id), 1, 1, TRUE, NULL);
				if (pix)
				{
					max_width = MAX(max_width, gdk_pixbuf_get_width(pix));
					max_height = MAX(max_height, gdk_pixbuf_get_height(pix));
					g_object_unref(pix);
				}
			}
		}
	}

	// Create main composite surface first - using export dimensions
	composite_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, export_width, export_height);
		
	// Compose current media items
	cr = cairo_create(composite_surface);

	// Fill the background colour with the user chosen color
	cairo_set_source_rgb(cr, img->background_color[0], img->background_color[1], img->background_color[2]);
	cairo_paint(cr);

	for (gint i = current_media_array->len - 1; i >= 0; i--)
	{
		media = g_array_index(current_media_array, media_timeline *, i);
		pix = gdk_pixbuf_new_from_file(img_get_media_filename(img, media->id), NULL);
		if (pix)
		{
			surface = gdk_cairo_surface_create_from_pixbuf(pix, 0, NULL);
			if (surface)
			{
				// Center the image on the export surface
				gint x = (export_width - cairo_image_surface_get_width(surface)) / 2;
				gint y = (export_height - cairo_image_surface_get_height(surface)) / 2;
				cairo_set_source_surface(cr, surface, x, y);
				cairo_paint(cr);
				cairo_surface_destroy(surface);
			}
			g_object_unref(pix);
		}
	}
	cairo_destroy(cr);

	if (is_transitioning && next_media_array && next_media_array->len > 0)
	{
		next_composite = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, export_width, export_height);
		cr = cairo_create(next_composite);

		// Fill the background colour with the user chosen color
		cairo_set_source_rgb(cr, img->background_color[0], img->background_color[1], img->background_color[2]);
		cairo_paint(cr);

		for (gint i = next_media_array->len - 1; i >= 0; i--)
		{
			media = g_array_index(next_media_array, media_timeline *, i);
			pix = gdk_pixbuf_new_from_file(img_get_media_filename(img, media->id), NULL);
			if (pix)
			{
				surface = gdk_cairo_surface_create_from_pixbuf(pix, 0, NULL);
				if (surface)
				{
					// Center the image on the export surface
					gint x = (export_width - cairo_image_surface_get_width(surface)) / 2;
					gint y = (export_height - cairo_image_surface_get_height(surface)) / 2;
					cairo_set_source_surface(cr, surface, x, y);
					cairo_paint(cr);
					cairo_surface_destroy(surface);
				}
				g_object_unref(pix);
			}
		}
		cairo_destroy(cr);

		// Apply transition effect
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
		img->transition_progress = 0.0;
	}
	
	if (next_media_array)
		g_array_free(next_media_array, FALSE);

	g_array_free(current_media_array, FALSE);
	
	// Draw to export surface
	cr = cairo_create(img->exported_image);
	cairo_set_source_surface(cr, composite_surface, 0, 0); 
	cairo_paint(cr);
	cairo_destroy(cr);
	
	cairo_surface_destroy(composite_surface);

	//Convert the cairo surface to AVframe and send it to the encoder
	img->export_slide++;
	gboolean ok = img_convert_cairo_frame_to_avframe(img, img->exported_image);
	
	if (img->current_timeline_index >= img->total_time)
	{
		g_source_remove(img->source_id);
		img->source_id = 0;
		img_post_export(img);
		return G_SOURCE_REMOVE;
	}

	img->current_timeline_index += 1.0;
	g_print("\r%d - %d", (int)img->current_timeline_index, (int)img->total_time);
	return G_SOURCE_CONTINUE;
}

gboolean on_close_export_dialog(GtkWidget * widget, GdkEvent * event,  img_window_struct *img)
{
    img_close_export_dialog(img);
    return TRUE;
}

/* If the export wasn't aborted by user display the close button
 * and wait for the user to click it to close the dialog. This
 * way the elapsed time is still shown until the dialog is closed */
void img_post_export(img_window_struct *img)
{
	/* Flush remaining packets out of video by sending a NULL frame */
	img_export_encode_av_frame(NULL, img->video_format_context, img->codec_context, img->video_packet, img->video_stream);
	av_write_trailer(img->video_format_context);
	
	/* Free all the allocated resources */
	img_stop_export(img);
	
	gtk_widget_hide(img->export_pause_button);
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( img->export_pbar1 ), 1);
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( img->export_pbar2 ), 1);
	gtk_button_set_label(GTK_BUTTON(img->export_cancel_button), _("Close"));
	g_signal_connect_swapped (img->export_cancel_button, "clicked", G_CALLBACK (gtk_widget_destroy), img->export_dialog);
}

void img_close_export_dialog(img_window_struct *img)
{
	img_stop_export(img);
	gtk_widget_destroy(img->export_dialog);
}

gboolean img_stop_export( img_window_struct *img )
{
	/* Do any additional tasks */
	if( img->export_is_running)
	{
		g_source_remove( img->source_id );

		/* Destroy images that were used */
		//~ if (img->image1) cairo_surface_destroy( img->image1 );
		//~ if (img->image2) cairo_surface_destroy( img->image2 );
		//~ cairo_surface_destroy( img->image_from );
		//~ cairo_surface_destroy( img->image_to );
		cairo_surface_destroy( img->exported_image );

		/* Stops the timer */
		g_timer_destroy(img->elapsed_timer);
	}
	/* Free AV resources*/
	if (img->video_format_context)
		avio_closep(&img->video_format_context->pb);

	avcodec_close(img->codec_context);
    avcodec_free_context(&img->codec_context);

	av_frame_free(&img->video_frame);
	//av_frame_free(&img->audio_frame);
	
	av_packet_free(&img->video_packet);
	//av_packet_unref(img->audio_packet);

	/* Indicate that export is not running any more */
	img->export_is_running = 0;

	/* Free the slideshow filename */
	g_free(img->slideshow_filename);
	img->slideshow_filename = NULL;

	/* Redraw preview area */
	gtk_widget_queue_draw( img->image_area );
	return( FALSE );
}

static gboolean img_export_still( img_window_struct *img )
{
	gdouble export_progress;
	gchar	*dummy;

	/* If there is next slide, connect transition preview, else finish
	 * preview. */
	if( img->slide_cur_frame == img->slide_nr_frames )
	{
		if( img_prepare_pixbufs( img) )
		{
			gchar *string;

			img_calc_next_slide_time_offset( img, img->export_fps );
			img->export_slide++;

			string = g_strdup_printf( _("Media %d export progress:"),
										  img->export_slide );
			gtk_label_set_label( GTK_LABEL( img->export_label ), string );
			g_free( string );

			img->export_idle_func = (GSourceFunc)img_export_transition;
			img->source_id = g_idle_add( (GSourceFunc)img_export_transition, img );

			img->cur_point = NULL;
		}
		else {
			img_post_export(img);
		}

		return( FALSE );
	}
	gchar   string[10];

	/* Draw frames until we have enough of them to fill slide duration gap. */
	
	 //img_render_still_frame( img, img->export_fps );

	/* Increment global frame counter and update progress bar */
	img->still_counter++;
	img->slide_cur_frame++;
	img->displayed_frame++;

	/* CLAMPS are needed here because of the loosy conversion when switching
	 * from floating point to integer arithmetics. */
	export_progress = CLAMP( (gdouble)img->slide_cur_frame / img->slide_nr_frames, 0, 1 );
	snprintf( string, 10, "%.2f%%", export_progress * 100 );
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( img->export_pbar1 ), export_progress );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( img->export_pbar1 ), string );

	export_progress = CLAMP( (gdouble)img->displayed_frame /  img->total_nr_frames, 0, 1 );
	snprintf( string, 10, "%.2f%%", export_progress * 100 );
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( img->export_pbar2 ),   export_progress );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( img->export_pbar2 ), string );

	/* Update the elapsed time */
	img->elapsed_time = g_timer_elapsed(img->elapsed_timer, NULL);  
	dummy = img_convert_seconds_to_time( (gint) img->elapsed_time);
	gtk_label_set_text(GTK_LABEL(img->elapsed_time_label), dummy);
	g_free(dummy);

	return(TRUE);
}

static void img_export_pause_unpause( GtkToggleButton *button,  img_window_struct *img )
{
	if( gtk_toggle_button_get_active(button))
	{
		// Pause export
		g_source_remove(img->source_id);
		g_timer_stop(img->elapsed_timer);
	}
	else
	{
		img->source_id = g_idle_add(img->export_idle_func, img);
		g_timer_continue(img->elapsed_timer);
	}
}

static gboolean img_convert_cairo_frame_to_avframe(img_window_struct *img, cairo_surface_t *surface)
{
	static int pts = 0;
	gint	width, height, stride, ret;
	unsigned char  *pix;

	/* Image info and pixel data */
	width  = cairo_image_surface_get_width( surface );
	height = cairo_image_surface_get_height( surface );
	stride = cairo_image_surface_get_stride( surface );
	pix    = cairo_image_surface_get_data( surface );
	
	// Set up conversion context
	img->sws_ctx = sws_getContext(
        img->video_size[0],
        img->video_size[1],
        AV_PIX_FMT_BGRA,
        img->video_size[0],
        img->video_size[1],
        img->codec_context->pix_fmt,
        SWS_BICUBIC,
        NULL,
        NULL,
        NULL);
	memcpy(img->video_frame->data[0], pix, width * height * 4);
	
	 // Allocate the output frame
    AVFrame *out_frame = av_frame_alloc();
    out_frame->format = img->codec_context->pix_fmt;
    out_frame->width = width;
    out_frame->height = height;
    av_frame_get_buffer(out_frame, 0);
	sws_scale(img->sws_ctx, (const uint8_t * const*)img->video_frame->data, img->video_frame->linesize, 0,
				img->video_size[1], out_frame->data, out_frame->linesize);

	out_frame->pts = pts;

	ret = img_export_encode_av_frame(out_frame, img->video_format_context, img->codec_context, img->video_packet, img->video_stream);
	if (ret < 0)
	{
		img_stop_export(img);
		img_close_export_dialog(img);
		img_message(img, av_err2str(ret));
		return ret;
	}
    av_frame_free(&out_frame);
	sws_freeContext(img->sws_ctx);
	
	pts++;
	return ret;
}

gboolean img_export_encode_av_frame(AVFrame *frame, AVFormatContext *fmt, AVCodecContext *ctx, AVPacket *pkt, AVStream *stream)
{
	 gint ret;

    /* send the frame to the encoder */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0)
	{
		g_print("Av error: %s\n",av_err2str(ret));
		g_print("Error sending a frame for encoding\n");
		return ret;
	}
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN))
			return TRUE;
        else if (ret < 0)
		{
			if (ret != AVERROR_EOF)
				g_print("Error during encoding\n");

			return FALSE;
		}
		pkt->stream_index = stream->index;
		av_packet_rescale_ts(pkt, ctx->time_base, stream->time_base);

		av_interleaved_write_frame(fmt, pkt);
		//av_write_frame(fmt, pkt);
		av_packet_unref(pkt);
    }
    return TRUE;
}

void img_show_export_dialog (GtkWidget *button, img_window_struct *img )
{
	gint			codec_id, ret, result, crf, slides_selected = 0;
	GtkIconTheme *theme;
	const gchar *filename;
	GtkListStore	*liststore;
	GtkTreeIter 	iter, iter2;
	GdkPixbuf		*icon_pixbuf;
	GtkWidget	*dialog;
	GtkWidget	*iconview;
	GtkWidget	*vbox, *range_menu, *export_grid, *sample_rate, *bitrate;
	GtkWidget	*ex_vbox, *audio_frame, *video_frame, *label;
	GtkWidget	*frame_rate, *slideshow_title_entry, *fill_filename;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkCellRenderer *cell;
	GList 		*selected = NULL;
	gchar *container[8];
	container[0] = "MPEG-1 Video";
	container[1] = "MPEG-2 Video";
	container[2] = "MPEG-4 Video";
	container[3] = "MPEG-TS";
	container[4] = "Matroska MKV";
	container[5] = "Ogg";
	container[6] = "QuickTime MOV";
	container[7] = "WebM";

	/* Abort if preview is running */
	if( img->preview_is_running )
		return;

	/* Abort if export is running */
	if( img->export_is_running )
		return;

	/* Abort if no slide is present */
	model = GTK_TREE_MODEL( img->media_model );
	if( ! gtk_tree_model_get_iter_first( model, &iter ) )
		return;

	/* Create dialog */
	
	dialog = gtk_dialog_new_with_buttons( _("Export settings"), GTK_WINDOW(img->imagination_window),
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  "_Cancel", GTK_RESPONSE_CANCEL,
										  "_Export", GTK_RESPONSE_ACCEPT,
										  NULL );

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
	
	liststore = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_list_store_append (liststore,&iter2);
	
	theme = gtk_icon_theme_get_default();
	icon_pixbuf = gtk_icon_theme_load_icon (theme, "applications-multimedia", 64, 0, NULL);
	
	gtk_list_store_set (liststore, &iter2, 0, icon_pixbuf, 1, _("<b><span font='13'>Export the slideshow</span></b>"), -1);
	if(icon_pixbuf)
		g_object_unref (icon_pixbuf);

	iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL(liststore));
    g_object_unref (liststore);	
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(iconview), GTK_SELECTION_NONE);
	gtk_icon_view_set_item_orientation (GTK_ICON_VIEW (iconview), GTK_ORIENTATION_HORIZONTAL);
	gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (iconview), 0);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW (iconview),1);
	gtk_icon_view_set_markup_column(GTK_ICON_VIEW (iconview), 1);
	gtk_icon_view_set_item_padding(GTK_ICON_VIEW (iconview), 0);
	gtk_box_pack_start(GTK_BOX( vbox ), iconview, FALSE, FALSE, 0);
	
	ex_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (ex_vbox), 0);
	gtk_box_pack_start (GTK_BOX (vbox), ex_vbox, TRUE, TRUE, 0);
 
    gtk_widget_set_margin_top(ex_vbox, 10);

	video_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), video_frame, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (video_frame), GTK_SHADOW_IN);
	gtk_widget_set_margin_start(video_frame, 8);
	gtk_widget_set_margin_end(video_frame, 8);

	label = gtk_label_new (_("<b>Video Settings</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (video_frame), label);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

	ex_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(video_frame), ex_vbox);
    gtk_widget_set_halign(GTK_WIDGET(ex_vbox), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top(GTK_WIDGET(ex_vbox), 5);
    gtk_widget_set_margin_bottom(GTK_WIDGET(ex_vbox), 10);
    gtk_widget_set_margin_start(GTK_WIDGET(ex_vbox), 10);
    gtk_widget_set_margin_end(GTK_WIDGET(ex_vbox), 10);

	export_grid = gtk_grid_new();
	gtk_grid_set_row_homogeneous(GTK_GRID(export_grid), TRUE);
	gtk_grid_set_row_spacing (GTK_GRID(export_grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID(export_grid), 10);
	gtk_box_pack_start (GTK_BOX (ex_vbox), export_grid, TRUE, FALSE, 0);

	label = gtk_label_new( _("Container:") );
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), label, 0,0,1,1);

	img->container_menu = gtk_combo_box_text_new();
	gtk_grid_attach( GTK_GRID(export_grid), img->container_menu, 1,0,1,1);

	label = gtk_label_new( _("Codec:") );
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), label, 0,1,1,1);

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
	model = GTK_TREE_MODEL(store);
	img->vcodec_menu = gtk_combo_box_new_with_model(model);
	g_object_unref(G_OBJECT(model));
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (img->vcodec_menu), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (img->vcodec_menu), cell,
										"text", 0,
										NULL);
	g_object_set(cell, "ypad", (guint)0, NULL);

	gtk_grid_attach( GTK_GRID(export_grid), img->vcodec_menu, 1,1,1,1);
	g_signal_connect(G_OBJECT (img->container_menu),"changed",G_CALLBACK (img_container_changed),img);

	label = gtk_label_new( _("Slides Range:") );
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), label, 0,2,1,1);

	range_menu = gtk_combo_box_text_new();
	gtk_grid_attach( GTK_GRID(export_grid), range_menu, 1,2,1,1);

	label = gtk_label_new( _("Frame Rate:") );
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), label, 0,3,1,1);

	frame_rate = gtk_spin_button_new_with_range(20, 30, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(frame_rate), 25);
	gtk_grid_attach( GTK_GRID(export_grid), frame_rate, 1,3,1,1);
	
	img->quality_label = gtk_label_new( _("Quality (CRF):") );
	gtk_label_set_xalign (GTK_LABEL(img->quality_label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), img->quality_label, 0,4,1,1);

	img->video_quality = gtk_spin_button_new_with_range(20, 51, 1);
	gtk_grid_attach( GTK_GRID(export_grid), img->video_quality, 1,4,1,1);

	label = gtk_label_new( _("Filename:") );
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), label, 0,5,1,1);
	
    slideshow_title_entry = gtk_entry_new();
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(slideshow_title_entry), GTK_ENTRY_ICON_SECONDARY, "document-open"), 
	g_signal_connect (slideshow_title_entry, "icon-press", G_CALLBACK (img_show_file_chooser), img);
	gtk_grid_attach( GTK_GRID(export_grid), slideshow_title_entry, 1,5,1,1);

	/* Define the popup error message */
	img->file_po = gtk_popover_new(slideshow_title_entry);
	fill_filename = gtk_label_new(_("\n Please fill this field \n"));
	gtk_container_add (GTK_CONTAINER(img->file_po), fill_filename);
	
	/* Fill range combo box */
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(range_menu), NULL, _("All slides"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(range_menu), 0);
	if (slides_selected > 1)
	{
		gchar *string = g_strdup_printf("Selected %d slides",slides_selected);
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(range_menu), NULL, string);
		gtk_combo_box_set_active(GTK_COMBO_BOX(range_menu), 1);
		g_free(string);
	}

	/* Audio Settings */
	audio_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), audio_frame, TRUE, TRUE, 10);
	gtk_frame_set_shadow_type (GTK_FRAME (audio_frame), GTK_SHADOW_IN);
	gtk_widget_set_margin_start(audio_frame, 8);
	gtk_widget_set_margin_end(audio_frame, 8);

	if ( ! img_timeline_check_for_media_audio(img->timeline))
		gtk_widget_set_sensitive(audio_frame, FALSE);

	label = gtk_label_new (_("<b>Audio Settings</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (audio_frame), label);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

	ex_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(audio_frame), ex_vbox);
    gtk_widget_set_halign(GTK_WIDGET(ex_vbox), GTK_ALIGN_FILL);
    gtk_widget_set_margin_top(GTK_WIDGET(ex_vbox), 5);
    gtk_widget_set_margin_bottom(GTK_WIDGET(ex_vbox), 5);
    gtk_widget_set_margin_start(GTK_WIDGET(ex_vbox), 10);
    gtk_widget_set_margin_end(GTK_WIDGET(ex_vbox), 10);

	export_grid = gtk_grid_new();
	gtk_grid_set_row_homogeneous(GTK_GRID(export_grid), TRUE);
	gtk_grid_set_row_spacing (GTK_GRID(export_grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID(export_grid), 10);
	gtk_box_pack_start (GTK_BOX (ex_vbox), export_grid, TRUE, FALSE, 0);

	label = gtk_label_new( _("Codec:") );
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), label, 0,0,1,1);

	store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
	model = GTK_TREE_MODEL(store);
	img->acodec_menu = gtk_combo_box_new_with_model(model);
	g_object_unref(G_OBJECT(model));
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (img->acodec_menu), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (img->acodec_menu), cell,
										"text", 0,
										NULL);
	g_object_set(cell, "ypad", (guint)0, NULL);
	gtk_grid_attach( GTK_GRID(export_grid), img->acodec_menu, 1,0,1,1);

	/* Fill container combo box and all the
	 * others connected to it */
	gint i;
	for (i = 0; i <= 7; i++)
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(img->container_menu), NULL, container[i]);
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(img->container_menu), 2);
	
	label = gtk_label_new( _("Sample Rate:") );
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), label, 0,1,1,1);

	sample_rate = gtk_spin_button_new_with_range(20000, 44100, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(sample_rate), 44100);
	gtk_grid_attach( GTK_GRID(export_grid), sample_rate, 1,1,1,1);
	
	label = gtk_label_new( _("Bitrate (Kbps/CBR):") );
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_grid_attach( GTK_GRID(export_grid), label, 0,2,1,1);

	bitrate = gtk_spin_button_new_with_range(96, 320, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(bitrate), 256);
	gtk_grid_attach( GTK_GRID(export_grid), bitrate, 1,2,1,1);

	gtk_widget_show_all (dialog );

    /* Run dialog and abort if needed */
	while ( (result = gtk_dialog_run(GTK_DIALOG(dialog)) ) )
	{
		if (result != GTK_RESPONSE_ACCEPT)
		{
			gtk_widget_destroy(dialog);
			return;
		}
		if (gtk_entry_get_text_length (GTK_ENTRY(slideshow_title_entry)) == 0)
		{
			gtk_widget_show_all(img->file_po);
			gtk_popover_popup(GTK_POPOVER(img->file_po));
			continue;
		}
		break;
	}

	if (img->slideshow_filename == NULL)
	{
		filename = gtk_entry_get_text(GTK_ENTRY(slideshow_title_entry));
		img->slideshow_filename = g_strdup(filename);
	}
	img->export_fps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(frame_rate));
	crf = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(img->video_quality));

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(img->vcodec_menu));
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(img->vcodec_menu), &iter);
	gtk_tree_model_get( model, &iter, 1, &codec_id, -1);

	result = gtk_combo_box_get_active(GTK_COMBO_BOX(range_menu));

	gtk_widget_destroy( dialog );
	
	ret = img_initialize_av_parameters(img, img->export_fps, crf, codec_id);

	if ( ret < 0)
	{
		img_message(img, av_err2str(ret));
		g_free(img->slideshow_filename);
		img->slideshow_filename = NULL;
		return;
	}
	img_start_export(img);	
}

void img_container_changed (GtkComboBox *combo, img_window_struct *img)
{
	GtkListStore	*store = NULL;
	gint x;
	
	x = gtk_combo_box_get_active(combo);
	
	store = GTK_LIST_STORE( gtk_combo_box_get_model(GTK_COMBO_BOX(img->vcodec_menu)));
    gtk_list_store_clear(store);
    
    store = GTK_LIST_STORE( gtk_combo_box_get_model(GTK_COMBO_BOX(img->acodec_menu)));
    gtk_list_store_clear(store);

	switch (x)
	{
		case 0:
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_MPEG1VIDEO);

		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_AC3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP2);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_PCM_S16LE);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->vcodec_menu), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->acodec_menu), 2);
		img_swap_quality_for_bitrate(img);
		break;

		case 1:
		case 3:
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_MPEG2VIDEO);
		
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_AC3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP2);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_PCM_S16LE);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->vcodec_menu), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->acodec_menu), 2);
		img_swap_quality_for_bitrate(img);
		break;

		case 2:
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_MPEG4);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_H264);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_H265);
		
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_AAC);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_AC3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP2);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP3);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->vcodec_menu), 1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->acodec_menu), 0);
		img_swap_bitrate_for_quality(img);
		break;
		
		case 4:
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_MPEG4);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_H264);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_H265);

		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_AAC);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_AC3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_EAC3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_FLAC);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP2);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_OPUS);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_PCM_S16LE);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_VORBIS);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->vcodec_menu), 1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->acodec_menu), 0);
		img_swap_bitrate_for_quality(img);
		break;

		case 5:
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_THEORA);

		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_OPUS);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_VORBIS);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->vcodec_menu), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->acodec_menu), 1);
		img_swap_quality_for_bitrate(img);
		break;
		
		case 6:
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_QTRLE);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_MPEG4);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_H264);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_H265);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_MJPEG);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_PRORES);

		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_AAC);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_AC3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP2);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_MP3);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_PCM_S16LE);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->vcodec_menu), 2);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->acodec_menu), 0);
		img_swap_bitrate_for_quality(img);
		break;
		
		case 7:
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_VP8);
		img_add_codec_to_container_combo(img->vcodec_menu, AV_CODEC_ID_VP9);

		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_OPUS);
		img_add_codec_to_container_combo(img->acodec_menu, AV_CODEC_ID_VORBIS);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->vcodec_menu), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(img->acodec_menu), 0);
		img_swap_quality_for_bitrate(img);
		break;
	}
}

void img_add_codec_to_container_combo(GtkWidget *combo, enum AVCodecID codec)
{
	GtkListStore *store;
	GtkTreeIter   iter;
	
	const AVCodec *codec_info;
	codec_info = avcodec_find_encoder(codec);

	store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, codec_info->long_name, 1, codec, -1 );
}

void img_swap_quality_for_bitrate(img_window_struct *img)
{
	gtk_label_set_text(GTK_LABEL(img->quality_label), _("Bitrate (Mbps):"));
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(img->video_quality), 5, 99);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(img->video_quality), 12);
	gtk_widget_set_tooltip_text(img->video_quality, "");
}

void img_swap_bitrate_for_quality(img_window_struct *img)
{
	gtk_label_set_text(GTK_LABEL(img->quality_label), _("Quality (CRF):"));
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(img->video_quality),20, 51);
	gtk_widget_set_tooltip_text(img->video_quality, _("The range of the CRF scale is 0–51:\n0 = lossless\n" \
	"17-28 = visually lossless (still compressed but unnoticeable)\n23 = high quality\n51 = worst quality possible"));
}

static gint img_initialize_av_parameters(	img_window_struct *img,
												gint frame_rate,
												gint bitrate_crf,
												enum AVCodecID codec_id)
{
	const AVCodec* vcodec;
    gint ret;

	/* Setup video container */
	avformat_alloc_output_context2(&img->video_format_context, NULL, NULL, img->slideshow_filename);
	if (img->video_format_context == NULL)
	{
		gchar *string;
		string = g_strconcat(_("Failed to find a suitable container for %s\n"),img->slideshow_filename, NULL);
		img_message(img, string);
		g_free(string);
		return FALSE;
	}
	ret = avio_open(&img->video_format_context->pb, img->slideshow_filename, AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		gchar *string;
		string = g_strconcat(_("Couldn't write output file %s\n"),img->slideshow_filename, NULL);
		img_message(img, string);
		g_free(string);
		return FALSE;
	}
	/* Setup video codec */
	vcodec = avcodec_find_encoder(codec_id);
	if (!vcodec)
	{
		gchar *string;
		string = g_strconcat(_("Couldn't find any encoder for %s\n"),img->slideshow_filename, NULL);
		img_message(img, string);
		g_free(string);
		return FALSE;
	}
	/* Create video stream */
	img->video_stream = avformat_new_stream(img->video_format_context, vcodec);
	img->video_stream->id = 0;
	if (! img->video_stream)
	{	
		img_message(img, _("Couldn't not allocate video stream\n"));
		return FALSE;
	}
	/* Allocate video encoding context */
	img->codec_context = avcodec_alloc_context3(vcodec);
	if (! img->codec_context)
	{
		img_message(img, _("Couldn't allocate video enconding context\n"));
		return FALSE;
	}
	/* Setup video enconding context parameters */
	img->codec_context->codec_id = codec_id;
	img->codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
	img->codec_context->width  = img->video_size[0];
	img->codec_context->height = img->video_size[1];
	img->codec_context->sample_aspect_ratio = (struct AVRational) {1, 1};
	
	switch (codec_id)
	{
		case AV_CODEC_ID_QTRLE:
		img->codec_context->pix_fmt = AV_PIX_FMT_RGB24;
		break;
		
		case AV_CODEC_ID_MJPEG:
		img->codec_context->pix_fmt = AV_PIX_FMT_YUVJ420P;
		break;
		
		case AV_CODEC_ID_PRORES:
		img->codec_context->pix_fmt = AV_PIX_FMT_YUV422P10LE;
		break;
		
		default:
		img->codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
	}
	
	img->codec_context->framerate = av_d2q(frame_rate, INT_MAX);

	if (codec_id == AV_CODEC_ID_VP8 || codec_id == AV_CODEC_ID_VP9 || codec_id == AV_CODEC_ID_THEORA || 
		AV_CODEC_ID_MPEG1VIDEO || codec_id == AV_CODEC_ID_MPEG2VIDEO)
		img->codec_context->bit_rate = round(bitrate_crf * 1000000);

	img->codec_context->time_base = av_inv_q(img->codec_context->framerate);
	img->video_stream->time_base = img->codec_context->time_base;

	if (img->video_format_context->oformat->flags & AVFMT_GLOBALHEADER)
		img->codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	/* Some codecs require the CRF value */
	if (codec_id == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_H265)
	{
		gchar *crf = g_strdup_printf("%i", bitrate_crf);
		av_opt_set(img->codec_context->priv_data, "crf", crf, AV_OPT_SEARCH_CHILDREN);
		g_free(crf);
	}
	/* Set exporting stage to be multithreaded */
	AVDictionary* opts = NULL;
	av_dict_set(&opts, "threads", "auto", 0);

	/* Open video encoder */
	ret = avcodec_open2(img->codec_context, vcodec, &opts);
	if (ret < 0)
	{
		img_message(img, av_err2str(ret));
		return ret;
	}
	/* Copy video encoder parameters to output stream */
	ret = avcodec_parameters_from_context(img->video_stream->codecpar, img->codec_context);
	if (ret < 0)
	{
		img_message(img, _("Failed to copy video encoder parameters to output stream\n"));
		return FALSE;
	}

	/* AVFRAME stuff */
	img->video_packet = av_packet_alloc();
	img->video_frame = av_frame_alloc();
	av_frame_make_writable(img->video_frame);
	img->video_frame->format = AV_PIX_FMT_BGRA;
	img->video_frame->width  = img->video_size[0];
	img->video_frame->height = img->video_size[1];
	
	ret = av_frame_get_buffer(img->video_frame, 0);
	if (ret < 0)
	{	
		img_message(img, _("Could not allocate the video frame data\n"));
		return FALSE;
	}
	if ( (ret = av_frame_make_writable(img->video_frame)) < 0)
	{
		img_message(img, av_err2str(ret));
		return FALSE;
	}

	/*						*/
	/* SETUP AUDIO
	 * 						*/
	
	/* Write Imagination header in the metadata */
	opts =  NULL;
	av_dict_set(&opts, "movflags", "use_metadata_tags", 0);
	av_dict_set(&img->video_format_context->metadata, "Made with Imagination", VERSION, 0);
	ret = avformat_write_header(img->video_format_context, &opts);
	if (ret < 0)
	{
		img_message(img, av_err2str(ret));
		return -1;
	}
	return TRUE;
}

static cairo_surface_t *img_get_next_composed_media(img_window_struct *img)
{
}
