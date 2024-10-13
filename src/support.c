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
sens_cell_func( GtkCellLayout   * layout,
				GtkCellRenderer *cell,
				GtkTreeModel    *model,
				GtkTreeIter     *iter,
				gpointer         data )
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
	g_signal_handlers_block_by_func((gpointer)img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);	
	gtk_combo_box_set_active( GTK_COMBO_BOX( img->transition_type ), 0 );
	g_signal_handlers_unblock_by_func((gpointer)img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);	
	
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

void img_show_file_chooser(GtkWidget *entry, GtkEntryIconPosition icon_pos,int button, img_window_struct *img)
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

media_struct *img_create_new_media()
{
	media_struct  *slide = NULL;

	slide = g_new0( media_struct, 1 );
	if(slide)
	{
		/* Still part */
		slide->duration = 1.0;

		/* Transition */
		slide->path = g_strdup("0");
		slide->transition_id = -1;

		/* Ken Burns */
		slide->cur_point = -1;

		/* Subtitles */
		slide->anim_duration = 1;
		slide->posX = 0;
		slide->posY = 1;
		slide->font_desc = NULL; //pango_font_description_from_string( "Sans 24" );
		slide->font_color[0] = 0; /* R */
		slide->font_color[1] = 0; /* G */
		slide->font_color[2] = 0; /* B */
		slide->font_color[3] = 1; /* A */
		/* default: no shadow color border */
        slide->font_shadow_color[0] = 1; /* R */
        slide->font_shadow_color[1] = 1; /* G */
        slide->font_shadow_color[2] = 1; /* B */
        slide->font_shadow_color[3] = 1; /* A */
        /* default: no font background color */
        slide->font_bg_color[0] = 1; /* R */
        slide->font_bg_color[1] = 1; /* G */
        slide->font_bg_color[2] = 1; /* B */
        slide->font_bg_color[3] = 0; /* A */
        /* default: no font outline color */
        slide->font_outline_color[0] = 1; /* R */
        slide->font_outline_color[1] = 1; /* G */
        slide->font_outline_color[2] = 1; /* B */
        slide->font_outline_color[3] = 1; /* A */
	}
	return slide;
}

void img_set_empty_slide_info( media_struct *slide,
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

GdkPixbuf *img_set_fade_gradient(img_window_struct *img, gint gradient, media_struct *slide_info)
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
img_set_slide_still_info( media_struct      *slide,
						  gdouble           duration,
						  img_window_struct *img )
{
	if( slide->duration != duration )
	{
		slide->duration = duration;
	}
}

void
img_set_slide_transition_info( media_struct      *slide,
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

}

void
img_set_slide_ken_burns_info( media_struct *slide,
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

void img_free_media_struct( media_struct *entry )
{
	GList *tmp;

	if (entry->full_path)
		g_free(entry->full_path);

	if (entry->image_type)
		g_free(entry->image_type);
		
	if (entry->audio_type)
		g_free(entry->audio_type);

	if (entry->audio_duration)
		g_free(entry->audio_duration);

	if (entry->text)
		g_free(entry->text);

	if (entry->pattern_filename)
		g_free(entry->pattern_filename);
	
	if (entry->font_desc)
		g_free(entry->font_desc);

	if (entry->path)
		g_free(entry->path);

	/* Free stop point list */
	for( tmp = entry->points; tmp; tmp = g_list_next( tmp ) )
		g_slice_free( ImgStopPoint, tmp->data );
	g_list_free( entry->points );

	g_free(entry);
}

gint img_calc_slide_duration_points( GList *list, gint   length )
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

void img_taint_project(img_window_struct *img)
{
    if (!img->project_is_modified) {
	img->project_is_modified = TRUE;
	img_refresh_window_title(img);
    }
}

void
img_sync_timings( media_struct  *slide, img_window_struct *img )
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
	media_struct 	*slide = img->current_media;
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
	gtk_widget_set_sensitive(img->sub_font_color, TRUE);
	gtk_widget_set_tooltip_text(img->sub_font_color, _("Click to choose the font color"));

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
	 gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	//gtk_widget_set_margin_bottom(action_area, 5);
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), message);
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

	dialog = gtk_message_dialog_new(GTK_WINDOW(img_struct->imagination_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL, "%s", msg);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), msg);
	gtk_container_set_border_width( GTK_CONTAINER(dialog ), 10 );		
	gtk_window_set_title(GTK_WINDOW(dialog),"Imagination");
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	return response;
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

gboolean img_find_media_in_list(img_window_struct *img, gchar *full_path_filename)
{
	GtkTreeIter iter;
	media_struct *entry;

	if( gtk_tree_model_get_iter_first(GTK_TREE_MODEL(img->media_model), &iter) )
	{
		do
		{
			gtk_tree_model_get(GTK_TREE_MODEL(img->media_model), &iter, 2, &entry, -1);
			if (g_strcmp0(entry->full_path, full_path_filename) == 0)
				return TRUE;
		}
		while( gtk_tree_model_iter_next(GTK_TREE_MODEL(img->media_model), &iter) );
	}
	else
		return FALSE;

}

void img_get_audio_data(media_struct *media)
{
	AVFormatContext *fmt_ctx = NULL;
	AVDictionaryEntry *tag = NULL;
	char metadata[1024] = {0};
	gchar *time = NULL;
	gint audio_stream_index = -1;
    
	//Open the file
	avformat_open_input(&fmt_ctx, media->full_path, NULL, NULL) ;
    
    // Retrieve stream information
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    return;

    for (int i = 0; i < fmt_ctx->nb_streams; i++)
    {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_stream_index = i;
            break;
        }
    }
    
    // Get the duration
    int64_t duration = fmt_ctx->duration;
    double duration_seconds = (double)duration / AV_TIME_BASE;
	media->audio_duration = img_convert_time_to_string(duration_seconds);
	
	//Get the audio type
	media->audio_type = g_strdup(fmt_ctx->iformat->name);
	to_upper(&media->audio_type);
	
	AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;

	//Get the bitrate
	if (fmt_ctx->bit_rate)
		media->bitrate = fmt_ctx->bit_rate / 1000;
	else if (codecpar->bit_rate)
        media->bitrate = codecpar->bit_rate / 1000;

	//Get the sample rate
	media->sample_rate = codecpar->sample_rate;
	
	//Get the metadata
	 while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
	{
		char temp[256];
		snprintf(temp, sizeof(temp), "%s: %s\n", tag->key, tag->value);
		strncat(media->metadata, temp, sizeof(metadata) - strlen(media->metadata)-1);
    }
	
	avformat_close_input(&fmt_ctx);
}

void rotate_point(double x, double y, double cx, double cy, double angle, double *rx, double *ry)
{
    double cos_angle = cos(angle);
    double sin_angle = sin(angle);
    *rx = (x - cx) * cos_angle - (y - cy) * sin_angle + cx;
    *ry = (x - cx) * sin_angle + (y - cy) * cos_angle + cy;
}

void transform_coords(img_textbox *textbox, double x, double y, double *tx, double *ty)
{
    double cx = textbox->x + textbox->width / 2;
    double cy = textbox->y + textbox->height / 2;
    
    // Translate to origin
    x -= cx;
    y -= cy;
    
    // Rotate
    double rx = x * cos(-textbox->angle) - y * sin(-textbox->angle);
    double ry = x * sin(-textbox->angle) + y * cos(-textbox->angle);
    
    // Translate back
    *tx = rx + cx;
    *ty = ry + cy;
}

void select_word_at_position(img_textbox *textbox, int position)
{
    if (!textbox->text || position < 0 || position >= textbox->text->len)
        return;

    int start = position;
    int end = position;

    // Find start of word
    while (start > 0 && ! isspace(textbox->text->str[start - 1]))
    {
        start--;
    }

    // Find end of word
    while (end < textbox->text->len && ! isspace(textbox->text->str[end]))
    {
        end++;
    }

    textbox->selection_start = start;
    textbox->selection_end = end;
    textbox->cursor_pos = end;
}

void to_upper(gchar **string)
{
	char *s = string[0];
	while (*s)
	{
		*s = toupper((unsigned char) *s);
		s++;
	}
}

gint img_convert_time_string_to_seconds(gchar *string)
{
	gchar **parts = g_strsplit(string, ":", -1);
	gint total_seconds = 0;
	
	if (g_strv_length(parts) == 3)
	{
		gint hours = g_ascii_strtoll(parts[0], NULL, 10);
		gint minutes = g_ascii_strtoll(parts[1], NULL, 10);
		gint seconds = g_ascii_strtoll(parts[2], NULL, 10);
		total_seconds = hours * 3600 + minutes * 60 + seconds;
	}
	g_strfreev(parts);
	return total_seconds;
}

gchar *img_convert_time_to_string(gint duration_seconds)
{
	gchar *string;
	
	  // Convert duration to HH:MM:SS format
    int hours = (int)duration_seconds / 3600;
    int minutes = ((int)duration_seconds % 3600) / 60;
    int seconds = (int)duration_seconds % 60;
	
	string = g_strdup_printf("%02d:%02d:%02d", hours, minutes, seconds);
	return string;
}

gint find_nearest_major_tick(gint pixels_per_second, gint x)
{
    gdouble tick_interval = 1.0;

    if (pixels_per_second < 0.05) tick_interval = 1800.0;
    else if (pixels_per_second < 0.1) tick_interval = 900.0;
    else if (pixels_per_second < 0.2) tick_interval = 600.0;
    else if (pixels_per_second < 0.5) tick_interval = 300.0;
    else if (pixels_per_second < 1) tick_interval = 120.0;
    else if (pixels_per_second < 2) tick_interval = 60.0;
    else if (pixels_per_second < 5) tick_interval = 30.0;
    else if (pixels_per_second < 10) tick_interval = 15.0;
    else if (pixels_per_second < 20) tick_interval = 5.0;
    else if (pixels_per_second < 40) tick_interval = 2.0;
    else tick_interval = 1.0;

    gdouble tick_pixels = tick_interval * pixels_per_second;
    gint nearest_tick = round(x / tick_pixels) * tick_pixels;
    
    return nearest_tick;
}

const gchar *img_get_media_id_and_filename(img_window_struct *img, gint id, gint *media_type)
{
	GtkTreeIter iter;
	const gchar *filename;
	media_struct *entry;

	if( gtk_tree_model_get_iter_first(GTK_TREE_MODEL(img->media_model), &iter) )
	{
		do
		{
			gtk_tree_model_get(GTK_TREE_MODEL(img->media_model), &iter, 2, &entry, -1);
			if (entry->id == id)
			{
				*media_type = entry->media_type;
				return entry->full_path;
			}
		}
		while( gtk_tree_model_iter_next(GTK_TREE_MODEL(img->media_model), &iter) );
	}
	
	return NULL; 
}
