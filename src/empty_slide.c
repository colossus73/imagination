/*
 *  Copyright (c) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
 *  Copyright (c) 2009 Tadej Borovšak 	<tadeboro@gmail.com>
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
 
#include "empty_slide.h"

void img_add_empty_slide( GtkMenuItem *item, img_window_struct *img )
{
	/* This structure retains values across invocations */
	ImgEmptySlide slide = { { 0, 0, 0 },         /* Start color */
								   { 1, 1, 1 },         /* Stop color */
								   { 1, 1, 1 },		/* Countdown color */
								   { 0, 0 },            /* Start point (l) */
								   { -1, 0 },           /* Stop point (l) */
								   { 0, 0 },            /* Start point (r) */
								   { 0, 0 },            /* Stop point (r) */
								   0,                   /* Drag */
								   0,                   /* Gradient type */
								   5,
								   0,
								   NULL,               /* Color button */
								   NULL,               /* Countdown Color button */
								   NULL,               /* Preview area */
								   NULL,				/* Label seconds */
								   NULL,				/* Range widget to be valued later */
							{ NULL, NULL, NULL, NULL, NULL }, /* Radio buttons */
								   1.0,					/* Fade alpha value */
								   0
								 }; 

	GList *where_to_insert = NULL;
	GtkWidget *dialog,
			  *vbox,
			  *frame,
			  *grid,
			  *radio1,
			  *radio2,
			  *radio3,
			  *radio4,
			  *radio5,
			  *color1,
			  *label_countdown,
			  *countdown,
			  *preview,
			  *hbox;
	GdkRGBA   color;
	gint       i, w, h, pos;

	w = img->video_size[0] / 2;
	h = img->video_size[1] / 2;

	if (GTK_WIDGET(item) == img->edit_empty_slide)
	{
		/* Let's copy the existing empty slide
		 * values in the empty slide struct so
		 * the preview dialog area shows the settings
		 * user applied when creating the empty one */
		slide.gradient = img->current_slide->gradient;
		slide.c_start[0] = img->current_slide->g_start_color[0];
		slide.c_start[1] = img->current_slide->g_start_color[1];
		slide.c_start[2] = img->current_slide->g_start_color[2];
		
		slide.c_stop[0] = img->current_slide->g_stop_color[0];
		slide.c_stop[1] = img->current_slide->g_stop_color[1];
		slide.c_stop[2] = img->current_slide->g_stop_color[2];
		
		if (slide.gradient < 2) /* solid and linear */
		{
			slide.pl_start[0] = img->current_slide->g_start_point[0] * w;
			slide.pl_start[1] = img->current_slide->g_start_point[1] * h;
			slide.pl_stop[0]  = img->current_slide->g_stop_point[0] * w;
			slide.pl_stop[1]  = img->current_slide->g_stop_point[1] * h;
		}
		else /* radial */
		{
			slide.pr_start[0] = img->current_slide->g_start_point[0] * w;
			slide.pr_start[1] = img->current_slide->g_start_point[1] * h;
			slide.pr_stop[0]  = img->current_slide->g_stop_point[0] * w;
			slide.pr_stop[1]  = img->current_slide->g_stop_point[1] * h;
		}
	}
	dialog = gtk_dialog_new_with_buttons(
					GTK_WIDGET(item) == img->edit_empty_slide ? _("Edit empty slide") : _("Create empty slide"),
					GTK_WINDOW( img->imagination_window ),
					GTK_DIALOG_MODAL,
					"_Cancel", GTK_RESPONSE_CANCEL,
					"_OK", GTK_RESPONSE_ACCEPT,
					NULL );

	vbox = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
	gtk_widget_set_margin_bottom(dialog, 5);

	frame = gtk_frame_new( _("Empty slide options:") );
	gtk_box_pack_start( GTK_BOX( vbox ), frame, TRUE, TRUE, 5 );
	gtk_container_set_border_width( GTK_CONTAINER( frame ), 5 );

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_add( GTK_CONTAINER( frame ), hbox );
	gtk_widget_set_margin_top(GTK_WIDGET(hbox), 10);

	grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID(grid), 6);
	gtk_box_pack_start( GTK_BOX( hbox ), grid, FALSE, FALSE, 10 );

	radio1 = gtk_radio_button_new_with_mnemonic( NULL, _("_Solid color") );
	gtk_grid_attach( GTK_GRID(grid), radio1, 0, 0, 1, 1);
	slide.radio[0] = radio1;

	radio2 = gtk_radio_button_new_with_mnemonic_from_widget(
				GTK_RADIO_BUTTON( radio1 ), _("_Linear gradient") );
	gtk_grid_attach( GTK_GRID(grid), radio2, 0, 1, 1, 1);
	slide.radio[1] = radio2;

	radio3 = gtk_radio_button_new_with_mnemonic_from_widget(
				GTK_RADIO_BUTTON( radio1 ), _("_Radial gradient") );
	gtk_grid_attach( GTK_GRID(grid), radio3, 0, 2, 1, 1);
	slide.radio[2] = radio3;
	
	radio4 = gtk_radio_button_new_with_mnemonic_from_widget(
				GTK_RADIO_BUTTON( radio1 ), _("_Fade gradient") );
	gtk_grid_attach( GTK_GRID(grid), radio4, 0, 3, 1, 1);
	slide.radio[3] = radio4;
	
	radio5 = gtk_radio_button_new_with_mnemonic_from_widget(
				GTK_RADIO_BUTTON( radio1 ), _("_Old movie countdown") );
	gtk_grid_attach( GTK_GRID(grid), radio5, 0, 4, 1, 1);
	slide.radio[4] = radio5;

	gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON( slide.radio[slide.gradient] ), TRUE );

	for( i = 0; i < 5; i++ )
		g_signal_connect( G_OBJECT( slide.radio[i] ), "toggled",
						  G_CALLBACK( img_gradient_toggled ), &slide );

	color.red   = slide.c_start[0];
	color.green = slide.c_start[1];
	color.blue  = slide.c_start[2];
	color.alpha = 1.0;
	color1 = gtk_color_button_new_with_rgba( &color );
	gtk_grid_attach( GTK_GRID(grid), color1, 0, 5, 1, 1);
	g_signal_connect( G_OBJECT( color1 ), "color-set",
					  G_CALLBACK( img_gradient_color_set ), &slide );

	color.red   = slide.c_stop[0];
	color.green = slide.c_stop[1];
	color.blue  = slide.c_stop[2];
	color.alpha = 1.0;
	slide.color2 = gtk_color_button_new_with_rgba( &color );
	gtk_grid_attach( GTK_GRID(grid), slide.color2, 0, 6, 1, 1);
	gtk_widget_set_sensitive( slide.color2, (gboolean)slide.gradient );
	g_signal_connect( G_OBJECT( slide.color2 ), "color-set",
					  G_CALLBACK( img_gradient_color_set ), &slide );

	color.red   = slide.countdown_color[0];
	color.green = slide.countdown_color[1];
	color.blue  = slide.countdown_color[2];
	color.alpha = 1.0;
	slide.color3 = gtk_color_button_new_with_rgba( &color );
	gtk_grid_attach( GTK_GRID(grid), slide.color3, 0, 7, 1, 1);
	g_signal_connect( G_OBJECT( slide.color3 ), "color-set",
					  G_CALLBACK( img_gradient_color_set ), &slide );

	slide.label_countdown = gtk_label_new (_("Seconds:"));
	gtk_grid_attach (GTK_GRID (grid), slide.label_countdown, 0, 8, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(slide.label_countdown), 0);
	gtk_label_set_yalign(GTK_LABEL(slide.label_countdown), 0.5);

	slide.range_countdown = gtk_spin_button_new_with_range (1,10, 1);
	gtk_grid_attach (GTK_GRID (grid), slide.range_countdown, 0, 9, 1, 1);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (slide.range_countdown),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (slide.range_countdown), 5.0);

	g_signal_connect (G_OBJECT (slide.range_countdown),"value-changed",G_CALLBACK (img_countdown_value_changed), &slide);
	
	slide.preview = gtk_drawing_area_new();
	gtk_widget_set_size_request( slide.preview, w, h );
	gtk_widget_add_events( slide.preview, GDK_BUTTON1_MOTION_MASK |
									GDK_BUTTON_PRESS_MASK |
									GDK_BUTTON_RELEASE_MASK );
	gtk_box_pack_start( GTK_BOX( hbox ), slide.preview, TRUE, TRUE, 5 );

	g_signal_connect( G_OBJECT( slide.preview ), "draw",  G_CALLBACK( img_gradient_draw ), &slide );
	g_signal_connect( G_OBJECT( slide.preview ), "button-press-event",   G_CALLBACK( img_gradient_press ), &slide );
	g_signal_connect( G_OBJECT( slide.preview ), "button-release-event",   G_CALLBACK( img_gradient_release ), &slide );
	g_signal_connect( G_OBJECT( slide.preview ), "motion-notify-event",   G_CALLBACK( img_gradient_move ), &slide );

	gtk_widget_show_all( dialog );
	if (slide.gradient != 4)
	{
		gtk_widget_hide(slide.color3);
		gtk_widget_hide(slide.label_countdown);
		gtk_widget_hide(slide.range_countdown);
	}
	if( slide.pl_stop[0] < 0 )
	{
		slide.pl_stop[0] = (gdouble)w;
		slide.pl_stop[1] = (gdouble)h;
		slide.pr_start[0] = w * 0.5;
		slide.pr_start[1] = h * 0.5;
	}

	if( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT )
	{
		GtkTreeIter   iter;
		slide_struct *slide_info;
		GdkPixbuf    *thumb, *pix;
		GtkTreeModel *model;
		gchar		 *path;

		if (GTK_WIDGET(item) == img->edit_empty_slide)
			slide_info = img->current_slide;
		else
			slide_info = img_create_new_slide();

		if (slide.gradient == 3)
			path = "10:0";
		else
			path = "0";

		/* Set the transition type to Misc Cross Fade or to None */
		g_signal_handlers_block_by_func(img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);
		{
			GtkTreeIter   iter;
			GtkTreeModel *model;

			model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->transition_type ) );
			gtk_tree_model_get_iter_from_string( model, &iter, path );
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(img->transition_type), &iter );
		}
		g_signal_handlers_unblock_by_func(img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);

		if( slide_info )
		{
			gdouble p_start[2],
					p_stop[2];

			/* Convert gradient points into relative offsets (this enables us to
			 * scale gradient on any surface size) */
			if( slide.gradient == 0 || slide.gradient == 1 || slide.gradient == 3 ) /* Solid, linear and fade */
			{
				p_start[0] = slide.pl_start[0] / w;
				p_start[1] = slide.pl_start[1] / h;
				p_stop[0] = slide.pl_stop[0] / w;
				p_stop[1] = slide.pl_stop[1] / h;
			}
			else /* Radial gradient */
			{
				p_start[0] = slide.pr_start[0] / w;
				p_start[1] = slide.pr_start[1] / h;
				p_stop[0]  = slide.pr_stop[0]  / w;
				p_stop[1]  = slide.pr_stop[1]  / h;
			}

			/* Update slide info */
			img_set_slide_gradient_info( slide_info, slide.gradient,
										 slide.c_start, slide.c_stop,
										 p_start, p_stop );

			/* Create thumbnail */
			img_scale_gradient( slide.gradient, p_start, p_stop,
								slide.c_start, slide.c_stop,
								88, 49,
								&thumb, NULL );

			/* If a slide is selected, add the empty slide after it */
			where_to_insert	= gtk_icon_view_get_selected_items(GTK_ICON_VIEW(img->active_icon));
			if (where_to_insert)
			{
				pos = gtk_tree_path_get_indices(where_to_insert->data)[0]+1;
				/* Add empty slide */
				if (GTK_WIDGET(item) != img->edit_empty_slide)
				{
					pix = img_set_fade_gradient(img, slide.gradient, slide_info);
					gtk_list_store_insert_with_values(img->thumbnail_model, &iter,
													pos,
													0, thumb,
													1, slide_info,
													2, pix,
													3, FALSE,
													-1 );
				}
				else
				{
					/* Edit empty slide */
					pix = img_set_fade_gradient(img, slide.gradient, slide_info);
					model =	GTK_TREE_MODEL( img->thumbnail_model );
					gtk_tree_model_get_iter(model, &iter, where_to_insert->data);
					gtk_list_store_set(img->thumbnail_model, &iter,
													0, thumb,
													1, slide_info,
													2, pix,
													3, slide_info->subtitle ? TRUE : FALSE,
													-1 );
					/* Update gradient in image area */
					img_scale_gradient( slide.gradient,
								slide_info->g_start_point,
								slide_info->g_stop_point,
								slide_info->g_start_color,
								slide_info->g_stop_color,
								img->video_size[0],
								img->video_size[1],
								NULL,
								&img->current_image );
				}
				GList *node18;
				for(node18 = where_to_insert;node18 != NULL;node18 = node18->next) {
					gtk_tree_path_free(node18->data);
				}
				g_list_free (where_to_insert);
			}
			else
			{
				/* Add empty slide */
				if (GTK_WIDGET(item) != img->edit_empty_slide)
				{
					pix = img_set_fade_gradient(img, slide.gradient, slide_info);
					gtk_list_store_append( img->thumbnail_model, &iter );
					gtk_list_store_set(img->thumbnail_model, &iter, 0, thumb, 1, slide_info, 2, pix, 3, FALSE, -1 );
				}
			}
				
			if (GTK_WIDGET(item) != img->edit_empty_slide)
			{
				img->slides_nr++;
				img_set_total_slideshow_duration( img );
				img_select_nth_slide( img, img->slides_nr );
			}
			g_object_unref( G_OBJECT( thumb ) );
		}
	img_taint_project(img);
	gtk_widget_queue_draw(img->image_area);
	}
	
	if (slide.source > 0)
	{
		g_source_remove(slide.source);
		slide.source = 0;
	}
	
	gtk_widget_destroy( dialog );
}

void img_countdown_value_changed (GtkSpinButton *spinbutton, ImgEmptySlide *slide)
{
	slide->countdown = gtk_spin_button_get_value(GTK_SPIN_BUTTON(slide->range_countdown));
	gtk_widget_queue_draw(slide->preview);
}

static void img_gradient_toggled( GtkToggleButton *button, ImgEmptySlide   *slide )
{
	GtkWidget *widget = GTK_WIDGET( button );
	gint       i;

	if( ! gtk_toggle_button_get_active( button ) )
		return;

	for( i = 0; widget != slide->radio[i]; i++ )
		;

	slide->gradient = i;
	gtk_widget_set_sensitive( slide->color2, (gboolean)i );
	gtk_widget_hide(slide->color3);
	gtk_widget_hide(slide->label_countdown);
	gtk_widget_hide(slide->range_countdown);

	if (i == 3)
	{
		if (slide->source > 0)
		{
			g_source_remove(slide->source);
			slide->source = 0;
		}
		slide->source = g_timeout_add( 2000 * 0.01, (GSourceFunc)img_fade_countdown, slide );
	} 
	else if(i == 4)
	{
		gtk_widget_show(slide->color3);
		gtk_widget_show(slide->label_countdown);
		gtk_widget_show(slide->range_countdown);
		slide->countdown = gtk_spin_button_get_value(GTK_SPIN_BUTTON(slide->range_countdown));
		if (slide->source > 0)
		{
			g_source_remove(slide->source);
			slide->source = 0;
		}
		slide->source = g_timeout_add( 100, (GSourceFunc)img_fade_countdown, slide );
	}
	gtk_widget_queue_draw( slide->preview );
}

static gboolean img_fade_countdown(ImgEmptySlide *slide)
{
	if (slide->gradient == 3)
	{
		slide->alpha -= 0.01;
	
		if (slide->alpha <= 0)
		{
			slide->alpha = 1.0;
			slide->source = 0;
			return FALSE;
		}
	}
	else
	{
		slide->angle += 0.5;
		if (slide->angle > 6.5)
		{
			slide->angle = 0;
			slide->countdown--;
		}
		if (slide->countdown < 1)
		{
				slide->source = 0;
				return FALSE;
		}
	}
	gtk_widget_queue_draw(slide->preview);

	return TRUE;
}

static void
img_gradient_color_set( GtkColorChooser *button,
						ImgEmptySlide  *slide )
{
	GdkRGBA  color;
	gdouble  *my_color;

	gtk_color_chooser_get_rgba( button, &color );
	
	if( (GtkWidget *)button == slide->color3 )
		my_color = slide->countdown_color;
	else if( (GtkWidget *)button == slide->color2 )
		my_color = slide->c_stop;
	else
		my_color = slide->c_start;

	my_color[0] = (gdouble)color.red;
	my_color[1] = (gdouble)color.green;
	my_color[2] = (gdouble)color.blue;

	gtk_widget_queue_draw( slide->preview );
}

gboolean img_gradient_draw( GtkWidget  *UNUSED(widget), cairo_t *cr, ImgEmptySlide  *slide )
{
	cairo_pattern_t *pattern;
	gdouble          radius, diffx, diffy;

	switch( slide->gradient )
	{
		case 0:
			cairo_set_source_rgb( cr, slide->c_start[0],
									  slide->c_start[1],
									  slide->c_start[2] );
			cairo_paint( cr );
			break;

		case 1:
			pattern = cairo_pattern_create_linear( slide->pl_start[0],
												   slide->pl_start[1],
												   slide->pl_stop[0],
												   slide->pl_stop[1] );
			cairo_pattern_add_color_stop_rgb( pattern, 0,
											  slide->c_start[0],
											  slide->c_start[1],
											  slide->c_start[2] );
			cairo_pattern_add_color_stop_rgb( pattern, 1,
											  slide->c_stop[0],
											  slide->c_stop[1],
											  slide->c_stop[2] );
			cairo_set_source( cr, pattern );
			cairo_paint( cr );
			cairo_pattern_destroy( pattern );

			/* Paint indicators */
			cairo_rectangle( cr, slide->pl_start[0] - 7,
								 slide->pl_start[1] - 7,
								 8, 8 );
			cairo_rectangle( cr, slide->pl_stop[0] - 7,
								 slide->pl_stop[1] - 7,
								 8, 8 );
			cairo_set_source_rgb( cr, 0, 0, 0 );
			cairo_stroke_preserve( cr );
			cairo_set_source_rgb( cr, 1, 1, 1 );
			cairo_fill( cr );
			break;

		case 2:
			diffx = ABS( slide->pr_start[0] - slide->pr_stop[0] );
			diffy = ABS( slide->pr_start[1] - slide->pr_stop[1] );
			radius = sqrt( pow( diffx, 2 ) + pow( diffy, 2 ) );
			pattern = cairo_pattern_create_radial( slide->pr_start[0],
												   slide->pr_start[1],
												   0,
												   slide->pr_start[0],
												   slide->pr_start[1],
												   radius );
			cairo_pattern_add_color_stop_rgb( pattern, 0,
											  slide->c_start[0],
											  slide->c_start[1],
											  slide->c_start[2] );
			cairo_pattern_add_color_stop_rgb( pattern, 1,
											  slide->c_stop[0],
											  slide->c_stop[1],
											  slide->c_stop[2] );
			cairo_set_source( cr, pattern );
			cairo_paint( cr );
			cairo_pattern_destroy( pattern );

			/* Paint indicators */
			cairo_rectangle( cr, slide->pr_start[0] - 7,
								 slide->pr_start[1] - 7,
								 8, 8 );
			cairo_rectangle( cr, slide->pr_stop[0] - 7,
								 slide->pr_stop[1] - 7,
								 8, 8 );
			cairo_set_source_rgb( cr, 0, 0, 0 );
			cairo_stroke_preserve( cr );
			cairo_set_source_rgb( cr, 1, 1, 1 );
			cairo_fill( cr );
		break;

		case 3:
			cairo_set_source_rgb(cr,	slide->c_stop[0],
										slide->c_stop[1],
										slide->c_stop[2] );
			cairo_paint( cr );
		
			cairo_set_source_rgb(cr, 	slide->c_start[0],
										slide->c_start[1],
										slide->c_start[2] ); 
			cairo_paint_with_alpha(cr, slide->alpha);
			if (slide->source == 0)
				slide->source = g_timeout_add( 2000 * 0.01, (GSourceFunc)img_fade_countdown, slide );
		break;
		
		case 4:
		cairo_text_extents_t te;
		int offset = 80;
		double x = 25, y = 30, width2 = 90, height2 = 60, aspect = 1.0, corner_radius = height2 / 10.0;
		double radius = corner_radius / aspect;
		double degrees = M_PI / 180.0;

		//Get the drawing area size
		gint width = gtk_widget_get_allocated_width (slide->preview);
		gint height = gtk_widget_get_allocated_height (slide->preview);
		width /= 2;
		height /= 2;

		//Clear the drawing area
		cairo_set_source_rgb(cr, slide->c_start[0],  slide->c_start[1], slide->c_start[2]);
		cairo_paint(cr);
		
		//Draw the left rectangles column
		cairo_set_source_rgb(cr, slide->c_stop[0],  slide->c_stop[1], slide->c_stop[2]);
		for (int i=0; i<=4; i++)
		{
			cairo_new_sub_path (cr);
			cairo_arc (cr, x + width2 - radius, y + i * (60 + offset) + radius, radius, -90 * degrees, 0 * degrees);
			cairo_arc (cr, x + width2 - radius, y + i * (60 + offset) + height2 - radius, radius, 0 * degrees, 90 * degrees);
			cairo_arc (cr, x + radius, y + i * (60 + offset) + height2 - radius, radius, 90 * degrees, 180 * degrees);
			cairo_arc (cr, x + radius, y + i * (60 + offset) + radius, radius, 180 * degrees, 270 * degrees);
			cairo_close_path (cr);
			cairo_fill(cr);
		}
		
		//Draw the right rectangles column
		for (int i=0; i<=4; i++)
		{
			cairo_new_sub_path (cr);
			cairo_arc (cr, (width * 2 - width2 - x) + width2 - radius, y + i * (60 + offset) + radius, radius, -90 * degrees, 0 * degrees);
			cairo_arc (cr, (width * 2 - width2 - x) + width2 - radius, y + i * (60 + offset) + height2 - radius, radius, 0 * degrees, 90 * degrees);
			cairo_arc (cr, (width * 2 - width2 - x) + radius, y + i * (60 + offset) + height2 - radius, radius, 90 * degrees, 180 * degrees);
			cairo_arc (cr, (width * 2 - width2 - x) + radius, y + i * (60 + offset) + radius, radius, 180 * degrees, 270 * degrees);
			cairo_close_path (cr);
			cairo_fill(cr);
		}
		//Draw the cross
		cairo_move_to(cr, 0,height);
		cairo_line_to(cr, width*2,height);

		cairo_move_to(cr, width,0);
		cairo_line_to(cr, width,height*2);

		//Draw the first circle
		cairo_set_source_rgb(cr, slide->c_stop[0],  slide->c_stop[1], slide->c_stop[2]);
		cairo_move_to(cr, width,height);
		cairo_arc(cr, width, height, width/2, 0, 2 * G_PI);
		cairo_stroke(cr);
		
		//Draw the second circle
		cairo_arc(cr, width, height, width/2 - 20, 0, 2 * G_PI);
		cairo_stroke(cr);

		//Draw sector
		cairo_move_to(cr, width, height);
		cairo_set_source_rgb(cr, slide->c_stop[0],  slide->c_stop[1], slide->c_stop[2]);
		cairo_arc(cr, width, height, 220, 0, slide->angle);
		cairo_fill(cr);

		// Draw the countdown number
		cairo_set_source_rgb(cr, slide->countdown_color[0],  slide->countdown_color[1], slide->countdown_color[2]);
		cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, 150);
		gchar *digit = g_strdup_printf("%d", slide->countdown);
		cairo_text_extents (cr, digit, &te);
		cairo_move_to(cr, width - te.x_bearing - te.width / 2, height - te.y_bearing - te.height / 2);
		cairo_show_text(cr, digit);
		g_free(digit);
		
		if (slide->source == 0)
				slide->source = g_timeout_add( 100, (GSourceFunc)img_fade_countdown, slide );
	break;
	}

	return( TRUE );
}

static gboolean img_gradient_press( GtkWidget      * UNUSED(widget),
					GdkEventButton *button,
					ImgEmptySlide  *slide )
{
	if( button->button != 1 )
		return( FALSE );

	switch( slide->gradient )
	{
		case 1:
			if( button->x < ( slide->pl_start[0] + 8 ) &&
				button->x > ( slide->pl_start[0] - 8 ) &&
				button->y < ( slide->pl_start[1] + 8 ) &&
				button->y > ( slide->pl_start[1] - 8 ) )
			{
				slide->drag = 1;
			}
			else if( button->x < ( slide->pl_stop[0] + 8 ) &&
					 button->x > ( slide->pl_stop[0] - 8 ) &&
					 button->y < ( slide->pl_stop[1] + 8 ) &&
					 button->y > ( slide->pl_stop[1] - 8 ) )
			{
				slide->drag = 2;
			}
			break;

		case 2:
			if( button->x < ( slide->pr_start[0] + 8 ) &&
				button->x > ( slide->pr_start[0] - 8 ) &&
				button->y < ( slide->pr_start[1] + 8 ) &&
				button->y > ( slide->pr_start[1] - 8 ) )
			{
				slide->drag = 1;
			}
			else if( button->x < ( slide->pr_stop[0] + 8 ) &&
					 button->x > ( slide->pr_stop[0] - 8 ) &&
					 button->y < ( slide->pr_stop[1] + 8 ) &&
					 button->y > ( slide->pr_stop[1] - 8 ) )
			{
				slide->drag = 2;
			}
			break;
	}

	return( TRUE );
}

static gboolean img_gradient_release( GtkWidget      * UNUSED(widget),
					  GdkEventButton * UNUSED(button),
					  ImgEmptySlide  *slide )
{
	slide->drag = 0;

	return( TRUE );
}

static gboolean img_gradient_move( GtkWidget      * UNUSED(widget),
				   GdkEventMotion *motion,
				   ImgEmptySlide  *slide )
{
	gint w, h;

	if( ! slide->drag )
		return( FALSE );

	gdk_window_get_geometry( motion->window, NULL, NULL, &w, &h);
	switch( slide->gradient )
	{
		case 1:
			if( slide->drag == 1 )
			{
				slide->pl_start[0] = CLAMP( motion->x, 0, w );
				slide->pl_start[1] = CLAMP( motion->y, 0, h );
			}
			else
			{
				slide->pl_stop[0] = CLAMP( motion->x, 0, w );
				slide->pl_stop[1] = CLAMP( motion->y, 0, h );
			}
			break;

		case 2:
			if( slide->drag == 1 )
			{
				slide->pr_start[0] = CLAMP( motion->x, 0, w );
				slide->pr_start[1] = CLAMP( motion->y, 0, h );
			}
			else
			{
				slide->pr_stop[0] = CLAMP( motion->x, 0, w );
				slide->pr_stop[1] = CLAMP( motion->y, 0, h );
			}
			break;
	}
	gtk_widget_queue_draw( slide->preview );

	return( TRUE );
}
