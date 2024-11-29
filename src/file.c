/*
 *  Copyright (C) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
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

#include "file.h"

static gboolean img_populate_hash_table( GtkTreeModel *, GtkTreePath *, GtkTreeIter *, GHashTable ** );

void img_save_project( img_window_struct *img,	const gchar *output,  gboolean relative )
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	GKeyFile *img_key_file;
	gchar *conf, *conf_media, *path, *filename, *file, *font_desc;
	gsize len;
	GtkTreeIter media_iter;
	GtkTreeModel *media_model;
	GtkAllocation allocation;
	Track *track;
	media_struct *entry;
	media_timeline *item;
	
	media_model = GTK_TREE_MODEL( img->media_model );
	if (!gtk_tree_model_get_iter_first (media_model, &media_iter))
		return;

	gtk_widget_get_allocation(img->image_area, &allocation);
	img_key_file = g_key_file_new();

	/* Slideshow settings */
	g_key_file_set_comment(img_key_file, NULL, NULL, comment_string, NULL);
	g_key_file_set_integer(img_key_file, "project settings", "video width", img->video_size[0]);
    g_key_file_set_integer(img_key_file, "project settings", "video height", img->video_size[1]);
    g_key_file_set_integer(img_key_file, "project settings", "image area width", allocation.width);
    g_key_file_set_integer(img_key_file, "project settings", "image area height", allocation.height);
	g_key_file_set_double_list( img_key_file, "project settings", "background color", img->background_color, 3 );
	g_key_file_set_integer(img_key_file, "project settings", "number of media", img->media_nr);
	g_key_file_set_integer(img_key_file, "project settings", "number of tracks", priv->tracks->len);

	/* Media settings */
	do
	{
		gtk_tree_model_get(media_model, &media_iter, 2, &entry, -1);
		conf = g_strdup_printf("media %d",entry->id);

        g_key_file_set_integer( img_key_file, conf, "media_type", entry->media_type);
        g_key_file_set_integer( img_key_file, conf, "id", entry->id);

        if (entry->media_type == 0 || entry->media_type == 1 || entry->media_type == 2)
            filename = g_strdup(entry->full_path);

		if (relative && filename)
		{
			gchar *_filename;
			_filename = g_path_get_basename(entry->full_path);
			filename = _filename;
		}
        if (filename)
			g_key_file_set_string( img_key_file, conf, "filename", filename);

		g_free(filename);
		//~ else
		//~ {
			//~ /* We are dealing with a media type text or transition */
			//~ g_key_file_set_integer(img_key_file, conf, "gradient",	entry->gradient);
			//~ g_key_file_set_double_list(img_key_file, conf, "start_color", entry->g_start_color, 3 );
			//~ g_key_file_set_double_list(img_key_file, conf, "stop_color" , entry->g_stop_color , 3 );
			//~ g_key_file_set_double_list(img_key_file, conf, "start_point", entry->g_start_point, 2 );
			//~ g_key_file_set_double_list(img_key_file, conf, "stop_point" , entry->g_stop_point, 2 );
			//~ if (entry->gradient == 4)
			//~ {
					//~ g_key_file_set_double_list(img_key_file, conf, "countdown_color", entry->countdown_color, 3 );
					//~ g_key_file_set_integer(img_key_file, conf, "countdown",	entry->countdown);
			//~ }
		switch(entry->media_type)
		{
			// Media image
			case 0:
				g_key_file_set_integer(img_key_file, conf, "width",	entry->width);
				g_key_file_set_integer(img_key_file, conf, "height",	entry->height);
				g_key_file_set_string(img_key_file, conf, "image_type", 	entry->image_type);
				//thumb = gdk_pixbuf_new_from_file_at_size(entry->full_path, 1, 1, NULL);
				//img_detect_media_orientation_from_pixbuf(thumb, &(entry->flipped), &(entry->angle));
			break;
			
			// Media audio
			case 1:
			
			break;
			
			//~ // Text
			//~ case 3:
				//~ g_key_file_set_string (img_key_file, conf,"text", (gchar*)entry->text);
				//~ if( entry->pattern_filename )
				//~ {
					//~ if (relative)
					//~ {
						//~ gchar *dummy;
						//~ dummy = g_path_get_basename(entry->pattern_filename);
						//~ g_key_file_set_string (img_key_file, conf,"pattern_filename", dummy);
						//~ g_free(dummy);
					//~ }
					//~ else
						//~ g_key_file_set_string (img_key_file, conf,"pattern_filename", entry->pattern_filename);
				//~ }
				//~ font_desc = pango_font_description_to_string(entry->font_desc);
				//~ g_key_file_set_string (img_key_file, conf,"font",			font_desc);
				//~ g_free(font_desc);
				//~ g_key_file_set_integer(img_key_file,conf, "anim_id",		entry->anim_id);
				//~ g_key_file_set_integer(img_key_file,conf, "anim_duration",	entry->anim_duration);
				//~ g_key_file_set_integer(img_key_file,conf, "posX",		entry->posX);
				//~ g_key_file_set_integer(img_key_file,conf, "posY",		entry->posY);
				//~ g_key_file_set_integer(img_key_file,conf, "subtitle_angle",		entry->subtitle_angle);
				
				//~ g_key_file_set_double_list(img_key_file, conf,"font_color",entry->font_color,4);
				//~ g_key_file_set_double_list(img_key_file, conf,"font_bg_color",entry->font_bg_color,4);
				//~ g_key_file_set_double_list(img_key_file, conf,"font_shadow_color",entry->font_shadow_color,4);
				//~ g_key_file_set_double_list(img_key_file, conf,"font_outline_color",entry->font_outline_color,4);
				//~ g_key_file_set_integer(img_key_file, conf,"alignment",entry->alignment);
			//~ break;			
		} // End swith entry->media type
	
		/* Stop points */
		//~ g_key_file_set_integer(img_key_file,conf, "no_points",		entry->no_points);
		//~ if (entry->no_points > 0)
		//~ {
			//~ gint    point_counter;
			//~ gdouble my_points[entry->no_points * 4];

			//~ for( point_counter = 0;
				 //~ point_counter < entry->no_points;
				 //~ point_counter++ )
			//~ {
				//~ ImgStopPoint *my_point = g_list_nth_data(entry->points,point_counter);
				//~ my_points[ (point_counter * 4) + 0] = (gdouble)my_point->time;
				//~ my_points[ (point_counter * 4) + 1] = my_point->offx;
				//~ my_points[ (point_counter * 4) + 2] = my_point->offy;
				//~ my_points[ (point_counter * 4) + 3] = my_point->zoom;
			//~ }
			//~ g_key_file_set_double_list(img_key_file,conf, "points", my_points, (gsize) entry->no_points * 4);
		//~ }

		g_free(conf);
	}
	while (gtk_tree_model_iter_next (media_model,&media_iter));

	/* Media on the timeline settings */
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		conf = g_strdup_printf("track %d", i);
		track = g_array_index(priv->tracks, Track *, i);
		g_key_file_set_integer(img_key_file, conf, "track_type", track->type);
		g_key_file_set_boolean(img_key_file, conf, "is_default", track->is_default);
		if (track->items)
		{
			GString *string_values = g_string_new(NULL);
			if (track->items->len > 0)
			{
				for (gint q = 0; q < track->items->len; q++)
				{
					item = g_array_index(track->items, media_timeline  *, q);
					if (item->media_type == 0)
						g_string_append_printf(string_values, "%d;%2.2f;%2.2f;%d;%2.2f;%2.2f;%2.2f;%d;%d;%d;%d;",
						item->id, item->start_time, item->duration, item->transition_id, item->opacity, item->x, item->y, item->color_filter, item->nr_rotations,item->flipped_horizontally, item->flipped_vertically);
					else
						g_string_append_printf(string_values, "%d;%2.2f;%2.2f;", item->id, item->start_time, item->duration);
				}
				g_key_file_set_string(img_key_file, conf, "media_sequence", string_values->str);
			}
			g_string_free(string_values, TRUE);
		}
		g_free(conf);
	}

	/* Write the project file */
	conf = g_key_file_to_data(img_key_file, &len, NULL);
	g_file_set_contents( output, conf, len, NULL );
	g_free (conf);

	g_key_file_free(img_key_file);

	if(img->project_filename)
		g_free(img->project_filename);

	img->project_filename = g_strdup(output);
	img->project_is_modified = FALSE;
	img_refresh_window_title(img);
}

void img_load_project( img_window_struct *img, GtkWidget *menuitem, const gchar *input )
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);

	GdkPixbuf 				*thumb, *pix = NULL;
	Track						*track;
	media_struct 		*media;
	media_timeline 	*item;
	GtkTreeIter 				parent_iter, iter;
	GKeyFile 					*img_key_file;
	gchar 						*dummy, *media_filename, *time, *mime_type, *full_path = NULL, *subtitle = NULL, *pattern_name = NULL, *font_desc, *background_color;
	gchar      					*spath, *conf, *conf2, *project_current_dir, *value_string, *trans_group;
	gchar						**groups, **values;
	GtkWidget 				*dialog, *menu;
	gint							track_nr, transition_id, no_points, alignment, number,anim_id,anim_duration, posy, gradient = 0, subtitle_length, subtitle_angle;
	gint							step, countdown = 0, media_type, media_id, image_area_width, image_area_height;
	GtkTreeModel 		*model;
	void 						(*render);
	GHashTable 			*table;
	gdouble    				posx, width, duration, *color, *font_color, *font_bg_color, *font_shadow_color, *font_outline_color;
	gdouble					*my_points = NULL, *p_start = NULL, *p_stop = NULL, *c_start = NULL, *c_stop = NULL, *countdown_color = NULL;
	gsize						length, num_keys;
    gboolean  				flipped, no_use, is_default;
	GtkIconTheme 		*icon_theme;
	GtkIconInfo  			*icon_info;
	GFile 						*file;
	GFileInfo 				*file_info;
	const gchar  			*icon_filename;
	const gchar 			*content_type;
	GString 					*media_not_found = NULL;
	GtkAllocation			allocation;
	
	if (img->media_nr > 0)
		img_close_project(NULL, img);
	
	media_not_found = g_string_new(NULL);
	
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
			if (img_ask_user_confirmation(img, _("The project file doesn't exist anymore on the disk.\nDo you want to remove it from the list?")))
				gtk_widget_destroy(menuitem);
		}
		else
			img_message(img, _("Error: project file doesn't exist") );
		return;
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
		g_key_file_free (img_key_file);
		return;
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
	table = g_hash_table_new_full( g_direct_hash, g_direct_equal, NULL, g_free);
	model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->transition_type));
	gtk_tree_model_foreach( model, (GtkTreeModelForeachFunc)img_populate_hash_table, &table);

	/* Video Size */
	img->video_size[0] = g_key_file_get_integer(img_key_file, "project settings", "video width", NULL);
	img->video_size[1] = g_key_file_get_integer(img_key_file, "project settings", "video height", NULL);
	image_area_width =  g_key_file_get_integer(img_key_file, "project settings", "image area width", NULL);
	image_area_height =  g_key_file_get_integer(img_key_file, "project settings", "image area height", NULL);
	
	gtk_widget_set_size_request(img->image_area, image_area_width, image_area_height);
	gtk_widget_get_allocation(img->image_area, &allocation);
	
	if (img->video_size[0] == 0)
		img->video_size[0] = 1280;
	
	if (img->video_size[1] == 0)
		img->video_size[1] = 720;
		
	img->video_ratio = (gdouble)img->video_size[0] / img->video_size[1];

   	/* Make loading more efficient by removing model from icon view */
	g_object_ref( G_OBJECT( img->media_model ) );
	gtk_icon_view_set_model(GTK_ICON_VIEW(img->media_iconview), NULL);
		
	/* Load project backgroud color */
	color = g_key_file_get_double_list( img_key_file, "project settings",	"background color", NULL, NULL );
	img->background_color[0] = color[0];
	img->background_color[1] = color[1];
	img->background_color[2] = color[2];
	g_free( color );

	/* Loads the media files */
	number = 	g_key_file_get_integer( img_key_file, "project settings",  "number of media", NULL);
	track_nr =	g_key_file_get_integer( img_key_file, "project settings",  "number of tracks", NULL);
	
    //groups = g_key_file_get_groups(img_key_file, &length);
    
	for( gint i = 1; i <= number ; i++ )
	{
		conf = g_strdup_printf("media %d", i);
		if ( ! g_key_file_has_group(img_key_file, conf))
		{
			number++;
			g_free(conf);
			continue;
		}
		
		media_id = g_key_file_get_integer(img_key_file, conf, "id", NULL);
		media_type = g_key_file_get_integer(img_key_file, conf, "media_type", NULL);
		if (media_type == 0 || media_type == 1 || media_type == 2);
			media_filename = g_key_file_get_string(img_key_file,conf, "filename", NULL);
			
		if(media_filename)
		{
			//Let's check if the media is found
			if ( ! g_file_test (media_filename, G_FILE_TEST_EXISTS))
			{
				gchar *msg = g_strdup_printf(_("Media %d: <b>%s</b> couldn't be found\n"), i, media_filename);
				g_string_append(media_not_found, msg);
				g_free(media_filename);
				continue;
			}
		}
		/* Create the media structure */
		media = g_new0(media_struct, 1);
		media->id = media_id;
		media->media_type = media_type;
		media->full_path = g_strdup(media_filename);
		g_free(media_filename);
		img->media_nr++;
		switch (media->media_type)
		{
			// Media image
			case 0:
				media->width 			= g_key_file_get_integer( img_key_file, conf, "width", NULL );
				media->height 			= g_key_file_get_integer( img_key_file, conf, "height", NULL );
				media->image_type 	= g_key_file_get_string( img_key_file, conf, "image_type", NULL );
				img_add_media(media->full_path, media, img);
			break;
			
			//Media audio
			case 1:
				img_add_media(media->full_path, media, img);
			break;
			
			//~ // Text
			//~ case 3:
				//~ media->text						=	g_key_file_get_string (img_key_file, conf, "text",	NULL);
				//~ media->pattern_filename	=	g_key_file_get_string (img_key_file, conf, "pattern filename",	NULL);
				//~ media->anim_id 	  			= g_key_file_get_integer(img_key_file, conf, "anim id", 		NULL);
				//~ media->anim_duration 		= g_key_file_get_integer(img_key_file, conf, "anim duration",	NULL);
				//~ media->posX     	  				= g_key_file_get_integer(img_key_file, conf, "posX",		NULL);
				//~ media->posY       	  			= g_key_file_get_integer(img_key_file, conf, "posY",		NULL);
				//~ media->subtitle_angle		= g_key_file_get_integer(img_key_file, conf, "subtitle angle",		NULL);
				//~ font_desc				     		= g_key_file_get_string (img_key_file, conf, "font", NULL);
				//~ media->font_desc				= pango_font_description_from_string(font_desc);
				//~ g_free(font_desc);
				//~ my_points				 	  		= g_key_file_get_double_list(img_key_file, conf, "font color", NULL, NULL );
                //~ for( i = 0; i < 3; i++ )
					//~ media->font_color[i] = my_points[i];
                //~ my_points				 	  		= g_key_file_get_double_list(img_key_file, conf, "font bgcolor", NULL, NULL );
                //~ for( i = 0; i < 3; i++ )
					//~ media->font_bg_color[i] = my_points[i];
                //~ my_points							= g_key_file_get_double_list(img_key_file, conf, "font shadow color", NULL, NULL );
                //~ for( i = 0; i < 3; i++ )
					//~ media->font_shadow_color[i] = my_points[i];
                //~ my_points						= g_key_file_get_double_list(img_key_file, conf, "font outline color", NULL, NULL );
                //~ for( i = 0; i < 3; i++ )
					//~ media->font_outline_color[i] = my_points[i];
                //~ media->alignment 				= g_key_file_get_integer(img_key_file, conf, "alignment", NULL);
			//~ break;

		} //End switch media type
		g_free(conf);
	}
	img->next_id = ++number;
	
	// If some media were not found display an error dialog
	if (media_not_found->len > 0)
		img_message(img, media_not_found->str);

	g_string_free(media_not_found, TRUE);

	// Add the additional tracks first to avoid messing up the pointers with the track sorting by type
	if (track_nr > 2)
	{
		for (gint i=0; i < track_nr; i++)
		{
			track = g_array_index(priv->tracks, Track *, i);
			conf = g_strdup_printf("track %d",  i);
			number = g_key_file_get_integer( img_key_file, conf, "track_type", NULL);
			is_default = g_key_file_get_boolean( img_key_file, conf, "is_default", NULL);
			if (is_default == FALSE)
			{
				background_color = (number == 0) ? "#CCCCFF" : "#d6d1cd";
				img_timeline_add_track(img->timeline, number, background_color); 
			}
			g_free(conf);
		}
	}
	//Finally add the media on the timeline
	posy = 32;
	for (gint i=0; i < track_nr; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		conf = g_strdup_printf("track %d",  i);
		value_string = g_key_file_get_string(img_key_file, conf, "media_sequence", NULL);
		// If no media are placed on track process next one
		if (value_string == NULL)
			goto next;

		values = g_strsplit(value_string, ";", -1);
		number = g_strv_length(values);
		step = (track->type == 0) ? 11 : 3;
		for (gint q = 0; q < number -2; q += step)
		{
			item = g_new0(media_timeline, 1);
			item->id 				=	g_ascii_strtoll(values[q+0], NULL, 10);
			item->start_time 	=	g_ascii_strtod(values[q+1], NULL);
			item->duration 		=	g_ascii_strtod(values[q+2], NULL);
			if (track->type == 0)
			{
				item->transition_id=	g_ascii_strtoll(values[q+3], NULL, 10);
				item->opacity		=	g_ascii_strtod(values[q+4], NULL);
				item->x					=	g_ascii_strtod(values[q+5], NULL);
				item->y					=	g_ascii_strtod(values[q+6], NULL);
				item->color_filter	=	g_ascii_strtoll(values[q+7], NULL, 10);
				item->nr_rotations	=	g_ascii_strtoll(values[q+8], NULL, 10);
				item->flipped_horizontally	=	g_ascii_strtoll(values[q+9], NULL, 10);
				item->flipped_vertically		=	g_ascii_strtoll(values[q+10], NULL, 10);
			}
			// Read the media filename and media type again to create the image in the toggle button
			conf2 = g_strdup_printf("media %d", item->id);
			media_filename = g_key_file_get_string(img_key_file, conf2, "filename",  NULL);
			media_type = g_key_file_get_integer(img_key_file, conf2, "media_type",  NULL);
			item->media_type = media_type;
			if (media_type == 0)
			{
				spath = (gchar *)g_hash_table_lookup(table, GINT_TO_POINTER(item->transition_id));
				gtk_tree_model_get_iter_from_string(model, &iter, spath);
				gtk_tree_model_get(model, &iter, 2, &render, 0, &pix, -1);
				item->render = render;
				item->tree_path = g_strdup(spath);
				img_create_cached_cairo_surface(img, item, media_filename);

				if(gtk_tree_model_iter_parent(model, &parent_iter, &iter))
				{
					gtk_tree_model_get(model, &parent_iter, 1, &trans_group, -1);
					item->trans_group = g_strdup(trans_group);
					g_free(trans_group);
				}
			}
			g_free(conf2);

			img_timeline_create_toggle_button(item, media_type, media_filename, img);
			g_free(media_filename);
				
			//Position the toggle button in the timeline
			width = item->duration * BASE_SCALE * priv->zoom_scale;
			gtk_widget_set_size_request(item->button, width, 50);
			posx = item->start_time * BASE_SCALE *priv->zoom_scale;
			gtk_layout_move(GTK_LAYOUT(img->timeline), item->button, posx, posy);
			item->old_x = posx;
			item->timeline_y = posy;
			item->is_positioned = TRUE;
			item->last_allocation_width = allocation.width;
			item->last_allocation_height = allocation.height;

			// Apply image filter
			img_apply_filter_on_surface(img, item, item->color_filter);

			// Apply rotations
			for (int r =0; r < item->nr_rotations; r++)
				img_rotate_surface(img, item, FALSE);
			
			// Apply flipping
			if (item->flipped_horizontally)
				img_flip_surface_horizontally(img, item);				
			if (item->flipped_vertically)
				img_flip_surface_vertically(img, item);

			g_array_append_val(track->items, item);
			
			// Make the first picture media the current one
			if (q == 0 && i == 0)
				img->current_item = item;
		}
		g_strfreev(values);
		g_free(value_string);
next:
		posy += TRACK_HEIGHT + TRACK_GAP;
		g_free(conf);
	}

	// Reinstate the model
	gtk_icon_view_set_model(GTK_ICON_VIEW(img->media_iconview), GTK_TREE_MODEL(img->media_model) );
	g_object_unref( G_OBJECT( img->media_model ) );

	g_key_file_free(img_key_file);
	g_hash_table_destroy(table);

	if( img->project_filename)
			g_free( img->project_filename);

	img->project_filename = g_strdup(input);

	if (img->project_current_dir)
			g_free(img->project_current_dir);

	img->project_current_dir = project_current_dir;
	img_refresh_window_title(img);
	
	gtk_widget_queue_draw(img->image_area);

	gint unused = img_timeline_get_final_time(img);
}

static gboolean img_populate_hash_table( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GHashTable **table )
{
	gint         id;

	gtk_tree_model_get( model, iter, 3, &id, -1 );

	/* Leave out family names, since they don't get saved. */
	if( ! id )
		return( FALSE );

	/* Freeing of this memory is done automatically when the list gets
	 * destroyed, since we supplied destroy notifier handler. */
	g_hash_table_insert( *table, GINT_TO_POINTER( id ), (gpointer)gtk_tree_path_to_string( path ) );

	return( FALSE );
}

