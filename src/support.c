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
static gboolean filter_function(GtkTreeModel *model, GtkTreeIter *iter, img_window_struct *);

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

//~ GdkPixbuf *img_set_fade_gradient(img_window_struct *img, gint gradient, media_struct *slide_info)
//~ {
	//~ GdkPixbuf		*pix = NULL;
	//~ GtkTreeIter 	iter;
	//~ GtkTreeModel 	*model;
	//~ gpointer     	address;

	//~ if (gradient == 3)
	//~ {
		//~ model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->transition_type ) );
		//~ /* 10:0 is the path string pointing to the Cross Fade transition */
		//~ gtk_tree_model_get_iter_from_string( model, &iter, "10:0" );
		//~ gtk_tree_model_get( model, &iter, 0, &pix,
										  //~ 2, &address,
										 //~ -1 );
		//~ slide_info->transition_id = 19;
		//~ slide_info->render = address;
		//~ if (slide_info->path)
		//~ {
			//~ g_free(slide_info->path);
			//~ slide_info->path = g_strdup("10:0");
		//~ }
	//~ }
	//~ else
	//~ {
		//~ if (slide_info->path)
		//~ {
			//~ g_free(slide_info->path);
			//~ slide_info->path = g_strdup("0");
		//~ }
		//~ slide_info->transition_id = -1;
		//~ slide_info->render = NULL;
	//~ }
	//~ return pix;
//~ }

//~ void
//~ img_set_slide_transition_info( media_struct      *slide,
							   //~ GtkListStore      *store,
							   //~ GtkTreeIter       *iter,
							   //~ GdkPixbuf         *pix,
							   //~ const gchar       *path,
							   //~ gint               transition_id,
							   //~ ImgRender          render,
							   //~ img_window_struct *img )
//~ {
	//~ /* Set transition render. */
	//~ if( path && ( slide->transition_id != transition_id ) )
	//~ {
		//~ if( slide->path )
			//~ g_free( slide->path );

		//~ slide->path = g_strdup( path );
		//~ slide->transition_id = transition_id;
		//~ slide->render = render;

		//~ gtk_list_store_set( store, iter, 2, pix, -1 );
	//~ }

//~ }

void img_set_slide_ken_burns_info( media_struct *slide,
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
	//~ if( full )
		//~ slide->duration = full;
}

void img_free_media_text_struct(media_text *entry)
{
	g_object_unref(entry->layout);
	cairo_destroy(entry->cr);
	cairo_surface_destroy(entry->surface);
	pango_font_description_free(entry->font_desc);
	pango_attr_list_unref(entry->attr_list);
	g_string_free(entry->text, TRUE);
    g_free(entry);
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

//~ void
//~ img_sync_timings( media_struct  *slide, img_window_struct *img )
//~ {
	//~ /* If times are already synchronized, return */
	//~ if( slide->duration >= slide->anim_duration )
		//~ return;

	//~ /* Do the right thing;) */
	//~ if( slide->no_points )
	//~ {
		//~ gint          diff;
		//~ ImgStopPoint *point;

		//~ /* Calculate difference that we need to accomodate */
		//~ diff = slide->anim_duration - slide->duration;

		//~ /* Elongate last point */
		//~ point = (ImgStopPoint *)g_list_last( slide->points )->data;
		//~ point->time += diff;
		
		//~ /* Update Ken Burns display */
		//~ gtk_spin_button_set_value( GTK_SPIN_BUTTON( img->ken_duration ),
								   //~ point->time );
	//~ }
//~ }

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
	media_text	*item = NULL;//img->current_text_item;
	GdkPixbuf 		*pixbuf;
	GtkWidget		*tmp_image,*fc;
	GtkIconTheme	*icon_theme;

	if (item->pattern_filename)
	{
		g_free(item->pattern_filename);
		item->pattern_filename = NULL;
	}
	icon_theme = gtk_icon_theme_get_default();
	pixbuf = gtk_icon_theme_load_icon(icon_theme,"image", 20, 0, NULL);
	tmp_image = gtk_image_new_from_pixbuf(pixbuf);
	gtk_widget_show(tmp_image);
	g_object_unref(pixbuf);
	
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(img->pattern_image), tmp_image);
	gtk_widget_set_sensitive(img->sub_font_color, TRUE);
	gtk_widget_set_tooltip_text(img->sub_font_color, _("Click to choose the font pattern"));

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
	GtkWidget *dialog, *action_area;
	gint response;
	
	dialog = gtk_message_dialog_new(GTK_WINDOW(img_struct->imagination_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL, "%s", msg);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), msg);
	gtk_container_set_border_width( GTK_CONTAINER(dialog ), 10 );		
	gtk_window_set_title(GTK_WINDOW(dialog),"Imagination");

	action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
    gtk_button_box_set_layout(GTK_BUTTON_BOX(action_area), GTK_BUTTONBOX_SPREAD);
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
	media->audio_type = g_strdup(fmt_ctx->iformat->long_name);
	to_upper(&media->audio_type);
	
	AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;

	//Get the bitrate
	if (fmt_ctx->bit_rate)
		media->bitrate = fmt_ctx->bit_rate / 1000;
	else if (codecpar->bit_rate)
        media->bitrate = codecpar->bit_rate / 1000;

	//Get the sample rate
	media->sample_rate = codecpar->sample_rate;
	
	//Get the channels
	media->channels = codecpar->ch_layout.nb_channels;

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

void transform_coords(media_text *textbox, double x, double y, double *tx, double *ty)
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

void select_word_at_position(media_text *textbox, int position)
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

gchar *img_convert_time_to_string(gdouble duration_seconds)
{
	gchar *string;
	
	  // Convert duration to HH:MM:SS format
    int hours = (int)duration_seconds / 3600;
    int minutes = ((int)duration_seconds % 3600) / 60;
    int seconds = (int)duration_seconds % 60;
	
	string = g_strdup_printf("%02d:%02d:%02d", hours, minutes, seconds);
	return string;
}

gdouble find_nearest_major_tick(gdouble pixels_per_second, gdouble x) 
{
    // Use gdouble for better precision
    gdouble tick_interval = 1.0;
    
    // Determine tick interval based on zoom level
    if (pixels_per_second < 0.05) tick_interval = 1800.0;         // 30 minutes
    else if (pixels_per_second < 0.1) tick_interval = 900.0;      // 15 minutes
    else if (pixels_per_second < 0.2) tick_interval = 600.0;      // 10 minutes
    else if (pixels_per_second < 0.5) tick_interval = 300.0;      // 5 minutes
    else if (pixels_per_second < 1) tick_interval = 120.0;        // 2 minutes
    else if (pixels_per_second < 2) tick_interval = 60.0;         // 1 minute
    else if (pixels_per_second < 5) tick_interval = 30.0;         // 30 seconds
    else if (pixels_per_second < 10) tick_interval = 15.0;        // 15 seconds
    else if (pixels_per_second < 20) tick_interval = 5.0;         // 5 seconds
    else if (pixels_per_second < 40) tick_interval = 2.0;         // 2 seconds
    else tick_interval = 1.0;                                     // 1 second

    // Calculate pixels between ticks
    gdouble tick_pixels = tick_interval * pixels_per_second;
    
    // Prevent division by zero
    if (tick_pixels < 1.0) tick_pixels = 1.0;
    
    // Calculate nearest tick using double precision before rounding to int
    gdouble exact_position = x / tick_pixels;
    gdouble rounded_position = round(exact_position);
    gdouble nearest_tick = rounded_position * tick_pixels;
    
    // Ensure we don't return negative values
    return MAX(0, nearest_tick);
}

const gchar *img_get_media_filename(img_window_struct *img, gint id)
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
				return entry->full_path;
			}
		}
		while( gtk_tree_model_iter_next(GTK_TREE_MODEL(img->media_model), &iter) );
	}
	
	return NULL; 
}

void img_free_cached_preview_surfaces(gpointer data)
{
	cairo_surface_t *surface = data;
	cairo_surface_destroy(surface);
}

void img_create_cached_cairo_surface(img_window_struct *img, media_timeline *item, gchar *full_path)
{
	GdkPixbuf *pix = NULL;
	gint item_id = 0, width, height;
	cairo_surface_t *surface;
	cairo_t *cr;

	if (! g_hash_table_contains(img->cached_preview_surfaces, GINT_TO_POINTER(item->id)))
	{
		pix = gdk_pixbuf_new_from_file_at_scale(full_path, img->video_size[0] * img->image_area_zoom, img->video_size[1] * img->image_area_zoom, TRUE, NULL);
		width =  gdk_pixbuf_get_width(pix);
		height = gdk_pixbuf_get_height(pix);
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		cr = cairo_create(surface);
		gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);
		g_object_unref(pix);
		item->width = width;
		item->height = height;

		g_hash_table_insert(img->cached_preview_surfaces, GINT_TO_POINTER(item->id), (gpointer) surface);
	}
}

void img_apply_button_styles(GtkWidget *button)
{
    GtkStyleContext *button_context = gtk_widget_get_style_context(button);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,"button{margin:0; padding:0; min-height: 0; min-width: 0}", -1, NULL);
    gtk_style_context_add_provider(button_context, GTK_STYLE_PROVIDER(provider),  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

GdkPixbuf* img_create_bordered_pixbuf(int width, int height, gboolean is_fit)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    
    // Start with black background
    gdk_pixbuf_fill(pixbuf, 0x000000FF);
    
    // Get direct access to pixel data
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    
    if (is_fit)
    {
        // Calculate blue center position
        int center_x = (width - 8) / 2;
        
        // Draw the blue center
        for (int y = 1; y < height - 1; y++) {
            for (int x = center_x; x < center_x + 8; x++) {
                guchar *p = pixels + y * rowstride + x * n_channels;
                p[0] = 0xA0;    // R
                p[1] = 0xA0;    // G
                p[2] = 0xA0;    // B
                p[3] = 0xFF;    // A
            }
        }
    } 
    else
    {
        // Fill everything except border with blue
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                guchar *p = pixels + y * rowstride + x * n_channels;
                p[0] = 0xA0;    // R
                p[1] = 0xA0;    // G
                p[2] = 0xA0;    // B
                p[3] = 0xFF;    // A
            }
        }
    }
    return pixbuf;
}

GtkWidget *img_create_flip_button(gboolean horizontal)
{
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 20, 20);
    cairo_t* cr = cairo_create(surface);

    GdkRGBA color;
    gdk_rgba_parse( &color, "#A0A0A0");

    cairo_set_source_rgb(cr, 0, 0, 0); 
    cairo_paint(cr);

    // Draw the arrow
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_set_line_width(cr, 3.0);

    if (horizontal)
    {
      cairo_move_to(cr, 3, 20 / 2);
      cairo_line_to(cr, 20 - 3, 20 / 2);
      cairo_stroke(cr);

      cairo_move_to(cr, 20 - 6, (20 / 2) - 3);
      cairo_line_to(cr, 20 - 3, 20 / 2);
      cairo_line_to(cr, 20 - 6, (20 / 2) + 3);
      cairo_stroke(cr);


      cairo_move_to(cr, 6, (20 / 2) - 3);
      cairo_line_to(cr, 3, 20 / 2);
      cairo_line_to(cr, 6, (20 / 2) + 3);
      cairo_stroke(cr);
    }
    else
    {
      cairo_move_to(cr, 20 / 2, 3);
      cairo_line_to(cr, 20 / 2, 20 - 3);
      cairo_stroke(cr);

      cairo_move_to(cr, (20 / 2) - 3, 6);
      cairo_line_to(cr, 20 / 2, 3);
      cairo_line_to(cr, (20 / 2) + 3, 6);
      cairo_stroke(cr);

      cairo_move_to(cr, (20 / 2) - 3, 20 - 6);
      cairo_line_to(cr, 20 / 2, 20 - 3);
      cairo_line_to(cr, (20 / 2) + 3, 20 - 6);
      cairo_stroke(cr);
    }
    cairo_destroy(cr);

    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, 20, 20);
	GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
	GtkWidget *button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(button), image);
	img_apply_button_styles(button);
	
	g_object_unref(pixbuf);
	cairo_surface_destroy(surface);
	return button;
}

GtkWidget *img_create_rotate_button()
{
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 20, 20);
    cairo_t* cr = cairo_create(surface);

    GdkRGBA color;
    gdk_rgba_parse( &color, "#A0A0A0");

    cairo_set_source_rgb(cr, 0, 0, 0); 
    cairo_paint(cr);

    // Draw the arrow
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_set_line_width(cr, 3.0);
    
    // Draw the L shape
	cairo_move_to(cr, (20 / 2) + 6, 4);
	cairo_rel_line_to(cr, 0, 6);
	cairo_rel_line_to(cr, -6, 0);
	cairo_stroke(cr);
	
	// Draw the arc
	cairo_arc(cr, (20 / 2), 10, 5, 30.0 * (G_PI / 180.0), 340.0 * (G_PI / 180.0));
	cairo_stroke(cr);
	cairo_destroy(cr);

    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, 20, 20);
	GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
	GtkWidget *button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(button), image);
	img_apply_button_styles(button);
	
	g_object_unref(pixbuf);
	cairo_surface_destroy(surface);
	return button;
}

static gboolean filter_function(GtkTreeModel *model, GtkTreeIter *iter, img_window_struct *img)
{
    gchar *filename;
    const gchar *filter_text;
	gboolean visible = FALSE;
	
	filter_text = gtk_entry_get_text(GTK_ENTRY(img->media_library_filter));
    // Avoid segfault if they're NULL
     if (!filter_text || !*filter_text)
        return TRUE;

	gtk_tree_model_get(model, iter, 1, &filename, -1);
    if (filename)
    {
		visible = (strstr(filename, filter_text) != NULL);
        g_free(filename);
	}
    return visible;
}

void img_filter_icon_view(GtkEntry *entry, img_window_struct *img)
{
    GtkTreeModel *current_model;
    const gchar *filter_text;
    
    g_return_if_fail(img != NULL);
    g_return_if_fail(GTK_IS_ENTRY(entry));
    
    filter_text = gtk_entry_get_text(entry);
    current_model = gtk_icon_view_get_model(GTK_ICON_VIEW(img->media_iconview));
    
    if (filter_text && *filter_text)
    {
        if (!img->media_model_filter)
        {
            // Create a new filter model
            GtkTreeModel *filter_model = gtk_tree_model_filter_new(GTK_TREE_MODEL(img->media_model), NULL);
            
            img->media_model_filter = GTK_TREE_MODEL_FILTER(g_object_ref(filter_model));
            gtk_tree_model_filter_set_visible_func(img->media_model_filter, (GtkTreeModelFilterVisibleFunc)filter_function, img, NULL);
            gtk_icon_view_unselect_all(GTK_ICON_VIEW(img->media_iconview));
            gtk_icon_view_set_model(GTK_ICON_VIEW(img->media_iconview), filter_model);
            g_object_unref(filter_model);
        }
        gtk_tree_model_filter_refilter(img->media_model_filter);
    }
    else
    {
        if (current_model != GTK_TREE_MODEL(img->media_model))
        {
            gtk_icon_view_set_model(GTK_ICON_VIEW(img->media_iconview), GTK_TREE_MODEL(img->media_model));
            if (img->media_model_filter)
            {
                g_object_unref(img->media_model_filter);
                img->media_model_filter = NULL;
            }
        }
    }
}

void img_select_surface_on_click(img_window_struct *img, gdouble x, gdouble y)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	GArray *active_media;
	Track *track;
	media_timeline *item;
	cairo_surface_t *surface = NULL;
	gdouble scale, w, h, current_time;
	GtkAllocation allocation;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(img->toggle_button_text)))
		return;

	gtk_widget_get_allocation(img->image_area, &allocation);
	g_object_get(G_OBJECT(img->timeline), "time_marker_pos", &current_time, NULL);
	
	img_deselect_all_surfaces(img);

	active_media = img_timeline_get_active_picture_media(img->timeline, current_time);
	for (gint i = 0; i < active_media->len; i++)
	{
		item = g_array_index(active_media, media_timeline *,i);
		surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
		if (surface)
		{
			w = cairo_image_surface_get_width(surface);
			h = cairo_image_surface_get_height(surface);
			scale = MIN((double)allocation.width / w, (double)allocation.height / h);
			if (x >= item->x && x <= item->x + w * scale && y >= item->y && y <= item->y + h * scale)
			{
				item->is_selected = TRUE;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->button), TRUE);
				gtk_notebook_set_current_page (GTK_NOTEBOOK(img->side_notebook), 1);
				item->drag_x = x - item->x;
				item->drag_y = y - item->y;
				img->current_item = item;
				img_timeline_set_media_properties(img,  item);
				goto end;
			}
		}
	}
end:
	g_array_free(active_media, FALSE);
}

void img_deselect_all_surfaces(img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		for (gint j = 0; j < track->items->len; j++)
		{
			item = g_array_index(track->items, media_timeline *, j);
			item->is_selected = FALSE;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->button), FALSE);
		}
	}	
}

void img_draw_rotation_angle(cairo_t *cr, gdouble x, gdouble width, gdouble y, gdouble height,  gdouble angle)
{
	cairo_save(cr);
	cairo_translate(cr, x + width / 2, y + height / 2);
	cairo_rotate(cr, -angle);
	cairo_translate(cr, -(x + width / 2), -(y + height / 2));
	
	gchar buf[8];
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
	cairo_rectangle(cr, 5, 5, 45, 25);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 16.0);
	g_snprintf(buf, sizeof(buf), "%2.0f%s", round(angle * (180.0 / G_PI)), "\xC2\xB0");
	cairo_move_to(cr, 15, 24);
	cairo_show_text(cr, buf);
	cairo_restore(cr);
}

void img_draw_rotating_handle(cairo_t *cr, gdouble x, gdouble width, gdouble y, gdouble height, gdouble scale, gboolean textbox)
{
	gdouble l_shape_offset, arc_offset, vertical_line_offset;

	if (textbox)
	{
		l_shape_offset = -8;
		arc_offset = -15;
		vertical_line_offset = -10;
	}
	else
	{
		l_shape_offset = 24;
		arc_offset = 17;
		vertical_line_offset = 13;
	}	
	// Draw the rotating handle
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 3.5);
	
	// Draw the L shape
	cairo_move_to(cr, x + (width * scale / 2) + 6, y + height * scale - l_shape_offset);
	cairo_rel_line_to(cr, 0, 6);
	cairo_rel_line_to(cr, -6, 0);
	cairo_stroke(cr);
	
	// Draw the arc
	cairo_arc(cr, x + (width * scale/ 2), y + height * scale - arc_offset, 5, 30.0 * (G_PI / 180.0), 340.0 * (G_PI / 180.0));
	cairo_stroke(cr);
	
	// Draw the vertical line under the rotate circle
	cairo_move_to(cr, x + (width * scale/ 2), y + height * scale - vertical_line_offset);
	cairo_line_to(cr, x + (width * scale/ 2), y + height * scale);
	cairo_stroke(cr);
	
	// Now draw the black lines on top
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1.5);  // Original line width
	
	// Draw the L shape again
	cairo_move_to(cr, x + (width * scale/ 2) + 6, y + height * scale- l_shape_offset);
	cairo_rel_line_to(cr, 0, 6);
	cairo_rel_line_to(cr, -6, 0);
	cairo_stroke(cr);
	
	// Draw the arc again
	cairo_arc(cr, x + (width * scale/ 2), y + height * scale - arc_offset, 5, 30.0 * (G_PI / 180.0), 340.0 * (G_PI / 180.0));
	cairo_stroke(cr);
	
	// Draw the vertical line under the rotate circle again
	cairo_set_line_width(cr, 1);
	cairo_move_to(cr, x + (width * scale/ 2), y + height * scale - vertical_line_offset);
	cairo_line_to(cr, x + (width * scale/ 2), y + height * scale);
	cairo_stroke(cr);
}

void img_flip_surface_horizontally(img_window_struct *img, media_timeline *item)
{
	cairo_surface_t *surface, *flipped_surface;
	cairo_t *cr;
	gint width, height;

	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	width = cairo_image_surface_get_width(surface);
	height = cairo_image_surface_get_height(surface);
	flipped_surface = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, width, height);
	cr = cairo_create(flipped_surface);

	// Flip horizontally around the center of the image
	cairo_translate(cr, width / 2.0, 0);
	cairo_scale(cr, -1, 1);
	cairo_translate(cr, -width / 2.0, 0);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	g_hash_table_remove(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	g_hash_table_insert(img->cached_preview_surfaces, GINT_TO_POINTER(item->id), (gpointer) flipped_surface);
}

void img_flip_surface_vertically(img_window_struct *img, media_timeline *item)
{
	cairo_surface_t *surface, *flipped_surface;
	cairo_t *cr;
	gint width, height;

	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	width = cairo_image_surface_get_width(surface);
	height = cairo_image_surface_get_height(surface);
	flipped_surface = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, width, height);
	cr = cairo_create(flipped_surface);

	// Flip vertically around the center of the image
	cairo_translate(cr, 0, height / 2.0);
	cairo_scale(cr, 1, -1);
	cairo_translate(cr, 0, -height / 2.0);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	g_hash_table_remove(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	g_hash_table_insert(img->cached_preview_surfaces, GINT_TO_POINTER(item->id), (gpointer) flipped_surface);
}

void img_rotate_surface(img_window_struct *img, media_timeline *item, gboolean update_angle)
{
	cairo_surface_t *surface, *rotated_surface;
	cairo_t *cr;
	gint width, height;
	gdouble old_cx, old_cy;
    gdouble new_cx, new_cy;
    
	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	width = cairo_image_surface_get_width(surface);
	height = cairo_image_surface_get_height(surface);
	
	rotated_surface = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, height, width);
	
	old_cx = item->x + width / 2.0;
    old_cy = item->y + height / 2.0;
    
    // Rotate the surface
	cr = cairo_create(rotated_surface);
	cairo_translate(cr, height / 2.0, width / 2.0);
	cairo_rotate(cr, G_PI / 2.0);
	cairo_translate(cr, -width / 2.0, -height / 2.0);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);

	// Update the hash table
	g_hash_table_remove(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	g_hash_table_insert(img->cached_preview_surfaces, GINT_TO_POINTER(item->id), (gpointer)rotated_surface);
	
	new_cx = old_cx;
    new_cy = old_cy;
    
    // Update position to maintain center point after rotation
    item->x = new_cx - height  / 2.0;
    item->y = new_cy - width  / 2.0;
    
    if (update_angle)
	{
		item->nr_rotations ++;
		if (item->nr_rotations >= 4)
			item->nr_rotations = 0;
	}
}

void img_turn_surface_black_and_white(img_window_struct *img, media_timeline *item)
{
	cairo_surface_t *surface;
	gint width, height, stride;
	unsigned char *data, *row, *pixel;
	
	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	
    width = cairo_image_surface_get_width(surface);
    height = cairo_image_surface_get_height(surface);
    stride = cairo_image_surface_get_stride(surface);
    data = cairo_image_surface_get_data(surface);

    cairo_surface_flush(surface);

    for (int y = 0; y < height; y++)
    {
        row = data + y * stride;
        for (int x = 0; x < width; x++)
        {
            pixel = row + x * 4;
            
            // Calculate grayscale value using luminosity method
            unsigned char r = pixel[2];
            unsigned char g = pixel[1];
            unsigned char b = pixel[0];
            
            unsigned char gray = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
            
            pixel[0] = gray;  // Blue
            pixel[1] = gray;  // Green
            pixel[2] = gray;  // Red
        }
    }
    cairo_surface_mark_dirty(surface);
}

void img_turn_surface_sepia(img_window_struct *img, media_timeline *item)
{
	cairo_surface_t *surface;
	gint width, height, stride;
	unsigned char *data, *row, *pixel;
	
	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	
    width = cairo_image_surface_get_width(surface);
    height = cairo_image_surface_get_height(surface);
    stride = cairo_image_surface_get_stride(surface);
    data = cairo_image_surface_get_data(surface);

    cairo_surface_flush(surface);
	
	for (int y = 0; y < height; y++)
	{
		row = data + y * stride;
		for (int x = 0; x < width; x++)
		{
			pixel = row + x * 4;
			
			// Original RGB values
			unsigned char r = pixel[2];
			unsigned char g = pixel[1];
			unsigned char b = pixel[0];
			
			// Sepia tone calculation
			unsigned char new_r = (unsigned char)fmin(255, 0.393 * r + 0.769 * g + 0.189 * b);
			unsigned char new_g = (unsigned char)fmin(255, 0.349 * r + 0.686 * g + 0.168 * b);
			unsigned char new_b = (unsigned char)fmin(255, 0.272 * r + 0.534 * g + 0.131 * b);
			
			pixel[0] = new_b;  // Blue
			pixel[1] = new_g;  // Green
			pixel[2] = new_r;  // Red
		}
	}
    cairo_surface_mark_dirty(surface);
}

void img_turn_surface_infrared(img_window_struct *img, media_timeline *item)
{
	cairo_surface_t *surface;
	gint width, height, stride;
	unsigned char *data, *row, *pixel;
	
	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	
    width = cairo_image_surface_get_width(surface);
    height = cairo_image_surface_get_height(surface);
    stride = cairo_image_surface_get_stride(surface);
    data = cairo_image_surface_get_data(surface);

    cairo_surface_flush(surface);

	for (int y = 0; y < height; y++)
	{
		row = data + y * stride;
		for (int x = 0; x < width; x++)
		{
			pixel = row + x * 4;
			
			unsigned char r = pixel[2];
			unsigned char g = pixel[1];
			unsigned char b = pixel[0];
			
			// Infrared simulation: swap channels, emphasize red
			unsigned char infrared_r = fmax(g, b);
			unsigned char infrared_g = fmin(r, 128);
			unsigned char infrared_b = fmin(b, 64);
			
			pixel[0] = infrared_b;  // Blue
			pixel[1] = infrared_g;  // Green
			pixel[2] = infrared_r;  // Red
		}
	}
    cairo_surface_mark_dirty(surface);
}

void img_turn_surface_pencil_sketch(img_window_struct *img, media_timeline *item)
{
	cairo_surface_t *surface;
	gint width, height, stride;
	unsigned char *data, *row, *pixel;
	
	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	
    width = cairo_image_surface_get_width(surface);
    height = cairo_image_surface_get_height(surface);
    stride = cairo_image_surface_get_stride(surface);
    data = cairo_image_surface_get_data(surface);

    cairo_surface_flush(surface);
	// First pass: edge detection
    for (int y = 1; y < height - 1; y++)
    {
        row = data + y * stride;
        for (int x = 1; x < width - 1; x++)
        {
            unsigned char *pixel = row + x * 4;
            unsigned char *pixel_left = row + (x-1) * 4;
            unsigned char *pixel_right = row + (x+1) * 4;
            unsigned char *row_above = data + (y-1) * stride;
            unsigned char *row_below = data + (y+1) * stride;
            
            // Simplified edge detection
            int edge_x = abs(
                (pixel_left[2] + pixel_left[1] + pixel_left[0]) - 
                (pixel_right[2] + pixel_right[1] + pixel_right[0])
            );
            int edge_y = abs(
                (row_above[x*4+2] + row_above[x*4+1] + row_above[x*4]) - 
                (row_below[x*4+2] + row_below[x*4+1] + row_below[x*4])
            );
            
            // Invert and scale edges
            unsigned char edge_intensity = 255 - fmin(255, (edge_x + edge_y) / 2);
            
            pixel[0] = edge_intensity;  // Blue
            pixel[1] = edge_intensity;  // Green
            pixel[2] = edge_intensity;  // Red
        }
    }

    cairo_surface_mark_dirty(surface);
}

void img_turn_surface_negative(img_window_struct *img, media_timeline *item)
{
	cairo_surface_t *surface;
	gint width, height, stride;
	unsigned char *data, *row, *pixel;
	
	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	
    width = cairo_image_surface_get_width(surface);
    height = cairo_image_surface_get_height(surface);
    stride = cairo_image_surface_get_stride(surface);
    data = cairo_image_surface_get_data(surface);

    cairo_surface_flush(surface);
	for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char *pixel = data + y * stride + x * 4;

            // Invert RGB channels (leave alpha unchanged)
            pixel[0] = 255 - pixel[0];  // Blue
            pixel[1] = 255 - pixel[1];  // Green
            pixel[2] = 255 - pixel[2];  // Red
        }
    }
    cairo_surface_mark_dirty(surface);
}

void img_turn_surface_emboss(img_window_struct *img, media_timeline *item)
{
	cairo_surface_t *surface;
	gint width, height, stride;
	unsigned char *data, *row, *pixel;
	
	surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
	
    width = cairo_image_surface_get_width(surface);
    height = cairo_image_surface_get_height(surface);
    stride = cairo_image_surface_get_stride(surface);
    data = cairo_image_surface_get_data(surface);

	unsigned char *buffer = malloc(width * height * 4);
    memcpy(buffer, data, width * height * 4);

    cairo_surface_flush(surface);
	for (int y = 1; y < height - 1; y++)
	{
        for (int x = 1; x < width - 1; x++)
        {
            // Different sampling directions for more robust embossing
            int emboss_kernels[4][6] = {
                // Horizontal kernel (left to right)
                {-1, -1, 0, 0, 1, 1},
                // Vertical kernel (top to bottom)
                {-1, 0, 1, -1, 0, 1},
                // Diagonal (top-left to bottom-right)
                {-1, 0, 0, 0, 1, 0},
                // Diagonal (top-right to bottom-left)
                {0, 1, 0, -1, 0, 0}
            };

            int total_emboss_r = 0;
            int total_emboss_g = 0;
            int total_emboss_b = 0;

            // Apply multiple emboss kernels
            for (int k = 0; k < 4; k++) {
                // Sample pixels based on kernel
                unsigned char *pixel1 = buffer + (y + emboss_kernels[k][1]) * stride + (x + emboss_kernels[k][0]) * 4;
                unsigned char *pixel2 = buffer + (y + emboss_kernels[k][3]) * stride + (x + emboss_kernels[k][2]) * 4;
                unsigned char *pixel3 = buffer + (y + emboss_kernels[k][5]) * stride + (x + emboss_kernels[k][4]) * 4;

                // Calculate difference
                total_emboss_r += 
                    (pixel1[2] * emboss_kernels[k][0]) + 
                    (pixel2[2] * emboss_kernels[k][2]) + 
                    (pixel3[2] * emboss_kernels[k][4]);
                
                total_emboss_g += 
                    (pixel1[1] * emboss_kernels[k][0]) + 
                    (pixel2[1] * emboss_kernels[k][2]) + 
                    (pixel3[1] * emboss_kernels[k][4]);
                
                total_emboss_b += 
                    (pixel1[0] * emboss_kernels[k][0]) + 
                    (pixel2[0] * emboss_kernels[k][2]) + 
                    (pixel3[0] * emboss_kernels[k][4]);
            }

            // Current pixel
            unsigned char *pixel = data + y * stride + x * 4;

            // Combine emboss effect with original pixel
            int emboss_r = 128 + total_emboss_r / 4;
            int emboss_g = 128 + total_emboss_g / 4;
            int emboss_b = 128 + total_emboss_b / 4;

            // Clamp values
            pixel[0] = fmin(255, fmax(0, emboss_b));  // Blue
            pixel[1] = fmin(255, fmax(0, emboss_g));  // Green
            pixel[2] = fmin(255, fmax(0, emboss_r));  // Red

            // Optional: Enhance contrast slightly
            pixel[0] = fmin(255, pixel[0] * 1.2);
            pixel[1] = fmin(255, pixel[1] * 1.2);
            pixel[2] = fmin(255, pixel[2] * 1.2);
        }
    }

    free(buffer);
    cairo_surface_mark_dirty(surface);
}

void img_apply_filter_on_surface(img_window_struct *img, media_timeline *item, gint filter_nr)
{
	switch (filter_nr)
	{
		case 0:
		// The original surface is restored above
		break;
		
		case 1:
			img_turn_surface_black_and_white(img, item);
		break;
		
		case 2:
			img_turn_surface_sepia(img, item);
		break;

		case 3:
			img_turn_surface_infrared(img, item);
		break;
		
		case 4:
			img_turn_surface_pencil_sketch(img, item);
		break;
		
		case 5:
			img_turn_surface_negative(img, item);
		break;

		case 6:
			img_turn_surface_emboss(img, item);
		break;
	}
}

void img_draw_horizontal_line(cairo_t *cr, GtkAllocation *allocation)
{
	gint center_y = allocation->height / 2;
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, 0, center_y);
	cairo_line_to(cr, allocation->width, center_y - 2);
	cairo_stroke(cr);
	cairo_set_source_rgb(cr, 0.8, 0.7, 0.3);
	cairo_move_to(cr, 0, center_y);
	cairo_line_to(cr, allocation->width, center_y);
	cairo_stroke(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_move_to(cr, 0, center_y + 2);
	cairo_line_to(cr, allocation->width, center_y + 2);
	cairo_stroke(cr);
}

void img_draw_vertical_line(cairo_t *cr, GtkAllocation *allocation)
{
	gint center_x = allocation->width / 2;
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, center_x - 2, 0);
	cairo_line_to(cr, center_x - 2, allocation->height);
	cairo_stroke(cr);
	cairo_set_source_rgb(cr, 0.8, 0.7, 0.3);
	cairo_move_to(cr, center_x, 0);
	cairo_line_to(cr, center_x, allocation->height);
	cairo_stroke(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_move_to(cr, center_x + 2, 0);
	cairo_line_to(cr, center_x + 2, allocation->height );
	cairo_stroke(cr);
}
