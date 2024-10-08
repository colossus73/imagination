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
#include <fcntl.h>
#include <glib/gstdio.h>

static gboolean
img_start_export( img_window_struct *img);

static gint
img_initialize_av_parameters(img_window_struct *, gint , gint , enum AVCodecID);

static gboolean
img_export_still( img_window_struct *img );

static void
img_export_pause_unpause( GtkToggleButton   *button,
						  img_window_struct *img );

static gboolean
img_convert_cairo_frame_to_avframe(img_window_struct *img,
						cairo_surface_t *surface);
						
static gboolean
img_export_encode_av_frame(AVFrame *frame, AVFormatContext *fmt, AVCodecContext *ctx, AVPacket *pkt, AVStream *stream);

static gboolean img_start_export( img_window_struct *img)
{
	GtkTreeIter   iter;
	media_struct *entry;
	GtkTreeModel *model;
	GtkWidget    *dialog;
	GtkWidget	 *image;
	GtkWidget    *vbox, *hbox;
	GtkWidget    *label;
	GtkWidget    *progress;
	AtkObject *atk;
	gchar        *string;
	cairo_t      *cr;

	/* Set export info */
	img->export_is_running = 1;
	
	/* Create progress window with cancel and pause buttons, calculate
	 * the total number of frames to display. */
	dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	img->export_dialog = dialog;
	gtk_window_set_title( GTK_WINDOW( img->export_dialog ),
						  _("Exporting the slideshow") );
	g_signal_connect (G_OBJECT(img->export_dialog), "delete_event", G_CALLBACK (on_close_export_dialog), img);
	gtk_container_set_border_width( GTK_CONTAINER( dialog ), 10 );
	gtk_window_set_default_size( GTK_WINDOW( dialog ), 400, -1 );
	gtk_window_set_type_hint( GTK_WINDOW( dialog ), GDK_WINDOW_TYPE_HINT_DIALOG );
	gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
	gtk_window_set_transient_for( GTK_WINDOW( dialog ),
								  GTK_WINDOW( img->imagination_window ) );

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_add( GTK_CONTAINER( dialog ), vbox );

	label = gtk_label_new( _("Preparing for export ...") );
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	atk = gtk_widget_get_accessible(label);
	atk_object_set_description(atk, _("Status of export"));
	img->export_label = label;
	gtk_box_pack_start( GTK_BOX( vbox ), label, FALSE, FALSE, 0 );

	progress = gtk_progress_bar_new();
	img->export_pbar1 = progress;
	string = g_strdup_printf( "%.2f", .0 );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( progress ), string );
	gtk_box_pack_start( GTK_BOX( vbox ), progress, FALSE, FALSE, 0 );

	label = gtk_label_new( _("Overall progress:") );
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
	gtk_button_set_always_show_image (GTK_BUTTON (img->export_cancel_button), TRUE);
	gtk_button_set_image (GTK_BUTTON (img->export_cancel_button), image);
	
	g_signal_connect_swapped( G_OBJECT( img->export_cancel_button ), "clicked",
							  G_CALLBACK( img_close_export_dialog ), img );
	gtk_box_pack_end( GTK_BOX( hbox ), img->export_cancel_button, FALSE, FALSE, 0 );

	image = gtk_image_new_from_icon_name( "media-playback-pause", GTK_ICON_SIZE_BUTTON );
	img->export_pause_button = gtk_toggle_button_new_with_label(_("Pause"));
	gtk_button_set_always_show_image (GTK_BUTTON (img->export_pause_button), TRUE);
	gtk_button_set_image (GTK_BUTTON (img->export_pause_button), image);

	g_signal_connect( G_OBJECT( img->export_pause_button ), "toggled",
					  G_CALLBACK( img_export_pause_unpause ), img );
	gtk_box_pack_end( GTK_BOX( hbox ), img->export_pause_button, FALSE, FALSE, 0 );

	gtk_widget_show_all( dialog );

	/* Create first slide */
	img->image1 = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
											  img->video_size[0],
											  img->video_size[1] );
	cr = cairo_create( img->image1 );
	cairo_set_source_rgb( cr, img->background_color[0],
							  img->background_color[1],
							  img->background_color[2] );
	cairo_paint( cr );
	cairo_destroy( cr );

	//~ /* Load first image from model */
	//~ model = GTK_TREE_MODEL( img->media_model );
	//~ gtk_tree_model_get_iter_first( model, &iter );
	//~ gtk_tree_model_get( model, &iter, 1, &entry, -1 );

	gboolean success = FALSE;

	if( ! entry->full_path )
	{
		success = img_scale_empty_slide( entry->gradient, entry->countdown, entry->g_start_point,
							entry->g_stop_point, entry->g_start_color,
							entry->g_stop_color, 
							entry->countdown_color, 
							0, entry->countdown_angle,
							img->video_size[0],
							img->video_size[1], NULL, &img->image2 );
	}
	else
	{
		success = img_scale_image( entry->full_path, img->video_ratio,
						 0, 0, FALSE,
						 img->background_color, NULL, &img->image2 );
	}

	if (!success) {
	    img->image2 = NULL;
	    img_stop_export(img);
	    return (FALSE);
	}

	/* Add export idle function and set initial values */
	img->current_media = entry;
	img->total_nr_frames = img->total_secs * img->export_fps;
	img->displayed_frame = 0;
	img->next_slide_off = 0;
	img_calc_next_slide_time_offset( img, img->export_fps );

	/* Create surfaces to be passed to transition renderer */
	img->image_from = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
												  img->video_size[0],
												  img->video_size[1] );
	img->image_to = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
												img->video_size[0],
												img->video_size[1] );
	img->exported_image = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
													  img->video_size[0],
													  img->video_size[1] );

	/* Fade empty slide */
	if (entry->gradient == 3)
	{
		cairo_t	*cr;
		cr = cairo_create(img->image_from);
		cairo_set_source_rgb(cr,	entry->g_start_color[0],
									entry->g_start_color[1],
									entry->g_start_color[2] );
		cairo_paint( cr );
			
		cr = cairo_create(img->image_to);
		cairo_set_source_rgb(cr,	entry->g_stop_color[0],
									entry->g_stop_color[1],
									entry->g_stop_color[2] );
		cairo_paint( cr );
	}	

	/* Set stop points */
	img->cur_point = NULL;
	img->point1 = NULL;
	img->point2 = (ImgStopPoint *)( img->current_media->no_points ?
									img->current_media->points->data :
									NULL );

	/* Set first slide */
	//gtk_tree_model_get_iter_first( GTK_TREE_MODEL( img->media_model ), &img->cur_ss_iter );

	img->export_slide = 1;

	img->export_idle_func = (GSourceFunc)img_export_transition;
	if (img->current_media->gradient == 4)
			img->source_id = g_timeout_add( 100, (GSourceFunc)img_empty_slide_countdown_preview, img );
	else
		img->source_id = g_idle_add( (GSourceFunc)img_export_transition, img );
	
	img->elapsed_timer = g_timer_new();

	string = g_strdup_printf( _("Slide %d export progress:"), 1 );
	gtk_label_set_label( GTK_LABEL( img->export_label ), string );
	g_free( string );

	/* Update display 
	gtk_widget_queue_draw( img->image_area );*/

	return( FALSE );
}

gboolean
on_close_export_dialog(GtkWidget * widget, GdkEvent * event,  img_window_struct *img)
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
		if (img->image1) cairo_surface_destroy( img->image1 );
		if (img->image2) cairo_surface_destroy( img->image2 );
		cairo_surface_destroy( img->image_from );
		cairo_surface_destroy( img->image_to );
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

/*
 * img_prepare_pixbufs:
 * @img: global img_window_struct
 * @preview: do we load image for preview
 *
 * This function is used when previewing or exporting slideshow. It goes
 * through the model and prepares everything for next transition.
 *
 *
 * This function also sets img->point[12] that are used for transitions.
 *
 * Return value: TRUE if images have been succefully prepared, FALSE otherwise.
 */
gboolean
img_prepare_pixbufs( img_window_struct *img)
{
	GtkTreeModel    *model;
	GtkTreePath     *path;
	gchar			*selected_slide_nr;
	static gboolean  last_transition = TRUE;
	gboolean success;

	model = GTK_TREE_MODEL( img->media_model );

	/* Get last stop point of current slide */
	img->point1 = (ImgStopPoint *)( img->current_media->no_points ?
									g_list_last( img->current_media->points )->data :
									NULL );

	/* We're done now */
	last_transition = TRUE;
	return( FALSE );
}

/*
 * img_calc_next_slide_time_offset:
 * @img: global img_window_struct structure
 * @rate: frame rate to be used for calculations
 *
 * This function will calculate:
 *   - time offset of next slide (img->next_slide_off)
 *   - number of frames for current slide (img->slide_nr_frames)
 *   - number of slides needed for transition (img->slide_trans_frames)
 *   - number of slides needed for still part (img->slide_still_franes)
 *   - reset current slide counter to 0 (img->slide_cur_frame)
 *   - number of frames for subtitle animation (img->no_text_frames)
 *   - reset current subtitle counter to 0 (img->cur_text_frame)
 *
 * Return value: new time offset. The same value is stored in
 * img->next_slide_off.
 */
gdouble
img_calc_next_slide_time_offset( img_window_struct *img,
								 gdouble            rate )
{
	int transition_speed = 3;
	
	if( img->current_media->render )
	{
		img->next_slide_off += img->current_media->duration + transition_speed;
		img->slide_trans_frames = transition_speed * rate;
	}
	else
	{
		img->next_slide_off += img->current_media->duration;
		img->slide_trans_frames = 0;
	}

	img->slide_nr_frames = img->next_slide_off * rate - img->displayed_frame;
	img->slide_cur_frame = 0;
	img->slide_still_frames = img->slide_nr_frames - img->slide_trans_frames;

	/* Calculate subtitle frames */
	if( img->current_media->text )
	{
		img->cur_text_frame = 0;
		img->no_text_frames = img->current_media->anim_duration * rate;
	}

	return( img->next_slide_off );
}

/*
 * img_export_transition:
 * @img:
 *
 * This is idle callback function that creates transition frames. When
 * transition is complete, it detaches itself from main context and connects
 * still export function.
 *
 * Return value: TRUE if transition isn't exported completely, FALSE otherwise.
 */
gboolean img_export_transition( img_window_struct *img )
{
	gchar   string[10];
	gchar	*dummy;
	gdouble export_progress;

	/* If we rendered all transition frames, connect still export */
	if( img->slide_cur_frame == img->slide_trans_frames )
	{
		img->export_idle_func = (GSourceFunc)img_export_still;
		img->source_id = g_idle_add( (GSourceFunc)img_export_still, img );

		return( FALSE );
	}

	/* Draw one frame of transition animation */
	img_render_transition_frame( img );

	/* Increment global frame counters and update progress bars */
	img->slide_cur_frame++;
	img->displayed_frame++;

	export_progress = CLAMP( (gdouble)img->slide_cur_frame /
									  img->slide_nr_frames, 0, 1 );
	snprintf( string, 10, "%.2f%%", export_progress * 100 );
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( img->export_pbar1 ),
								   export_progress );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( img->export_pbar1 ), string );
	export_progress = CLAMP( (gdouble)img->displayed_frame /
									  img->total_nr_frames, 0, 1 );
	snprintf( string, 10, "%.2f%%", export_progress * 100 );
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( img->export_pbar2 ),
								   export_progress );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( img->export_pbar2 ), string );

	/* Update the elapsed time */
	img->elapsed_time = g_timer_elapsed(img->elapsed_timer, NULL);  
	dummy = img_convert_seconds_to_time( (gint) img->elapsed_time);
	gtk_label_set_text(GTK_LABEL(img->elapsed_time_label), dummy);
	g_free(dummy);

	/* Draw every 10th frame of animation on screen 
	if( img->displayed_frame % 10 == 0 )
		gtk_widget_queue_draw( img->image_area );
*/
	return( TRUE );
}

/*
 * img_export_still:
 * @img:
 *
 * Idle callback that outputs still image frames. When enough frames has been
 * outputed, it connects transition export.
 *
 * Return value: TRUE if more still frames need to be exported, else FALSE.
 */
static gboolean
img_export_still( img_window_struct *img )
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

			/* Make dialog more informative */
			if( img->current_media->duration == 0 )
				string = g_strdup_printf( _("Final transition export progress:") );
			else
				string = g_strdup_printf( _("Slide %d export progress:"),
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
	
	 img_render_still_frame( img, img->export_fps );

	/* Increment global frame counter and update progress bar */
	img->still_counter++;
	img->slide_cur_frame++;
	img->displayed_frame++;

	/* CLAMPS are needed here because of the loosy conversion when switching
	 * from floating point to integer arithmetics. */
	export_progress = CLAMP( (gdouble)img->slide_cur_frame /
									  img->slide_nr_frames, 0, 1 );
	snprintf( string, 10, "%.2f%%", export_progress * 100 );
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( img->export_pbar1 ),
								   export_progress );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( img->export_pbar1 ), string );

	export_progress = CLAMP( (gdouble)img->displayed_frame /
									  img->total_nr_frames, 0, 1 );
	snprintf( string, 10, "%.2f%%", export_progress * 100 );
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( img->export_pbar2 ),
								   export_progress );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( img->export_pbar2 ), string );

	/* Update the elapsed time */
	img->elapsed_time = g_timer_elapsed(img->elapsed_timer, NULL);  
	dummy = img_convert_seconds_to_time( (gint) img->elapsed_time);
	gtk_label_set_text(GTK_LABEL(img->elapsed_time_label), dummy);
	g_free(dummy);
	
	/* Draw every 10th frame of animation on screen 
	if( img->displayed_frame % 10 == 0 )
		gtk_widget_queue_draw( img->image_area );*/

	return( TRUE );
}

/*
 * img_export_pause_unpause:
 * @img:
 *
 * Temporarily disconnect export functions. This doesn't stop ffmpeg!!!
 */
static void
img_export_pause_unpause( GtkToggleButton   *button,
						  img_window_struct *img )
{
	if( gtk_toggle_button_get_active( button ) )
	{
		/* Pause export */
		g_source_remove( img->source_id );
		g_timer_stop(img->elapsed_timer);
	}
	else
	{
		img->source_id = g_idle_add(img->export_idle_func, img);
		g_timer_continue(img->elapsed_timer);
	}
}

void
img_render_transition_frame( img_window_struct *img )
{
	ImgStopPoint  point = { 0, 0, 0, 1.0 }; /* Default point */
	gdouble       progress;
	cairo_t      *cr;

	/* Do image composing here and place result in exported_image */
	
	/* Create first image
	 * this is a dirt hack to have Imagination use the image_from painted
	 * with the second color set in the empty slide fade gradient */
	if (img->current_media->full_path && img->gradient_slide)
	{
		cr = cairo_create( img->image_from );
		cairo_set_source_rgb(cr,	img->g_stop_color[0],
									img->g_stop_color[1],
									img->g_stop_color[2]);
		cairo_paint( cr );
	}
	else
	{
		cr = cairo_create( img->image_from );
		if (img->current_media->gradient != 3)
			img_draw_image_on_surface( cr, img->video_size[0], img->image1,
								( img->point1 ? img->point1 : &point ), img );
	}
#if 0
	/* Render subtitle if present */
	if( img->current_media->subtitle )
	{
		gdouble       progress;     /* Text animation progress */
		ImgStopPoint *p_draw_point; 

		progress = (gdouble)img->cur_text_frame / ( img->no_text_frames - 1 );
		progress = CLAMP( progress, 0, 1 );
		img->cur_text_frame++;

		p_draw_point = ( img->point1 ? img->point1 : &point );

		img_render_subtitle( img,
							 cr,
							 img->video_size[0],
							 img->video_size[1],
							 1.0,
							 img->current_media->position,
							 p_draw_point->zoom,
							 p_draw_point->offx,
							 p_draw_point->offy,
							 img->current_media->subtitle,
							 img->current_media->font_desc,
							 img->current_media->font_color,
							 img->current_media->anim,
							 FALSE,
							 FALSE,
							 progress );
	}
#endif
	cairo_destroy( cr );

	/* Create second image */
	cr = cairo_create( img->image_to );
	if (img->current_media->gradient != 3)
		img_draw_image_on_surface( cr, img->video_size[0], img->image2,
							   ( img->point2 ? img->point2 : &point ), img );
	/* FIXME: Add subtitles here */
	cairo_destroy( cr );

	/* Compose them together */
	progress = (gdouble)img->slide_cur_frame / ( img->slide_trans_frames - 1 );
	cr = cairo_create( img->exported_image );
	cairo_save( cr );
	img->current_media->render( cr, img->image_from, img->image_to, progress );
	cairo_restore( cr );
	
	/* Export frame */
	if (img->export_is_running)
	{
		gint ok = img_convert_cairo_frame_to_avframe( img, img->exported_image);
		if (ok < 0)
		{
			img_stop_export(img);
			img_close_export_dialog(img);
			img_message(img, av_err2str(ok));
		}
	}
	cairo_destroy( cr );
}

void img_render_still_frame( img_window_struct *img, gdouble rate )
{
	cairo_t      *cr;
	ImgStopPoint *p_draw_point;                  /* Pointer to current sp */
	ImgStopPoint  draw_point = { 0, 0, 0, 1.0 }; /* Calculated stop point */

	/* If no stop points are specified, we simply draw img->image2 with default
	 * stop point on each frame.
	 *
	 * If we have only one stop point, we draw img->image2 on each frame
	 * properly scaled, with no movement.
	 *
	 * If we have more than one point, we draw movement from point to point.
	 */
	switch( img->current_media->no_points )
	{
		case( 0 ): /* No stop points */
			p_draw_point = &draw_point;
			break;

		case( 1 ): /* Single stop point */
			p_draw_point = (ImgStopPoint *)img->current_media->points->data;
			break;

		default:   /* Many stop points */
			{
				ImgStopPoint *point1,
							 *point2;
				gdouble       progress;
				GList        *tmp;

				if( ! img->cur_point )
				{
					/* This is initialization */
					img->cur_point = img->current_media->points;
					point1 = (ImgStopPoint *)img->cur_point->data;
					img->still_offset = point1->time;
					img->still_max = img->still_offset * rate;
					img->still_counter = 0;
					img->still_cmlt = 0;
				}
				else if( img->still_counter == img->still_max )
				{
					/* This is advancing to next point */
					img->cur_point = g_list_next( img->cur_point );
					point1 = (ImgStopPoint *)img->cur_point->data;
					img->still_offset += point1->time;
					img->still_cmlt += img->still_counter;
					img->still_max = img->still_offset * rate -
									 img->still_cmlt;
					img->still_counter = 0;
				}

				point1 = (ImgStopPoint *)img->cur_point->data;
				tmp = g_list_next( img->cur_point );
				if( tmp )
				{
					point2 = (ImgStopPoint *)tmp->data;
					progress = (gdouble)img->still_counter /
										( img->still_max - 1);
					img_calc_current_ken_point( &draw_point, point1, point2,
												progress, 0 );
					p_draw_point = &draw_point;
				}
				else
					p_draw_point = point1;
			}
			break;
	}

	/* Paint surface */
	cr = cairo_create( img->exported_image );
	if (img->current_media->gradient == 3)
		img_draw_image_on_surface( cr, img->video_size[0], img->image_to,
							   p_draw_point, img );
	else
		img_draw_image_on_surface( cr, img->video_size[0], img->image2,
							   p_draw_point, img );

	/* Render subtitle if present */
	if( img->current_media->text )
	{
		gdouble progress; /* Text animation progress */

		progress = (gdouble)img->cur_text_frame / ( img->no_text_frames - 1 );
		progress = CLAMP( progress, 0, 1 );
		img->cur_text_frame++;

		img_render_subtitle( img,
							 cr,
							 1.0,
							 img->current_media->posX,
							 img->current_media->posY,
							 img->current_media->subtitle_angle,
							 img->current_media->alignment,
							 p_draw_point->zoom,
							 p_draw_point->offx,
							 p_draw_point->offy,
							 FALSE,
							 FALSE,
							 progress );
	}

	/* Export frame */
	if (img->export_is_running) {
		gboolean ok = img_convert_cairo_frame_to_avframe( img, img->exported_image);
		if (ok < 0)
		{
			img_stop_export(img);
			img_close_export_dialog(img);
			img_message(img, av_err2str(ok));
		}
	}
	
	/* Destroy drawing context */
	cairo_destroy( cr );
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
	gint			codec_id, ret;
	GtkIconTheme *theme;
	const gchar *filename;
	GtkListStore	*liststore;
	GtkTreeIter 	iter2;
	GdkPixbuf		*icon_pixbuf;
	GtkWidget	*dialog;
	GtkWidget	*iconview;
	GtkWidget	*vbox, *range_menu, *export_grid, *sample_rate, *bitrate;
	GtkWidget	*ex_vbox, *audio_frame, *video_frame, *label;
	GtkWidget	*frame_rate, *slideshow_title_entry, *fill_filename;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkCellRenderer *cell;
	GtkTreeIter   iter;
	GList 		*selected = NULL;
	gint		result,
				fr,
				crf,
				slides_selected = 0;

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

	/* Disable the whole Audio frame if there
	 * is no music in the project */
	//~ if (img->music_file_treeview)
	//~ {
		//~ model = gtk_tree_view_get_model(GTK_TREE_VIEW(img->music_file_treeview));
		//~ if(gtk_tree_model_get_iter_first(model, &iter) == FALSE)
			//~ gtk_widget_set_sensitive(audio_frame, FALSE);
	//~ }
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
	fr = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(frame_rate));
	crf = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(img->video_quality));

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(img->vcodec_menu));
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(img->vcodec_menu), &iter);
	gtk_tree_model_get( model, &iter, 1, &codec_id, -1);

	result = gtk_combo_box_get_active(GTK_COMBO_BOX(range_menu));

	gtk_widget_destroy( dialog );
	
	ret = img_initialize_av_parameters(img, fr, crf, codec_id);

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
	av_dict_set(&img->video_format_context->metadata, "Made with Imagination:", VERSION, 0);
	ret = avformat_write_header(img->video_format_context, &opts);
	if (ret < 0)
	{
		img_message(img, av_err2str(ret));
		return -1;
	}
	return TRUE;
}
