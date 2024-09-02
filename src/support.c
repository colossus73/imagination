/*
 *  Copyright (c) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
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

#include "support.h"
#include <glib/gstdio.h> //This is for g_unlink

static gboolean img_plugin_is_loaded(img_window_struct *, GModule *);

GtkWidget *img_load_icon(gchar *filename, GtkIconSize size)
{
    GtkWidget *file_image;
	gchar *path;
	GdkPixbuf *file_pixbuf = NULL;

	path = g_strconcat(DATADIR, "/imagination/pixmaps/",filename,NULL);
	file_pixbuf = gdk_pixbuf_new_from_file(path,NULL);
	g_free (path);

	if (file_pixbuf == NULL)
		file_image = gtk_image_new_from_icon_name("image-missing", size);
    else
	{
		file_image = gtk_image_new_from_pixbuf(file_pixbuf);
	    g_object_unref (file_pixbuf);
	}
    return file_image;
}

gchar *img_convert_seconds_to_time(gint total_secs)
{
	gint h, m, s;

	h =  total_secs / 3600;
	m = (total_secs % 3600) / 60;
	s =  total_secs - (h * 3600) - (m * 60);
	return g_strdup_printf("%02d:%02d:%02d", h, m, s);
}

static void
sens_cell_func( GtkCellLayout   * UNUSED(layout),
				GtkCellRenderer *cell,
				GtkTreeModel    *model,
				GtkTreeIter     *iter,
				gpointer         UNUSED(data) )
{
	gboolean sensitive = ! gtk_tree_model_iter_has_child( model, iter );
	g_object_set( cell, "sensitive", sensitive, NULL );
}

GtkWidget *_gtk_combo_box_new_text(gboolean pointer)
{
	GtkWidget *combo_box;
	GtkCellRenderer *cell;
	GtkListStore *list;
	GtkTreeStore *tree;
	GtkTreeModel *model;

	if (pointer)
	{
		tree = gtk_tree_store_new( 6, GDK_TYPE_PIXBUF,
									  G_TYPE_STRING,
									  G_TYPE_POINTER,
									  G_TYPE_INT,
									  GDK_TYPE_PIXBUF_ANIMATION,
									  G_TYPE_BOOLEAN );
		model = GTK_TREE_MODEL( tree );

		combo_box = gtk_combo_box_new_with_model (model);
		g_object_unref (G_OBJECT( model ));
		cell = img_cell_renderer_anim_new ();
		gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo_box ), cell, FALSE );
		gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT (combo_box), cell,
										"anim", 4,
										NULL );
		gtk_cell_layout_set_cell_data_func( GTK_CELL_LAYOUT( combo_box ), cell,
											sens_cell_func, NULL, NULL );
		
		cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
										"text", 1,
										NULL);
		gtk_cell_layout_set_cell_data_func( GTK_CELL_LAYOUT( combo_box ), cell,
											sens_cell_func, NULL, NULL );
		g_object_set(cell, "ypad", (guint)0, NULL);
	}
	else
	{
		list = gtk_list_store_new (1, G_TYPE_STRING);
		model = GTK_TREE_MODEL( list );
		
		combo_box = gtk_combo_box_new_with_model (model);
		g_object_unref (G_OBJECT( model ));

		cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
										"text", 0,
										NULL);
		g_object_set(cell, "ypad", (guint)0, NULL);
	}

	return combo_box;
}

void img_set_fadeout_duration(img_window_struct *img_struct, gint duration)
{
    //gtk_spin_button_set_value(GTK_SPIN_BUTTON(img_struct->fadeout_duration), duration);
    //img_struct->audio_fadeout = duration;
}

void img_set_statusbar_message(img_window_struct *img_struct, gint selected)
{
	gchar *message = NULL;
	gchar *total_slides = NULL;

	if (img_struct->slides_nr == 0)
	{
		message = g_strdup_printf(
				ngettext( "Welcome to Imagination - %d transition loaded.",
						  "Welcome to Imagination - %d transitions loaded.",
						  img_struct->nr_transitions_loaded ),
				img_struct->nr_transitions_loaded );
		gtk_statusbar_push( GTK_STATUSBAR( img_struct->statusbar ),
							img_struct->context_id, message );
		g_free( message );
		//gtk_label_set_text( GTK_LABEL( img_struct->total_slide_number_label ), NULL );
		gtk_icon_view_set_columns (GTK_ICON_VIEW (img_struct->thumbnail_iconview), -1);
	}
	else 
	{
		total_slides = g_strdup_printf("%d",img_struct->slides_nr);
		//gtk_label_set_text(GTK_LABEL(img_struct->total_slide_number_label),total_slides);
	    if (selected)
			message = g_strdup_printf( ngettext( "%d slide selected",
						"%d slides selected",
						selected ), selected);
		else
			message = g_strdup_printf( ngettext( "%d slide loaded %s",
						"%d slides loaded %s",
						img_struct->slides_nr ),
					img_struct->slides_nr,
					_(" - Use the CTRL key to select/unselect "
						"or SHIFT for multiple select") );
		gtk_statusbar_push( GTK_STATUSBAR( img_struct->statusbar ),
							img_struct->context_id, message );
		gtk_icon_view_set_columns (GTK_ICON_VIEW (img_struct->thumbnail_iconview), img_struct->slides_nr + 1);
		g_free(total_slides);
		g_free(message);
	}
}

void img_load_available_transitions(img_window_struct *img)
{
	GtkTreeIter    piter, citer;
	GtkTreeStore  *model;
	gpointer       address;
	gchar         *search_paths[3],
				 **path;

	model = GTK_TREE_STORE( gtk_combo_box_get_model(
								GTK_COMBO_BOX( img->transition_type ) ) );
	
	/* Fill the combo box with no transition */
	gtk_tree_store_append( model, &piter, NULL );
	gtk_tree_store_set( model, &piter, 0, NULL,
									   1, _("None"),
									   2, NULL,
									   3, -1,
									   4, NULL,
									   -1);
	gtk_combo_box_set_active( GTK_COMBO_BOX( img->transition_type ), 0 );

	/* Create NULL terminated array of paths that we'll be looking at */
#if PLUGINS_INSTALLED
	search_paths[0] = g_build_path(G_DIR_SEPARATOR_S, PACKAGE_LIB_DIR, "imagination", NULL );
#else
	search_paths[0] = g_strdup("./transitions");
#endif
	search_paths[1] = g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(), ".imagination",
									"plugins", NULL );
	search_paths[2] = NULL;

	/* Search all paths listed in array */
	for( path = search_paths; *path; path++ )
	{
		GDir *dir;

		dir = g_dir_open( *path, 0, NULL );
		if( dir == NULL )
		{
			g_free( *path );
			continue;
		}
		
		while( TRUE )
		{
			const gchar  *transition_name;
			gchar        *fname = NULL;
			GModule      *module;
			void (*plugin_set_name)(gchar **, gchar ***);

			transition_name = g_dir_read_name( dir );
			if ( transition_name == NULL )
				break;
			
			fname = g_build_filename( *path, transition_name, NULL );
			module = g_module_open( fname, G_MODULE_BIND_LOCAL );
			if( module && img_plugin_is_loaded(img, module) == FALSE )
			{
				gchar  *name,
					  **trans,
					  **bak;

				/* Obtain the name from the plugin function */
				g_module_symbol( module, "img_get_plugin_info",
								 (void *)&plugin_set_name);
				plugin_set_name( &name, &trans );
				
				/* Add group name to the store */
				gtk_tree_store_append( model, &piter, NULL );
				gtk_tree_store_set( model, &piter, 0, NULL, 1, name, 3, 0, -1 );
				img->plugin_list = g_slist_append(img->plugin_list, module);
				
				/* Add transitions */
				for( bak = trans; *trans; trans += 3 )
				{
					gchar              *pix_name,
									   *anim_name,
									   *tmp;
					GdkPixbuf          *pixbuf;
					GdkPixbufAnimation *anim;
					gint                id = GPOINTER_TO_INT( trans[2] );

#if PLUGINS_INSTALLED
					tmp = g_build_filename( DATADIR, "imagination",
											"pixmaps", NULL );
#else /* PLUGINS_INSTALLED */
					tmp = g_strdup( "./pixmaps" );
#endif /* ! PLUGINS_INSTALLED */

					pix_name = g_strdup_printf( "%s%simagination-%d.png",
												tmp, G_DIR_SEPARATOR_S, id );
					anim_name = g_strdup_printf( "%s%simagination-%d.gif",
												 tmp, G_DIR_SEPARATOR_S, id );
					g_free( tmp );

					pixbuf = gdk_pixbuf_new_from_file( pix_name, NULL );

					/* Local plugins will fail to load images from system
					 * folder, so we'll try to load the from home folder. */
					if( ! pixbuf )
					{
						g_free( pixbuf );

						tmp = g_build_filename( g_get_home_dir(),
												".imagination",
												"pixmaps",
												NULL );

						pix_name =
							g_strdup_printf( "%s%simagination-%d.png",
											 tmp, G_DIR_SEPARATOR_S, id );
						anim_name =
							g_strdup_printf( "%s%simagination-%d.gif",
											 tmp, G_DIR_SEPARATOR_S, id );
						g_free( tmp );

						pixbuf = gdk_pixbuf_new_from_file( pix_name, NULL );
					}
					anim = gdk_pixbuf_animation_new_from_file( anim_name,
															   NULL );
					g_free( pix_name );
					g_free( anim_name );
					g_module_symbol( module, trans[1], &address );
					gtk_tree_store_append( model, &citer, &piter );
					gtk_tree_store_set( model, &citer, 0, pixbuf,
													   1, "",
													   2, address,
													   3, id,
													   4, anim,
													   -1 );
					img->nr_transitions_loaded++;
					g_object_unref( G_OBJECT( pixbuf ) );
					g_object_unref( G_OBJECT( anim ) );
				}
				g_free( bak );
			}
			g_free( fname );
		}
		g_free( *path );
		g_dir_close( dir );
	}
}

static gboolean img_plugin_is_loaded(img_window_struct *img, GModule *module)
{
	return (g_slist_find(img->plugin_list,module) != NULL);
}

void img_show_file_chooser(GtkWidget *entry, GtkEntryIconPosition UNUSED(icon_pos),int UNUSED(button),img_window_struct *img)
{
	GtkWidget		*file_selector;
	gchar			*dest_dir,
					*ext;
	const gchar 	*filter_name = NULL;
	gint			response,
					codec;
    GtkFileFilter	*all_files_filter,
					*video_filter;

	file_selector = gtk_file_chooser_dialog_new (_("Please type the video filename"),
							GTK_WINDOW (img->imagination_window),
							GTK_FILE_CHOOSER_ACTION_SAVE,
							"_Cancel",
							GTK_RESPONSE_CANCEL,
							"_Save",
							GTK_RESPONSE_ACCEPT,
							NULL);

    /* Video files filter */
	codec = gtk_combo_box_get_active(GTK_COMBO_BOX(img->container_menu));

	switch(codec)
    {
		case 0:
		ext = "*.mpg";
		filter_name = "MPEG-1 Video (*.mpg)";
		break;

		case 1:
		ext = "*.mpg";
		filter_name = "MPEG-2 Video (*.mpg)";
		break;

		case 2:
		ext = "*.mp4";
		filter_name = "MPEG-4 Video (*.mp4)";
		break;
		
		case 3:
		ext = "*.ts";
		filter_name = "MPEG-TS (*.ts)";
		break;
		
		case 4:
		ext = "*.mkv";
		filter_name = "Matroska (*.mkv)";
		break;
		
		case 5:
		ext = "*.ogg";
		filter_name = "OGG (*.ogg)";
		break;
		
		case 6:
		ext = "*.mov";
		filter_name = "QuickTime (*.mov)";
		break;
		
		case 7:
		ext = "*.webm";
		filter_name = "WebM (*.webm)";
		break;
	}

	if (filter_name)
	{
		video_filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (video_filter, filter_name);
		gtk_file_filter_add_pattern (video_filter, ext);
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_selector), video_filter);
	}
	/* All files filter */
	all_files_filter = gtk_file_filter_new ();
    gtk_file_filter_set_name(all_files_filter, _("All files"));
    gtk_file_filter_add_pattern(all_files_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_selector), all_files_filter);
    
    /* set current dir to the project current dir */
    if (img->project_current_dir)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_selector), img->project_current_dir);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (file_selector),TRUE);
	response = gtk_dialog_run (GTK_DIALOG(file_selector));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_selector));
		if (strstr(dest_dir,++ext) == NULL)
		{	
			gchar *dummy2 = g_strconcat(dest_dir, ext, NULL);
			g_free(dest_dir);
			dest_dir = dummy2;
		}
		gtk_entry_set_text(GTK_ENTRY(entry),dest_dir);
		g_free(dest_dir);
		
		gtk_popover_popdown(GTK_POPOVER(img->file_po) );
	}
	gtk_widget_destroy(file_selector);
}

slide_struct *
img_create_new_slide( void )
{
	slide_struct    *slide = NULL;

	slide = g_slice_new0( slide_struct );
	if( slide )
	{
		/* Still part */
		slide->duration = 1.0;

		/* Transition */
		slide->path = g_strdup( "0" );
		slide->transition_id = -1;

		/* Ken Burns */
		slide->cur_point = -1;

		/* Subtitles */
		slide->anim_duration = 1;
		slide->posX = 0;
		slide->posY = 1;
		slide->font_desc = pango_font_description_from_string( "Sans 24" );
		slide->font_color[0] = 0; /* R */
		slide->font_color[1] = 0; /* G */
		slide->font_color[2] = 0; /* B */
		slide->font_color[3] = 1; /* A */
		/* default: no font color border */
        slide->font_brdr_color[0] = 1; /* R */
        slide->font_brdr_color[1] = 1; /* G */
        slide->font_brdr_color[2] = 1; /* B */
        slide->font_brdr_color[3] = 1; /* A */
        /* default: no font background color */
        slide->font_bg_color[0] = 1; /* R */
        slide->font_bg_color[1] = 1; /* G */
        slide->font_bg_color[2] = 1; /* B */
        slide->font_bg_color[3] = 0; /* A */
        /* default: no font border color */
        slide->border_color[0] = 1; /* R */
        slide->border_color[1] = 1; /* G */
        slide->border_color[2] = 1; /* B */
        slide->border_color[3] = 1; /* A */

		slide->border_width = 1;

        /* Load error handling */
        slide->load_ok = TRUE;
        slide->original_filename = NULL;
	}

	return( slide );
}

void
img_set_slide_file_info( slide_struct *slide,
						 const gchar  *filename )
{
	GdkPixbufFormat *format;
	gint             width,
					 height;

	format = gdk_pixbuf_get_file_info( filename, &width, &height );

	slide->o_filename = g_strdup( filename );
	slide->p_filename = g_strdup( filename );
	slide->angle = 0;

	slide->resolution = g_strdup_printf( "%d x %d", width, height );
	slide->type = format ? gdk_pixbuf_format_get_name( format ) : NULL;
}

void
img_set_empty_slide_info( slide_struct *slide,
							 gint          gradient,
							 gint          countdown,
							 gdouble      *start_color,
							 gdouble      *stop_color,
							 gdouble      *countdown_color,
							 gdouble      *start_point,
							 gdouble      *stop_point )
{
	gint i;

	slide->gradient = gradient;
	for( i = 0; i < 3; i++ )
	{
		slide->g_start_color[i] = start_color[i];
		slide->g_stop_color[i]  = stop_color[i];
	}
	for( i = 0; i < 2; i++ )
	{
		slide->g_start_point[i] = start_point[i];
		slide->g_stop_point[i]  = stop_point[i];
	}
	if (gradient == 4)
	{
		slide->countdown = countdown;
		for( i = 0; i < 3; i++ )
			slide->countdown_color[i] = countdown_color[i];
	}
}

GdkPixbuf *img_set_fade_gradient(img_window_struct *img, gint gradient, slide_struct *slide_info)
{
	GdkPixbuf		*pix = NULL;
	GtkTreeIter 	iter;
	GtkTreeModel 	*model;
	gpointer     	address;

	if (gradient == 3)
	{
		model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->transition_type ) );
		/* 10:0 is the path string pointing to the Cross Fade transition */
		gtk_tree_model_get_iter_from_string( model, &iter, "10:0" );
		gtk_tree_model_get( model, &iter, 0, &pix,
										  2, &address,
										 -1 );
		slide_info->transition_id = 19;
		slide_info->render = address;
		if (slide_info->path)
		{
			g_free(slide_info->path);
			slide_info->path = g_strdup("10:0");
		}
	}
	else
	{
		if (slide_info->path)
		{
			g_free(slide_info->path);
			slide_info->path = g_strdup("0");
		}
		slide_info->transition_id = -1;
		slide_info->render = NULL;
	}
	return pix;
}

void
img_set_slide_still_info( slide_struct      *slide,
						  gdouble           duration,
						  img_window_struct *img )
{
	if( slide->duration != duration )
	{
		slide->duration = duration;
		img_set_total_slideshow_duration(img);
	}
}

void
img_set_slide_transition_info( slide_struct      *slide,
							   GtkListStore      *store,
							   GtkTreeIter       *iter,
							   GdkPixbuf         *pix,
							   const gchar       *path,
							   gint               transition_id,
							   ImgRender          render,
							   img_window_struct *img )
{
	/* Set transition render. */
	if( path && ( slide->transition_id != transition_id ) )
	{
		if( slide->path )
			g_free( slide->path );

		slide->path = g_strdup( path );
		slide->transition_id = transition_id;
		slide->render = render;

		gtk_list_store_set( store, iter, 2, pix, -1 );
	}

	img_set_total_slideshow_duration(img);
}

void
img_set_slide_ken_burns_info( slide_struct *slide,
							  gint          cur_point,
							  gsize         length,
							  gdouble      *points )
{
	ImgStopPoint *point;
	gsize          i,
				  full;

	if( slide->no_points )
	{
		g_list_free( slide->points );
		slide->no_points = 0;
	}

	for( i = 0; i < length; i += 4 )
	{
		/* Create new point */
		point = g_slice_new( ImgStopPoint );
		point->time = (gint)( points[0 + i] + 0.5 );
		point->offx = points[1 + i];
		point->offy = points[2 + i];
		point->zoom = points[3 + i];
		
		/* Append it to the list */
		slide->points = g_list_append( slide->points, point );
		slide->no_points++;
	}

	slide->cur_point = CLAMP( cur_point, -1, slide->no_points - 1 );

	full = img_calc_slide_duration_points( slide->points,
										   slide->no_points );
	if( full )
		slide->duration = full;
}

void
img_free_slide_struct( slide_struct *entry )
{
	GList *tmp;

	img_slide_set_p_filename(entry, NULL);

	g_free(entry->o_filename);
	g_free(entry->resolution);
	if (entry->type) g_free(entry->type);
	
	if (entry->subtitle)
		g_free(entry->subtitle);
	if (entry->pattern_filename)
		g_free(entry->pattern_filename);
	
	/* Free stop point list */
	for( tmp = entry->points; tmp; tmp = g_list_next( tmp ) )
		g_slice_free( ImgStopPoint, tmp->data );
	g_list_free( entry->points );

	g_slice_free( slide_struct, entry );
}

gboolean
img_set_total_slideshow_duration( img_window_struct *img )
{
	gchar        *time;
	GtkTreeIter   iter;
	slide_struct *entry;
	GtkTreeModel *model;

	img->total_secs = 0;

	model = GTK_TREE_MODEL( img->thumbnail_model );
	if( gtk_tree_model_get_iter_first( model, &iter ) )
	{
		do
		{
			gtk_tree_model_get( model, &iter, 1, &entry, -1 );
			img->total_secs += entry->duration;

			if(entry->render)
				img->total_secs += 3; //transition speed set to 3 seconds
		}
		while( gtk_tree_model_iter_next( model, &iter ) );

		/* Add time of last pseudo slide */
		if( img->final_transition.render && img->bye_bye_transition)
			img->total_secs += 3; //transition speed set to 3 seconds
	}
	img->total_secs = ceil(img->total_secs);
	time = img_convert_seconds_to_time((gint)img->total_secs);
	gtk_label_set_text(GTK_LABEL (img->slideshow_duration),time);
	g_free(time);

	return( FALSE );
}

gint
img_calc_slide_duration_points( GList *list,
								gint   length )
{
	GList        *tmp;
	gint          i, duration = 0;
	ImgStopPoint *point;

	/* If we have no points, return 0 */
	if( length == 0 )
		return( 0 );

	/* Calculate length */
	for( tmp = list, i = 0; i < length; tmp = g_list_next( tmp ), i++ )
	{
		point = (ImgStopPoint *)tmp->data;
		duration += point->time;
	}

	return( duration );
}

/*
 * img_scale_image:
 *
 * This function should be called for all image loading needs. It'll properly
 * scale and trim loaded images, add borders if needed and return surface or
 * pixbuf of requested size.
 *
 * If one of the size requests is 0, the other one will be calculated from
 * first one with aspect ratio calculation. If both dimensions are 0, image
 * will be loaded from disk at original size (this is mainly used for export,
 * when we want to have images at their best quality).
 *
 * Return value: TRUE if image loading succeded, FALSE otherwise.
 */
gboolean
img_scale_image( const gchar      *filename,
				 gdouble           ratio,
				 gint              width,
				 gint              height,
				 gboolean          distort,
				 gdouble          *color,
				 GdkPixbuf       **pixbuf,
				 cairo_surface_t **surface )
{
	GdkPixbuf *loader;             /* Pixbuf used for loading */
	gint       i_width, i_height;  /* Image dimensions */
	gint       offset_x, offset_y; /* Offset values for borders */
	gdouble    i_ratio;            /* Export and image aspect ratios */
	gdouble    skew;               /* Transformation between ratio and
											 i_ratio */
	GError     *error = NULL;
	gboolean   transform = FALSE;  /* Flag that controls scalling */

	/* MAximal distortion values */
	gdouble max_stretch = 0.1280;
	gdouble max_crop    = 0.8500;

	/* Borderline skew values */
	gdouble max_skew = ( 1 + max_stretch ) / max_crop;
	gdouble min_skew = ( 1 - max_stretch ) * max_crop;

	/* Obtain information about image being loaded */
	if (!filename) {
	    return FALSE;
	}
	if (!gdk_pixbuf_get_file_info( filename, &i_width, &i_height )) {
	    g_message("Cannot load image info from %s", filename);
	    return FALSE;
	}

	/* How distorted images would be if we scaled them */
	i_ratio = (gdouble)i_width / i_height;
	skew = ratio / i_ratio;

	/* Calculationg surface dimensions.
	 *
	 * In order to be as flexible as possible, this function can load images at
	 * various sizes, but at aspect ration that matches the aspect ratio of main
	 * preview area. How size is determined? If width argument is not -1, this
	 * is taken as a reference dimension from which height is calculated (if
	 * height argument also present, it's ignored). If width argument is -1,
	 * height is taken as a reference dimension. If both width and height are
	 * -1, surface dimensions are calculated to to fit original image.
	 */
	if( width > 0 )
	{
		/* Calculate height according to width */
		height = width / ratio;
	}
	else if( height > 0 )
	{
		/* Calculate width from height */
		width = height * ratio;
	}
	else
	{
		/* Load image at maximum quality
		 *
		 * If the user doesn't want to have distorted images, we create slightly
		 * bigger surface that will hold borders too.
		 *
		 * If images should be distorted, we first check if we're able to fit
		 * image without distorting it too much. If images would be largely
		 * distorted, we simply load them undistorted.
		 *
		 * If we came all the way to  here, then we're able to distort image.
		 */
		if( ( ! distort )       || /* Don't distort */
			( skew > max_skew ) || /* Image is too wide */
			( skew < min_skew )  ) /* Image is too tall */
		{
			/* User doesn't want images to be distorted or distortion would be
			 * too intrusive. */
			if( ratio < i_ratio )
			{
				/* Borders will be added on left and right */
				width = i_width;
				height = width / ratio;
			}
			else
			{
				/* Borders will be added on top and bottom */
				height = i_height;
				width = height * ratio;
			}
		}
		else
		{
			/* User wants images to be distorted and we're able to do it
			 * without ruining images. */
			if( ratio < i_ratio )
			{
				/* Image will be distorted horizontally */
				height = i_height;
				width = height * ratio;
			}
			else
			{
				/* Image will be distorted vertically */
				width = i_width;
				height = width / ratio;
			}
		}
	}

	/* Will image be disotrted?
	 *
	 * Conditions:
	 *  - user allows us to do it
	 *  - skew is in sensible range
	 *  - image is not smaller than exported wideo size
	 */
	transform = distort && skew < max_skew && skew > min_skew &&
				( i_width >= width || i_height >= height );

	/* Load image into pixbuf at proper size */
	if( transform )
	{
		gint lw, lh;

		/* Images will be loaded at slightly modified dimensions */
		if( ratio < i_ratio )
		{
			/* Horizontal scaling */
			lw = (gdouble)width / ( skew + 1 ) * 2;
			lh = height;
		}
		else
		{
			/* Vertical scaling */
			lw = width;
			lh = (gdouble)height * ( skew + 1 ) / 2;
		}
		loader = gdk_pixbuf_new_from_file_at_scale( filename, lw, lh, FALSE, &error );
	}
	else
	{
		/* Simply load image into pixbuf at size */
		loader = gdk_pixbuf_new_from_file_at_size( filename, width, height, &error );
	}
	if( ! loader ) {
		g_message( "While loading image data from %s: %s.", filename, error->message );
		g_error_free( error );
		return( FALSE );
	}

	i_width  = gdk_pixbuf_get_width( loader );
	i_height = gdk_pixbuf_get_height( loader );

	/* Calculate offsets */
	offset_x = ( width - i_width ) / 2;   /* CAN BE NEGATIVE!!! */
	offset_y = ( height - i_height ) / 2; /* CAN BE NEGATIVE!!! */

	/* Prepare output
	 *
	 * We can give two different output formats: cairo_surface_t and GdkPixbuf.
	 */
	if( pixbuf )
	{
		/* Create new pixbuf with loaded image */
		GdkPixbuf *tmp_pix;   /* Pixbuf used for loading */
		guint32    tmp_color; /* Background color */

		tmp_color = ( (gint)( color[0] * 0xff ) << 24 ) |
					( (gint)( color[1] * 0xff ) << 16 ) |
					( (gint)( color[2] * 0xff ) <<  8 );
		tmp_pix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, FALSE, 8, width, height );
		gdk_pixbuf_fill( tmp_pix, tmp_color );
		gdk_pixbuf_composite( loader, tmp_pix,
							  MAX( 0, offset_x ), MAX( 0, offset_y ),
							  MIN( i_width, width ), MIN( i_height, height ),
							  offset_x, offset_y, 1, 1,
							  GDK_INTERP_BILINEAR, 255 );

		*pixbuf = tmp_pix;
	}
	if( surface )
	{
		/* Paint surface with loaded image
		 * 
		 * If image cannot be scalled, transform is FALSE. In this case, just
		 * borders are added. If transform is not 0, than scale image before
		 * painting it. */
		cairo_t         *cr;       /* Cairo, used to transform image */
		cairo_surface_t *tmp_surf; /* Surface to draw on */

		/* Create image surface with proper dimensions */
		tmp_surf = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
											   width, height );

		cr = cairo_create( tmp_surf );
		
		if( ! transform )
		{
			/* Fill with background color */
			cairo_set_source_rgb( cr, color[0], color[1], color[2] );
			cairo_paint( cr );
		}
		
		/* Paint image */
		gdk_cairo_set_source_pixbuf( cr, loader, offset_x, offset_y );
		cairo_paint( cr );
		
		cairo_destroy( cr );

		/* Return surface */
		*surface = tmp_surf;
	}

	/* Free temporary pixbuf */
	g_object_unref( G_OBJECT( loader ) );

	return( TRUE );
}

void
img_sync_timings( slide_struct  *slide, img_window_struct *img )
{
	/* If times are already synchronized, return */
	if( slide->duration >= slide->anim_duration )
		return;

	/* Do the right thing;) */
	if( slide->no_points )
	{
		gint          diff;
		ImgStopPoint *point;

		/* Calculate difference that we need to accomodate */
		diff = slide->anim_duration - slide->duration;

		/* Elongate last point */
		point = (ImgStopPoint *)g_list_last( slide->points )->data;
		point->time += diff;
		
		/* Update Ken Burns display */
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( img->ken_duration ),
								   point->time );
	}
}

void img_select_nth_slide(img_window_struct *img, gint slide_to_select)
{
	GtkTreePath *path;

	gtk_icon_view_unselect_all(GTK_ICON_VIEW (img->active_icon));
	path = gtk_tree_path_new_from_indices(slide_to_select, -1);
	gtk_icon_view_set_cursor (GTK_ICON_VIEW (img->active_icon), path, NULL, FALSE);
	gtk_icon_view_select_path (GTK_ICON_VIEW (img->active_icon), path);
	gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (img->active_icon), path, FALSE, 0, 0);
	gtk_tree_path_free (path);
}

GdkPixbuf *img_convert_surface_to_pixbuf( cairo_surface_t *surface )
{
	GdkPixbuf *pixbuf;
	gint       w, h, ss, sp, row, col;
	guchar    *data_s, *data_p;

	/* Information about surface */
	w = cairo_image_surface_get_width( surface );
	h = cairo_image_surface_get_height( surface );
	ss = cairo_image_surface_get_stride( surface );
	data_s = cairo_image_surface_get_data( surface );

	/* Create new pixbuf according to upper specs */
	pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, FALSE, 8, w, h );

	/* Get info about new pixbuf */
	sp = gdk_pixbuf_get_rowstride( pixbuf );
	data_p = gdk_pixbuf_get_pixels( pixbuf );

	/* Copy pixels */
	for( row = 0; row < h; row++ )
	{
		for( col = 0; col < w; col++ )
		{
			gint index_s, index_p;

			index_s = row * ss + col * 4;
			index_p = row * sp + col * 3;

			data_p[index_p + 0] = data_s[index_s + 2];
			data_p[index_p + 1] = data_s[index_s + 1];
			data_p[index_p + 2] = data_s[index_s + 0];
		}
	}

	return( pixbuf );
}

gboolean img_scale_empty_slide( gint gradient,  gint countdown,
					gdouble          *p_start,
					gdouble          *p_stop,
					gdouble          *c_start,
					gdouble          *c_stop,
					gdouble          *countdown_color,
					gboolean			preview,
					gdouble 			countdown_angle,
					gint              width,
					gint              height,
					GdkPixbuf       **pixbuf,
					cairo_surface_t **surface )
{
	cairo_surface_t *sf;
	cairo_t         *cr;
	cairo_pattern_t *pat;
	gdouble          diffx, diffy, radius;

	sf = cairo_image_surface_create( CAIRO_FORMAT_RGB24, width, height );
	cr = cairo_create( sf );

	switch( gradient )
	{
		case 0: case 3: /* Solid and gradient fade */
			cairo_set_source_rgb( cr, c_start[0], c_start[1], c_start[2] );
			cairo_paint( cr );
			break;

		case 1: /* Linear gradient */
			pat = cairo_pattern_create_linear( p_start[0] * width,
											   p_start[1] * height,
											   p_stop[0] * width,
											   p_stop[1] * height );
			cairo_pattern_add_color_stop_rgb( pat, 0, c_start[0],
											  c_start[1], c_start[2] );
			cairo_pattern_add_color_stop_rgb( pat, 1, c_stop[0],
											  c_stop[1], c_stop[2] );
			cairo_set_source( cr, pat );
			cairo_paint( cr );
			cairo_pattern_destroy( pat );
			break;

		case 2: /* Radial gradient */
			diffx = ABS( p_start[0] - p_stop[0] ) * width;
			diffy = ABS( p_start[1] - p_stop[1] ) * height;
			radius = sqrt( pow( diffx, 2 ) + pow( diffy, 2 ) );

			pat = cairo_pattern_create_radial( p_start[0] * width,
											   p_start[1] * height, 0,
											   p_start[0] * width,
											   p_start[1] * height, radius );
			cairo_pattern_add_color_stop_rgb( pat, 0, c_start[0],
											  c_start[1], c_start[2] );
			cairo_pattern_add_color_stop_rgb( pat, 1, c_stop[0],
											  c_stop[1], c_stop[2] );
			cairo_set_source( cr, pat );
			cairo_paint( cr );
			cairo_pattern_destroy( pat );
			break;
			
		case 4:
		cairo_text_extents_t te;
		int offset = 215;
		int font_size = 250;
		double x = 55, y = 60, width2 = width / 8, height2 = height / 8, aspect = 1.0, corner_radius = height2 / 10.0;
		double radius = corner_radius / aspect;
		double degrees = M_PI / 180.0;
		
		cairo_set_line_width(cr,4.0);

		if (width == 88 && height == 49) /* Are we creating the thumbnail? If so we need smaller values */
		{	
			font_size = 20;
			cairo_set_line_width(cr,1.0);
		}

		width /= 2;
		height /= 2;

		//Clear the drawing area
		cairo_set_source_rgb(cr, c_start[0],  c_start[1], c_start[2]);
		cairo_paint(cr);
		
		//Draw the left rectangles column
		cairo_set_source_rgb(cr, c_stop[0],  c_stop[1], c_stop[2]);
		for (int i=0; i<=3; i++)
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
		for (int i=0; i<=3; i++)
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
		cairo_set_source_rgb(cr, c_stop[0], c_stop[1], c_stop[2]);
		cairo_move_to(cr, width,height);
		cairo_arc(cr, width, height, width / 2, 0, 2 * G_PI);
		cairo_stroke(cr);
		
		//Draw the second circle
		cairo_arc(cr, width, height, width / 2 - 40, 0, 2 * G_PI);
		cairo_stroke(cr);
		
		if (preview)
		{
			//Draw sector
			cairo_move_to(cr, width, height);
			cairo_set_source_rgb(cr, c_stop[0],  c_stop[1], c_stop[2]);
			cairo_arc(cr, width, height, 440, 0, countdown_angle);
			cairo_fill(cr);
		}

		// Draw the countdown number
		cairo_set_source_rgb(cr, countdown_color[0],  countdown_color[1], countdown_color[2]);
		cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, font_size);
		gchar *digit = g_strdup_printf("%d", countdown);
		cairo_text_extents (cr, digit, &te);
		cairo_move_to(cr, width - te.x_bearing - te.width / 2, height - te.y_bearing - te.height / 2);
		cairo_show_text(cr, digit);
		g_free(digit);
		break;
	}
	cairo_destroy( cr );

	if( surface )
		*surface = sf;
	else
	{
		*pixbuf = img_convert_surface_to_pixbuf( sf );
		cairo_surface_destroy( sf );
	}

	return( TRUE );
}

void img_delete_subtitle_pattern(GtkButton *button, img_window_struct *img)
{
	slide_struct 	*slide = img->current_slide;
	GdkPixbuf 		*pixbuf;
	GtkWidget		*tmp_image,*fc;
	GtkIconTheme	*icon_theme;

	if (slide->pattern_filename)
	{
		g_free(slide->pattern_filename);
		slide->pattern_filename = NULL;
	}
	icon_theme = gtk_icon_theme_get_default();
	pixbuf = gtk_icon_theme_load_icon(icon_theme,"image", 20, 0, NULL);
	tmp_image = gtk_image_new_from_pixbuf(pixbuf);
	gtk_widget_show(tmp_image);
	g_object_unref(pixbuf);
	
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(img->pattern_image), tmp_image);
	gtk_widget_set_sensitive(img->sub_color, TRUE);
	gtk_widget_set_tooltip_text(img->sub_color, _("Click to choose the font color"));

	fc = gtk_widget_get_toplevel(GTK_WIDGET(button));
	gtk_widget_destroy(fc);
	gtk_widget_queue_draw( img->image_area );
}

void img_save_relative_filenames(GtkCheckButton *togglebutton, img_window_struct *img)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
		img->relative_filenames = TRUE;
	else
		img->relative_filenames = FALSE;
}

void str_replace(gchar *str, const gchar *search, const gchar *replace)
{
	for (gchar *cursor = str; (cursor = strstr(cursor, search)) != NULL;)
	{
		memmove(cursor + strlen(replace), cursor + strlen(search), strlen(cursor) - strlen(search) + 1);
        for (gint i = 0; replace[i] != '\0'; i++)
            cursor[i] = replace[i];
        cursor += strlen(replace);
	}
}

void img_set_text_buffer_tags(img_window_struct *img)
{
	GtkTextTag *tag;

	img->tag_table = gtk_text_buffer_get_tag_table(img->slide_text_buffer);
	tag = gtk_text_tag_new("bold");
	g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_tag_table_add (img->tag_table, tag);
	
	tag = gtk_text_tag_new("italic");
	g_object_set(tag, "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_tag_table_add (img->tag_table, tag);
	
	tag = gtk_text_tag_new("underline");
	g_object_set(tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
	gtk_text_tag_table_add (img->tag_table, tag);
	
	tag = gtk_text_tag_new("foreground");
	gtk_text_tag_table_add (img->tag_table, tag);
	
	tag = gtk_text_tag_new("background");
	gtk_text_tag_table_add (img->tag_table, tag);
}

void img_store_rtf_buffer_content(img_window_struct *img)
{
	GdkAtom		format;
	GtkTextIter start, end;

	format = gtk_text_buffer_register_serialize_tagset(img->slide_text_buffer, NULL);
	gtk_text_buffer_get_bounds(img->slide_text_buffer, &start, &end);
	img->current_slide->subtitle = gtk_text_buffer_serialize(img->slide_text_buffer,
																img->slide_text_buffer,
																format,
																&start, 
																&end,
																&img->current_slide->subtitle_length
																); 
	gtk_text_buffer_unregister_serialize_format (img->slide_text_buffer, format); 
}

void img_check_for_rtf_colors(img_window_struct *img, gchar *subtitle)
{
	GtkTextTag	*tag;
	gchar		*dummy, *dummy2, *rgb;
	gint		len;

	/* foreground */
	dummy = strstr(subtitle, "foreground-rgba");
	if (dummy)
	{
		dummy = strstr(dummy, "value=");
		dummy2 = strstr(dummy+7, "\"");
		len = strlen(dummy+7) - strlen(dummy2);

		rgb = g_new0(gchar, len + 2);
		rgb[0] = '#';
		strncpy(rgb+1, dummy+7, len);

		str_replace(rgb, ":", "");

		tag = gtk_text_tag_table_lookup(img->tag_table, "foreground");
		g_object_set(tag, "foreground-rgba", rgb, NULL);
		g_free(rgb);
	}
	/* background */
	dummy = strstr(subtitle, "background-rgba");
	if (dummy)
	{
		dummy = strstr(dummy, "value=");
		dummy2 = strstr(dummy+7, "\"");
		len = strlen(dummy+7) - strlen(dummy2);

		rgb = g_new0(gchar, len + 2);
		rgb[0] = '#';
		strncpy(rgb+1, dummy+7, len);
		str_replace(rgb, ":", "");
		
		tag = gtk_text_tag_table_lookup(img->tag_table, "background");
		g_object_set(tag, "background-rgba", rgb, NULL);
		g_free(rgb);
	}
}

void img_message(img_window_struct *img, gchar *message)
{
	GtkWidget *dialog, *action_area, *image, *content_area, *box, *label;

	dialog = gtk_dialog_new_with_buttons ("Imagination",
                                      GTK_WINDOW(img->imagination_window),
                                      GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                      _("Close"),
                                      GTK_BUTTONS_CLOSE,
                                      NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG(dialog));
	action_area  = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
	
	/* Yeah it's deprecated since 3.12 but it works. What is the alternative? */
	g_object_set(G_OBJECT(action_area), "halign", GTK_ALIGN_CENTER);
	gtk_widget_set_margin_bottom(action_area, 5);
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	
	label = gtk_label_new(message);
	image = gtk_image_new();
	gtk_image_set_from_icon_name(GTK_IMAGE(image), "dialog-error", GTK_ICON_SIZE_DIALOG);
	g_signal_connect_swapped(dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);

	gtk_box_pack_start (GTK_BOX(box), image, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 15);
	gtk_container_add (GTK_CONTAINER(content_area), box);
	gtk_widget_show_all (dialog);
}

gint img_ask_user_confirmation(img_window_struct *img_struct, gchar *msg)
{
	GtkWidget *dialog;
	gint response;

	dialog = gtk_message_dialog_new(GTK_WINDOW(img_struct->imagination_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL, "%s.", msg);
	gtk_widget_set_margin_bottom(dialog, 5);
	gtk_window_set_title(GTK_WINDOW(dialog),"Imagination");
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	return response;
}

// Resets info_slide->p_filename to filename and removes the associated temp file if applicable
void img_slide_set_p_filename(slide_struct *info_slide, gchar *filename)
{
    if (!info_slide->p_filename) return;
    if ( g_strcmp0(info_slide->o_filename, info_slide->p_filename) ) {
	g_assert(info_slide->flipped || info_slide->angle);
	g_unlink( info_slide->p_filename );
    }
    g_free(info_slide->p_filename);
    info_slide->p_filename = filename;
}

void img_update_zoom_variables(img_window_struct *img)
{
	/* Store old zoom for calculations */
	gdouble old_zoom = img->current_point.zoom;
	gdouble fracx, fracy;
	gint    tmpoffx, tmpoffy;
		
	img->current_point.zoom = gtk_range_get_value( GTK_RANGE(img->ken_zoom) );
		
	gdouble aw  = img->video_size[0];
	gdouble ah  = img->video_size[1];
	gdouble aw2 = aw / 2;
	gdouble ah2 = ah / 2;

	fracx = (gdouble)( aw2 - img->current_point.offx ) / ( aw2 * old_zoom );
	fracy = (gdouble)( ah2 - img->current_point.offy ) / ( ah2 * old_zoom );
	img->maxoffx = aw * ( 1 - img->current_point.zoom );
	img->maxoffy = ah * ( 1 - img->current_point.zoom );
	tmpoffx = aw2 * ( 1 - fracx * img->current_point.zoom );
	tmpoffy = ah2 * ( 1 - fracy * img->current_point.zoom );

	img->current_point.offx = CLAMP( tmpoffx, img->maxoffx, 0 );
	img->current_point.offy = CLAMP( tmpoffy, img->maxoffy, 0 );
}

gboolean img_check_for_recent_file(img_window_struct *img, const gchar *input)
{
	GList *menu_items, *node0, *last_item;
	const gchar *label;

	menu_items = gtk_container_get_children(GTK_CONTAINER(img->recent_slideshows));
	for(node0 = menu_items; node0 != NULL; node0 = node0->next)
	{
		label = gtk_menu_item_get_label(GTK_MENU_ITEM(node0->data));
		if (g_strcmp0(label, input) == 0)
		{
			g_list_free(menu_items);
			return TRUE;
		}
	}
	if (g_list_length(menu_items) >= 10)
	{
		last_item = g_list_last(menu_items);
		gtk_widget_destroy(last_item->data);
		menu_items = g_list_remove(menu_items, last_item->data);
	}
	g_list_free(menu_items);
	return FALSE;
}

