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

#include "callbacks.h"
#include "empty_slide.h"
#include "export.h"
#include <math.h>
#include <sys/stat.h>

static void img_file_chooser_add_preview(img_window_struct *);
static void img_update_preview_file_chooser(GtkFileChooser *,img_window_struct *);
gboolean img_transition_timeout(img_window_struct *);
static gboolean img_still_timeout(img_window_struct *);
static void img_swap_toolbar_images( img_window_struct *, gboolean);
static void img_clean_after_preview(img_window_struct *);
static GdkPixbuf *img_rotate_pixbuf( GdkPixbuf *, ImgAngle );
static void img_rotate_selected_slides( img_window_struct *, gboolean );

static void img_update_textbox_boundaries(img_window_struct *img)
{
    int width = gtk_widget_get_allocated_width(img->image_area);
    int height = gtk_widget_get_allocated_height(img->image_area);

	pango_layout_get_size(img->textbox->layout, &img->textbox->lw, &img->textbox->lh);
	
	img->textbox->lw /= PANGO_SCALE;
	img->textbox->lh /= PANGO_SCALE;
  
    // Adjust textbox width and height if needed
    if (img->textbox->lw > img->textbox->width - 15 && img->textbox->lw + 50 <= width)
		img->textbox->width = img->textbox->lw + 50;
	
	if (img->textbox->lh > img->textbox->height && img->textbox->lh + 20 <= height)
		img->textbox->height = img->textbox->lh + 20;
		
	pango_layout_set_width(img->textbox->layout, pango_units_from_double(img->textbox->width));
}

static void
img_image_area_change_zoom( gdouble            step,
							gboolean           reset,
							img_window_struct *img );

/* Asks the user before discarding changes */
gboolean img_can_discard_unsaved_project(img_window_struct *img) {
	if (!img->project_is_modified)
	{
	    return TRUE;
	}
	int response = img_ask_user_confirmation( img,
		_("You didn't save your slideshow yet.\nAre you sure you want to close it?"));
	if (response == GTK_RESPONSE_OK)
	    return TRUE;
	return FALSE;
}

/* Mark the project as to be saved */
void img_taint_project(img_window_struct *img)
{
    if (!img->project_is_modified) {
	img->project_is_modified = TRUE;
	img_refresh_window_title(img);
    }
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

void img_new_slideshow(GtkMenuItem *item, img_window_struct *img_struct)
{
    if (!img_can_discard_unsaved_project(img_struct))
		return;

    img_new_slideshow_settings_dialog(img_struct, FALSE);
}

void img_project_properties(GtkMenuItem *item, img_window_struct *img_struct)
{
	img_new_slideshow_settings_dialog(img_struct, TRUE);
}

static void detect_slide_orientation_from_pixbuf(GdkPixbuf *image, gboolean *flipped, ImgAngle *angle) {
        int transform = 0;
	const gchar *orientation_string;

	orientation_string = gdk_pixbuf_get_option(image, "orientation");

	if (orientation_string) {
		/* If an orientation option was found, convert the 
		   orientation string into an integer. */
		transform = (int) g_ascii_strtoll (orientation_string, NULL, 10);
	}

	*flipped = FALSE;
	*angle = ANGLE_0;

	/* The meaning of orientation values 1-8 and the required transforms
	   are defined by the TIFF and EXIF (for JPEGs) standards. */
        switch (transform) {
	case 1:
	    break;
        case 2:
	    *flipped = TRUE;
	    break;
        case 3:
	    *angle = ANGLE_180;
	    break;
        case 4:
	    *angle = ANGLE_180;
	    *flipped = TRUE;
	    break;
        case 5:
	    *angle = ANGLE_90;
	    /* cannot flip image vertically */
	    break;
        case 6:
	    *angle = ANGLE_270;
	    break;
        case 7:
	    *angle = ANGLE_270;
	    /* cannot flip image vertically */
	    break;
        case 8:
	    *angle = ANGLE_90;
	    break;
	default:
	    /* if no orientation tag was present */
	    break;
        }
}


void img_add_slides_thumbnails(GtkMenuItem *item, img_window_struct *img) {
	GSList *slides = img_import_slides_file_chooser(img);

	if (slides == NULL)
		return;

	img_add_slides(slides, img);
}

// slides is a list of filenames of slides to add. The list and its content
// is freed by the function.
void img_add_slides(GSList *slides, img_window_struct *img)
{
	GSList *bak;
	GdkPixbuf *thumb;
	GtkTreeIter iter;
	slide_struct *slide_info;
	GError *error = NULL;
	gchar *filename;
	gint slides_cnt = 0, actual_slides = 0;

	actual_slides = img->slides_nr;
	img->slides_nr += g_slist_length(slides);

	/* Remove model from thumbnail iconview for efficiency */
	g_object_ref( G_OBJECT( img->thumbnail_model ) );
	gtk_icon_view_set_model( GTK_ICON_VIEW( img->thumbnail_iconview ), NULL );

	bak = slides;
	while (slides)
	{
	    slide_info = img_create_new_slide();
	    g_assert(slide_info);
	    filename = slides->data;
	    // to get the orientation tag, load a tiny version of the image
	    thumb = gdk_pixbuf_new_from_file_at_size(filename, 1, 1, &error);
	    if (!thumb)
	    {
			img_message(img, error->message);
			g_error_free (error);
			error = NULL;
			img->slides_nr--;
			goto next_slide;
	    }
	    /* the file exists and can be read, create the slide_info */
	    img_set_slide_file_info( slide_info, slides->data );
	    /* actually read the orientation tag */
	    detect_slide_orientation_from_pixbuf(thumb, &(slide_info->flipped),
		    &(slide_info->angle));
	    g_object_unref(thumb);
	    /* create the associated p_filename taking orientation into account */
	    img_rotate_flip_slide(slide_info, slide_info->angle, slide_info->flipped);
	    /* create the thumbnail */
	    img_scale_image( slide_info->p_filename, img->video_ratio, 88, 0,
			img->distort_images, img->background_color,
			&thumb, NULL );
	    if (!thumb)
	    {
			gchar *string;
			string = g_strconcat( _("Cannot open the thumbnail %s of the new slide %s"), slide_info->p_filename, slide_info->o_filename, NULL);
			img_message(img, string);
			g_free(string);
			img->slides_nr--;
			goto next_slide;
	    }
	    gtk_list_store_append (img->thumbnail_model,&iter);
	    gtk_list_store_set (img->thumbnail_model, &iter, 0, thumb,
		    1, slide_info,
		    2, NULL,
		    3, FALSE,
		    -1);
	    g_object_unref (thumb);
	    slides_cnt++;
	next_slide:
	    g_free(slides->data);
	    slides = slides->next;
	}
	g_slist_free(bak);
	img_set_total_slideshow_duration(img);
	img_taint_project(img);

	gtk_icon_view_set_model( GTK_ICON_VIEW( img->thumbnail_iconview ),
							 GTK_TREE_MODEL( img->thumbnail_model ) );
	g_object_unref( G_OBJECT( img->thumbnail_model ) );
	
	img_select_nth_slide(img, actual_slides);
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
	gtk_file_filter_set_name(sound_filter,_("All sound files"));
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

void img_free_allocated_memory(img_window_struct *img_struct)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	slide_struct *entry;

	/* Free the memory allocated the single slides one by one */
	if (img_struct->slides_nr)
	{
		model = GTK_TREE_MODEL( img_struct->thumbnail_model );

		gtk_tree_model_get_iter_first(model,&iter);
		do
		{
			gtk_tree_model_get(model, &iter,1,&entry,-1);
			img_free_slide_struct( entry );
			img_struct->slides_nr--;
		}
		while (gtk_tree_model_iter_next (model,&iter));
		g_signal_handlers_block_by_func((gpointer)img_struct->thumbnail_iconview, (gpointer)img_iconview_selection_changed, img_struct);
		gtk_list_store_clear(GTK_LIST_STORE(img_struct->thumbnail_model));
		g_signal_handlers_unblock_by_func((gpointer)img_struct->thumbnail_iconview, (gpointer)img_iconview_selection_changed, img_struct);
	}

	/* Unlink the possible created rotated pictures and free the GSlist */
	/* NOTE: This is now done by img_free_slide_struct function */
	
	/* Free gchar pointers */
	if (img_struct->current_dir)
	{
		g_free(img_struct->current_dir);
		img_struct->current_dir = NULL;
	}
	
	if (img_struct->project_current_dir)
    {
        g_free(img_struct->project_current_dir);
        img_struct->project_current_dir = NULL;
    }

	if (img_struct->project_filename)
	{
		g_free(img_struct->project_filename);
		img_struct->project_filename = NULL;
	}
	if (img_struct->slideshow_filename)
	{
		g_free(img_struct->slideshow_filename);
		img_struct->slideshow_filename = NULL;
	}
	if (img_struct->first_selected_path)
	{
		gtk_tree_path_free(img_struct->first_selected_path);
		img_struct->first_selected_path = NULL;
	}
}

gboolean img_window_key_pressed(GtkWidget *widget, GdkEventKey *event, img_window_struct *img)
{
	//This hack to give image area key events as when the toplevel window contains other widgets due
	//to focus reasons the drawing area doesn't get any keyboard events despite mask and focus are set
	if (img->textbox->draw_rect)
		 return gtk_widget_event(img->image_area, (GdkEvent*)event);

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
	else if (event->keyval == GDK_KEY_space)
		img_start_stop_preview(NULL, img);
	else
			if (event->keyval == GDK_KEY_F11)
				img_go_fullscreen(NULL, img);

	return TRUE;
}

gboolean img_image_area_key_press(GtkWidget *widget, GdkEventKey *event, img_window_struct *img)
{
	gboolean shift_pressed;
	gboolean ctrl_pressed;
	
	if (!img->textbox->draw_rect)
        return FALSE;

	shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;
	ctrl_pressed = (event->state & GDK_CONTROL_MASK) != 0;

	
	switch (event->keyval)
    {
		case GDK_KEY_a:
		case GDK_KEY_A:
         if (ctrl_pressed)
         {
			img->textbox->selection_start = 0;
			img->textbox->selection_end = img->textbox->text->len;
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
				g_string_insert(img->textbox->text, img->textbox->cursor_pos, text);
				g_free(text);
			}
		}
		else
			goto handle;
		break;
	 
		case GDK_KEY_Left:
		 if (img->textbox->cursor_pos > 0)
		 {
			 img->textbox->cursor_pos--;
			 if (shift_pressed)
			 {
				 if (img->textbox->selection_start == -1) img->textbox->selection_start = img->textbox->cursor_pos + 1;
				 img->textbox->selection_end = img->textbox->cursor_pos;
			 }
			 else 
				img->textbox->selection_start = img->textbox->selection_end = -1;
		 }
		 break;
		 
		case GDK_KEY_Right:
		if (img->textbox->text->str && img->textbox->cursor_pos < img->textbox->text->len)
		{
			img->textbox->cursor_pos++;
			if (shift_pressed)
			{
				 if (img->textbox->selection_start == -1) img->textbox->selection_start = img->textbox->cursor_pos - 1;
				 img->textbox->selection_end = img->textbox->cursor_pos;
			}
			else
				 img->textbox->selection_start = img->textbox->selection_end = -1; 
		}
		break;
   		
   		case GDK_KEY_Up:
		case GDK_KEY_Down:
            int index, trailing;
            PangoRectangle pos;
            
            pango_layout_get_cursor_pos(img->textbox->layout, img->textbox->cursor_pos, &pos, NULL);
            int line = pango_layout_xy_to_index(img->textbox->layout, 
                pos.x, 
                pos.y + (event->keyval == GDK_KEY_Down ? pos.height : -pos.height),
                &index, &trailing);
            if (line != -1)
            {
                img->textbox->cursor_pos = index + trailing;
                if (shift_pressed)
                {
                    if (img->textbox->selection_start == -1)
						img->textbox->selection_start = img->textbox->cursor_pos;
                    
                    img->textbox->selection_end = img->textbox->cursor_pos;
                }
                else
					img->textbox->selection_start = img->textbox->selection_end = -1;
            }
		break;

		case GDK_KEY_BackSpace:
		if (img->textbox->text->len > 0)
		{
			// Text is selected
			if (img->textbox->selection_start != -1 && img->textbox->selection_end != -1)
			{
				int start = MIN(img->textbox->selection_start, img->textbox->selection_end);
				int end = MAX(img->textbox->selection_start, img->textbox->selection_end);
				
				// User pressed CTRL+A
				if(img->textbox->selection_start == 0 && img->textbox->selection_end == img->textbox->text->len)
					g_string_erase(img->textbox->text,  0, img->textbox->selection_end);
				else
					g_string_erase(img->textbox->text,  start,  end - start);

				img->textbox->cursor_pos = start;
				img->textbox->selection_start = img->textbox->selection_end = -1;
			}
			else
			{
				if (img->textbox->cursor_pos > 0)
				{
					g_string_erase(img->textbox->text, img->textbox->cursor_pos-1, 1);
					img->textbox->cursor_pos--;
				}
			}
		}
		break;

		case GDK_KEY_Return:
			g_string_insert_c(img->textbox->text, img->textbox->cursor_pos, '\n');
			img->textbox->cursor_pos++;
		break;

handle:
		default:
			if (g_ascii_isprint(event->keyval))
			{
				// Text is selected
				if (img->textbox->selection_start != -1 && img->textbox->selection_end != -1)
				{
					int start = MIN(img->textbox->selection_start, img->textbox->selection_end);
					int end = MAX(img->textbox->selection_start, img->textbox->selection_end);
					g_string_erase(img->textbox->text,  start,  end - start);
					g_string_insert_unichar(img->textbox->text, start, event->string[0]);
					img->textbox->cursor_pos = start+1;
					img->textbox->selection_start = img->textbox->selection_end = -1;
				}
				else
				{
					g_string_insert_unichar(img->textbox->text, img->textbox->cursor_pos, event->string[0]);
					img->textbox->cursor_pos++;
				}
			}
	}

	pango_layout_set_text(img->textbox->layout, img->textbox->text->str, -1);
	img->textbox->cursor_visible = TRUE;
    if (img->textbox->cursor_source_id != 0)
		g_source_remove(img->textbox->cursor_source_id);

	img->textbox->cursor_source_id = g_timeout_add(750, (GSourceFunc) blink_cursor, img);
 
	img_update_textbox_boundaries(img);
	
    gtk_widget_queue_draw(widget);
    return TRUE;
}

void img_exit_fullscreen(img_window_struct *img)
{
	GdkWindow *win;

	gtk_widget_show(img->thumb_scrolledwindow);
	gtk_widget_show (img->main_horizontal_box);
	gtk_widget_show(img->menubar);
	gtk_widget_show(img->side_notebook);
	gtk_widget_show(img->sidebar);
	gtk_widget_show(img->preview_hbox);
	
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
	//gtk_widget_remove_accelerator (img->fullscreen, img->accel_group, GDK_KEY_F11, 0);

	gtk_widget_hide (img->thumb_scrolledwindow);
	gtk_widget_hide (img->menubar);
	gtk_widget_hide (img->side_notebook);
	gtk_widget_hide (img->sidebar);
	gtk_widget_set_halign(img->image_area, GTK_ALIGN_FILL);
	gtk_widget_set_valign(img->image_area, GTK_ALIGN_FILL);
	gtk_widget_hide (img->preview_hbox);
	
	gtk_window_fullscreen(GTK_WINDOW(img->imagination_window));
	img->window_is_fullscreen =TRUE;

	gtk_widget_queue_draw(img->image_area);
}

gboolean img_quit_application(GtkWidget *widget, GdkEvent * event, img_window_struct *img_struct)
{
	if (img_struct->no_recent_item_menu)
		gtk_widget_destroy(img_struct->no_recent_item_menu);

	if (!img_can_discard_unsaved_project(img_struct)) {
	    return TRUE;
	}
	if( img_save_window_settings( img_struct ) )
		return( TRUE );
	img_free_allocated_memory(img_struct);

	/* Unloads the plugins */
	GSList *node1;
	for(node1 = img_struct->plugin_list;node1 != NULL;node1 = node1->next) {
		g_module_close(node1->data);
	}
	g_slist_free(img_struct->plugin_list);

	return FALSE;
}

static void img_file_chooser_add_preview(img_window_struct *img_struct)
{
	GtkWidget *vbox;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width (GTK_CONTAINER(vbox), 10);

	img_struct->preview_image = gtk_image_new ();

	img_struct->dim_label  = gtk_label_new (NULL);
	img_struct->size_label = gtk_label_new (NULL);

	gtk_box_pack_start (GTK_BOX (vbox), img_struct->preview_image, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), img_struct->dim_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), img_struct->size_label, FALSE, TRUE, 0);
	gtk_widget_show_all (vbox);

	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(img_struct->import_slide_chooser), vbox);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER(img_struct->import_slide_chooser), FALSE);

	g_signal_connect (img_struct->import_slide_chooser, "update-preview",G_CALLBACK (img_update_preview_file_chooser), img_struct);
}

static void	img_update_preview_file_chooser(GtkFileChooser *file_chooser,img_window_struct *img_struct)
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
		gtk_image_set_from_pixbuf (GTK_IMAGE(img_struct->preview_image), pixbuf);
		g_object_unref (pixbuf);

		size = g_strdup_printf(ngettext("%d x %d pixels", "%d x %d pixels", height),width,height);
		gtk_label_set_text(GTK_LABEL(img_struct->dim_label),size);
		g_free(size);
	}
	g_free(filename);
	gtk_file_chooser_set_preview_widget_active (file_chooser, has_preview);
}

void img_delete_selected_slides(GtkMenuItem *item,img_window_struct *img)
{
	GList *selected, *bak;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;
	slide_struct *entry;
	gint *indices;

	/* slide to select after all slides are removed: the slide after the first one being removed */
	gint next_slide = img->slides_nr;
	
	model =	GTK_TREE_MODEL( img->thumbnail_model );
	
	selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(img->thumbnail_iconview));
	if (selected == NULL || img->preview_is_running)
		return;
	
	/* Free the slide struct for each slide and remove it from the iconview */
	bak = selected;
	g_signal_handlers_block_by_func( (gpointer)img->thumbnail_iconview,
									 (gpointer)img_iconview_selection_changed,
									 img );
	
	while (selected)
	{
		gtk_tree_model_get_iter(model, &iter,selected->data);
		gtk_tree_model_get(model, &iter,1,&entry,-1);
		path = gtk_tree_model_get_path(model, &iter);
		indices = gtk_tree_path_get_indices(path);
		if (indices)
		{
		    gint index = indices[0];
		    if (index < next_slide)
				next_slide = index;
		}
		img_free_slide_struct( entry );
		gtk_list_store_remove(GTK_LIST_STORE(img->thumbnail_model),&iter);
		img->slides_nr--;
		selected = selected->next;
	}
	g_signal_handlers_unblock_by_func( (gpointer)img->thumbnail_iconview,
									   (gpointer)img_iconview_selection_changed,
									   img );
	GList *node11;
	for(node11 = bak;node11 != NULL;node11 = node11->next) {
		gtk_tree_path_free(node11->data);
	}
	g_list_free(bak);

	cairo_surface_destroy( img->current_image );
	img->current_image = NULL;
	gtk_widget_queue_draw( img->image_area );

	/* Select the next slide */
	if (next_slide < 0 || next_slide >= img->slides_nr - 1)
		next_slide = img->slides_nr - 1;
		
	if (next_slide > -1 )
	{
		path = gtk_tree_path_new_from_indices(next_slide, -1);
		gtk_icon_view_set_cursor (GTK_ICON_VIEW (img->thumbnail_iconview), path, NULL, FALSE);
		gtk_icon_view_select_path (GTK_ICON_VIEW (img->thumbnail_iconview), path);
		gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (img->thumbnail_iconview), path, FALSE, 0, 0);
		gtk_tree_path_free (path);
	}

	img_taint_project(img);
	img_iconview_selection_changed(GTK_ICON_VIEW(img->thumbnail_iconview),img);
}

void
img_rotate_slides_left( GtkWidget         *widget,
						img_window_struct *img )
{
	img_rotate_selected_slides( img, TRUE );
}

void
img_rotate_slides_right( GtkWidget         *widget,
						 img_window_struct *img )
{
	img_rotate_selected_slides( img, FALSE );
}

static void
img_rotate_selected_slides( img_window_struct *img,
							gboolean           clockwise )
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GList        *selected,
				 *bak;
	GdkPixbuf    *thumb;
	slide_struct *info_slide;

	/* Obtain the selected slideshow filename */
	model = GTK_TREE_MODEL( img->thumbnail_model );
	selected = gtk_icon_view_get_selected_items(
					GTK_ICON_VIEW( img->thumbnail_iconview ) );

	if( selected == NULL)
		return;

	img_taint_project(img);

	bak = selected;
	while (selected)
	{
		ImgAngle angle;

		gtk_tree_model_get_iter( model, &iter, selected->data );
		gtk_tree_model_get( model, &iter, 1, &info_slide, -1 );

		/* Avoid seg-fault when dealing with text/gradient slides */
		if (info_slide->o_filename != NULL)
		{
			angle = ( info_slide->angle + ( clockwise ? 1 : -1 ) ) % 4;
			img_rotate_flip_slide( info_slide, angle, info_slide->flipped );

			/* Display the rotated image in thumbnails iconview */
			img_scale_image( info_slide->p_filename, img->video_ratio, 88, 0,
							 img->distort_images, img->background_color,
							 &thumb, NULL );
			gtk_list_store_set( img->thumbnail_model, &iter, 0, thumb, -1 );
		}
		selected = selected->next;
	}
	GList *node12;
	for(node12 = bak;node12 != NULL;node12 = node12->next) {
		gtk_tree_path_free(node12->data);
	}
	g_list_free(bak);

	/* If no slide is selected currently, simply return */
	if( ! img->current_slide )
		return;

	if (info_slide->o_filename != NULL)
	{
		cairo_surface_destroy( img->current_image );
		img_scale_image( img->current_slide->p_filename, img->video_ratio,
							 0, img->video_size[1], img->distort_images,
							 img->background_color, NULL, &img->current_image );
		
		gtk_widget_queue_draw( img->image_area );
	}
}

/* Rotate clockwise */
static GdkPixbuf *img_rotate_pixbuf( GdkPixbuf *original, ImgAngle  angle )
{
	GdkPixbuf     *new;
	gint           w, h, r1, r2, channels, bps;
	GdkColorspace  colorspace;
	gboolean       alpha;
	guchar        *pixels1, *pixels2;
	gint           i, j;

	/* Get data from source */
	g_object_get( G_OBJECT( original ), "width", &w,
										"height", &h,
										"rowstride", &r1,
										"n_channels", &channels,
										"bits_per_sample", &bps,
										"colorspace", &colorspace,
										"has_alpha", &alpha,
										"pixels", &pixels1,
										NULL );

	switch( angle )
	{
		case ANGLE_90:
			/* Create new rotated image */
			new = gdk_pixbuf_new( colorspace, alpha, bps, h, w );
			g_object_get( G_OBJECT( new ), "rowstride", &r2,
										   "pixels", &pixels2,
										   NULL );
			
			/* Copy data, applying transormation along the way */
			for( j = 0; j < h; j++ )
			{
				for( i = 0; i < w; i++ )
				{
					int source = i * channels + r1 * j;
					int dest   = j * channels + r2 * ( w - i - 1 );
					int n;
					
					for( n = 0; n < channels; n++ )
						pixels2[dest + n] = pixels1[source + n];
				}
				
				if( j % 100 )
					continue;

				while( gtk_events_pending() )
					gtk_main_iteration();
			}
			break;

		case ANGLE_180:
			/* Create new rotated image */
			new = gdk_pixbuf_new( colorspace, alpha, bps, w, h );
			g_object_get( G_OBJECT( new ), "rowstride", &r2,
										   "pixels", &pixels2,
										   NULL );

			/* Copy data, applying transormation along the way */
			for( j = 0; j < h; j++ )
			{
				for( i = 0; i < w; i++ )
				{
					int source = i * channels + r1 * j;
					int dest   = ( w - i - 1 ) * channels + r2 * ( h - j - 1 );
					int n;

					for( n = 0; n < channels; n++ )
						pixels2[dest + n] = pixels1[source + n];
				}

				if( j % 100 )
					continue;

				while( gtk_events_pending() )
					gtk_main_iteration();
			}
			break;

		case ANGLE_270:
			/* Create new rotated image */
			new = gdk_pixbuf_new( colorspace, alpha, bps, h, w );
			g_object_get( G_OBJECT( new ), "rowstride", &r2,
										   "pixels", &pixels2,
										   NULL );

			/* Copy data, applying transormation along the way */
			for( j = 0; j < h; j++ )
			{
				for( i = 0; i < w; i++ )
				{
					int source = i * channels + r1 * j;
					int dest   = ( h - j - 1 ) * channels + r2 * i;
					int n;

					for( n = 0; n < channels; n++ )
						pixels2[dest + n] = pixels1[source + n];
				}

				if( j % 100 )
					continue;

				while( gtk_events_pending() )
					gtk_main_iteration();
			}
			break;
		default:
			g_message("tried to rotate image in img_rotate_pixbug by an illegal amount %d", angle);
			/* fallthrough */
		case ANGLE_0:
			g_object_ref( G_OBJECT( original ) );
			new = original;
			break;

	}

	return( new );
}

void img_show_about_dialog (GtkMenuItem *item, img_window_struct *img_struct)
{
	static GtkWidget *about = NULL;
	static gchar version[] = VERSION;
    const char *authors[] = {"\nDevelopers:\nGiuseppe Torelli <colossus73@gmail.com>",
								"Tadej Borovšak <tadeboro@gmail.com>",
								"Symphorien Gibol <symphorien.gibol@gmail.com>",
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
			"copyright","Copyright \xC2\xA9 2009-2024 Giuseppe Torelli",
			"comments","A simple and lightweight slideshow maker",
			"authors",authors,
			"documenters",NULL,
			"translator_credits",_("translator-credits"),
			"logo_icon_name","imagination",
			"website","http://imagination.sf.net",
			"license","Copyright \xC2\xA9 2009-2020 Giuseppe Torelli - Colossus <colossus73@gmail.com>\n\n"
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

void img_start_stop_preview(GtkWidget *item, img_window_struct *img)
{
	GtkTreeIter iter, prev;
	GtkTreePath *path = NULL;
	slide_struct *entry;
	GtkTreeModel *model;
	GList *list = NULL;

	/* If no images are present, abort */
	if( img->slides_nr == 0 )
		return;

	if(img->export_is_running)
		return;

	/* Save the current selected slide to restore it when the preview is finished */
	if (img->preview_is_running == FALSE)
	{
		gtk_tree_path_free(img->first_selected_path);
		gtk_icon_view_get_cursor(GTK_ICON_VIEW( img->thumbnail_iconview ), &img->first_selected_path, NULL);
	}
	if (img->preview_is_running)
	{
		/* Remove timeout function from main loop */
		g_source_remove(img->source_id);

		/* Clean resources used by preview and prepare application for
		 * next preview. */
		img_clean_after_preview(img);

		/* Unselect the last selected item during the preview */
		GList *list;
		list = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(img->thumbnail_iconview));

		if (list)
		{
			gtk_icon_view_unselect_path (GTK_ICON_VIEW(img->thumbnail_iconview), (GtkTreePath*)list->data);
			GList *node13;
			for(node13 = list;node13 != NULL;node13 = node13->next) {
				gtk_tree_path_free(node13->data);
			}
			g_list_free (list);
		}

		/*  Reselect the first selected slide before the preview if any */
		if (img->first_selected_path)
		{
			gtk_icon_view_select_path (GTK_ICON_VIEW(img->thumbnail_iconview), img->first_selected_path);
			gtk_icon_view_scroll_to_path(GTK_ICON_VIEW(img->thumbnail_iconview), img->first_selected_path, FALSE, 0.0, 0.0);
		}
	}
	else
	{
		model = GTK_TREE_MODEL( img->thumbnail_model );
		list = gtk_icon_view_get_selected_items(
					GTK_ICON_VIEW( img->thumbnail_iconview ) );
		if( list )
			gtk_icon_view_get_cursor( GTK_ICON_VIEW(img->thumbnail_iconview), &path, NULL);
		if( list )
		{
			/* Start preview from this slide */
			if( path )
				gtk_tree_model_get_iter( model, &iter, path );
			GList *node14;
			for(node14 = list;node14 != NULL;node14 = node14->next) {
				gtk_tree_path_free(node14->data);
			}
			g_list_free( list );
		}
		else
		{
			/* Start preview from the beginning */
			if( ! gtk_tree_model_get_iter_first( model, &iter ) )
				return;
		}
		img->cur_ss_iter = iter;

		/* Replace button and menu images */
		img_swap_toolbar_images( img, FALSE );

		/* Load the first image in the pixbuf */
		gtk_tree_model_get( model, &iter, 1, &entry, -1);

		if( ! entry->o_filename )
			img_scale_empty_slide( entry->gradient, entry->countdown, entry->g_start_point,
								entry->g_stop_point, entry->g_start_color,
								entry->g_stop_color, 
								entry->countdown_color, 
								1, entry->countdown_angle,
								img->video_size[0],
								img->video_size[1], NULL, &img->image2 );
		else
			img_scale_image( entry->p_filename, img->video_ratio,
								0, img->video_size[1], img->distort_images,
								img->background_color, NULL, &img->image2 );
	
		/* Load first stop point */
		img->point2 = (ImgStopPoint *)( entry->no_points ?
										entry->points->data :
										NULL );

		img->current_slide = entry;
		img->cur_point = NULL;

		/* If we started our preview from beginning, create empty pixbuf and
		 * fill it with background color otherwise load image that is before
		 * currently selected slide. */
		if( path != NULL && gtk_tree_path_prev( path ) )
		{
			gtk_tree_model_get_iter( model, &prev, path );
			gtk_tree_model_get( model, &prev, 1, &entry, -1 );
			
			if( ! entry->o_filename )
			{
				img_scale_empty_slide( entry->gradient, entry->countdown, entry->g_start_point,
									entry->g_stop_point, entry->g_start_color,
									entry->g_stop_color, 
									entry->countdown_color, 
									1, entry->countdown_angle,
									img->video_size[0],
									img->video_size[1], NULL, &img->image1 );
			}
			else
				img_scale_image( entry->p_filename, img->video_ratio,
									0, img->video_size[1], img->distort_images,
									img->background_color, NULL, &img->image1 );
			
			/* Load last stop point */
			img->point1 = (ImgStopPoint *)( entry->no_points ?
											g_list_last( entry->points )->data :
											NULL );
		}
		else
		{
			cairo_t *cr;

			img->image1 = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
													  img->video_size[0],
													  img->video_size[1] );
			cr = cairo_create( img->image1 );
			cairo_set_source_rgb( cr, img->background_color[0],
									  img->background_color[1],
									  img->background_color[2] );
			cairo_paint( cr );
			cairo_destroy( cr );
		}
		if( path )
			gtk_tree_path_free( path );

		/* Add transition timeout function */
		img->preview_is_running = TRUE;
		img->total_nr_frames = img->total_secs * img->preview_fps;
		img->displayed_frame = 0;
		img->next_slide_off = 0;
		img_calc_next_slide_time_offset( img, img->preview_fps );

		/* Create surfaces to be passed to transition renderer */
		img->image_from = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
													  img->video_size[0],
													  img->video_size[1] );
		img->image_to = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
													img->video_size[0],
													img->video_size[1] );
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
		img->exported_image = cairo_image_surface_create( CAIRO_FORMAT_RGB24,
														  img->video_size[0],
														  img->video_size[1] );

		if (entry->gradient == 4)
			img->source_id = g_timeout_add( 100, (GSourceFunc)img_empty_slide_countdown_preview, img );
		else
			img->source_id = g_timeout_add( 1000 / img->preview_fps, (GSourceFunc)img_transition_timeout, img );
	}
}

void img_on_drag_data_received (GtkWidget *widget, GdkDragContext
	*context , int x, int y, GtkSelectionData *data, 
	unsigned int info, unsigned int time, img_window_struct *img)
{
	gchar **pictures = NULL;
	gchar *filename;
	GSList *slides = NULL;
	GtkWidget *dialog;
	GFile *file;
	GFileInfo *file_info;
	const gchar *content_type;
	gchar *mime_type;
	int len = 0;

	pictures = gtk_selection_data_get_uris(data);
	if (pictures == NULL)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(img->imagination_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry, I could not perform the operation!"));
		gtk_window_set_title(GTK_WINDOW(dialog),"Imagination");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	gtk_drag_finish (context,TRUE,FALSE,time);

	while(pictures[len])
	{
		filename = g_filename_from_uri (pictures[len],NULL,NULL);
		//Determine the mime type
		if (filename == NULL)
			goto end;
		file = g_file_new_for_path (filename);
		file_info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
		content_type = g_file_info_get_content_type (file_info);
		mime_type = g_content_type_get_mime_type (content_type);
		if (strstr(mime_type, "image"))
			img_add_thumbnail_widget_area(0, filename, img);
		else if (strstr(mime_type, "audio"))
			img_add_thumbnail_widget_area(1, filename, img);
		else if (strstr(mime_type, "video"))
			g_print("Video!");
		else
		{
			gchar *string = g_strconcat( _("Can't recognize file type of media\n"), filename, NULL);
			img_message(img, string);
			g_free(string);
		}
		g_free(mime_type);
		g_free(filename);
end:
		len++;
	}
	g_strfreev (pictures);
}

gboolean img_on_draw_event( GtkWidget *widget, cairo_t *cr, img_window_struct *img )
{
	GtkAllocation allocation;
	gtk_widget_get_allocation(img->image_area, &allocation);

	/* If we're previewing or exporting, only paint frame that is being
	 * currently produced. */
	if( img->preview_is_running || img->export_is_running )
	{
		gdouble factor;

		/* Do the drawing */
		factor = (gdouble)allocation.width / img->video_size[0];
		cairo_scale( cr, factor, factor );
		cairo_set_source_surface( cr, img->exported_image, 0, 0 );
		cairo_paint( cr );
	}
	else
	{
		if( ! img->current_image )
		{
			/* Use default handler */
			return FALSE;
		}

		/* Do the drawing */
		img_draw_image_on_surface( cr, allocation.width, img->current_image, &img->current_point, img );

		/* Render subtitle if present */
		if( img->current_slide->subtitle )
			img_render_subtitle( img,
								 cr,
								 img->image_area_zoom,
								 img->current_slide->posX,
								 img->current_slide->posY,
								 img->current_slide->subtitle_angle,
								 img->current_slide->alignment,
								 img->current_point.zoom,
								 img->current_point.offx,
								 img->current_point.offy,
								 FALSE,
								 FALSE,
								 1.0 );

	int center_x = allocation.width /2;
	int center_y = allocation.height /2;

    // Apply rotation for the entire textbox
    cairo_translate(cr, img->textbox->x + img->textbox->width / 2, img->textbox->y + img->textbox->height / 2);
    cairo_rotate(cr, img->textbox->angle);
    cairo_translate(cr, -(img->textbox->x + img->textbox->width / 2), -(img->textbox->y + img->textbox->height / 2));

    if (img->textbox->draw_rect)
    {
        // Draw the angle
        if (img->textbox->action == IS_ROTATING && img->textbox->button_pressed)
        {
            cairo_save(cr);
            // Reset the transformation for angle text
            cairo_identity_matrix(cr);
            gchar buf[4];
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, 12.0);
            g_snprintf(buf, sizeof(buf), "%2.0f", round(img->textbox->angle * (180.0 / G_PI)));
            cairo_move_to(cr, 10, 20);
            cairo_show_text(cr, buf);
            cairo_restore(cr);
        }

        // Set the color to white for the outline effect
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_set_line_width(cr, 3.5);
		
		// Draw the L shape
		cairo_move_to(cr, img->textbox->x + (img->textbox->width / 2) + 6, img->textbox->y + img->textbox->height + 8);
		cairo_rel_line_to(cr, 0, 6);
		cairo_rel_line_to(cr, -6, 0);
		cairo_stroke(cr);

		// Draw the arc
		cairo_arc(cr, img->textbox->x + (img->textbox->width / 2), img->textbox->y + img->textbox->height + 15, 5, 30.0 * (G_PI / 180.0), 340.0 * (G_PI / 180.0));
		cairo_stroke(cr);

		// Draw the vertical line under the rotate circle
		cairo_move_to(cr, img->textbox->x + (img->textbox->width / 2), img->textbox->y + img->textbox->height + 10);
		cairo_line_to(cr, img->textbox->x + (img->textbox->width / 2), img->textbox->y + img->textbox->height);
		cairo_stroke(cr);

		// Now draw the black lines on top
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_set_line_width(cr, 1.5);  // Original line width

		// Draw the L shape again
		cairo_move_to(cr, img->textbox->x + (img->textbox->width / 2) + 6, img->textbox->y + img->textbox->height + 8);
		cairo_rel_line_to(cr, 0, 6);
		cairo_rel_line_to(cr, -6, 0);
		cairo_stroke(cr);

		// Draw the arc again
		cairo_arc(cr, img->textbox->x + (img->textbox->width / 2), img->textbox->y + img->textbox->height + 15, 5, 30.0 * (G_PI / 180.0), 340.0 * (G_PI / 180.0));
		cairo_stroke(cr);

		// Draw the vertical line under the rotate circle again
		cairo_set_line_width(cr, 1);
		cairo_move_to(cr, img->textbox->x + (img->textbox->width / 2), img->textbox->y + img->textbox->height + 10);
		cairo_line_to(cr, img->textbox->x + (img->textbox->width / 2), img->textbox->y + img->textbox->height);
		cairo_stroke(cr);

        // Draw the rectangle
        cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_rectangle(cr, img->textbox->x, img->textbox->y, img->textbox->width, img->textbox->height);
		cairo_stroke(cr);
        cairo_rectangle(cr, img->textbox->x , img->textbox->y , img->textbox->width , img->textbox->height );
        cairo_stroke_preserve(cr);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_fill(cr);

		//Let's translate to origin of the drawing area allocation to avoid
		//the cross lines to be rotated too
		cairo_save(cr);
		cairo_identity_matrix(cr);
		cairo_translate(cr, allocation.x, allocation.y);
		
		//This to compensate the allocation menubar height gap
		//which prevents the horizontal line to be drawn at the center
		//I wonder why the coordinates in the drawing area are shifted
		//when the blessed menubar is hidden by the TAB key, shouldn't
		//they always be 0,0 at the top left?

		int menubar_height = gtk_widget_get_allocated_height(img->menubar);
		if (img->textbox->draw_horizontal_line)
		{
			if (gtk_widget_is_visible(img->menubar))
				center_y = menubar_height + center_y;
			else
				center_y = allocation.height / 2;

		//Draw the horizontal centering line
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_set_line_width(cr, 1.0);
			cairo_move_to(cr, 0, center_y - 2);
			cairo_line_to(cr, allocation.width, center_y - 2);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0.8, 0.7, 0.3);
			cairo_move_to(cr, 0, center_y);
			cairo_line_to(cr, allocation.width, center_y);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_move_to(cr, 0, center_y + 2);
			cairo_line_to(cr, allocation.width, center_y + 2);
			cairo_stroke(cr);
		}
		
		// Draw the vertical centering line
		if (img->textbox->draw_vertical_line)
		{
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_set_line_width(cr, 1.0);
			cairo_move_to(cr, center_x - 2, 0);
			cairo_line_to(cr, center_x - 2, allocation.height + menubar_height);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0.8, 0.7, 0.3);
			cairo_move_to(cr, center_x, 0);
			cairo_line_to(cr, center_x, allocation.height + menubar_height);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_move_to(cr, center_x + 2, 0);
			cairo_line_to(cr, center_x + 2, allocation.height + menubar_height);
			cairo_stroke(cr);
		}
		cairo_restore(cr);
        cairo_set_line_width(cr, 2.5);
        
        // Draw the bottom right handle
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_arc(cr, img->textbox->x + img->textbox->width, img->textbox->y + img->textbox->height, 3, 0.0, 2 * G_PI);
        cairo_stroke_preserve(cr);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_fill(cr);
    
		// Draw selection highlight
        if (img->textbox->selection_start != img->textbox->selection_end)
        {
            int start_index = MIN(img->textbox->selection_start, img->textbox->selection_end);
            int end_index = MAX(img->textbox->selection_start, img->textbox->selection_end);
            
            PangoRectangle start_pos, end_pos;
            pango_layout_get_cursor_pos(img->textbox->layout, start_index, &start_pos, NULL);
            pango_layout_get_cursor_pos(img->textbox->layout, end_index, &end_pos, NULL);

            cairo_set_source_rgba(cr, 0.5, 0.5, 1.0, 0.6);  // Light blue, semi-transparent
            cairo_rectangle(cr, 
                img->textbox->x + 3 + start_pos.x / PANGO_SCALE, 
                img->textbox->y + start_pos.y / PANGO_SCALE,
                (end_pos.x - start_pos.x) / PANGO_SCALE, 
                (end_pos.y - start_pos.y + end_pos.height) / PANGO_SCALE);
            cairo_fill(cr);
        }

		// Draw the cursor
		if (img->textbox->cursor_visible && img->textbox->cursor_source_id)
		{
			PangoRectangle strong_pos;
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); 
			pango_layout_get_cursor_pos(img->textbox->layout, img->textbox->cursor_pos, &strong_pos, NULL);
			cairo_move_to(cr, (double)strong_pos.x / PANGO_SCALE + 5 + img->textbox->x, (double)strong_pos.y / PANGO_SCALE + 5 + img->textbox->y);
			cairo_line_to(cr, (double)strong_pos.x / PANGO_SCALE + 5 + img->textbox->x, (double)(strong_pos.y + strong_pos.height) / PANGO_SCALE + 5 + img->textbox->y);
			cairo_stroke(cr);
		}
	}
	// Draw the text
	cairo_set_source_rgb(cr, 0, 0, 0);
		
	pango_layout_set_markup(img->textbox->layout, img->textbox->text->str, -1);	
	cairo_move_to(cr, img->textbox->x + 3, img->textbox->y + 3);
	pango_cairo_show_layout(cr, img->textbox->layout);
	}

	return FALSE;
}

/*
 * img_draw_image_on_surface:
 * @cr: cairo context
 * @width: width of the surface that @cr draws on
 * @surface: cairo surface to be drawn on @cr
 * @point: stop point holding zoom and offsets
 * @img: global img_window_struct
 *
 * This function takes care of scaling and moving of @surface to fit properly on
 * cairo context passed in.
 */
void
img_draw_image_on_surface( cairo_t           *cr,
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

gboolean img_transition_timeout(img_window_struct *img)
{
	/* If we output all transition slides (or if there is no slides to output in
	 * transition part), connect still preview phase. */
	if( img->slide_cur_frame == img->slide_trans_frames )
	{
		img->source_id = g_timeout_add( 1000 / img->preview_fps, (GSourceFunc)img_still_timeout, img );
		return FALSE;
	}

	/* Render single frame */
	if (img->current_slide->gradient == 3)
	{
		img->gradient_slide = TRUE;
		memcpy(img->g_stop_color, img->current_slide->g_stop_color,  3 * sizeof(gdouble));
	}
	img_render_transition_frame( img );

	/* Schedule our image redraw */
	gtk_widget_queue_draw( img->image_area );

	/* Increment counters */
	img->slide_cur_frame++;
	img->displayed_frame++;

	return TRUE;
}

static gboolean img_still_timeout(img_window_struct *img)
{
	/* If there is next slide, connect transition preview, else finish
	 * preview. */
	if( img->slide_cur_frame == img->slide_nr_frames )
	{
		if( img_prepare_pixbufs( img) )
		{
			img_calc_next_slide_time_offset( img, img->preview_fps );
			img->source_id = g_timeout_add( 1000 / img->preview_fps,
										    (GSourceFunc)img_transition_timeout,
											img );
		
		}
		else
		{
			/* Clean resources used in preview and prepare application for
			 * next preview. */
			img_clean_after_preview( img );
		}

		/* Indicate that we must start fresh with new slide */
		img->cur_point = NULL;

		return FALSE;
	}

	/* This is a dirty hack to prevent Imagination
	keep painting the source image with the second
	* color set in the empty slide fade gradient 
	if (strcmp(gtk_entry_get_text(GTK_ENTRY(img->slide_number_entry)) , "2") == 0)
		img->gradient_slide = FALSE;
*/
	/* Render frame */
	img_render_still_frame( img, img->preview_fps );

	/* Increment counters */
	img->still_counter++;
	img->slide_cur_frame++;
	img->displayed_frame++;

	/* Redraw */
	gtk_widget_queue_draw( img->image_area );
	return( TRUE );
}

static void img_swap_toolbar_images( img_window_struct *img,gboolean flag )
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

static void img_clean_after_preview(img_window_struct *img)
{
	/* Swap toolbar and menu icons */
	img_swap_toolbar_images( img, TRUE );

	/* Indicate that preview is not running */
	img->preview_is_running = FALSE;
	
	/* Destroy images that were used */
	cairo_surface_destroy( img->image1 );
	cairo_surface_destroy( img->image2 );
	cairo_surface_destroy( img->image_from );
	cairo_surface_destroy( img->image_to );
	cairo_surface_destroy( img->exported_image );

	gtk_widget_queue_draw( img->image_area );

	return;
}

void img_choose_slideshow_filename(GtkWidget *widget, img_window_struct *img)
{
    GtkWidget *fc;
    GtkFileChooserAction action = 0;
    gint	 response;
    gboolean	 append, open_replace, save;
    gchar *filename = NULL;
    GtkFileFilter *project_filter, *all_files_filter;


    /* Determine what to do */
    append = widget == img->import_project_menu;
    open_replace = widget == img->open_menu;
    save = widget == img->save_as_menu || widget == img->save_menu;
    /* Determine the mode of the chooser. */
    if (open_replace || append) {
	action = GTK_FILE_CHOOSER_ACTION_OPEN;
    } else if (save) {
	action = GTK_FILE_CHOOSER_ACTION_SAVE;
    } else {
	img_message(img, _("I do not know what to do with this slideshow filename."));
	return;
    }

    /* close old slideshow if we import */
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
	fc = gtk_file_chooser_dialog_new (action == GTK_FILE_CHOOSER_ACTION_OPEN ? _("Load an Imagination slideshow project") : 
		_("Save an Imagination slideshow project"),
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
		img_load_slideshow(img,NULL, filename);
    else if (append) 
		img_append_slides_from( img, NULL, filename );
    else if (save)
		img_save_slideshow( img, filename, img->relative_filenames );

    img_refresh_window_title(img);

    g_free( filename );
}

void img_close_slideshow(GtkWidget *widget, img_window_struct *img)
{
    /* When called from close_menu, ask for confirmation */
    if (widget && widget == img->close_menu && !img_can_discard_unsaved_project(img))
		return;

	gtk_list_store_clear(GTK_LIST_STORE(img->media_model));
	img->media_nr = 0;
	img->project_is_modified = FALSE;
	img_free_allocated_memory(img);
	img_refresh_window_title(img);
	if( img->current_image )
		cairo_surface_destroy( img->current_image );
	img->current_image = NULL;
	gtk_widget_queue_draw( img->image_area );
	gtk_label_set_text(GTK_LABEL (img->slideshow_duration),"");

	/* Reset slideshow properties */
	img->distort_images = TRUE;
	img->background_color[0] = 0;
	img->background_color[1] = 0;
	img->background_color[2] = 0;
	img->final_transition.render = NULL;

	/* Disable the video tab */
    img_disable_videotab (img);

    //gtk_entry_set_text(GTK_ENTRY(img->slide_number_entry), "");
    gtk_widget_set_valign(img->thumbnail_iconview, GTK_ALIGN_BASELINE);
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

/*
 * img_image_area_scroll:
 * @widget: image area
 * @event: event description
 * @img: global img_window_struct structure
 *
 * This function catched mouse wheel event and changes
 * the zoom of the stop point accordingly to the wheel
 * scroll direction
 *
 * Return value: TRUE, indicating that we handled this event.
 */
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
	//~ if( event->button != 1 )
		//~ return( FALSE );
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(img->toggle_button_text)) && event->button == GDK_BUTTON_PRIMARY)
		img_textbox_button_pressed(event, img);
		//img->textbox->draw_rect = TRUE;
	
	img->x = event->x;
	img->y = event->y;
	img->bak_offx = img->current_point.offx;
	img->bak_offy = img->current_point.offy;

	return TRUE;
}

gboolean img_image_area_button_release(	GtkWidget *widget, GdkEventButton *event, img_window_struct *img)
{
	img_update_stop_point(NULL, img);
	
	if (event->button == GDK_BUTTON_PRIMARY)
    {
		if (img->textbox->action == IS_DRAGGING && (img->textbox->draw_horizontal_line || img->textbox->draw_vertical_line == TRUE))
			img->textbox->draw_horizontal_line = img->textbox->draw_vertical_line = FALSE;

		if (img->textbox->action == IS_RESIZING)
		{
			if (!img->textbox->cursor_source_id)
				img->textbox->cursor_source_id = g_timeout_add(750, (GSourceFunc) blink_cursor, img);			
		}
		if (img->textbox->width < 0)
		{
			img->textbox->width = img->textbox->orig_width;
			img->textbox->x = event->x;
		}
		if (img->textbox->height < 0)
		{
			img->textbox->height = img->textbox->orig_height;
			img->textbox->y= event->y;
		}
        img->textbox->button_pressed = FALSE;
        img->textbox->action = NONE;
        if (img->textbox->corner != GDK_FLEUR)
            gdk_window_set_cursor(gtk_widget_get_window(img->image_area), NULL);
    }

	return TRUE;
}

gboolean img_image_area_motion( GtkWidget * widget,  GdkEventMotion *event,  img_window_struct *img )
{
	gdouble deltax, deltay;
	gint width, height;
    double x, y;
	int snap_zone = 10;

	deltax = ( event->x - img->x ) / img->image_area_zoom;
	deltay = ( event->y - img->y ) / img->image_area_zoom;

	img->current_point.offx = CLAMP( deltax + img->bak_offx, img->maxoffx, 0 );
	img->current_point.offy = CLAMP( deltay + img->bak_offy, img->maxoffy, 0 );

	if (img->textbox->draw_rect == FALSE)
		return FALSE;

    transform_coords(img->textbox, event->x, event->y, &x, &y);

    if (img->textbox->button_pressed)
    {
		//This to draw the center lines to ease the img->textbox centering
		//later in the IS_DRAGGING action
		width  = gtk_widget_get_allocated_width(widget) / 2;
		height = gtk_widget_get_allocated_height(widget) / 2;

        switch (img->textbox->action)
        {
            case IS_RESIZING:
                switch (img->textbox->corner)
                {
					 case GDK_BOTTOM_RIGHT_CORNER:
					 {
							if (img->textbox->cursor_source_id)
							{
								g_source_remove(img->textbox->cursor_source_id);
								img->textbox->cursor_source_id = 0;
							}
							double new_width  = x - img->textbox->x;
							double new_height = y - img->textbox->y;
							
							//The following code is taken from https://shihn.ca/posts/2020/resizing-rotated-elements/

							// Calculate center of the rectangle
							double cx = img->textbox->x + img->textbox->width / 2;
							double cy = img->textbox->y + img->textbox->height / 2;

							// Calculate rotated coordinates of the top-left corner (A')
							double rotated_ax, rotated_ay;
							rotate_point(img->textbox->x, img->textbox->y, cx, cy, img->textbox->angle, &rotated_ax, &rotated_ay);

							// The bottom-right corner (C') is at the mouse position (x, y)
							double rotated_cx = event->x;
							double rotated_cy = event->y;

							// Calculate the new center
							double new_cx = (rotated_ax + rotated_cx) / 2;
							double new_cy = (rotated_ay + rotated_cy) / 2;

							// Calculate the new top-left corner (newA)
							double new_ax, new_ay;
							rotate_point(rotated_ax, rotated_ay, new_cx, new_cy, -img->textbox->angle, &new_ax, &new_ay);

							// Update img->textbox position
							img->textbox->x = new_ax;
							img->textbox->y = new_ay;
							//End of the code from the above URL
							
							img->textbox->width = new_width;
							img->textbox->height = new_height;
							
							//This to automatically word wrap the text when the user resizes the img->textbox
							pango_layout_set_width(img->textbox->layout, pango_units_from_double(img->textbox->width));
					}
                    break;
                
                    default:
                        break;
                }
                break;
                
            case IS_DRAGGING:
                 double textbox_center_x = img->textbox->x + img->textbox->width / 2;
                 double textbox_center_y = img->textbox->y + img->textbox->height / 2;
               
                // Calculate the distance moved
                double dx = event->x - img->textbox->dragx - img->textbox->x;
                double dy = event->y - img->textbox->dragy - img->textbox->y;
                
                // Check if we're entering or leaving the x snap zone
                if (!img->textbox->is_x_snapped && fabs(textbox_center_x - width) <= snap_zone)
                {
                    img->textbox->is_x_snapped = TRUE;
                    img->textbox->x = width - img->textbox->width / 2;
                    img->textbox->draw_vertical_line = TRUE;
                }
                else if (img->textbox->is_x_snapped && fabs(dx) > snap_zone)
                {
                    img->textbox->is_x_snapped = FALSE;
                    img->textbox->draw_vertical_line = FALSE;
                }
                // Update position if not x snapped
                if (!img->textbox->is_x_snapped)
                    img->textbox->x = event->x - img->textbox->dragx;

                // Check if we're entering or leaving the x snap zone
                if (!img->textbox->is_y_snapped && fabs(textbox_center_y - height) <= snap_zone)
                {
                    img->textbox->is_y_snapped = TRUE;
                    img->textbox->y = height - img->textbox->height / 2;
                    img->textbox->draw_horizontal_line = TRUE;
                }
                else if (img->textbox->is_y_snapped && fabs(dy) > snap_zone)
                {
                    img->textbox->is_y_snapped = FALSE;
                    img->textbox->draw_horizontal_line = FALSE;
                }
                // Update position if not y snapped
                if (!img->textbox->is_y_snapped)
					img->textbox->y = event->y - img->textbox->dragy;

                img->textbox->corner = GDK_FLEUR;
                break;
                
            case IS_ROTATING:
                dx = event->x - img->textbox->x;
                dy = event->y - img->textbox->y - img->textbox->height;
                img->textbox->angle = atan2(dy, dx);
                img->textbox->corner = GDK_EXCHANGE;
                break;
        }
        gdk_window_set_cursor(gtk_widget_get_window(img->image_area), gdk_cursor_new_for_display(gdk_display_get_default(), img->textbox->corner));

        gtk_widget_queue_draw(img->image_area);
        return TRUE;
    }

    // Check if the mouse pointer is hovering the rotate circle
    if (img->textbox->draw_rect && 
        x >= img->textbox->x + (img->textbox->width/2) - 5 && 
        x <= (img->textbox->x + (img->textbox->width/2)) + 5 && 
        y >= img->textbox->y + img->textbox->height + 10 && 
        y <= img->textbox->y + img->textbox->height + 20)
    {
        img->textbox->action = IS_ROTATING;
        img->textbox->corner = GDK_EXCHANGE;
        gdk_window_set_cursor(gtk_widget_get_window(img->image_area), gdk_cursor_new_for_display(gdk_display_get_default(), img->textbox->corner));
    }
    // If mouse pointer is inside the visible img->textbox change the cursor shape accordingly
    else if (!img->textbox->button_pressed && img->textbox->draw_rect && 
             x >= img->textbox->x && x <= img->textbox->x + img->textbox->width && 
             y >= img->textbox->y && y <= img->textbox->y + img->textbox->height)
    {
        if (x >= img->textbox->x + img->textbox->width - 5 && x <= img->textbox->x + img->textbox->width && 
            y >= img->textbox->y + img->textbox->height - 5 && y <= img->textbox->y + img->textbox->height)
        {
            img->textbox->corner = GDK_BOTTOM_RIGHT_CORNER;
            img->textbox->action = IS_RESIZING;
        }
        else
        {
            img->textbox->corner = GDK_XTERM;
            img->textbox->action = IS_DRAGGING;
        }
        gdk_window_set_cursor(gtk_widget_get_window(img->image_area), gdk_cursor_new_for_display(gdk_display_get_default(), img->textbox->corner));
    }
    else if (!img->textbox->button_pressed)
        gdk_window_set_cursor(gtk_widget_get_window(img->image_area), NULL);

	return TRUE;
}

void img_add_stop_point( GtkButton  *button, img_window_struct *img )
{
	ImgStopPoint *point;
	GList        *tmp;

	if (img->current_slide == NULL)
		return;

	/* Create new point */
	point = g_slice_new( ImgStopPoint );
	*point = img->current_point;
	point->time = gtk_spin_button_get_value_as_int(
						GTK_SPIN_BUTTON( img->ken_duration ) );

	/* Append it to the list */
	tmp = img->current_slide->points;
	tmp = g_list_append( tmp, point );
	img->current_slide->points = tmp;
	img->current_slide->cur_point = img->current_slide->no_points;
	img->current_slide->no_points++;

	/* Update display */
	img_update_stop_display( img, FALSE );

	/* Sync timings */
	img_sync_timings( img->current_slide, img );
	
	img_taint_project(img);
}

void
img_update_stop_point( GtkSpinButton  *button,
					   img_window_struct *img )
{
	ImgStopPoint *point;
	gint full;

	if( img->current_slide == NULL || img->current_slide->points == NULL)
		return;

	/* Get selected point */
	point = g_list_nth_data( img->current_slide->points,
							 img->current_slide->cur_point );

	/* Update data */
	*point = img->current_point;
	point->time = gtk_spin_button_get_value_as_int(	GTK_SPIN_BUTTON( img->ken_duration ) );
	
	/* Update display */
	img_update_stop_display( img, FALSE );

	/* Update total slideshow duration */
	full = img_calc_slide_duration_points(img->current_slide->points,
										img->current_slide->no_points);

	img->current_slide->duration = full;
	img_set_total_slideshow_duration(img);

	/* Sync timings */
	img_sync_timings( img->current_slide, img );

	img_taint_project(img);
}

void
img_delete_stop_point( GtkButton         *button,
					   img_window_struct *img )
{
	GList *node;
	gint  full;

	if( img->current_slide == NULL )
		return;

	/* Get selected node and free it */
	node = g_list_nth( img->current_slide->points,
					   img->current_slide->cur_point );
	g_slice_free( ImgStopPoint, node->data );
	img->current_slide->points = 
			g_list_delete_link( img->current_slide->points, node );

	/* Update counters */
	img->current_slide->no_points--;
	img->current_slide->cur_point = MIN( img->current_slide->cur_point,
										 img->current_slide->no_points - 1 );

	/* Update display */
	img_update_stop_display( img, TRUE );

	/* Update total slideshow duration */
	full = img_calc_slide_duration_points(img->current_slide->points,
										img->current_slide->no_points);

	img->current_slide->duration = full;
	img_set_total_slideshow_duration(img);
	
	/* Sync timings */
	img_sync_timings( img->current_slide, img );
	
	img_taint_project(img);
}

void
img_update_stop_display( img_window_struct *img,
						 gboolean           update_pos )
{
	gchar        *string;
	gdouble      full;

	/* Disable/enable slide duration */
	gtk_widget_set_sensitive( img->duration,
							  img->current_slide->no_points == 0 );

	/* Set slide duration */
	full = img_calc_slide_duration_points( img->current_slide->points,
										   img->current_slide->no_points );

	if( ! full )
		full = img->current_slide->duration;
	
	g_signal_handlers_block_by_func( img->duration,
									 img_spinbutton_value_changed, img );
	
	gtk_spin_button_set_value( GTK_SPIN_BUTTON( img->duration ), full );
	
	g_signal_handlers_unblock_by_func( img->duration,
									 img_spinbutton_value_changed, img );
									 
	/* Set point count */
	string = g_strdup_printf( "%d", img->current_slide->no_points );
	gtk_label_set_text( GTK_LABEL( img->total_stop_points_label), string );
	g_free( string );

	if( img->current_slide->no_points )
	{
		ImgStopPoint *point;

		/* Set current point */
		string = g_strdup_printf( "%d", img->current_slide->cur_point + 1 );
		gtk_entry_set_text( GTK_ENTRY( img->ken_entry ), string );
		g_free( string );
		
		/* Set duration of this point */
		point = (ImgStopPoint *)g_list_nth_data( img->current_slide->points,
												 img->current_slide->cur_point );
		
		g_signal_handlers_block_by_func( img->ken_duration,
									 img_update_stop_point, img );
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( img->ken_duration ),
								   point->time );
		g_signal_handlers_unblock_by_func( img->ken_duration,
									 img_update_stop_point, img );
	
		/* Set zoom value */
		g_signal_handlers_block_by_func( img->ken_zoom,
									img_ken_burns_zoom_changed, img );
		gtk_range_set_value( GTK_RANGE( img->ken_zoom ), point->zoom );
		g_signal_handlers_unblock_by_func( img->ken_zoom,
									img_ken_burns_zoom_changed, img );
		
		img_update_zoom_variables(img);
		
		/* Do we need to refresh current stop point on screen */
		if( update_pos )
			img->current_point = *point;
	}
	else
	{
		/* If the slide has no ken burn effect use default one */
		ImgStopPoint point = { 1, 0, 0, 1.0 };

		gtk_entry_set_text( GTK_ENTRY( img->ken_entry ), "" );
		gtk_spin_button_set_value( GTK_SPIN_BUTTON( img->ken_duration ), 1 );
		g_signal_handlers_block_by_func( img->ken_zoom,
									img_ken_burns_zoom_changed, img );
		gtk_range_set_value( GTK_RANGE( img->ken_zoom ), 1.0 );
		g_signal_handlers_unblock_by_func( img->ken_zoom,
									img_ken_burns_zoom_changed, img );
		if( update_pos )
			img->current_point = point;
	}

	/* Force update on preview area */
	gtk_widget_queue_draw( img->image_area );
}

void
img_update_subtitles_widgets( img_window_struct *img )
{
	gchar       	*string;
	GdkRGBA     	color;
	gdouble     	*f_colors;
	GtkTextIter		start;
	GtkWidget		*tmp_image;
	GdkPixbuf		*pattern_pix;
	GtkIconTheme	*icon_theme;
	GdkAtom			format;

	if (img->current_slide->subtitle)
	{
		//~ if (strstr((const gchar*)img->current_slide->subtitle, "GTKTEXTBUFFERCONTENTS-0001"))
		//~ {
			//~ format = gtk_text_buffer_register_deserialize_tagset(img->slide_text_buffer, NULL);
			//~ gtk_text_buffer_get_start_iter(img->slide_text_buffer, &start);

			//~ gtk_text_buffer_deserialize(img->slide_text_buffer,
										//~ img->slide_text_buffer,
										//~ format,
										//~ &start,
										//~ img->current_slide->subtitle,
										//~ img->current_slide->subtitle_length,
										//~ NULL);
			//~ gtk_text_buffer_unregister_deserialize_format(img->slide_text_buffer, format); 
		//~ }
		//~ else
			//~ g_object_set( G_OBJECT( img->slide_text_buffer ), "text", img->current_slide->subtitle, NULL );
	}	

	/* Update text pattern */
	if (img->current_slide->pattern_filename)
		pattern_pix = gdk_pixbuf_new_from_file_at_scale( img->current_slide->pattern_filename, 32, 32, TRUE, NULL);
	else
	{
		icon_theme = gtk_icon_theme_get_default();
		pattern_pix = gtk_icon_theme_load_icon(icon_theme,"image", 20, 0, NULL);
	}
	tmp_image = gtk_image_new_from_pixbuf(pattern_pix);
	gtk_widget_show(tmp_image);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(img->pattern_image), tmp_image);
	g_object_unref(pattern_pix);

	//~ /* Update font button */
	//~ string = pango_font_description_to_string(img->current_slide->font_desc);	
	//~ gtk_font_chooser_set_font(GTK_FONT_CHOOSER(img->sub_font), string);
	//~ g_free(string);

	//~ /* Update color button */
	//~ f_colors = img->current_slide->font_color;
	//~ color.red   = f_colors[0];
	//~ color.green = f_colors[1];
	//~ color.blue  = f_colors[2];
	//~ color.alpha = f_colors[3];
	//~ gtk_color_chooser_set_rgba( GTK_COLOR_CHOOSER( img->subtitle_font_color ), &color ); 
	
    //~ /* Update border color button */
    //~ f_colors = img->current_slide->font_brdr_color;
    //~ color.red   = f_colors[0];
    //~ color.green = f_colors[1];
    //~ color.blue  = f_colors[2];
    //~ color.alpha = f_colors[3];
    //~ gtk_color_chooser_set_rgba( GTK_COLOR_CHOOSER( img->sub_brdr_color ), &color ); 
                            
    //~ /* Update background color button */
    //~ f_colors = img->current_slide->font_bg_color;
    //~ color.red   = f_colors[0];
    //~ color.green = f_colors[1];
    //~ color.blue  = f_colors[2];
    //~ color.alpha = f_colors[3];
    //~ gtk_color_chooser_set_rgba( GTK_COLOR_CHOOSER( img->sub_font_bg_color ), &color ); 
                            
   /* Update animation */
	gtk_combo_box_set_active( GTK_COMBO_BOX( img->sub_anim ),
							  img->current_slide->anim_id );

	/* Update duration */
	gtk_spin_button_set_value( GTK_SPIN_BUTTON( img->sub_anim_duration ),
							  img->current_slide->anim_duration );
 								   
}

void
img_goto_prev_point( GtkButton         *button,
					 img_window_struct *img )
{
	if( img->current_slide && img->current_slide->no_points )
	{
		img->current_slide->cur_point =
				CLAMP( img->current_slide->cur_point - 1,
					   0, img->current_slide->no_points - 1 );

		img_update_stop_display( img, TRUE );
	}
}

void
img_goto_next_point( GtkButton         *button,
					 img_window_struct *img )
{
	if( img->current_slide && img->current_slide->no_points )
	{
		img->current_slide->cur_point =
				CLAMP( img->current_slide->cur_point + 1,
					   0, img->current_slide->no_points - 1 );

		img_update_stop_display( img, TRUE );
	}
}

void
img_goto_point ( GtkEntry          *entry,
				 img_window_struct *img )
{
	const gchar *string;
	gint         number;

	string = gtk_entry_get_text( entry );
	number = (gint)strtol( string, NULL, 10 );

	if( img->current_slide && img->current_slide->no_points )
	{
		img->current_slide->cur_point =
				CLAMP( number - 1, 0, img->current_slide->no_points - 1 );

		img_update_stop_display( img, TRUE );
	}
}


void
img_calc_current_ken_point( ImgStopPoint *res,
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
	int       w, h, g, f; /* Width, height, gutter, flags */
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
	g_key_file_set_integer( kf, group, "paned",  g );
	g_key_file_set_double(  kf, group, "zoom_p",  img->image_area_zoom );
	g_key_file_set_boolean( kf, group, "max",     max );
	g_key_file_set_integer( kf, group, "preview", img->preview_fps );
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
	gint      w, h, g, m; /* Width, height, gutter, mode */
	gint	  i;
	gboolean  max;

	rc_file = g_build_filename( g_get_home_dir(), ".config",
								"imagination", "imaginationrc", NULL );
	if( ! g_file_test( rc_file, G_FILE_TEST_EXISTS ) )
		return( FALSE );

	kf = g_key_file_new();
	g_key_file_load_from_file( kf, rc_file, G_KEY_FILE_NONE, NULL );

	w                    = g_key_file_get_integer( kf, group, "width",   NULL );
	h                    = g_key_file_get_integer( kf, group, "height",  NULL );
	g                    = g_key_file_get_integer( kf, group, "paned",  NULL );
	m                    = g_key_file_get_integer( kf, group, "mode",    NULL );
	img->image_area_zoom = g_key_file_get_double(  kf, group, "zoom_p",  NULL );
	max                  = g_key_file_get_boolean( kf, group, "max",     NULL );
	recent_files		 = g_key_file_get_string(  kf, group, "recent_files", NULL);

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
	/* New addition to environment settings */
	img->preview_fps     = g_key_file_get_integer( kf, group, "preview", NULL );
	if( ! img->preview_fps )
		img->preview_fps = 25;

	g_key_file_free( kf );

	/* Update window size and gutter position */
	gtk_window_set_default_size( GTK_WINDOW( img->imagination_window ), w, h );
	
	g_signal_handlers_block_by_func(img->vpaned, img_change_image_area_size, img);  
	gtk_paned_set_position( GTK_PANED( img->vpaned ), g );
	g_signal_handlers_unblock_by_func(img->vpaned, img_change_image_area_size, img);
	
	if( max )
		gtk_window_maximize( GTK_WINDOW( img->imagination_window ) );

	gtk_widget_set_size_request( img->image_area, img->video_size[0] * img->image_area_zoom,  img->video_size[1] * img->image_area_zoom );

	return TRUE;
}

void img_set_window_default_settings( img_window_struct *img )
{
	img->image_area_zoom = 1.0;
	img->preview_fps = 25;

	/* Update window size and gutter position */
	gtk_window_set_default_size( GTK_WINDOW( img->imagination_window ), 800, 600 );

}

static void img_reset_rotation_flip( slide_struct *slide) {
    gchar *filename = g_strdup( slide->o_filename );
    img_slide_set_p_filename(slide, filename);
    slide->angle = 0;
    slide->flipped = FALSE;
}

void
img_rotate_flip_slide( slide_struct   *slide,
				  ImgAngle        angle,
				  gboolean        flipped)
{
	/* If this slide is gradient, do nothing */
	if( ! slide->o_filename )
		return;

	/* If the angle is ANGLE_0 and the image is not flipped, then simply
	 * copy original filename into rotated filename. */
	if( (!angle) && (!flipped) )
	{
	    img_reset_rotation_flip(slide);
	    return;
	}
	GdkPixbuf *image;
	gint       handle;
	gboolean   ok;
	GError    *error = NULL;

	// load file
	image = gdk_pixbuf_new_from_file( slide->o_filename, &error );
	if (!image) {
		g_message( "%s.", error->message );
		g_error_free( error );
		img_reset_rotation_flip(slide);
		return;
	}

	GdkPixbuf *processed = NULL;

	// do the rotation, flipping
	if (angle) {
	    processed = gdk_pixbuf_rotate_simple( image, angle * 90 );
	    g_object_unref(image);
	    image = processed;
	}
	if (flipped) {
	    processed = gdk_pixbuf_flip(image, TRUE);
	    g_object_unref(image);
	}

	// save result
	gchar *filename;
	handle = g_file_open_tmp( "img-XXXXXX.jpg", &filename, NULL );
	close( handle );
	ok = gdk_pixbuf_save( processed, filename, "jpeg", &error, NULL );
	g_object_unref( processed );
	if( !ok )
	{
		g_message( "%s.", error->message );
		g_error_free( error );
		g_free( filename );
		img_reset_rotation_flip(slide);
		return;
	}
	/* Delete any temporary image that is present from previous rotation */
	img_slide_set_p_filename(slide, filename);
	slide->angle = angle;
	slide->flipped = flipped;
}

void img_align_text_horizontally_vertically(GtkMenuItem *item, img_window_struct *img)
{
	cairo_t	*cr;
	GdkWindow *window;
	GdkDrawingContext *drawingContext;
	cairo_region_t *cairoRegion;

	gboolean centerX = FALSE, centerY = FALSE;

	cairoRegion = cairo_region_create();
	window = gtk_widget_get_window(img->image_area);	
	drawingContext = gdk_window_begin_draw_frame(window, cairoRegion);
    cr = gdk_drawing_context_get_cairo_context(drawingContext);
	
	//~ if (GTK_WIDGET(item) == img->x_justify)
	//~ {
		//~ centerX = TRUE;
		//~ centerY = FALSE;
	//~ }
	//~ else if (GTK_WIDGET(item) == img->y_justify)
	//~ {
		//~ centerX = FALSE;
		//~ centerY = TRUE;
	//~ }
	//~ else
	//~ {	
		//~ img->current_slide->subtitle_angle = 0;
		//~ gtk_range_set_value( GTK_RANGE(img->sub_angle), 0);
	//~ }

	img_render_subtitle( img,
								 cr,
								 img->image_area_zoom,
								 img->current_slide->posX,
								 img->current_slide->posY,
								 img->current_slide->subtitle_angle,
								 img->current_slide->alignment,
								 img->current_point.zoom,
								 img->current_point.offx,
								 img->current_point.offy,
								 centerX,
								 centerY,
								 1.0 );
	
	gtk_widget_queue_draw(img->image_area);

	gdk_window_end_draw_frame(window, drawingContext);
	cairo_region_destroy(cairoRegion);
	
	img_taint_project(img);
}

void
img_pattern_clicked(GtkMenuItem *item,
					img_window_struct *img)
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
		if ( (img->current_slide)->pattern_filename)
			g_free( ( img->current_slide )->pattern_filename);

		(img->current_slide)->pattern_filename = g_strdup(filename);
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

void img_spinbutton_value_changed (GtkSpinButton *spinbutton, img_window_struct *img)
{
	gdouble duration = 0;
	GList *selected, *bak;
	GtkTreeIter iter;
	GtkTreeModel *model;
	slide_struct *info_slide;

	model = GTK_TREE_MODEL( img->thumbnail_model );
	selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(img->thumbnail_iconview));
	if (selected == NULL)
		return;

	duration = gtk_spin_button_get_value(spinbutton);
	bak = selected;
	while (selected)
	{
		gtk_tree_model_get_iter(model, &iter,selected->data);
		gtk_tree_model_get(model, &iter,1,&info_slide,-1);
		img_set_slide_still_info( info_slide, duration, img );
		selected = selected->next;
		img_taint_project(img);
	}

	GList *node19;
	for(node19 = bak;node19 != NULL;node19 = node19->next) {
		gtk_tree_path_free(node19->data);
	}
	g_list_free(bak);

	/* Sync timings */
	img_sync_timings( img->current_slide, img );
	img_taint_project(img);
}

void img_fadeout_duration_changed (GtkSpinButton *spinbutton, img_window_struct *img)
{
	//img->audio_fadeout = gtk_spin_button_get_value(spinbutton);
	img_taint_project(img);
}

void img_set_slide_text_align(GtkButton *button, img_window_struct *img)
{
	gint alignment = 0;

	if (img->current_slide == NULL)
		return;

	if (GTK_WIDGET(button) == img->left_justify)
		alignment = PANGO_ALIGN_LEFT;
	else if (GTK_WIDGET(button) == img->fill_justify)
		alignment = PANGO_ALIGN_CENTER;
	else if (GTK_WIDGET(button) == img->right_justify)
		alignment = PANGO_ALIGN_RIGHT;

	img->current_slide->alignment = alignment;
	gtk_widget_queue_draw(img->image_area);
	img_taint_project(img);
}

void img_subtitle_style_changed(GtkButton *button, img_window_struct *img)
{
	gboolean 	selection;
	GtkTextIter start, end;
	GtkTextTag 	*tag;
	gchar *string;

	img_taint_project(img);

	/* Which button did the user press? */
	if (GTK_WIDGET(button) == img->bold_style)
		string = "bold";
	else if (GTK_WIDGET(button) == img->italic_style)
		string = "italic";
	else if (GTK_WIDGET(button) == img->underline_style)
		string = "underline";
	//~ else {
		//~ gtk_text_buffer_remove_all_tags(img->slide_text_buffer, &start, &end);
		//~ img_store_rtf_buffer_content(img);

		//~ gtk_widget_queue_draw(img->image_area);
		//~ return;
	//~ }
	//~ tag = gtk_text_tag_table_lookup(img->tag_table, string);

	//~ /* Was the style already applied? */
	//~ if (gtk_text_iter_starts_tag( &start, tag))
		//~ gtk_text_buffer_remove_tag(img->slide_text_buffer, tag, &start, &end);
	//~ else
		//~ gtk_text_buffer_apply_tag(img->slide_text_buffer, tag, &start, &end);

	gtk_widget_queue_draw(img->image_area);
}

void img_flip_horizontally(GtkMenuItem *item, img_window_struct *img)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GList        *selected,
				 *bak;
	GdkPixbuf    *thumb;
	slide_struct *info_slide;

	/* Obtain the selected slideshow filename */
	model = GTK_TREE_MODEL( img->thumbnail_model );
	selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW( img->thumbnail_iconview ) );

	if( selected == NULL)
		return;
	img_taint_project(img);

	bak = selected;
	while (selected)
	{
		gtk_tree_model_get_iter( model, &iter, selected->data );
		gtk_tree_model_get( model, &iter, 1, &info_slide, -1 );

		/* Avoid seg-fault when dealing with text/gradient slides */
		if (info_slide->o_filename != NULL)
		{
			img_rotate_flip_slide(info_slide, info_slide->angle, !(info_slide->flipped));

			/* Display the flipped image in thumbnails iconview */
			img_scale_image( info_slide->p_filename, img->video_ratio, 88, 0,
							 img->distort_images, img->background_color,
							 &thumb, NULL );
			gtk_list_store_set( img->thumbnail_model, &iter, 0, thumb, -1 );
		}
		selected = selected->next;
	}
	GList *node20;
	for(node20 = bak;node20 != NULL;node20 = node20->next) {
		gtk_tree_path_free(node20->data);
	}
	g_list_free(bak);

	/* If no slide is selected currently, simply return */
	if( ! img->current_slide )
		return;

	if (info_slide->o_filename != NULL)
	{
		cairo_surface_destroy( img->current_image );
		img_scale_image( img->current_slide->p_filename, img->video_ratio,
							 0, img->video_size[1], img->distort_images,
							 img->background_color, NULL, &img->current_image );
		
		gtk_widget_queue_draw( img->image_area );
	}
}

void img_open_recent_slideshow(GtkWidget *menu, img_window_struct *img)
{
	const gchar *filename;
	
	filename = gtk_menu_item_get_label(GTK_MENU_ITEM(menu));
	if (img_can_discard_unsaved_project(img))
		img_load_slideshow(img, menu, filename);
}

void img_add_any_media_callback( GtkButton *button,  img_window_struct *img )
{
	GSList *bak;
	GFile *file;
	GFileInfo *file_info;
	const gchar *content_type;
	gchar *mime_type;

	GSList *list = img_import_slides_file_chooser(img);
	if (list == NULL)
		return;

	bak = list;
	while (list)
	{
		//Determine the mime type
		file = g_file_new_for_path (list->data);
		file_info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
		content_type = g_file_info_get_content_type (file_info);
		mime_type = g_content_type_get_mime_type (content_type);
		if (strstr(mime_type, "image"))
			img_add_thumbnail_widget_area(0, list->data, img);
		else if (strstr(mime_type, "audio"))
			img_add_thumbnail_widget_area(1, list->data, img);
		else if (strstr(mime_type, "video"))
			g_print("Video!");
		else
		{
			gchar *string = g_strconcat( _("Can't recognize file type of media\n"), list->data, NULL);
			img_message(img, string);
			g_free(string);
		}
		g_free(mime_type);
		g_object_unref(file);
		list = list->next;
	}
	g_slist_free(bak);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(img->side_notebook), 0);
}

void img_add_thumbnail_widget_area(gint type, gchar *full_path_filename, img_window_struct *img)
{
	/* type:
	 * 0 image
	 * 1 audio
	 * 2 video 
	 */
	GdkPixbuf *pb;
	GtkTreeIter iter;
	gchar *filename;
	
	if (img_find_media_in_list(img, full_path_filename))
		return;

	filename = g_path_get_basename(full_path_filename);
	
	gtk_list_store_append (img->media_model, &iter);
	img->media_nr++;
	img_taint_project(img);
	switch (type)
	{
		case 0:
		//Load and scale the pixbuf and add it to the iconview
		pb = gdk_pixbuf_new_from_file_at_scale(full_path_filename, 120, 60, TRUE, NULL);
		gtk_list_store_set (img->media_model, &iter, 0, pb, 1, filename, 2 , full_path_filename, 3, 0 , -1);
		g_object_unref(pb);
		break;
		
		case 1:
		pb = gtk_icon_theme_load_icon(img->icon_theme, "audio-x-generic", 46, 0, NULL);
		gtk_list_store_set (img->media_model, &iter, 0, pb, 1, filename, 2 , full_path_filename, 3, 1 , -1);
		break;
	}
	g_free(filename);
}

