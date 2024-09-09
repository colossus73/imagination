/*
 *  Copyright (C) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
 *  Copyright (c) 2009 Tadej Borovšak 	<tadeboro@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include "slideshow_project.h"

static gboolean img_populate_hash_table( GtkTreeModel *, GtkTreePath *, GtkTreeIter *, GHashTable ** );

void img_save_slideshow( img_window_struct *img,	const gchar *output,  gboolean relative )
{
	GKeyFile *img_key_file;
	gchar *conf, *conf_media, *path, *filename, *file, *font_desc;
	gint count = 0;
	gsize len;
	GtkTreeIter iter;
	GtkTreeIter media_iter;
	slide_struct *entry;
	GtkTreeModel *model;
	GtkTreeModel *media_model;

	//Save the bottom iconview, to be removed when the timeline will be finally ready
	model = GTK_TREE_MODEL( img->thumbnail_model );
	media_model = GTK_TREE_MODEL( img->media_model );
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;
		
	if (!gtk_tree_model_get_iter_first (media_model, &media_iter))
		return;

	img_key_file = g_key_file_new();

	/* Slideshow settings */
	g_key_file_set_comment(img_key_file, NULL, NULL, comment_string, NULL);
	g_key_file_set_integer(img_key_file, "slideshow settings", "video width", img->video_size[0]);
    g_key_file_set_integer(img_key_file, "slideshow settings", "video height", img->video_size[1]);
	g_key_file_set_boolean( img_key_file, "slideshow settings",  "blank slide", img->bye_bye_transition);
	g_key_file_set_double_list( img_key_file, "slideshow settings", "background color", img->background_color, 3 );
	g_key_file_set_boolean(img_key_file,"slideshow settings", "distort images", img->distort_images);
	g_key_file_set_integer(img_key_file, "slideshow settings", "number of slides", img->slides_nr);
	g_key_file_set_integer(img_key_file, "slideshow settings", "number of media", img->media_nr);

	do
	{
		count++;
		gtk_tree_model_get(media_model, &media_iter,2,&filename,-1);
		conf = g_strdup_printf("media %d",count);
		g_key_file_set_string( img_key_file, conf, "filename", filename);
	}
	while (gtk_tree_model_iter_next (media_model,&media_iter));

	/* Slide settings */
	count = 0;
	do
	{
		count++;
		gtk_tree_model_get(model, &iter,1,&entry,-1);
		conf = g_strdup_printf("slide %d",count);

        if (entry->load_ok)
            filename = entry->o_filename;
        else
            filename = entry->original_filename;

		if (relative && filename)
		{
			gchar *_filename;
			_filename = g_path_get_basename(filename);
			filename = _filename;
		}
        if (filename)
		{
			/* Save original filename and rotation */
			g_key_file_set_string( img_key_file, conf, "filename", filename);
			g_key_file_set_integer( img_key_file, conf, "angle", entry->angle );
		}
		else
		{
			/* We are dealing with an empty slide */
			g_key_file_set_integer(img_key_file, conf, "gradient",	entry->gradient);
			g_key_file_set_double_list(img_key_file, conf, "start_color", entry->g_start_color, 3 );
			g_key_file_set_double_list(img_key_file, conf, "stop_color" , entry->g_stop_color , 3 );
			g_key_file_set_double_list(img_key_file, conf, "start_point", entry->g_start_point, 2 );
			g_key_file_set_double_list(img_key_file, conf, "stop_point" , entry->g_stop_point, 2 );
			if (entry->gradient == 4)
			{
					g_key_file_set_double_list(img_key_file, conf, "countdown_color", entry->countdown_color, 3 );
					g_key_file_set_integer(img_key_file, conf, "countdown",	entry->countdown);
			}
		}

		/* Duration */
		g_key_file_set_double(img_key_file,conf, "duration",		entry->duration);

		/* Flipped horizontally */
		g_key_file_set_boolean(img_key_file,conf, "flipped",	entry->flipped);

		/* Transition */
		g_key_file_set_integer(img_key_file,conf, "transition_id",	entry->transition_id);

		/* Stop points */
		g_key_file_set_integer(img_key_file,conf, "no_points",		entry->no_points);
		if (entry->no_points > 0)
		{
			gint    point_counter;
			gdouble my_points[entry->no_points * 4];

			for( point_counter = 0;
				 point_counter < entry->no_points;
				 point_counter++ )
			{
				ImgStopPoint *my_point = g_list_nth_data(entry->points,point_counter);
				my_points[ (point_counter * 4) + 0] = (gdouble)my_point->time;
				my_points[ (point_counter * 4) + 1] = my_point->offx;
				my_points[ (point_counter * 4) + 2] = my_point->offy;
				my_points[ (point_counter * 4) + 3] = my_point->zoom;
			}
			g_key_file_set_double_list(img_key_file,conf, "points", my_points, (gsize) entry->no_points * 4);
		}

		/* Subtitle */
		if( entry->subtitle )
		{
			entry->subtitle[26] =  32;
			entry->subtitle[27] =  32;
			entry->subtitle[28] =  32;
			entry->subtitle[29] =  32;

			g_key_file_set_string (img_key_file, conf,"text", (gchar*)entry->subtitle);
			g_key_file_set_integer(img_key_file,conf, "text length", entry->subtitle_length - 30);

			entry->subtitle[26] = (entry->subtitle_length >> 24) & 0xFF;
			entry->subtitle[27] = (entry->subtitle_length >> 16) & 0xFF;
			entry->subtitle[28] = (entry->subtitle_length >> 8) & 0xFF;
			entry->subtitle[29] = entry->subtitle_length & 0xFF;

			if( entry->pattern_filename )
			{
				if (relative)
				{
					gchar *dummy;
					dummy = g_path_get_basename(entry->pattern_filename);
					g_key_file_set_string (img_key_file, conf,"pattern filename", dummy);
					g_free(dummy);
				}
				else
					g_key_file_set_string (img_key_file, conf,"pattern filename", entry->pattern_filename);
			}
			font_desc = pango_font_description_to_string(entry->font_desc);
			g_key_file_set_integer(img_key_file,conf, "anim id",		entry->anim_id);
			g_key_file_set_integer(img_key_file,conf, "anim duration",	entry->anim_duration);
			g_key_file_set_integer(img_key_file,conf, "posX",		entry->posX);
			g_key_file_set_integer(img_key_file,conf, "posY",		entry->posY);
			g_key_file_set_integer(img_key_file,conf, "subtitle angle",		entry->subtitle_angle);
			g_key_file_set_string (img_key_file, conf,"font",			font_desc);
			g_key_file_set_double_list(img_key_file, conf,"font color",entry->font_color,4);
			g_key_file_set_double_list(img_key_file, conf,"font bgcolor",entry->font_brdr_color,4);
			g_key_file_set_double_list(img_key_file, conf,"font bgcolor2",entry->font_bg_color,4);
			g_key_file_set_double_list(img_key_file, conf,"border color",entry->border_color,4);
			g_key_file_set_boolean(img_key_file, conf,"top border",entry->top_border);
			g_key_file_set_boolean(img_key_file, conf,"bottom border",entry->bottom_border);
			g_key_file_set_integer(img_key_file, conf,"border width",entry->border_width);
			g_key_file_set_integer(img_key_file, conf,"alignment",entry->alignment);
			g_free(font_desc);
		}
			g_free(conf);
	}
	while (gtk_tree_model_iter_next (model,&iter));
	count = 0;

	/* Write the project file */
	conf = g_key_file_to_data(img_key_file, &len, NULL);
	g_file_set_contents( output, conf, len, NULL );
	g_free (conf);

	g_key_file_free(img_key_file);

	if( img->project_filename )
		g_free( img->project_filename );
	img->project_filename = g_strdup( output );
	img->project_is_modified = FALSE;
	img_refresh_window_title(img);
}

gboolean img_append_slides_from( img_window_struct *img, GtkWidget *menuitem, const gchar *input )
{
	GdkPixbuf *thumb;
	slide_struct *slide_info;
	GtkTreeIter iter;
	GKeyFile *img_key_file;
	gchar *dummy, *slide_filename, *time;
	GtkWidget *dialog, *menu;
	gint number,i, n_invalid, transition_id, no_points, previous_nr_of_slides, border_width, alignment;
	guint speed;
	GtkTreeModel *model;
	void (*render);
	GHashTable *table;
	gchar      *spath, *conf, *project_current_dir;
	gdouble    duration, *color, *font_color, *font_brdr_color, *font_bg_color, *border_color;
	gboolean   first_slide = TRUE;

	if (img->no_recent_item_menu)
	{
		gtk_widget_destroy(img->no_recent_item_menu);
		img->no_recent_item_menu = NULL;
	}
	/* Check if the file still exist on the disk */
	if (!g_file_test (input, G_FILE_TEST_EXISTS))
	{
		if (menuitem)
		{
			if (img_ask_user_confirmation(img, _("The file doesn't exist anymore on the disk.\nDo you want to remove it from the list?")))
				gtk_widget_destroy(menuitem);
		}
		else
			img_message(img, _("Error: File doesn't exist") );
		return FALSE;
	}

	/* Create new key file */
	img_key_file = g_key_file_new();
	g_key_file_load_from_file( img_key_file, input, G_KEY_FILE_KEEP_COMMENTS, NULL );
	dummy = g_key_file_get_comment( img_key_file, NULL, NULL, NULL);

	if (dummy == NULL)
	{
		dialog = gtk_message_dialog_new(
				GTK_WINDOW( img->imagination_window ), GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				_("This is not an Imagination project file!") );
		gtk_window_set_title( GTK_WINDOW( dialog ), "Imagination" );
		gtk_dialog_run( GTK_DIALOG( dialog ) );
		gtk_widget_destroy( GTK_WIDGET( dialog ) );
		g_free( dummy );
		return FALSE;
	}
	g_free( dummy );

	/* Add the slideshow filename into the Open Recent submenu */
	if ( ! img_check_for_recent_file(img, input))
	{
		menu = gtk_menu_item_new_with_label(input);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(img->recent_slideshows), menu);
		gtk_widget_show(menu);
	}
	project_current_dir = g_path_get_dirname(input);

	/* Create hash table for efficient searching */
	table = g_hash_table_new_full( g_direct_hash, g_direct_equal, NULL, g_free );
	model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->transition_type ) );
	gtk_tree_model_foreach( model, (GtkTreeModelForeachFunc)img_populate_hash_table, &table );

	/* Video Size */
	img->video_size[0] = g_key_file_get_integer(img_key_file, "slideshow settings", "video width", NULL);
	img->video_size[1] = g_key_file_get_integer(img_key_file, "slideshow settings", "video height", NULL);
	
	if (img->video_size[0] == 0)
		img->video_size[0] = 1280;
	
	if (img->video_size[1] == 0)
		img->video_size[1] = 720;
		
	img->video_ratio = (gdouble)img->video_size[0] / img->video_size[1];

   	/* Make loading more efficient by removing model from icon view */
	g_object_ref( G_OBJECT( img->thumbnail_model ) );
	gtk_icon_view_set_model( GTK_ICON_VIEW( img->thumbnail_iconview ), NULL );

	gchar *subtitle = NULL, *pattern_name = NULL, *font_desc;
	gdouble *my_points = NULL, *p_start = NULL, *p_stop = NULL, *c_start = NULL, *c_stop = NULL, *countdown_color = NULL;
	gsize length;
	gint anim_id,anim_duration, posx, posy, gradient = 0, subtitle_length, subtitle_angle, countdown = 0;
	GdkPixbuf *pix = NULL;
    gboolean      load_ok, flipped, img_load_ok, top_border, bottom_border;
	gchar *original_filename = NULL;
	GtkIconTheme *icon_theme;
	GtkIconInfo  *icon_info;
	GFile *file;
	GFileInfo *file_info;
	const gchar  *icon_filename;
	const gchar *content_type;
	gchar *mime_type;
	ImgAngle   angle = 0;

		/* Load last slide setting (bye bye transition) */
		img->bye_bye_transition = g_key_file_get_boolean( img_key_file, "slideshow settings",  "blank slide", NULL);
		
		/* Load project backgroud color */
		color = g_key_file_get_double_list( img_key_file, "slideshow settings",	"background color", NULL, NULL );
		img->background_color[0] = color[0];
		img->background_color[1] = color[1];
		img->background_color[2] = color[2];
		g_free( color );

		/* Loads the media files */
		 number = g_key_file_get_integer( img_key_file, "slideshow settings",  "number of media", NULL);
		 if (number > 0)
		{
			 for( i = 1; i <= number; i++ )
			{
				conf = g_strdup_printf("media %d", i);
				slide_filename = g_key_file_get_string(img_key_file,conf,"filename", NULL);
				file = g_file_new_for_path (slide_filename);
				
				file_info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
				content_type = g_file_info_get_content_type (file_info);
				if (content_type == NULL)
				{
					gchar *string;
					string = g_strconcat(_("Can't load media %s\n"), slide_filename, NULL);
					n_invalid++;
					img_message(img, string);
					g_free(string);
				}
				else
				{
					mime_type = g_content_type_get_mime_type (content_type);
					if (strstr(mime_type, "image"))
						img_add_thumbnail_widget_area(0, slide_filename, img);
					else if (strstr(mime_type, "audio"))
						img_add_thumbnail_widget_area(1, slide_filename, img);
				}
				g_free(conf);
				g_free(slide_filename);
				g_object_unref(file);
			}
		}
		/* Loads the thumbnails and set the slides info */
		number = g_key_file_get_integer( img_key_file, "slideshow settings",  "number of slides", NULL);
		/* Store the previous number of slides and set img->slides_nr so to have the correct number of slides displayed on the status bar */
		previous_nr_of_slides = img->slides_nr;
		img->slides_nr = number;

		n_invalid = 0;
		for( i = 1; i <= number; i++ )
		{
			conf = g_strdup_printf("slide %d", i);
			slide_filename = g_key_file_get_string(img_key_file,conf,"filename", NULL);

			if( slide_filename )
			{
				if ( g_path_is_absolute(slide_filename) == FALSE)
					original_filename = g_strconcat(project_current_dir, "/", slide_filename, NULL);
				else
					original_filename = g_strdup (slide_filename);

				angle = (ImgAngle)g_key_file_get_integer( img_key_file, conf,
														  "angle", NULL );
				load_ok = img_scale_image( original_filename, img->video_ratio,
										   88, 0, img->distort_images,
										   img->background_color, &thumb, NULL );
                img_load_ok = load_ok;
                if (! load_ok)
                {
                    icon_theme = gtk_icon_theme_get_default();
                    icon_info = gtk_icon_theme_lookup_icon(icon_theme,
                                                           "image-missing",
                                                           256,
                                                           GTK_ICON_LOOKUP_FORCE_SVG);
                    icon_filename = gtk_icon_info_get_filename(icon_info);

					gchar *string;
					string = g_strconcat( _("Slide %i: can't load image %s\n"), i, slide_filename, NULL);
                    img_message(img, string);
					g_free(string);
                    g_free (slide_filename);
                    slide_filename = g_strdup(icon_filename);
                    load_ok = img_scale_image( slide_filename, img->video_ratio,
                                                88, 0, img->distort_images,
                                                img->background_color, &thumb, NULL );

                }
			}
			else
			{
				angle = 0;
				/* We are loading an empty slide */
				gradient = g_key_file_get_integer(img_key_file, conf, "gradient", NULL);
				c_start = g_key_file_get_double_list(img_key_file, conf, "start_color", NULL, NULL);
				c_stop  = g_key_file_get_double_list(img_key_file, conf, "stop_color", NULL, NULL);
				p_start = g_key_file_get_double_list(img_key_file, conf, "start_point", NULL, NULL);
				p_stop = g_key_file_get_double_list(img_key_file, conf, "stop_point", NULL, NULL);
				if (gradient == 4)
				{
					countdown_color = g_key_file_get_double_list(img_key_file, conf, "countdown_color", NULL, NULL);
					if (countdown_color == NULL)
					{
						countdown_color = g_new (gdouble, 3);
						countdown_color[0] = 0.36862745098039218;
						countdown_color[1] = 0.36078431372549019;
						countdown_color[2] = 0.39215686274509803;
					}
					countdown = g_key_file_get_integer(img_key_file, conf, "countdown", NULL);
				}
				/* Create thumbnail */
				load_ok = img_scale_empty_slide( gradient, countdown, p_start, p_stop,
											  c_start, c_stop, 
											  countdown_color, 
											  0,
											  -1,
											  88, 49,
											  &thumb, NULL );
                /* No image is loaded, so img_load_ok is OK if load_ok is */
                img_load_ok = load_ok;
			}

			/* Try to load image. If this fails, skip this slide */
			if( load_ok )
			{
				duration	  = g_key_file_get_double(img_key_file, conf, "duration", NULL);
				flipped		  = g_key_file_get_boolean(img_key_file, conf, "flipped", NULL);
				transition_id = g_key_file_get_integer(img_key_file, conf, "transition_id", NULL);
				speed 		  =	g_key_file_get_integer(img_key_file, conf, "speed",	NULL);

				/* Load the stop points if any */
				no_points	  =	g_key_file_get_integer(img_key_file, conf, "no_points",	NULL);
				if (no_points > 0)
					my_points = g_key_file_get_double_list(img_key_file, conf, "points", &length, NULL);

				/* Load the slide text related data */
				subtitle	  =	g_key_file_get_string (img_key_file, conf, "text",	NULL);
				subtitle_length	  =	g_key_file_get_integer (img_key_file, conf, "text length",	NULL);
				pattern_name  =	g_key_file_get_string (img_key_file, conf, "pattern filename",	NULL);
				anim_id 	  = g_key_file_get_integer(img_key_file, conf, "anim id", 		NULL);
				anim_duration = g_key_file_get_integer(img_key_file, conf, "anim duration",	NULL);
				posx     	  = g_key_file_get_integer(img_key_file, conf, "posX",		NULL);
				posy       	  = g_key_file_get_integer(img_key_file, conf, "posY",		NULL);
				subtitle_angle= g_key_file_get_integer(img_key_file, conf, "subtitle angle",		NULL);
				font_desc     = g_key_file_get_string (img_key_file, conf, "font", 			NULL);
				font_color 	  = g_key_file_get_double_list(img_key_file, conf, "font color", NULL, NULL );
                font_brdr_color  = g_key_file_get_double_list(img_key_file, conf, "font bgcolor", NULL, NULL );
                font_bg_color = g_key_file_get_double_list(img_key_file, conf, "font bgcolor2", NULL, NULL );
                border_color = g_key_file_get_double_list(img_key_file, conf, "border color", NULL, NULL );
                top_border = g_key_file_get_boolean(img_key_file, conf, "top border", NULL);
                bottom_border = g_key_file_get_boolean(img_key_file, conf, "bottom border", NULL);
                border_width = g_key_file_get_integer(img_key_file, conf, "border width", NULL);
                alignment = g_key_file_get_integer(img_key_file, conf, "alignment", NULL);

				/* Get the mem address of the transition */
				spath = (gchar *)g_hash_table_lookup( table, GINT_TO_POINTER( transition_id ) );
				gtk_tree_model_get_iter_from_string( model, &iter, spath );
				gtk_tree_model_get( model, &iter, 2, &render, 0, &pix, -1 );

				slide_info = img_create_new_slide();
				if( slide_info )
				{
					if( slide_filename )
						img_set_slide_file_info( slide_info, original_filename );
					else
					{
						g_return_val_if_fail(c_start && c_stop && p_start && p_stop, FALSE);
						img_set_empty_slide_info( slide_info, gradient, countdown,
													 c_start, c_stop, countdown_color,
													 p_start, p_stop );
					}

                    /* Handle load errors */
                    slide_info->load_ok = img_load_ok;
                    slide_info->original_filename = original_filename;

					
					/* If image has been flipped or rotated, do it now too. */
					if( (flipped || angle) && ! g_strrstr(icon_filename, "image-missing"))
					{
						img_rotate_flip_slide( slide_info, angle, flipped);
						g_object_unref( thumb );
						img_scale_image( slide_info->p_filename, img->video_ratio,
										 88, 0, img->distort_images,
										 img->background_color, &thumb, NULL );
					}

					gtk_list_store_append( img->thumbnail_model, &iter );
					gtk_list_store_set( img->thumbnail_model, &iter,
										0, thumb,
										1, slide_info,
										-1 );
					g_object_unref( G_OBJECT( thumb ) );

					/* Set duration */
					img_set_slide_still_info( slide_info, duration, img );

					/* Set transition */
					img_set_slide_transition_info( slide_info,
												   img->thumbnail_model, &iter,
												   pix, spath, transition_id,
												   render,  img );

					/* Set stop points */
					if( no_points > 0 )
					{
						img_set_slide_ken_burns_info( slide_info, 0,
													  length, my_points );
						g_free( my_points );
					}

					/* Set subtitle */
					if (subtitle)
					{
						gtk_list_store_set( img->thumbnail_model, &iter, 3, TRUE, -1 );
						if ( pattern_name && g_path_is_absolute(pattern_name) == FALSE)
						{
							gchar *_pattern_filename;
							_pattern_filename = g_strconcat(project_current_dir, "/", pattern_name, NULL);
							g_free(pattern_name);
							pattern_name = _pattern_filename;
						}
						/* Does the slide have a foreground color? */
						img_check_for_rtf_colors(img, subtitle);

						if (strstr((const gchar*)subtitle, "GTKTEXTBUFFERCONTENTS-0001"))
						{
							subtitle[26] = (subtitle_length >> 24);
							subtitle[27] = (subtitle_length >> 16) & 0xFF;
							subtitle[28] = (subtitle_length >> 8) & 0xFF;
							subtitle[29] = subtitle_length & 0xFF;

							slide_info->subtitle_length = subtitle_length + 30;
						}
						slide_info->subtitle = (guint8*)subtitle;

						img_set_slide_text_info( slide_info, img->thumbnail_model,
												 &iter, NULL, pattern_name, anim_id,
												 anim_duration, posx, posy, subtitle_angle,
												 font_desc, font_color, font_brdr_color, font_bg_color, border_color, 
												 top_border, bottom_border, border_width, alignment, img );
					}
					/* If we're loading the first slide, apply some of it's
				 	 * data to final pseudo-slide */
					if( first_slide && img->bye_bye_transition)
					{
						first_slide = FALSE;
						img->final_transition.render = slide_info->render;
					}
				}
				if (pix)
					g_object_unref( G_OBJECT( pix ) );
				g_free( font_desc );
			}
			else
            {
				gchar *string;
				string = g_strconcat(_("Can't load image %s\n"), slide_filename, NULL);
				n_invalid++;
				img_message(img, string);
				g_free(string);
            }
			g_free(slide_filename);
			g_free(conf);
		}
	
	img->slides_nr += previous_nr_of_slides - n_invalid;

	img->distort_images = g_key_file_get_boolean( img_key_file,
												  "slideshow settings",
												  "distort images", NULL );
	gtk_icon_view_set_model( GTK_ICON_VIEW( img->thumbnail_iconview ),
							 GTK_TREE_MODEL( img->thumbnail_model ) );
	g_object_unref( G_OBJECT( img->thumbnail_model ) );

	g_key_file_free (img_key_file);
	img_set_total_slideshow_duration(img);

	g_hash_table_destroy( table );
	g_free(project_current_dir);

	gtk_widget_set_valign(img->thumbnail_iconview, GTK_ALIGN_CENTER);
	return TRUE;
}

void
img_load_slideshow( img_window_struct *img, GtkWidget *menu, const gchar *input )
{
	img_close_slideshow(NULL, img);
	if (img_append_slides_from(img, menu, input))
	{
		/* Select the first slide */
		img_goto_first_slide(NULL, img);
		img->project_is_modified = FALSE;

		/* If we made it to here, we succesfully loaded project, so it's safe to set
		 * filename field in global data structure. */
		if( img->project_filename )
			g_free( img->project_filename );
		img->project_filename = g_strdup( input );

		if (img->project_current_dir)
			g_free(img->project_current_dir);
		img->project_current_dir = g_path_get_dirname(input);

		img_refresh_window_title(img);
	}
}

static gboolean img_populate_hash_table( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GHashTable **table )
{
	gint         id;

	gtk_tree_model_get( model, iter, 3, &id, -1 );

	/* Leave out family names, since hey don't get saved. */
	if( ! id )
		return( FALSE );

	/* Freeing of this memory is done automatically when the list gets
	 * destroyed, since we supplied destroy notifier handler. */
	g_hash_table_insert( *table, GINT_TO_POINTER( id ), (gpointer)gtk_tree_path_to_string( path ) );

	return( FALSE );
}

