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

#include "callbacks.h"
#include "empty_slide.h"
#include "export.h"
#include <math.h>
#include <sys/stat.h>

static void img_file_chooser_add_preview(img_window_struct *);
static void img_update_preview_file_chooser(GtkFileChooser *,img_window_struct *);
static gboolean img_still_timeout(img_window_struct *);
static void img_image_area_change_zoom( gdouble step, gboolean reset, img_window_struct *img );

static void img_update_textbox_boundaries(img_window_struct *img)
{
	int width = gtk_widget_get_allocated_width(img->image_area);
	int height = gtk_widget_get_allocated_height(img->image_area);

	pango_layout_get_size(img->current_item->text->layout, &img->current_item->text->lw, &img->current_item->text->lh);
	
	img->current_item->text->lw /= PANGO_SCALE;
	img->current_item->text->lh /= PANGO_SCALE;
  
	// Adjust textbox width and height if needed
	if (img->current_item->text->lw > img->current_item->text->width - 15 && img->current_item->text->lw + 50 <= width)
		img->current_item->text->width = img->current_item->text->lw + 50;
	
	if (img->current_item->text->lh > img->current_item->text->height && img->current_item->text->lh + 20 <= height)
		img->current_item->text->height = img->current_item->text->lh + 20;
		
	pango_layout_set_width(img->current_item->text->layout, pango_units_from_double(img->current_item->text->width));
}

/* Asks the user before discarding changes */
gboolean img_can_discard_unsaved_project(img_window_struct *img)
{
	if (!img->project_is_modified)
		return TRUE;

	int response = img_ask_user_confirmation( img, _("You didn't save your project yet.\nAre you sure you want to close it?"));
	if (response == GTK_RESPONSE_OK)
		return TRUE;
	return FALSE;
}

/* Recomputes the title to the main window depending on whether a project is open
 * and whether it is modified */
void img_refresh_window_title(img_window_struct *img)
{
	gchar *title = NULL;

	if (img->project_filename == NULL)
	{
		title = g_strconcat("Imagination ", VERSION , NULL);
	}
	else
	{
		gchar *name = g_path_get_basename( img->project_filename );
		title = g_strconcat(
			img->project_is_modified ? "*" : "",
			name,
			" - Imagination ",
			VERSION,
			NULL);
		g_free(name);
	}
	gtk_window_set_title (GTK_WINDOW (img->imagination_window), title);
	g_free(title);
}

void img_new_project(GtkMenuItem *item, img_window_struct *img_struct)
{
	if (!img_can_discard_unsaved_project(img_struct))
		return;

	img_new_project_dialog(img_struct, FALSE);
}

void img_project_properties(GtkMenuItem *item, img_window_struct *img_struct)
{
	img_new_project_dialog(img_struct, TRUE);
}

void img_add_media_items(GtkMenuItem *item, img_window_struct *img)
{
	GSList  *media, *bak;
	
	media = img_import_slides_file_chooser(img);

	if (media == NULL)
		return;

	bak = media;
	while (media)
	{
		if (img_create_media_struct(media->data, img))
			img_taint_project(img);
		 
		media = media->next;
	}
	g_slist_free(bak);
}

gboolean img_create_media_struct(gchar *full_path, img_window_struct *img)
{
	GSList *bak;
	media_struct *media;
	GError *error = NULL;
	gint type;
	GFile *file;
	GFileInfo *file_info;
	const gchar *content_type;
	gchar *mime_type;
	gboolean flag = TRUE;

	if (img_find_media_in_list(img, full_path))
		return FALSE;
	
	/* Remove model from thumbnail iconview for efficiency */
	g_object_ref( G_OBJECT( img->media_model ) );
	gtk_icon_view_set_model(GTK_ICON_VIEW(img->media_iconview), NULL);

	//Determine the mime type
	file = g_file_new_for_path (full_path);
	file_info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
	content_type = g_file_info_get_content_type (file_info);
	mime_type = g_content_type_get_mime_type (content_type);

	if (strstr(mime_type, "image"))
		type = 0;
	else if (strstr(mime_type, "audio"))
		type = 1;
	else if (strstr(mime_type, "video"))
		type = 2;
	else
	{
		gchar *string = g_strconcat( _("Unsupported media:\n"), full_path, NULL);
		img_message(img, string);
		g_free(string);
		flag = FALSE;
		goto next_media;
	}
	media = g_new0(media_struct, 1);
	media->full_path = g_strdup(full_path);
	media->media_type = type;
	media->id = img->next_id++;
	img->media_nr++;
	img_add_media(media->full_path, media, img);

next_media:
		g_free(mime_type);
		g_object_unref(file_info);
		g_object_unref(file);

	// Reinstate the model
	gtk_icon_view_set_model(GTK_ICON_VIEW(img->media_iconview), GTK_TREE_MODEL(img->media_model) );
	g_object_unref( G_OBJECT( img->media_model ) );
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(img->side_notebook), 0);
	return flag;
}

void img_add_media(gchar *full_path, media_struct *media, img_window_struct *img)
{
	gchar *filename;
	GtkTreeIter iter;
	GdkPixbufFormat *format;
	GdkPixbuf *pb;

	filename = g_path_get_basename(full_path);
	gtk_list_store_append (img->media_model, &iter);

	switch (media->media_type)
	{
		case 0:
			format = gdk_pixbuf_get_file_info(full_path, &media->width, &media->height);
			media->image_type = format ? gdk_pixbuf_format_get_name(format) : NULL;
			to_upper(&media->image_type);
			pb = gdk_pixbuf_new_from_file_at_scale(full_path, 120, 60, TRUE, NULL);
			gtk_list_store_set (img->media_model, &iter, 0, pb, 1, filename, 2, media, -1);
			g_object_unref(pb);
		break;

		case 1:
			img_get_audio_data(media);
			pb = gtk_icon_theme_load_icon(img->icon_theme, "audio-x-generic", 46, 0, NULL);
			gtk_list_store_set (img->media_model, &iter, 0, pb, 1, filename, 2, media, -1);
			g_object_unref(pb);
		break;
	}
	img->current_media = media;
	g_free(filename);
}

GSList *img_import_slides_file_chooser(img_window_struct *img)
{
	GtkFileFilter *image_filter, *sound_filter, *all_files_filter;
	GSList *slides = NULL;
	int response;

	img->import_slide_chooser =
		gtk_file_chooser_dialog_new( _("Import any media, use SHIFT key for  multiple selection"),
									 GTK_WINDOW (img->imagination_window),
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 "_Cancel",
									 GTK_RESPONSE_CANCEL,
									 "_Open",
									 GTK_RESPONSE_ACCEPT,
									 NULL);
	img_file_chooser_add_preview(img);

	/* Image files filter */
	image_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name(image_filter,_("All image files"));
	gtk_file_filter_add_pixbuf_formats(image_filter);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(img->import_slide_chooser), image_filter);
	
	/* Sound files filter */
	sound_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name(sound_filter,_("All audio files"));
	gtk_file_filter_add_pattern(sound_filter,"*.aac");
	gtk_file_filter_add_pattern(sound_filter,"*.aiff");
	gtk_file_filter_add_pattern(sound_filter,"*.flac");
	gtk_file_filter_add_pattern(sound_filter,"*.mp2");
	gtk_file_filter_add_pattern(sound_filter,"*.mp3");
	gtk_file_filter_add_pattern(sound_filter,"*.ogg");
	gtk_file_filter_add_pattern(sound_filter,"*.pcm");
	gtk_file_filter_add_pattern(sound_filter,"*.wav");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(img->import_slide_chooser),sound_filter);

	/* All files filter */
	all_files_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name(all_files_filter,_("All files"));
	gtk_file_filter_add_pattern(all_files_filter,"*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(img->import_slide_chooser),all_files_filter);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(img->import_slide_chooser),TRUE);
	if (img->current_dir)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(img->import_slide_chooser),img->current_dir);
	response = gtk_dialog_run (GTK_DIALOG(img->import_slide_chooser));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		slides = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(img->import_slide_chooser));
		if (img->current_dir)
			g_free(img->current_dir);
		img->current_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(img->import_slide_chooser));
	}
	gtk_widget_destroy (img->import_slide_chooser);
	return slides;
}

void img_free_allocated_memory(img_window_struct *img)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	media_struct *entry;

	if (img->media_nr)
	{
		model = GTK_TREE_MODEL( img->media_model );

		gtk_tree_model_get_iter_first(model,&iter);
		do
		{
			gtk_tree_model_get(model, &iter, 2, &entry,-1);
			img_free_media_struct(entry);
		}
		while (gtk_tree_model_iter_next (model,&iter));
		gtk_list_store_clear(GTK_LIST_STORE(img->media_model));
		
		img_timeline_delete_all_media((ImgTimeline*)img->timeline);
		img_timeline_delete_additional_tracks((ImgTimeline*)img->timeline);
		g_hash_table_remove_all(img->cached_preview_surfaces);
	}
	img->media_nr = 0;

	/* Free gchar pointers */
	if (img->current_dir)
	{
		g_free(img->current_dir);
		img->current_dir = NULL;
	}
	
	if (img->project_current_dir)
	{
		g_free(img->project_current_dir);
		img->project_current_dir = NULL;
	}

	if (img->project_filename)
	{
		g_free(img->project_filename);
		img->project_filename = NULL;
	}
	if (img->slideshow_filename)
	{
		g_free(img->slideshow_filename);
		img->slideshow_filename = NULL;
	}
}

gboolean img_window_key_pressed(GtkWidget *widget, GdkEventKey *event, img_window_struct *img)
{
	//This hack to give image area key events as when the toplevel window contains other widgets due
	//to focus reasons the drawing area doesn't get any keyboard events despite mask and focus are set
	if (img->current_item->text)
	{
		if (img->current_item->text->visible)
			return gtk_widget_event(img->image_area, (GdkEvent*)event);
	}
	if (event->keyval == GDK_KEY_Tab)
	{
		if (gtk_widget_get_visible(img->sidebar))
		{
			gtk_widget_hide(img->menubar);
			gtk_widget_hide(img->sidebar);
			gtk_widget_hide(img->side_notebook);
		}
		else	
		{
			gtk_widget_show(img->menubar);
			gtk_widget_show(img->sidebar);
			gtk_widget_show(img->side_notebook);
		}
	}
		
	if ( img->window_is_fullscreen && (event->keyval == GDK_KEY_F11 || event->keyval == GDK_KEY_Escape))
		img_exit_fullscreen(img);
	else
			if (event->keyval == GDK_KEY_F11)
				img_go_fullscreen(NULL, img);

	return FALSE;	//If this returns TRUE menu accelerators do not work anymore
}

gboolean img_image_area_key_press(GtkWidget *widget, GdkEventKey *event, img_window_struct *img)
{
	gboolean shift_pressed;
	gboolean ctrl_pressed;
	
	if (!img->current_item->text->visible)
		return FALSE;

	shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;
	ctrl_pressed = (event->state & GDK_CONTROL_MASK) != 0;

	
	switch (event->keyval)
	{
		case GDK_KEY_a:
		case GDK_KEY_A:
		 if (ctrl_pressed)
		 {
			img->current_item->text->selection_start = 0;
			img->current_item->text->selection_end = img->current_item->text->text->len;
		 }
		else
			goto handle;
		break;
	
		case GDK_KEY_v:
		case GDK_KEY_V:
		if (ctrl_pressed)
		{
			GtkClipboard *clipboard;
			gchar *text;
		
			clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
			text = gtk_clipboard_wait_for_text(clipboard);
			if (text)
			{
				g_string_insert(img->current_item->text->text, img->current_item->text->cursor_pos, text);
				g_free(text);
				//If there was any selection clear it
				if (img->current_item->text->selection_start != -1 && img->current_item->text->selection_end != -1)
					img->current_item->text->selection_start = img->current_item->text->selection_end = -1;
			}
		}
		else
			goto handle;
		break;
	 
		case GDK_KEY_Left:
		 if (img->current_item->text->cursor_pos > 0)
		 {
			 img->current_item->text->cursor_pos--;
			 if (shift_pressed)
			 {
				 if (img->current_item->text->selection_start == -1) img->current_item->text->selection_start = img->current_item->text->cursor_pos + 1;
				 img->current_item->text->selection_end = img->current_item->text->cursor_pos;
			 }
			 else 
				img->current_item->text->selection_start = img->current_item->text->selection_end = -1;
		 }
		 img_move_surfaces_left(img);
		 break;
		 
		case GDK_KEY_Right:
		if (img->current_item->text->text->str && img->current_item->text->cursor_pos < img->current_item->text->text->len)
		{
			img->current_item->text->cursor_pos++;
			if (shift_pressed)
			{
				 if (img->current_item->text->selection_start == -1) img->current_item->text->selection_start = img->current_item->text->cursor_pos - 1;
				 img->current_item->text->selection_end = img->current_item->text->cursor_pos;
			}
			else
				 img->current_item->text->selection_start = img->current_item->text->selection_end = -1; 
		}
		img_move_surfaces_right(img);
		break;
		
		case GDK_KEY_Up:
		case GDK_KEY_Down:
			int index, trailing;
			PangoRectangle pos;
			
			pango_layout_get_cursor_pos(img->current_item->text->layout, img->current_item->text->cursor_pos, &pos, NULL);
			int line = pango_layout_xy_to_index(img->current_item->text->layout, 
				pos.x, 
				pos.y + (event->keyval == GDK_KEY_Down ? pos.height : -pos.height),
				&index, &trailing);
			if (line != -1)
			{
				img->current_item->text->cursor_pos = index + trailing;
				if (shift_pressed)
				{
					if (img->current_item->text->selection_start == -1)
						img->current_item->text->selection_start = img->current_item->text->cursor_pos;
					
					img->current_item->text->selection_end = img->current_item->text->cursor_pos;
				}
				else
					img->current_item->text->selection_start = img->current_item->text->selection_end = -1;
			}
		break;

		case GDK_KEY_BackSpace:
		if (img->current_item->text->text->len > 0)
		{
			// Text is selected
			if (img->current_item->text->selection_start != -1 && img->current_item->text->selection_end != -1)
			{
				int start = MIN(img->current_item->text->selection_start, img->current_item->text->selection_end);
				int end = MAX(img->current_item->text->selection_start, img->current_item->text->selection_end);
				
				// User pressed CTRL+A
				if(img->current_item->text->selection_start == 0 && img->current_item->text->selection_end == img->current_item->text->text->len)
				{
					pango_layout_set_attributes(img->current_item->text->layout, NULL);
					img->current_item->text->attr_list = pango_attr_list_new();
					pango_layout_set_attributes(img->current_item->text->layout, img->current_item->text->attr_list);
					pango_attr_list_unref(img->current_item->text->attr_list);
					
					
					g_string_erase(img->current_item->text->text,  0, img->current_item->text->selection_end);
				}
				else
					g_string_erase(img->current_item->text->text,  start,  end - start);

				img->current_item->text->cursor_pos = start;
				img->current_item->text->selection_start = img->current_item->text->selection_end = -1;
			}
			else
			{
				if (img->current_item->text->cursor_pos > 0)
				{
					g_string_erase(img->current_item->text->text, img->current_item->text->cursor_pos-1, 1);
					img->current_item->text->cursor_pos--;
				}
			}
		}
		break;

		case GDK_KEY_Return:
			g_string_insert_c(img->current_item->text->text, img->current_item->text->cursor_pos, '\n');
			img->current_item->text->cursor_pos++;
		break;

handle:
		default:
			if (g_unichar_isprint(g_utf8_get_char (event->string)))
			{
				// Text is selected
				if (img->current_item->text->selection_start != -1 && img->current_item->text->selection_end != -1)
				{
					int start = MIN(img->current_item->text->selection_start, img->current_item->text->selection_end);
					int end = MAX(img->current_item->text->selection_start, img->current_item->text->selection_end);
					g_string_erase(img->current_item->text->text,  start,  end - start);
					g_string_insert_unichar(img->current_item->text->text, start, event->string[0]);
					img->current_item->text->cursor_pos = start+1;
					img->current_item->text->selection_start = img->current_item->text->selection_end = -1;
				}
				else
				{
					g_string_insert_unichar(img->current_item->text->text, img->current_item->text->cursor_pos, event->string[0]);
					img->current_item->text->cursor_pos++;
				}
			}
	}

	pango_layout_set_text(img->current_item->text->layout, img->current_item->text->text->str, -1);
	img->current_item->text->cursor_visible = TRUE;
	if (img->current_item->text->cursor_source_id != 0)
		g_source_remove(img->current_item->text->cursor_source_id);

	img->current_item->text->cursor_source_id = g_timeout_add(750, (GSourceFunc) blink_cursor, img);
 
	img_update_textbox_boundaries(img);
	gtk_widget_queue_draw(widget);
	return TRUE;
}

void img_exit_fullscreen(img_window_struct *img)
{
	GdkWindow *win;

	gtk_widget_show(img->menubar);
	gtk_widget_show(img->side_notebook);
	gtk_widget_show(img->sidebar);
	gtk_widget_show(img->preview_hbox);
	gtk_widget_show (img->timeline_scrolled_window);

	gtk_widget_set_halign(img->image_area, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(img->image_area, GTK_ALIGN_CENTER);
	
	gtk_window_unfullscreen(GTK_WINDOW(img->imagination_window));
	//gtk_widget_add_accelerator (img->fullscreen, "activate", img->accel_group, GDK_KEY_F11, 0, GTK_ACCEL_VISIBLE);
	img->window_is_fullscreen = FALSE;

	/* Restore the cursor */
	win = gtk_widget_get_window(img->imagination_window);
	gdk_window_set_cursor(win, img->cursor);
}

void img_go_fullscreen(GtkMenuItem *item, img_window_struct *img)
{
	/* Hide the cursor */
	GdkCursor *cursor;
	GdkWindow *win;
	GdkDisplay *display1;

	win = gtk_widget_get_window(img->imagination_window);
	img->cursor = gdk_window_get_cursor(win);
	display1 = gdk_display_get_default();
	cursor = gdk_cursor_new_for_display(display1, GDK_BLANK_CURSOR);
	gdk_window_set_cursor(win, cursor);

	gtk_widget_hide (img->menubar);
	gtk_widget_hide (img->side_notebook);
	gtk_widget_hide (img->sidebar);
	gtk_widget_hide (img->preview_hbox);
	gtk_widget_hide (img->timeline_scrolled_window);

	gtk_widget_set_halign(img->image_area, GTK_ALIGN_FILL);
	gtk_widget_set_valign(img->image_area, GTK_ALIGN_FILL);
	
	gtk_window_fullscreen(GTK_WINDOW(img->imagination_window));
	img->window_is_fullscreen =TRUE;

	gtk_widget_queue_draw(img->image_area);
}

gboolean img_quit_application(GtkWidget *widget, GdkEvent * event, img_window_struct *img)
{
	if (img->no_recent_item_menu)
		gtk_widget_destroy(img->no_recent_item_menu);

	if (!img_can_discard_unsaved_project(img))
		return TRUE;
	if( img_save_window_settings( img ) )
		return( TRUE );
	img_free_allocated_memory(img);

	GSList *node1;
	for(node1 = img->plugin_list;node1 != NULL;node1 = node1->next) {
		g_module_close(node1->data);
	}
	g_slist_free(img->plugin_list);

	g_hash_table_destroy(img->cached_preview_surfaces);
	return FALSE;
}

static void img_file_chooser_add_preview(img_window_struct *img)
{
	GtkWidget *vbox;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width (GTK_CONTAINER(vbox), 10);

	img->preview_image = gtk_image_new ();

	img->dim_label  = gtk_label_new (NULL);
	img->size_label = gtk_label_new (NULL);

	gtk_box_pack_start (GTK_BOX (vbox), img->preview_image, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), img->dim_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), img->size_label, FALSE, TRUE, 0);
	gtk_widget_show_all (vbox);

	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(img->import_slide_chooser), vbox);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER(img->import_slide_chooser), FALSE);

	g_signal_connect (img->import_slide_chooser, "update-preview",G_CALLBACK (img_update_preview_file_chooser), img);
}

static void img_update_preview_file_chooser(GtkFileChooser *file_chooser,img_window_struct *img)
{
	gchar *filename,*size;
	gboolean has_preview = FALSE;
	gint width,height;
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *pixbuf2;

	filename = gtk_file_chooser_get_filename(file_chooser);
	if (filename == NULL)
	{
		gtk_file_chooser_set_preview_widget_active (file_chooser, has_preview);
		return;
	}
	pixbuf2 = gdk_pixbuf_new_from_file_at_scale(filename, 93, 70, TRUE, NULL);
	if (pixbuf2) {
		pixbuf = gdk_pixbuf_apply_embedded_orientation(pixbuf2);
		g_object_unref (pixbuf2);
	}
	has_preview = (pixbuf != NULL);
	if (has_preview)
	{
		gdk_pixbuf_get_file_info(filename,&width,&height);
		gtk_image_set_from_pixbuf (GTK_IMAGE(img->preview_image), pixbuf);
		g_object_unref (pixbuf);

		size = g_strdup_printf(ngettext("%d x %d pixels", "%d x %d pixels", height),width,height);
		gtk_label_set_text(GTK_LABEL(img->dim_label),size);
		g_free(size);
	}
	g_free(filename);
	gtk_file_chooser_set_preview_widget_active (file_chooser, has_preview);
}

void img_select_all_media(GtkWidget *widget, img_window_struct *img)
{
	gtk_icon_view_select_all(GTK_ICON_VIEW(img->media_iconview));
}

void img_unselect_all_media(GtkWidget *widget, img_window_struct *img)
{
	gtk_icon_view_unselect_all(GTK_ICON_VIEW(img->media_iconview));
}

void img_show_about_dialog (GtkMenuItem *item, img_window_struct *img_struct)
{
	static GtkWidget *about = NULL;
	static gchar version[] = VERSION;
	const char *authors[] = {"\nDevelopers:\nGiuseppe Torelli <colossus73@gmail.com>",
								NULL
							};
	
	if (about == NULL)
	{
		about = gtk_about_dialog_new ();
		gtk_window_set_position (GTK_WINDOW (about),GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_window_set_transient_for (GTK_WINDOW (about),GTK_WINDOW (img_struct->imagination_window));
		gtk_window_set_destroy_with_parent (GTK_WINDOW (about),TRUE);
		g_object_set (about,
			"name", "Imagination",
			"version", VERSION,
			"copyright","Copyright \xC2\xA9 2009-2024 Giuseppe Torelli\nTransition effects & dropdown animated widget:\n\xC2\xA9 2009 Tadej Borovšak",
			"comments","A simple and lightweight slideshow maker",
			"authors",authors,
			"documenters",NULL,
			"translator_credits",_("translator-credits"),
			"logo_icon_name","imagination",
			"website","http://imagination.sf.net",
			"license","Copyright \xC2\xA9 2009-2024 Giuseppe Torelli - Colossus <colossus73@gmail.com>\n\n"
						"This is free software; you can redistribute it and/or\n"
						"modify it under the terms of the GNU Library General Public License as\n"
						"published by the Free Software Foundation; either version 2 of the\n"
						"License,or (at your option) any later version.\n"
						"\n"
						"This software is distributed in the hope that it will be useful,\n"
						"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
						"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
						"Library General Public License for more details.\n"
						"\n"
						"You should have received a copy of the GNU Library General Public\n"
						"License along with the Gnome Library; see the file COPYING.LIB.  If not,\n"
						"write to the Free Software Foundation,Inc.,59 Temple Place - Suite 330,\n"
						"Boston,MA 02111-1307,USA.\n",
			  NULL);
	}
	gtk_dialog_run ( GTK_DIALOG(about));
	gtk_widget_hide (about);
}

void img_media_library_drag_begin(GtkWidget *widget, GdkDragContext *context, img_window_struct *img)
{
	GtkStyleContext *style_context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_class (style_context, "iconview-dragging");
}

void img_media_library_drag_data_received (GtkWidget *widget, GdkDragContext *context , int x, int y, GtkSelectionData *data, unsigned int info, unsigned int time, img_window_struct *img)
{
	gchar **media = NULL;
	gchar *filename;
	GtkWidget *dialog;
	int i = 0;
	gboolean dummy;

	media = gtk_selection_data_get_uris(data);
	if (media == NULL)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(img->imagination_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry, I could not perform the operation!"));
		gtk_window_set_title(GTK_WINDOW(dialog),"Imagination");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	gtk_drag_finish (context,TRUE,FALSE,time);
	
	 while(media[i])
	{
		filename = g_filename_from_uri (media[i], NULL, NULL);
		if ( img_create_media_struct(filename, img))
			img_taint_project(img);
		g_free(filename);
		i++;
	}

	g_strfreev(media);
}

void img_media_library_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint info, guint time, img_window_struct *img)
{
	media_struct *media;
	GtkTreeModel *model;
	GList *selected_items, *item;
	GPtrArray *media_structs;
	GtkTreePath *path;
	GtkTreeIter iter;

	model = gtk_icon_view_get_model(GTK_ICON_VIEW(widget));
	selected_items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(widget));

	media_structs = g_ptr_array_new();

	for (item = selected_items; item != NULL; item = g_list_next(item))
	{
		path = (GtkTreePath *)item->data;
		if (gtk_tree_model_get_iter(model, &iter, path))
		{
			gtk_tree_model_get(model, &iter, 2, &media, -1);
			g_ptr_array_add(media_structs, media);
		}
	}
	// Set the data
	gtk_selection_data_set(selection_data,
						   gdk_atom_intern( "application/x-media-item", FALSE),
						   32,  // 32 bits per unit for pointers
						   (guchar *)media_structs->pdata,
						   media_structs->len * sizeof(media_struct*));

	// Free allocated memory
	g_ptr_array_free(media_structs, TRUE);
	g_list_free_full(selected_items, (GDestroyNotify)gtk_tree_path_free);
}

void img_media_library_drag_end (GtkWidget *widget, GdkDragContext *context, img_window_struct *img)
{
    GtkStyleContext *style_context = gtk_widget_get_style_context(widget);
    gtk_style_context_remove_class(style_context, "iconview-dragging");
}
 
gboolean img_on_draw_event( GtkWidget *widget, cairo_t *cr, img_window_struct *img )
{
	GArray* active_media = NULL;
	gint img_width, img_height, media_type;
	gdouble current_time, x, y, x_ratio, y_ratio, scale;
	media_timeline* media = NULL;
	cairo_surface_t *surface = NULL;

	g_object_get(G_OBJECT(img->timeline), "time_marker_pos", &current_time, NULL);

	GtkAllocation allocation;
	gtk_widget_get_allocation(img->image_area, &allocation);

	//Paint the canvas with the user chosen project background color
	cairo_set_source_rgb(cr, img->background_color[0], img->background_color[1], img->background_color[2]);
	cairo_paint(cr);

	// Draw all the placed media items on the tracks
	if (img->preview_is_running)
	{
		// Are there any pictures placed on the timeline?
		if (img->exported_image)
		{
			img_width 	= cairo_image_surface_get_width( img->exported_image);
			img_height 	= cairo_image_surface_get_height( img->exported_image);
			scale = MIN((double)allocation.width / img_width, (double)allocation.height / img_height);
			x = (allocation.width - (img_width * scale)) / 2;
			y = (allocation.height - (img_height * scale)) / 2;
						
			cairo_save(cr);
			cairo_translate(cr, x, y);
			cairo_scale(cr, scale, scale);
			cairo_set_source_surface(cr, img->exported_image, 0, 0);
			cairo_paint(cr);
			cairo_restore(cr);
		}
	}
	else
	{
		// Get the list of active media on all tracks according to the position of the red needle
		active_media = img_timeline_get_active_picture_media(img->timeline, current_time);
		for (gint i = active_media->len-1; i >=0; i--)
		{
			media = g_array_index(active_media, media_timeline *, i);
			if (media->media_type == 3)
				goto text;

			// Calculate offset x and y to center the image in the image area the first time
			surface = g_hash_table_lookup(img->cached_preview_surfaces, GINT_TO_POINTER(media->id));			
			img_width 	= cairo_image_surface_get_width(surface);
			img_height 	= cairo_image_surface_get_height(surface);
			if (media->nr_rotations == 1 || media->nr_rotations == 3)
				scale = MIN((double)allocation.width / img_height, (double)allocation.height / img_width);
			else
				scale = MIN((double)allocation.width / img_width, (double)allocation.height / img_height);

			if (! media->is_positioned)					
			{
				media->x = (allocation.width - (img_width * scale)) / 2;
				media->y = (allocation.height - (img_height * scale)) / 2;
				media->last_allocation_width = allocation.width;
				media->last_allocation_height = allocation.height;
				media->is_positioned = TRUE;
			}
			// Calculate relative position ratio of the surface
			else if (media->last_allocation_width != allocation.width || media->last_allocation_height != allocation.height)
			{
				x_ratio = media->x / media->last_allocation_width;
				y_ratio = media->y / media->last_allocation_height;
				
				media->x = x_ratio * allocation.width;
				media->y = y_ratio * allocation.height;
				media->last_allocation_width = allocation.width;
				media->last_allocation_height = allocation.height;
			}
			cairo_save(cr);
				cairo_translate(cr, media->x, media->y);
				cairo_scale(cr, scale, scale);
				cairo_set_source_surface(cr, surface, 0, 0);
				cairo_paint_with_alpha(cr, media->opacity);	
			cairo_restore(cr);
text:
			// Render textbox and text
			if (media->text)
				img_render_textbox(img, cr, media);

			// If the user clicked on a picture draw a rectangle and the handles
			if (media->is_selected)
			{
				cairo_set_source_rgb(cr, 1.0, 1.0, 0);
				cairo_rectangle(cr, media->x, media->y, img_width * scale, img_height * scale);
				cairo_stroke(cr);
				cairo_save(cr);
					cairo_translate(cr, media->x + (img_width * scale) / 2, media->y + (img_height * scale) / 2);
					if (media->nr_rotations == 1 || media->nr_rotations == 3)
					{
						gdouble temp = img_width;
						img_width = img_height;
						img_height = temp;
					}
					cairo_rotate(cr, G_PI / 2.0 * media->nr_rotations);
					cairo_translate(cr, -(media->x + (img_width * scale) / 2), -(media->y + (img_height * scale) / 2));

					cairo_rectangle(cr, (media->x + img_width * scale) - 11, (media->y + img_height * scale) - 11, 10, 10);
					cairo_stroke_preserve(cr);
					cairo_set_source_rgb(cr, 0, 0, 0);
					cairo_fill(cr);
					img_draw_rotating_handle(cr, media->x, img_width, media->y, img_height, scale, FALSE);
				cairo_restore(cr);
			}
		}
		g_array_free(active_media, FALSE);
	}

	return TRUE;
}

void img_draw_image_on_surface( cairo_t           *cr,
							   gint               width,
							   cairo_surface_t   *surface,
							   ImgStopPoint      *point,
							   img_window_struct *img )
{
	gdouble  offxr, offyr;  /* Relative offsets */
	gdouble  factor_c;      /* Scaling factor for cairo context */
	gdouble  factor_o;      /* Scalng factor for offset mods */
	gint     cw;            /* Width of the surface */

	cw = cairo_image_surface_get_width( surface );
	factor_c = (gdouble)width / cw * point->zoom;
	factor_o = (gdouble)img->video_size[0] / cw * point->zoom;

	offxr = point->offx / factor_o;
	offyr = point->offy / factor_o;

	/* Make sure that matrix modifications are only visible from this function
	 * and they don't interfere with text drawing. */
	cairo_save( cr );
	cairo_scale( cr, factor_c, factor_c );
	cairo_set_source_surface( cr, surface, offxr, offyr );
	cairo_paint( cr );
	cairo_restore( cr );
}

void img_swap_preview_button_images(img_window_struct *img, gboolean flag)
{
	GtkWidget *tmp_image;

	if( flag )
	{
		tmp_image = gtk_image_new_from_icon_name ("media-playback-start", GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_show(tmp_image);
		gtk_button_set_image(GTK_BUTTON(img->preview_button), tmp_image);
		gtk_widget_set_tooltip_text(GTK_WIDGET(img->preview_button),_("Starts the preview"));
	}
	else
	{
		tmp_image = gtk_image_new_from_icon_name ("media-playback-stop", GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_show(tmp_image);
		gtk_button_set_image(GTK_BUTTON(img->preview_button), tmp_image);
		gtk_widget_set_tooltip_text(GTK_WIDGET(img->preview_button),_("Stops the preview"));
	}
}

void img_choose_project_filename(GtkWidget *widget, img_window_struct *img)
{
	GtkWidget *fc;
	GtkFileChooserAction action = 0;
	gint	 response;
	gboolean	 open_replace, save;
	gchar *filename = NULL;
	GtkFileFilter *project_filter, *all_files_filter;


	/* Determine what to do */
	open_replace = widget == img->open_menu;
	save = widget == img->save_as_menu || widget == img->save_menu;
	/* Determine the mode of the chooser. */
	if (open_replace) {
	action = GTK_FILE_CHOOSER_ACTION_OPEN;
	} else if (save) {
	action = GTK_FILE_CHOOSER_ACTION_SAVE;
	} else {
	img_message(img, _("I do not know what to do with this slideshow filename."));
	return;
	}

	/* close old slideshow if the user open a new one */
	if (open_replace)
	{
		if (!img_can_discard_unsaved_project(img))
			return;
	}

	/* If user wants to save empty slideshow, do nothing */
	if( img->media_nr == 0 && save )
		return;

	/* ask for a file name if the project has never been saved yet, if we save as or if we open a project */
	if (img->project_filename == NULL || widget == img->save_as_menu || action == GTK_FILE_CHOOSER_ACTION_OPEN)
	{
	fc = gtk_file_chooser_dialog_new (action == GTK_FILE_CHOOSER_ACTION_OPEN ? _("Load an Imagination project") : 
		_("Save an Imagination project"),
		GTK_WINDOW (img->imagination_window),
		action,
		"_Cancel",
		GTK_RESPONSE_CANCEL,
		action == GTK_FILE_CHOOSER_ACTION_OPEN ?  "_Open" : "_Save",
		GTK_RESPONSE_ACCEPT,NULL);

	/* Filter .img files */
	project_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name(project_filter, _("Imagination projects (*.img)"));
	gtk_file_filter_add_pattern(project_filter, "*.img");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fc), project_filter);

	/* All files filter */
	all_files_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name(all_files_filter, _("All files"));
	gtk_file_filter_add_pattern(all_files_filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fc), all_files_filter);

	/* if we are saving, propose a default filename */
	if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
	{
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (fc), "unknown.img");
		/* Add the checkbok to save filenames only */
		GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_set_homogeneous(GTK_BOX(box), TRUE);
		GtkWidget *relative_names = gtk_check_button_new_with_mnemonic(_("Don't include folder names in the filenames"));
		gtk_widget_set_tooltip_text(relative_names, _("If you check this button, be sure to include ALL the project files in the SAME folder before attempting to open it again."));
		gtk_box_pack_start (GTK_BOX (box), relative_names, FALSE, FALSE, 0);
		gtk_widget_show(relative_names);
		g_signal_connect (relative_names, "toggled", G_CALLBACK (img_save_relative_filenames), img);
		gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER(fc), box );
	}
	if (img->project_current_dir)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fc),img->project_current_dir);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (fc),TRUE);
	response = gtk_dialog_run (GTK_DIALOG (fc));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
		if( ! filename )
		{
			gtk_widget_destroy(fc);
			return;
		}
		
		if (img->project_current_dir)
			g_free(img->project_current_dir);

		img->project_current_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(fc));
	}
	else if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy(fc);
		return;
	}
	gtk_widget_destroy(fc);
	}

	if( ! filename )
		filename = g_strdup( img->project_filename );

	if (open_replace)
		img_load_project(img, NULL, filename);
	else if (save)
		img_save_project( img, filename, img->relative_filenames );

	img_refresh_window_title(img);

	g_free( filename );
}

void img_close_project(GtkWidget *widget, img_window_struct *img)
{
	/* When called from close_menu, ask for confirmation */
	if (widget && widget == img->close_menu && ! img_can_discard_unsaved_project(img))
		return;

	img_free_allocated_memory(img);
	img->project_is_modified = FALSE;
		
	img_refresh_window_title(img);
	if( img->current_image )
		cairo_surface_destroy( img->current_image );
	img->current_image = NULL;
	gtk_widget_queue_draw( img->image_area );
	gtk_label_set_text(GTK_LABEL (img->current_time),"00:00:00");
	gtk_label_set_text(GTK_LABEL (img->total_time_label),"00:00:00");

	/* Reset slideshow properties */
	img->background_color[0] = 0;
	img->background_color[1] = 0;
	img->background_color[2] = 0;
	
	// This is needed to reset the id counter when loading a new slideshow without quitting Imagination
	img->next_id = 1;
}

/*
 * img_ken_burns_zoom_changed:
 * @range: GtkRange that will provide proper value for us
 * @img: global img_window_strct structure
 *
 * This function modifies current zoom value and queues redraw of preview area.
 *
 * To keep image center in focus, we also do some calculation on offsets.
 *
 * IMPORTANT: This function is part of the Ken Burns effect and doesn't change
 * preview area size!! If you're looking for function that does that,
 * img_image_area_change_zoom is the thing to look at.
 */
void
img_ken_burns_zoom_changed( GtkRange *range, img_window_struct *img )
{
	img_update_zoom_variables(img);

	img_update_stop_point(NULL, img);
	img_taint_project(img);
	gtk_widget_queue_draw( img->image_area );
}

gboolean img_image_area_scroll( GtkWidget			*widget,
								GdkEvent			*event,
								img_window_struct	*img )
{
	gint direction;
	gdouble zoom_value;

	direction = event->scroll.direction;
	
	zoom_value = gtk_range_get_value( GTK_RANGE(img->ken_zoom) );
	
	if( direction == GDK_SCROLL_UP )
		zoom_value += 0.1;
	else if ( direction == GDK_SCROLL_DOWN )
		zoom_value -= 0.1;
		
	gtk_range_set_value( GTK_RANGE(img->ken_zoom), zoom_value );

	return (TRUE);
}

gboolean img_image_area_button_press(GtkWidget *widget, GdkEventButton *event, img_window_struct *img)
{
	if (event->button == GDK_BUTTON_PRIMARY)
		img_select_surface_on_click(img, event->x, event->y);

	if (img->current_item->media_type == 3)
		img_textbox_button_pressed(event, img);
	
	img->x = event->x;
	img->y = event->y;
	img->bak_offx = img->current_point.offx;
	img->bak_offy = img->current_point.offy;

	return TRUE;
}

gboolean img_image_area_button_release(	GtkWidget *widget, GdkEventButton *event, img_window_struct *img)
{
	//img_update_stop_point(NULL, img);
	
	if (event->button == GDK_BUTTON_PRIMARY && img->current_item->text)
	{
		if (img->current_item->text->action == IS_DRAGGING && (img->current_item->text->draw_horizontal_line || img->current_item->text->draw_vertical_line == TRUE))
			img->current_item->text->draw_horizontal_line = img->current_item->text->draw_vertical_line = FALSE;

		if (img->current_item->text->action == IS_RESIZING)
		{
			if (!img->current_item->text->cursor_source_id)
				img->current_item->text->cursor_source_id = g_timeout_add(750, (GSourceFunc) blink_cursor, img);			
		}
		if (img->current_item->text->width < 0)
		{
			img->current_item->text->width = img->current_item->text->orig_width;
			img->current_item->text->x = event->x;
		}
		if (img->current_item->text->height < 0)
		{
			img->current_item->text->height = img->current_item->text->orig_height;
			img->current_item->text->y= event->y;
		}
		img->current_item->text->button_pressed = FALSE;
		img->current_item->text->action = NONE;
		if (img->current_item->text->corner != GDK_FLEUR)
			gdk_window_set_cursor(gtk_widget_get_window(img->image_area), NULL);
	
		gtk_widget_queue_draw(img->image_area);
	}

	return TRUE;
}

gboolean img_image_area_motion( GtkWidget *widget,  GdkEventMotion *event,  img_window_struct *img )
{
	gdouble deltax, deltay;
	gint width, height;
	gdouble textbox_x, textbox_y;
	gint snap_zone = 10;

	deltax = ( event->x - img->x ) / img->image_area_zoom;
	deltay = ( event->y - img->y ) / img->image_area_zoom;

	img->current_point.offx = CLAMP( deltax + img->bak_offx, img->maxoffx, 0 );
	img->current_point.offy = CLAMP( deltay + img->bak_offy, img->maxoffy, 0 );

	// This is to let the textbox handles detection to work
	if (img->current_item->text && img->current_item->text->visible)
		transform_coords(img->current_item->text, event->x, event->y, &textbox_x, &textbox_y);
	
	//transform_coords(img->current_item, event->x, event->y, &x, &y);

	// Move the selected picture in the image area
	if (event->state & GDK_BUTTON1_MASK &&  img->current_item && img->current_item->is_selected)// && img->current_item->text && ! img->current_item->text->visible)
	{
		img->current_item->x = event->x - img->current_item->drag_x;
		img->current_item->y = event->y - img->current_item->drag_y;
		gtk_widget_queue_draw(img->image_area);
	}

	if (img->current_item->media_type == 3)
	{
		if (img->current_item->text->visible && img->current_item->text->button_pressed)
		{
			//This to draw the center lines to ease the img->current_item->text centering
			//later in the IS_DRAGGING action
			width  = gtk_widget_get_allocated_width(widget) / 2;
			height = gtk_widget_get_allocated_height(widget) / 2;

			switch (img->current_item->text->action)
			{
				case IS_RESIZING:
					switch (img->current_item->text->corner)
					{
						 case GDK_BOTTOM_RIGHT_CORNER:
						 {
								if (img->current_item->text->cursor_source_id)
								{
									g_source_remove(img->current_item->text->cursor_source_id);
									img->current_item->text->cursor_source_id = 0;
								}
								double new_width  = textbox_x - img->current_item->text->x;
								double new_height = textbox_y - img->current_item->text->y;
								
								//The following code is taken from https://shihn.ca/posts/2020/resizing-rotated-elements/

								// Calculate center of the rectangle
								double cx = img->current_item->text->x + img->current_item->text->width / 2;
								double cy = img->current_item->text->y + img->current_item->text->height / 2;

								// Calculate rotated coordinates of the top-left corner (A')
								double rotated_ax, rotated_ay;
								rotate_point(img->current_item->text->x, img->current_item->text->y, cx, cy, img->current_item->text->angle, &rotated_ax, &rotated_ay);

								// The bottom-right corner (C') is at the mouse position (x, y)
								double rotated_cx = event->x;
								double rotated_cy = event->y;

								// Calculate the new center
								double new_cx = (rotated_ax + rotated_cx) / 2;
								double new_cy = (rotated_ay + rotated_cy) / 2;

								// Calculate the new top-left corner (newA)
								double new_ax, new_ay;
								rotate_point(rotated_ax, rotated_ay, new_cx, new_cy, -img->current_item->text->angle, &new_ax, &new_ay);

								// Update img->current_item->text position
								img->current_item->text->x = new_ax;
								img->current_item->text->y = new_ay;
								//End of the code from the above URL
								
								img->current_item->text->width = new_width;
								img->current_item->text->height = new_height;
								
								//This to automatically word wrap the text when the user resizes the img->current_item->text
								pango_layout_set_width(img->current_item->text->layout, pango_units_from_double(img->current_item->text->width));
						}
						break;
					
						default:
							break;
					}
					break;
					
				case IS_DRAGGING:
					 double textbox_center_x = img->current_item->text->x + img->current_item->text->width / 2;
					 double textbox_center_y = img->current_item->text->y + img->current_item->text->height / 2;
				   
					// Calculate the distance moved
					double dx = event->x - img->current_item->text->dragx - img->current_item->text->x;
					double dy = event->y - img->current_item->text->dragy - img->current_item->text->y;
					
					// Check if we're entering or leaving the x snap zone
					if (!img->current_item->text->is_x_snapped && fabs(textbox_center_x - width) <= snap_zone)
					{
						img->current_item->text->is_x_snapped = TRUE;
						img->current_item->text->x = width - img->current_item->text->width / 2;
						img->current_item->text->draw_vertical_line = TRUE;
					}
					else if (img->current_item->text->is_x_snapped && fabs(dx) > snap_zone)
					{
						img->current_item->text->is_x_snapped = FALSE;
						img->current_item->text->draw_vertical_line = FALSE;
					}
					// Update position if not x snapped
					if (!img->current_item->text->is_x_snapped)
						img->current_item->text->x = event->x - img->current_item->text->dragx;

					// Check if we're entering or leaving the x snap zone
					if (!img->current_item->text->is_y_snapped && fabs(textbox_center_y - height) <= snap_zone)
					{
						img->current_item->text->is_y_snapped = TRUE;
						img->current_item->text->y = height - img->current_item->text->height / 2;
						img->current_item->text->draw_horizontal_line = TRUE;
					}
					else if (img->current_item->text->is_y_snapped && fabs(dy) > snap_zone)
					{
						img->current_item->text->is_y_snapped = FALSE;
						img->current_item->text->draw_horizontal_line = FALSE;
					}
					// Update position if not y snapped
					if (!img->current_item->text->is_y_snapped)
						img->current_item->text->y = event->y - img->current_item->text->dragy;

					img->current_item->text->corner = GDK_FLEUR;
					break;
					
				case IS_ROTATING:
					dx = event->x - img->current_item->text->x;
					dy = event->y - img->current_item->text->y - img->current_item->text->height;
					img->current_item->text->angle = atan2(dy, dx);
					img->current_item->text->corner = GDK_EXCHANGE;
					break;
			}
			gdk_window_set_cursor(gtk_widget_get_window(img->image_area), gdk_cursor_new_for_display(gdk_display_get_default(), img->current_item->text->corner));

			gtk_widget_queue_draw(img->image_area);
			return TRUE;
		}
	
		// Check if the mouse pointer is hovering the rotate circle
		if (img->current_item->text && img->current_item->text->visible && 
			textbox_x >= img->current_item->text->x + (img->current_item->text->width/2) - 5 && 
			textbox_x <= (img->current_item->text->x + (img->current_item->text->width/2)) + 5 && 
			textbox_y >= img->current_item->text->y + img->current_item->text->height + 10 && 
			textbox_y <= img->current_item->text->y + img->current_item->text->height + 20)
		{
			img->current_item->text->action = IS_ROTATING;
			img->current_item->text->corner = GDK_EXCHANGE;
			gdk_window_set_cursor(gtk_widget_get_window(img->image_area), gdk_cursor_new_for_display(gdk_display_get_default(), img->current_item->text->corner));
		}
		// If mouse pointer is inside the visible img->current_item->text change the cursor shape accordingly
		else if (img->current_item->text && !img->current_item->text->button_pressed && img->current_item->text->visible && 
				 textbox_x >= img->current_item->text->x && textbox_x <= img->current_item->text->x + img->current_item->text->width && 
				 textbox_y >= img->current_item->text->y && textbox_y <= img->current_item->text->y + img->current_item->text->height)
		{
			if (textbox_x >= img->current_item->text->x + img->current_item->text->width - 5 && textbox_x <= img->current_item->text->x + img->current_item->text->width && 
				textbox_y >= img->current_item->text->y + img->current_item->text->height - 5 && textbox_y <= img->current_item->text->y + img->current_item->text->height)
			{
				img->current_item->text->corner = GDK_BOTTOM_RIGHT_CORNER;
				img->current_item->text->action = IS_RESIZING;
			}
			else
			{
				img->current_item->text->corner = GDK_XTERM;
				img->current_item->text->action = IS_DRAGGING;
			}
			gdk_window_set_cursor(gtk_widget_get_window(img->image_area), gdk_cursor_new_for_display(gdk_display_get_default(), img->current_item->text->corner));
		}
		else if (img->current_item->text && !img->current_item->text->button_pressed)
			gdk_window_set_cursor(gtk_widget_get_window(img->image_area), NULL);
	}
	
	return TRUE;
}

void img_add_stop_point( GtkButton  *button, img_window_struct *img )
{
	ImgStopPoint *point;
	GList        *tmp;

	if (img->current_media == NULL)
		return;

	/* Create new point */
	point = g_slice_new( ImgStopPoint );
	*point = img->current_point;
	point->time = gtk_spin_button_get_value_as_int(
						GTK_SPIN_BUTTON( img->ken_duration ) );

	/* Append it to the list */
	tmp = img->current_media->points;
	tmp = g_list_append( tmp, point );
	img->current_media->points = tmp;
	img->current_media->cur_point = img->current_media->no_points;
	img->current_media->no_points++;

	/* Sync timings */
	//img_sync_timings( img->current_media, img );
	
	img_taint_project(img);
}

void
img_update_stop_point( GtkSpinButton  *button,
					   img_window_struct *img )
{
	ImgStopPoint *point;
	gint full;

	if( img->current_media == NULL || img->current_media->points == NULL)
		return;

	/* Get selected point */
	point = g_list_nth_data( img->current_media->points,
							 img->current_media->cur_point );

	/* Update data */
	*point = img->current_point;
	point->time = gtk_spin_button_get_value_as_int(	GTK_SPIN_BUTTON( img->ken_duration ) );
	
	/* Update total slideshow duration */
	full = img_calc_slide_duration_points(img->current_media->points,
										img->current_media->no_points);

	//img->current_media->duration = full;


	/* Sync timings */
	//img_sync_timings( img->current_media, img );

	img_taint_project(img);
}

void
img_delete_stop_point( GtkButton         *button,
					   img_window_struct *img )
{
	GList *node;
	gint  full;

	if( img->current_media == NULL )
		return;

	/* Get selected node and free it */
	node = g_list_nth( img->current_media->points,
					   img->current_media->cur_point );
	g_slice_free( ImgStopPoint, node->data );
	img->current_media->points = 
			g_list_delete_link( img->current_media->points, node );

	/* Update counters */
	img->current_media->no_points--;
	img->current_media->cur_point = MIN( img->current_media->cur_point,
										 img->current_media->no_points - 1 );

	/* Update total slideshow duration */
	full = img_calc_slide_duration_points(img->current_media->points,
										img->current_media->no_points);

	//img->current_media->duration = full;
	
	/* Sync timings */
	//img_sync_timings( img->current_media, img );
	
	img_taint_project(img);
}

void img_goto_prev_point( GtkButton *button, img_window_struct *img)
{
	if( img->current_media && img->current_media->no_points )
	{
		img->current_media->cur_point =
				CLAMP( img->current_media->cur_point - 1,
					   0, img->current_media->no_points - 1 );
	}
}

void img_goto_next_point(GtkButton *button, img_window_struct *img)
{
	if( img->current_media && img->current_media->no_points )
	{
		img->current_media->cur_point =
				CLAMP( img->current_media->cur_point + 1,
					   0, img->current_media->no_points - 1 );

	}
}

void img_goto_point ( GtkEntry *entry, img_window_struct *img)
{
	const gchar *string;
	gint         number;

	string = gtk_entry_get_text( entry );
	number = (gint)strtol( string, NULL, 10 );

	if( img->current_media && img->current_media->no_points )
	{
		img->current_media->cur_point =
				CLAMP( number - 1, 0, img->current_media->no_points - 1 );

	}
}


void img_calc_current_ken_point( ImgStopPoint *res,
							ImgStopPoint *from,
							ImgStopPoint *to,
							gdouble       progress,
							gint          mode )
{
	gdouble fracx = 0; /* Factor for x offset */
	gdouble fracy = 0; /* Factor for y offset */
	gdouble fracz = 0; /* Factor for zoom */

	switch( mode )
	{
		case( 0 ): /* Linear mode */
			fracx = progress;
			fracy = progress;
			fracz = progress;
			break;

		case( 1 ): /* Acceleration mode */
			break;

		case( 2 ): /* Deceleration mode */
			break;
	}

	res->offx = from->offx * ( 1 - fracx ) + to->offx * fracx;
	res->offy = from->offy * ( 1 - fracy ) + to->offy * fracy;
	res->zoom = from->zoom * ( 1 - fracz ) + to->zoom * fracz;
}

gboolean img_save_window_settings( img_window_struct *img )
{
	GList	  *menu_items, *node0;
	GPtrArray *recent_files;
	const gchar *label;
	GKeyFile *kf;
	gchar    *group = "Interface settings";
	gchar    *rc_file, *rc_path, *contents;
	/* Width, height, v_paned, h_paned, flags */
	int       w, h, g, m, f;
	gdouble current_time;
	gboolean  max;

	recent_files = g_ptr_array_new();
	menu_items = gtk_container_get_children(GTK_CONTAINER(img->recent_slideshows));
	for(node0 = menu_items; node0 != NULL; node0 = node0->next)
	{
		label = gtk_menu_item_get_label(GTK_MENU_ITEM(node0->data));
		g_ptr_array_add (recent_files, (gchar *)label);
	}

	gtk_window_get_size( GTK_WINDOW( img->imagination_window ), &w, &h );
	g = gtk_paned_get_position( GTK_PANED( img->vpaned));
	m = gtk_paned_get_position( GTK_PANED( img->hpaned));
	f = gdk_window_get_state( gtk_widget_get_window( img->imagination_window ) );
	max = f & GDK_WINDOW_STATE_MAXIMIZED;

	/* If window is maximized, store sizes that are a bit smaller than full
	 * screen, else making window non-fullscreen will have no effect. */
	if( max )
	{
		w -= 100;
		h -= 100;
	}

	kf = g_key_file_new();
	g_key_file_set_integer( kf, group, "width",   w );
	g_key_file_set_integer( kf, group, "height",  h );
	g_key_file_set_integer( kf, group, "v_paned",  g );
	g_key_file_set_integer( kf, group, "h_paned",  m );
	g_key_file_set_double(  kf, group, "zoom_p",  img->image_area_zoom );
	g_key_file_set_integer( kf, group, "image_area_width",   gtk_widget_get_allocated_width(img->image_area));
	g_key_file_set_integer( kf, group, "image_area_height",  gtk_widget_get_allocated_height(img->image_area));
	g_key_file_set_boolean( kf, group, "max",     max );
	g_key_file_set_double( kf, group, "time_marker_pos",  current_time);
	g_key_file_set_integer( kf, group, "preview", img->preview_fps);
	g_key_file_set_string_list(kf, group, "recent_files", (const gchar * const *)recent_files->pdata, recent_files->len);

	rc_path = g_build_filename( g_get_home_dir(), ".config", "imagination", NULL );
	rc_file = g_build_filename( rc_path, "imaginationrc", NULL );
	contents = g_key_file_to_data( kf, NULL, NULL );
	g_key_file_free( kf );

	g_mkdir_with_parents( rc_path, S_IRWXU );
	g_file_set_contents( rc_file, contents, -1, NULL );

	g_free(contents);
	g_free(rc_file);
	g_free(rc_path);
	g_list_free(menu_items);
	g_ptr_array_free(recent_files, FALSE);

	return( FALSE );
}

gboolean img_load_window_settings( img_window_struct *img )
{
	GtkWidget *menu;
	GKeyFile  *kf;
	gchar     *group = "Interface settings";
	gchar	  **recent_slideshows;
	gchar     *rc_file, *recent_files = NULL;
	gint      w, h, g, m, w2,h2;
	gint	  i;
	gboolean  max;
	gdouble current_time;

	rc_file = g_build_filename( g_get_home_dir(), ".config", "imagination", "imaginationrc", NULL);
	if( ! g_file_test( rc_file, G_FILE_TEST_EXISTS ) )
		return( FALSE );

	kf = g_key_file_new();
	g_key_file_load_from_file( kf, rc_file, G_KEY_FILE_NONE, NULL );
	g_free(rc_file);

	w                   	= g_key_file_get_integer( kf, group, "width",   NULL );
	h                    	= g_key_file_get_integer( kf, group, "height",  NULL );
	g                    	= g_key_file_get_integer( kf, group, "v_paned",  NULL );
	m                    	= g_key_file_get_integer( kf, group, "h_paned",  NULL );
	img->image_area_zoom = g_key_file_get_double(  kf, group, "zoom_p",  NULL );
	w2                  	= g_key_file_get_integer( kf, group, "image_area_width",   NULL );
	h2                  	= g_key_file_get_integer( kf, group, "image_area_height",  NULL );
	max                	= g_key_file_get_boolean( kf, group, "max",     					NULL );
	current_time  	= g_key_file_get_double( kf, group, "time_marker_pos",     NULL );
	recent_files	 	= g_key_file_get_string(  kf, group, "recent_files", 				NULL);

	if (recent_files)
	{
		if (strlen(recent_files) > 1)
		{
			recent_slideshows = g_strsplit(recent_files, ";", -1);
			for (i = 0; i <= g_strv_length(recent_slideshows) - 1; i++)
			{
				if (strlen(recent_slideshows[i]) > 1)
				{
					menu = gtk_menu_item_new_with_label(recent_slideshows[i]);
					gtk_menu_shell_append(GTK_MENU_SHELL(img->recent_slideshows), menu);
					g_signal_connect(menu, "activate", G_CALLBACK(img_open_recent_slideshow), img);
					gtk_widget_show(menu);
				}
			}
			g_free(recent_files);
			g_strfreev(recent_slideshows);
		}
		else
		{
			img->no_recent_item_menu = gtk_menu_item_new_with_label(_("No recent items found"));
			gtk_widget_set_sensitive(img->no_recent_item_menu, FALSE);
			gtk_menu_shell_append(GTK_MENU_SHELL(img->recent_slideshows), img->no_recent_item_menu);
			gtk_widget_show(img->no_recent_item_menu);
		}
	}
	img->preview_fps     = g_key_file_get_integer( kf, group, "preview", NULL );
	if( ! img->preview_fps )
		img->preview_fps = 25;

	g_key_file_free( kf );

	gtk_window_set_default_size( GTK_WINDOW( img->imagination_window ), w, h );
	gtk_widget_set_size_request(img->image_area, w2, h2);

	g_signal_handlers_block_by_func(img->vpaned, img_change_image_area_size, img);  
	gtk_paned_set_position( GTK_PANED( img->vpaned ), g );
	g_signal_handlers_unblock_by_func(img->vpaned, img_change_image_area_size, img);
	
	g_signal_handlers_block_by_func(img->hpaned, img_change_media_library_size, img);  
	gtk_paned_set_position( GTK_PANED( img->hpaned ), m );	
	g_signal_handlers_unblock_by_func(img->hpaned, img_change_media_library_size, img);  

	if( max )
		gtk_window_maximize( GTK_WINDOW( img->imagination_window ) );

	img_timeline_set_time_marker((ImgTimeline *)img->timeline, current_time);
	
	return TRUE;
}

void img_set_window_default_settings( img_window_struct *img )
{
	img->image_area_zoom = 1.0;
	img->preview_fps = 25;
	g_signal_handlers_block_by_func(img->vpaned, img_change_image_area_size, img);  
	gtk_paned_set_position(GTK_PANED(img->vpaned), 520);
	g_signal_handlers_unblock_by_func(img->vpaned, img_change_image_area_size, img);

	gtk_window_set_default_size( GTK_WINDOW( img->imagination_window ), 800, 600 );
}

void img_pattern_clicked(GtkMenuItem *item, img_window_struct *img)
{
	GtkWidget		*fc;
	GtkWidget		*tmp_image;
	gchar			*filename;
	gint			response;
	GtkFileFilter	*png_filter;
	GError			*error = NULL;
	GdkPixbuf		*pattern_pix;

	fc = gtk_file_chooser_dialog_new (_("Please choose a PNG file"),
							GTK_WINDOW (img->imagination_window),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							"_Cancel",
							GTK_RESPONSE_CANCEL,
							"_Open",
							GTK_RESPONSE_ACCEPT,
							NULL);

	/*if (img->project_current_dir)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fc),img->project_current_dir);*/

	/* Image files filter */
	png_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name(png_filter,_("PNG files"));
	gtk_file_filter_add_pattern( png_filter, "*.png" );
	gtk_file_filter_add_pattern( png_filter, "*.PNG" );
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fc),png_filter);

	/* Set the delete pattern button */
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(box), TRUE);
	GtkWidget *delete_pattern = gtk_button_new_with_mnemonic(_("Delete current pattern"));
	gtk_box_pack_start (GTK_BOX (box), delete_pattern, FALSE, FALSE, 0);
	gtk_widget_show(delete_pattern);
	g_signal_connect (delete_pattern, "clicked", G_CALLBACK (img_delete_subtitle_pattern), img);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER(fc), box );

	response = gtk_dialog_run (GTK_DIALOG (fc));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
		if( ! filename )
		{
			gtk_widget_destroy(fc);
			return;
		}
		if (img->current_item->text->pattern_filename)
			g_free(img->current_item->text->pattern_filename);

		img->current_item->text->pattern_filename = g_strdup(filename);
		pattern_pix = gdk_pixbuf_new_from_file_at_scale( filename, 32, 32, TRUE, &error);
		g_free(filename);
		if (! pattern_pix)
		{
			img_message(img, error->message);
			g_error_free(error);
			gtk_widget_destroy(fc);
			return;
		}
		/* Swap the current image button
		 * with the loaded one */
		tmp_image = gtk_image_new_from_pixbuf(pattern_pix);
		gtk_widget_show(tmp_image);
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(img->pattern_image), tmp_image);
	
		gtk_widget_set_sensitive(img->sub_font_color, FALSE);

		if (pattern_pix)
			g_object_unref(pattern_pix); 
	}
	if (GTK_IS_WIDGET(fc))
		gtk_widget_destroy(fc);
}

void img_fadeout_duration_changed (GtkSpinButton *spinbutton, img_window_struct *img)
{
	//img->audio_fadeout = gtk_spin_button_get_value(spinbutton);
	img_taint_project(img);
}

void img_open_recent_slideshow(GtkWidget *menu, img_window_struct *img)
{
	const gchar *filename;
	
	filename = gtk_menu_item_get_label(GTK_MENU_ITEM(menu));
	if (img_can_discard_unsaved_project(img))
		img_load_project(img, menu, filename);
}

void img_add_any_media_callback(GtkButton *button,  img_window_struct *img)
{
	img_add_media_items(NULL, img);	
}

void img_zoom_fit( GtkWidget *item, img_window_struct *img)
{
	GtkAllocation allocation;
	gdouble step, level1, level2;
	
	gtk_widget_get_allocation(gtk_widget_get_parent(GTK_WIDGET(img->image_area)), &allocation);

	level1 = (float)allocation.width / img->video_size[0];
	level2 = (float)allocation.height / img->video_size[1];

	if (level1 < level2)
		step = level1 - 1;
	else
		step = level2 - 1;

	img->image_area_zoom = 1;

	img->image_area_zoom = CLAMP( img->image_area_zoom + step, 0.1, 1.0);
	gtk_widget_set_size_request(img->image_area, img->video_size[0] * img->image_area_zoom, img->video_size[1] * img->image_area_zoom);
}

void img_media_duration_value_changed (GtkSpinButton *spinbutton, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;

	gdouble width, duration = 0;
	duration = gtk_spin_button_get_value(spinbutton);
	
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->type == 0)
		{
			for (gint q = 0; q < track->items->len; q++)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)))
				{
					item->duration = duration;
					width = item->duration * priv->pixels_per_second;
					gtk_widget_set_size_request(GTK_WIDGET(item->button), width, 50);
				}
			}
		}
	}
	gtk_widget_queue_draw(img->timeline);
	img_taint_project(img);
}

void img_surface_effect_changed(GtkComboBox *widget, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;

	const gchar *filename;
	gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->type == 0)
		{
			for (gint q = 0; q < track->items->len; q++)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)))
				{
					filename = img_get_media_filename(img,  item->id);
					g_hash_table_remove(img->cached_preview_surfaces, GINT_TO_POINTER(item->id));
					img_create_cached_cairo_surface(img, item, (gchar*) filename);
					item->color_filter = active;
					img_apply_filter_on_surface(img, item, active);
					
				}
			}
		}
	}
	img_taint_project(img);
	gtk_widget_queue_draw(img->image_area);
}

void img_opacity_value_changed(GtkRange *range, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;
	gdouble opacity;
	
	opacity = gtk_range_get_value(range);
	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->type == 0)
		{
			for (gint q = 0; q < track->items->len; q++)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)))
					item->opacity = opacity / 100.0;
			}
		}
	}
	gtk_widget_queue_draw(img->image_area);
	img_taint_project(img);
}

void img_volume_value_changed(GtkRange *range, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;
}

void img_flip_horizontal_button_clicked(GtkButton *button, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->type == 0)
		{
			for (gint q = 0; q < track->items->len; q++)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)))
				{
					img_flip_surface_horizontally(img, item);
					item->flipped_horizontally = !item->flipped_horizontally;
				}
			}
		}
	}
	img_taint_project(img);
	gtk_widget_queue_draw(img->image_area);
}

void img_flip_vertical_button_clicked(GtkButton *button, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->type == 0)
		{
			for (gint q = 0; q < track->items->len; q++)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)))
				{
					img_flip_surface_vertically(img, item);
					item->flipped_vertically = !item->flipped_vertically;
				}
			}
		}
	}
	img_taint_project(img);
	gtk_widget_queue_draw(img->image_area);
}

void img_rotate_button_clicked(GtkButton *button, img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->type == 0)
		{
			for (gint q = 0; q < track->items->len; q++)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->button)))
					img_rotate_surface(img, item, TRUE);
			}
		}
	}
	gtk_widget_queue_draw(img->image_area);
}

void img_move_surfaces_left(img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->type == 0)
		{
			for (gint q = 0; q < track->items->len; q++)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				if (item->is_selected)
					item->x--;
			}
		}
	}
	gtk_widget_queue_draw(img->image_area);
}

void img_move_surfaces_right(img_window_struct *img)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track;
	media_timeline *item;

	for (gint i = 0; i < priv->tracks->len; i++)
	{
		track = g_array_index(priv->tracks, Track *, i);
		if (track->type == 0)
		{
			for (gint q = 0; q < track->items->len; q++)
			{
				item = g_array_index(track->items, media_timeline  *, q);
				if (item->is_selected)
					item->x++;
			}
		}
	}
	gtk_widget_queue_draw(img->image_area);
}

gboolean img_image_area_leave_event(GtkWidget *button, GdkEventCrossing *event, img_window_struct *img)
{
	if (img->current_item->text)
	{
		if (img->current_item->text->cursor_source_id > 0)
		{
			g_source_remove(img->current_item->text->cursor_source_id);
			img->current_item->text->cursor_source_id = 0;
		}
	}
	return FALSE;
}
