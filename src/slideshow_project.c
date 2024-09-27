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

#include "slideshow_project.h"

static gboolean img_populate_hash_table( GtkTreeModel *, GtkTreePath *, GtkTreeIter *, GHashTable ** );

void img_save_slideshow( img_window_struct *img,	const gchar *output,  gboolean relative )
{
	GKeyFile *img_key_file;
	gchar *conf, *conf_media, *path, *filename, *file, *font_desc;
	gint count = 0;
	gsize len;
	GtkTreeIter media_iter;
	media_struct *entry;
	GtkTreeModel *media_model;

	media_model = GTK_TREE_MODEL( img->media_model );
	if (!gtk_tree_model_get_iter_first (media_model, &media_iter))
		return;

	img_key_file = g_key_file_new();

	/* Slideshow settings */
	g_key_file_set_comment(img_key_file, NULL, NULL, comment_string, NULL);
	g_key_file_set_integer(img_key_file, "slideshow settings", "video width", img->video_size[0]);
    g_key_file_set_integer(img_key_file, "slideshow settings", "video height", img->video_size[1]);
	g_key_file_set_double_list( img_key_file, "slideshow settings", "background color", img->background_color, 3 );
	g_key_file_set_integer(img_key_file, "slideshow settings", "number of media", img->media_nr);

	/* Media settings */
	count = 0;
	do
	{
		count++;
		gtk_tree_model_get(media_model, &media_iter, 2, &entry, -1);
		conf = g_strdup_printf("media %d",count);
        g_key_file_set_integer( img_key_file, conf, "media_type", entry->media_type );

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
				g_key_file_set_int64(img_key_file, conf, "filesize", 	entry->filesize);
				g_key_file_set_integer(img_key_file, conf, "angle",	entry->angle);
				g_key_file_set_boolean(img_key_file,conf, "flipped",	entry->flipped);
				//thumb = gdk_pixbuf_new_from_file_at_size(entry->full_path, 1, 1, NULL);
				//img_detect_media_orientation_from_pixbuf(thumb, &(entry->flipped), &(entry->angle));
			break;
			
			// Media audio
			case 1:
				g_key_file_set_string(img_key_file, conf, "audio_type", 			entry->audio_type);
				g_key_file_set_string(img_key_file, conf, "audio_duration", 	entry->audio_duration);
				g_key_file_set_int64(img_key_file, conf, "filesize", 					entry->filesize);
				g_key_file_set_integer(img_key_file, conf, "bitrate",			 	entry->bitrate);
				g_key_file_set_integer(img_key_file, conf, "sample_rate",		entry->sample_rate);
				g_key_file_set_string(img_key_file, conf, "metadata", 			entry->metadata);
			break;
			
			// Text
			case 3:
				g_key_file_set_string (img_key_file, conf,"text", (gchar*)entry->subtitle);
				if( entry->pattern_filename )
				{
					if (relative)
					{
						gchar *dummy;
						dummy = g_path_get_basename(entry->pattern_filename);
						g_key_file_set_string (img_key_file, conf,"pattern_filename", dummy);
						g_free(dummy);
					}
					else
						g_key_file_set_string (img_key_file, conf,"pattern_filename", entry->pattern_filename);
				}
				font_desc = pango_font_description_to_string(entry->font_desc);
				g_key_file_set_string (img_key_file, conf,"font",			font_desc);
				g_free(font_desc);
				g_key_file_set_integer(img_key_file,conf, "anim_id",		entry->anim_id);
				g_key_file_set_integer(img_key_file,conf, "anim_duration",	entry->anim_duration);
				g_key_file_set_integer(img_key_file,conf, "posX",		entry->posX);
				g_key_file_set_integer(img_key_file,conf, "posY",		entry->posY);
				g_key_file_set_integer(img_key_file,conf, "subtitle_angle",		entry->subtitle_angle);
				
				g_key_file_set_double_list(img_key_file, conf,"font_color",entry->font_color,4);
				g_key_file_set_double_list(img_key_file, conf,"font_bg_color",entry->font_bg_color,4);
				g_key_file_set_double_list(img_key_file, conf,"font_shadow_color",entry->font_shadow_color,4);
				g_key_file_set_double_list(img_key_file, conf,"font_outline_color",entry->font_outline_color,4);
				g_key_file_set_integer(img_key_file, conf,"alignment",entry->alignment);
			break;
			
			// Transition
			case 4:
				g_key_file_set_integer(img_key_file,conf, "transition_id",	entry->transition_id);
			break;
			
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
	count = 0;

	/* Write the project file */
	conf = g_key_file_to_data(img_key_file, &len, NULL);
	g_file_set_contents( output, conf, len, NULL );
	g_free (conf);

	g_key_file_free(img_key_file);

	if( img->project_filename )
		g_free( img->project_filename );

	img->project_filename = g_strdup(output);
	img->project_is_modified = FALSE;
	img_refresh_window_title(img);
}

gboolean img_append_slides_from( img_window_struct *img, GtkWidget *menuitem, const gchar *input )
{
	GdkPixbuf *thumb;
	media_struct *media;
	GtkTreeIter iter;
	GKeyFile *img_key_file;
	gchar *dummy, *media_filename, *time;
	GtkWidget *dialog, *menu;
	gint i, transition_id, no_points, alignment, previous_nr_of_slides, number,anim_id,anim_duration, posx, posy, gradient = 0, subtitle_length, subtitle_angle, countdown = 0, media_type;
	guint speed;
	GtkTreeModel *model;
	void (*render);
	GHashTable *table;
	gchar      *spath, *conf, *project_current_dir;
	gdouble    duration, *color, *font_color, *font_bg_color, *font_shadow_color, *font_outline_color;
	gboolean   first_slide = TRUE;
	gchar *subtitle = NULL, *pattern_name = NULL, *font_desc;
	gdouble *my_points = NULL, *p_start = NULL, *p_stop = NULL, *c_start = NULL, *c_stop = NULL, *countdown_color = NULL;
	gsize length;
	GdkPixbuf *pix = NULL;
    gboolean  flipped, no_use;
	gchar *original_filename = NULL;
	GtkIconTheme *icon_theme;
	GtkIconInfo  *icon_info;
	GFile *file;
	GFileInfo *file_info;
	const gchar  *icon_filename;
	const gchar *content_type;
	gchar *mime_type, *full_path = NULL;
	ImgAngle   angle = 0;
	GString *media_not_found = NULL;

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
			if (img_ask_user_confirmation(img, _("The file doesn't exist anymore on the disk.\nDo you want to remove it from the list?")))
				gtk_widget_destroy(menuitem);
		}
		else
			img_message(img, _("Error: Project file doesn't exist") );
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
		g_key_file_free (img_key_file);
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
	g_object_ref( G_OBJECT( img->media_model ) );
	gtk_icon_view_set_model(GTK_ICON_VIEW(img->media_iconview), NULL);
		
	/* Load project backgroud color */
	color = g_key_file_get_double_list( img_key_file, "slideshow settings",	"background color", NULL, NULL );
	img->background_color[0] = color[0];
	img->background_color[1] = color[1];
	img->background_color[2] = color[2];
	g_free( color );

	/* Loads the media files */
	number = g_key_file_get_integer( img_key_file, "slideshow settings",  "number of media", NULL);
	
	previous_nr_of_slides = img->media_nr;
	img->media_nr = number;
	
	for( i = 1; i <= number ; i++ )
	{
		conf = g_strdup_printf("media %d", i);
		//~ if ( !g_key_file_has_group(img_key_file, conf))
		//~ {
			//~ img->media_nr--;
			//~ continue;
		//~ }
		media_type = g_key_file_get_integer(img_key_file, conf, "media_type", NULL);
		if (media_type == 0 || media_type == 1 || media_type == 2);
			media_filename = g_key_file_get_string(img_key_file,conf, "filename", NULL);

		if(media_filename)
		{
			if ( g_path_is_absolute(media_filename) == FALSE)
			{
				full_path = g_strconcat(project_current_dir, "/", media_filename, NULL);
				//Let's chech if the media is found
				if ( ! g_file_test (full_path, G_FILE_TEST_EXISTS))
				{
					gchar *msg = g_strdup_printf(_("Media %d: <b>%s</b> couldn't be found\n"), i, full_path);
					g_string_append(media_not_found, msg);
					previous_nr_of_slides--;
					continue;
				}
			}
		}
		/* Create the media structure */
		media = img_create_new_media();
		media->media_type = media_type;
		
		if (full_path || media_filename)
			media->full_path = full_path ? full_path : media_filename;

		switch (media->media_type)
		{
			// Media image
			case 0:
				media->width 			= g_key_file_get_integer( img_key_file, conf, "width", NULL );
				media->height 			= g_key_file_get_integer( img_key_file, conf, "height", NULL );
				media->image_type = g_key_file_get_string( img_key_file, conf, "image_type", NULL );
				media->filesize 		= g_key_file_get_int64( img_key_file, conf, "filesize", NULL );
				media->angle 			= g_key_file_get_integer( img_key_file, conf, "angle", NULL );
				media->flipped			= g_key_file_get_boolean( img_key_file, conf, "flipped", NULL );
				no_use = img_add_media_widget_area(media, media->full_path, img);
			break;
			
			//Media audio
			case 1:
				gchar *metadata = NULL;
				media->audio_type 			= g_key_file_get_string( img_key_file, conf, "audio_type", NULL );
				media->filesize 				= g_key_file_get_int64( img_key_file, conf, "filesize", NULL );
				media->audio_duration	= g_key_file_get_string( img_key_file, conf, "audio_duration", NULL );
				media->bitrate 				= g_key_file_get_integer( img_key_file, conf, "bitrate", NULL );
				media->sample_rate		= g_key_file_get_integer( img_key_file, conf, "sample_rate", NULL );
				metadata							= g_key_file_get_string(img_key_file, conf, "metadata", NULL);
				g_stpcpy(media->metadata, metadata);
				no_use = img_add_media_widget_area(media, media->full_path, img);
			break;
			
			// Text
			case 3:
				media->subtitle				=	g_key_file_get_string (img_key_file, conf, "text",	NULL);
				media->pattern_filename=	g_key_file_get_string (img_key_file, conf, "pattern filename",	NULL);
				media->anim_id 	  			= g_key_file_get_integer(img_key_file, conf, "anim id", 		NULL);
				media->anim_duration 	= g_key_file_get_integer(img_key_file, conf, "anim duration",	NULL);
				media->posX     	  			= g_key_file_get_integer(img_key_file, conf, "posX",		NULL);
				media->posY       	  			= g_key_file_get_integer(img_key_file, conf, "posY",		NULL);
				media->subtitle_angle		= g_key_file_get_integer(img_key_file, conf, "subtitle angle",		NULL);
				font_desc				     		= g_key_file_get_string (img_key_file, conf, "font", NULL);
				media->font_desc			= pango_font_description_from_string(font_desc);
				g_free(font_desc);
				my_points				 	  		= g_key_file_get_double_list(img_key_file, conf, "font color", NULL, NULL );
                for( i = 0; i < 3; i++ )
					media->font_color[i] = my_points[i];
                my_points				 	  		= g_key_file_get_double_list(img_key_file, conf, "font bgcolor", NULL, NULL );
                for( i = 0; i < 3; i++ )
					media->font_bg_color[i] = my_points[i];
                my_points							= g_key_file_get_double_list(img_key_file, conf, "font shadow color", NULL, NULL );
                for( i = 0; i < 3; i++ )
					media->font_shadow_color[i] = my_points[i];
                my_points						= g_key_file_get_double_list(img_key_file, conf, "font outline color", NULL, NULL );
                for( i = 0; i < 3; i++ )
					media->font_outline_color[i] = my_points[i];
                media->alignment 				= g_key_file_get_integer(img_key_file, conf, "alignment", NULL);
			break;
			
			// Transition
			case 4:
				media->duration	 		= g_key_file_get_double(img_key_file, conf, "duration", NULL);
				media->transition_id 	= g_key_file_get_integer(img_key_file, conf, "transition_id", NULL);
			break;
			
				//~ load_ok = img_scale_image( original_filename, img->video_ratio,
										   //~ 88, 0, FALSE,
										   //~ img->background_color, &thumb, NULL );
                //~ img_load_ok = load_ok;
                //~ if (! load_ok)
                //~ {
                    //~ icon_theme = gtk_icon_theme_get_default();
                    //~ icon_info = gtk_icon_theme_lookup_icon(icon_theme,
                                                           //~ "image-missing",
                                                           //~ 256,
                                                           //~ GTK_ICON_LOOKUP_FORCE_SVG);
                    //~ icon_filename = gtk_icon_info_get_filename(icon_info);

					//~ gchar *string;
					//~ string = g_strconcat( _("Media %i: can't load image %s\n"), i, media_filename, NULL);
                    //~ img_message(img, string);
					//~ g_free(string);
                    //~ g_free (media_filename);
                    //~ media_filename = g_strdup(icon_filename);
                    //~ load_ok = img_scale_image( media_filename, img->video_ratio,
                                                //~ 88, 0, FALSE,
                                                //~ img->background_color, &thumb, NULL );

                //~ }
			
			//~ else
			//~ {
				//~ angle = 0;
				//~ /* We are loading an empty slide */
				//~ gradient = g_key_file_get_integer(img_key_file, conf, "gradient", NULL);
				//~ c_start = g_key_file_get_double_list(img_key_file, conf, "start_color", NULL, NULL);
				//~ c_stop  = g_key_file_get_double_list(img_key_file, conf, "stop_color", NULL, NULL);
				//~ p_start = g_key_file_get_double_list(img_key_file, conf, "start_point", NULL, NULL);
				//~ p_stop = g_key_file_get_double_list(img_key_file, conf, "stop_point", NULL, NULL);
				//~ if (gradient == 4)
				//~ {
					//~ countdown_color = g_key_file_get_double_list(img_key_file, conf, "countdown_color", NULL, NULL);
					//~ if (countdown_color == NULL)
					//~ {
						//~ countdown_color = g_new (gdouble, 3);
						//~ countdown_color[0] = 0.36862745098039218;
						//~ countdown_color[1] = 0.36078431372549019;
						//~ countdown_color[2] = 0.39215686274509803;
					//~ }
					//~ countdown = g_key_file_get_integer(img_key_file, conf, "countdown", NULL);
				//~ }
				//~ /* Create thumbnail */
				//~ load_ok = img_scale_empty_slide( gradient, countdown, p_start, p_stop,
											  //~ c_start, c_stop, 
											  //~ countdown_color, 
											  //~ 0,
											  //~ -1,
											  //~ 88, 49,
											  //~ &thumb, NULL );
                //~ /* No image is loaded, so img_load_ok is OK if load_ok is */
                //~ img_load_ok = load_ok;
			//~ }

			/* Try to load image. If this fails, skip this slide */
			//~ if( load_ok )
			//~ {
				

				/* Load the stop points if any */
				//~ no_points	  =	g_key_file_get_integer(img_key_file, conf, "no_points",	NULL);
				//~ if (no_points > 0)
					//~ my_points = g_key_file_get_double_list(img_key_file, conf, "points", &length, NULL);

				//~ /* Get the mem address of the transition */
				//~ spath = (gchar *)g_hash_table_lookup( table, GINT_TO_POINTER( transition_id ) );
				//~ gtk_tree_model_get_iter_from_string( model, &iter, spath );
				//~ gtk_tree_model_get( model, &iter, 2, &render, 0, &pix, -1 );
				
				

                  //~ /* If image has been flipped or rotated, do it now too. */
					//~ if( (flipped || angle) && ! g_strrstr(icon_filename, "image-missing"))
					//~ {
						//~ img_rotate_flip_slide( media, angle, flipped);
						//~ g_object_unref( thumb );
						//~ img_scale_image( media->full_path, img->video_ratio,
										 //~ 88, 0, FALSE,
										 //~ img->background_color, &thumb, NULL );
					//~ }

									/* Set duration */
					//img_set_slide_still_info( media, duration, img );

					/* Set transition */
					//~ img_set_slide_transition_info( media,
												   //~ img->thumbnail_model, &iter,
												   //~ pix, spath, transition_id,
												   //~ render,  img );

					/* Set stop points */
					//~ if( no_points > 0 )
					//~ {
						//~ img_set_slide_ken_burns_info( media, 0,
													  //~ length, my_points );
						//~ g_free( my_points );
					//~ }

					/* Set subtitle */
					//~ if (subtitle)
					//~ {
						//~ if ( pattern_name && g_path_is_absolute(pattern_name) == FALSE)
						//~ {
							//~ gchar *_pattern_filename;
							//~ _pattern_filename = g_strconcat(project_current_dir, "/", pattern_name, NULL);
							//~ g_free(pattern_name);
							//~ pattern_name = _pattern_filename;
						//~ }

						//~ img_set_slide_text_info( media, img->thumbnail_model,
												 //~ &iter, NULL, pattern_name, anim_id,
												 //~ anim_duration, posx, posy, subtitle_angle,
												 //~ font_desc, font_color, font_bg_color, font_shadow_color, font_outline_color, 
												//~ alignment, img);
				
			
				//~ if (pix)
					//~ g_object_unref( G_OBJECT( pix ) );
				//~ g_free( font_desc );
			//~ }
			//~ else
            //~ {
				//~ gchar *string;
				//~ string = g_strconcat(_("Can't load image %s\n"), media_filename, NULL);
				//~ img_message(img, string);
				//~ g_free(string);
            //~ }
			g_free(media_filename);
			g_free(conf);
		} //End switch media type
	}
	
	img->media_nr += previous_nr_of_slides;
	
	// If some media were not found display an error dialog
	if (media_not_found->len > 0)
	{
		img_message(img, media_not_found->str);
		g_string_free(media_not_found, TRUE);
	}
	
	// Reinstate the model
	gtk_icon_view_set_model(GTK_ICON_VIEW(img->media_iconview), GTK_TREE_MODEL(img->media_model) );
	g_object_unref( G_OBJECT( img->media_model ) );

	g_key_file_free (img_key_file);

	g_hash_table_destroy( table );
	g_free(project_current_dir);
	
	return TRUE;
}

void
img_load_slideshow( img_window_struct *img, GtkWidget *menu, const gchar *input )
{
	if (img->media_nr > 0)
		img_close_slideshow(NULL, img);

	if (img_append_slides_from(img, menu, input))
	{
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

