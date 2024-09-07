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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <assert.h>

#include "main-window.h"
#include "callbacks.h"
#include "export.h"
#include "subtitles.h"
#include "imgcellrendererpixbuf.h"

static const GtkTargetEntry drop_targets[] =
{
	{ "text/uri-list",0,0 },
};

/* ****************************************************************************
 * Local function declarations
 * ************************************************************************* */
static void img_toggle_button_callback(GtkToggleButton *button, GtkWidget *notebook);
static void img_random_button_clicked(GtkButton *, img_window_struct *);
static void img_font_color_clicked(GtkButton *, img_window_struct *);
static GdkPixbuf *img_set_random_transition(img_window_struct *, slide_struct *);
static void img_transition_speed_changed (GtkRange *,  img_window_struct *);
static void img_report_slides_transitions(img_window_struct *);
static void img_clear_audio_files(GtkButton *, img_window_struct *);
static gboolean img_sub_textview_focus_in (GtkWidget *, GdkEventFocus *, img_window_struct *);
static gboolean img_sub_textview_focus_out(GtkWidget *, GdkEventFocus *, img_window_struct *);
static void img_quit_menu(GtkMenuItem *, img_window_struct *);
static void img_select_all_thumbnails(GtkMenuItem *, img_window_struct *);
static void img_unselect_all_thumbnails(GtkMenuItem *, img_window_struct *);
static void img_goto_line_entry_activate(GtkWidget *, img_window_struct *);
static gint img_sort_none_before_other(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, gpointer);
static void img_check_numeric_entry (GtkEditable *entry, gchar *text, gint lenght, gint *position, gpointer data);
static void img_show_uri(GtkMenuItem *, img_window_struct *);
static void img_select_slide_from_slide_report_dialog(GtkButton *, img_window_struct *);
static void img_show_slides_report_dialog(GtkMenuItem *, img_window_struct *);

GtkWidget *button1,  *button2, *button3, *button4, *begining, *end;

static void img_toggle_button_callback(GtkToggleButton *button, GtkWidget *notebook)
{
    GtkWidget *buttons[] = {button2, button3};
    for (int i = 0; i < 2; i++)
    {
        if (buttons[i] != GTK_WIDGET(button))
        {
			g_signal_handlers_block_by_func( buttons[i],  img_toggle_button_callback, notebook );
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[i]), FALSE);
			g_signal_handlers_unblock_by_func( buttons[i],  img_toggle_button_callback, notebook );
		}
        else
        {
			gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), i);
			g_signal_handlers_block_by_func( buttons[i],  img_toggle_button_callback, notebook );
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[i]), TRUE);
			g_signal_handlers_unblock_by_func( buttons[i],  img_toggle_button_callback, notebook );
        }
	}
}

static gboolean img_iconview_selection_button_press( GtkWidget         *widget,
									 GdkEventButton    *button,
									 img_window_struct *img );

static gboolean img_scroll_thumb( GtkWidget         *widget,
				  GdkEventScroll    *scroll,
				  img_window_struct *img );

static gboolean img_subtitle_update( img_window_struct *img );

static GtkWidget *img_create_subtitle_animation_combo( void );

static gint img_sort_report_transitions( gconstpointer a,
							 gconstpointer b );

static void img_toggle_frame_rate( GtkCheckMenuItem  *item,
					   img_window_struct *img );

static GtkWidget *img_load_icon_from_theme(GtkIconTheme *icon_theme, gchar *name, gint size);

/* ****************************************************************************
 * Function definitions
 * ************************************************************************* */
img_window_struct *img_create_window (void)
{
	img_window_struct *img_struct = NULL;
	GtkWidget *main_vertical_box;
	GtkWidget *menuitem1;
	GtkWidget *menuitem2;
	GtkWidget *menu1;
	GtkWidget *imagemenuitem1;
	GtkWidget *imagemenuitem5;
	GtkWidget *separatormenuitem1;
	GtkWidget *slide_menu;
	GtkWidget *deselect_all_menu;
	GtkWidget *menuitem3;
	GtkWidget *tmp_image;
	GtkWidget *menu3;
	GtkWidget *about;
	GtkWidget *contents;
	GtkWidget *add_slide;
	GtkWidget *label_of;
	GtkWidget *sidebar_notebook;
	GtkWidget *scrollable_window;
	GtkWidget *vbox_frames;
	GtkWidget *frame1, *frame2, *frame4, *frame_label;
	GtkWidget *transition_label;
	GtkWidget *vbox_info_slide, *vbox_slide_motion, *vbox_slide_caption, *vbox_slide_position;
	GtkWidget *grid;
	GtkWidget *duration_label;
	GtkWidget *total_time;
	GtkWidget *hbox_stop_points, *stop_points_label;
	GtkWidget *hbox_time_offset, *time_offset_label;
	GtkWidget *hbox_textview, *text_animation_hbox;
	GtkWidget *image_buttons;
	GtkCellRenderer *pixbuf_cell;
	GdkPixbuf *icon;
	GtkWidget *eventbox;
	GtkWidget *a_label;
	GtkWidget *a_hbox;
	GtkWidget *right_vertical_box;
	GtkWidget *preview_menu;
	GtkWidget *import_menu;
	GtkWidget *properties_menu;
	GtkWidget *export_menu;
	GdkPixbuf *pixbuf;
	GtkWidget *flip_horizontally_menu;
	GtkWidget *rotate_left_menu;
	GtkWidget *rotate_right_menu;
	AtkObject *atk;

	img_struct = g_new0(img_window_struct, 1);

	/* Set some default values */
	img_struct->background_color[0] = 0;
	img_struct->background_color[1] = 0;
	img_struct->background_color[2] = 0;
	img_struct->slides_nr = 0;
	img_struct->distort_images = TRUE;

	img_struct->maxoffx = 0;
	img_struct->maxoffy = 0;
	img_struct->current_point.offx = 0;
	img_struct->current_point.offy = 0;
	img_struct->current_point.zoom = 1;

	img_struct->video_size[0] = 1280;
	img_struct->video_size[1] = 720;
	img_struct->video_ratio = (gdouble)1280 / 720;
	img_struct->export_fps = 30;
    //img_struct->audio_fadeout = 5;

	img_struct->final_transition.duration = 0;
	img_struct->final_transition.render = NULL;

	/* GUI STUFF */
	img_struct->icon_theme = gtk_icon_theme_get_default();
	icon = gtk_icon_theme_load_icon(img_struct->icon_theme, "imagination", 24, 0, NULL);

	img_struct->imagination_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_icon (GTK_WINDOW(img_struct->imagination_window),icon);
	gtk_window_set_position (GTK_WINDOW(img_struct->imagination_window),GTK_WIN_POS_CENTER);
	img_refresh_window_title(img_struct);
	g_signal_connect (G_OBJECT (img_struct->imagination_window),"delete-event",G_CALLBACK (img_quit_application),img_struct);
	g_signal_connect (G_OBJECT (img_struct->imagination_window), "destroy", G_CALLBACK (gtk_main_quit), NULL );
	g_signal_connect (G_OBJECT (img_struct->imagination_window), "key_press_event", G_CALLBACK(img_key_pressed), img_struct);
	img_struct->accel_group = gtk_accel_group_new();

	main_vertical_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (main_vertical_box);
	gtk_container_add (GTK_CONTAINER (img_struct->imagination_window), main_vertical_box);
    
	/* Create the menu items */
	img_struct->menubar = gtk_menu_bar_new ();
	gtk_box_pack_start (GTK_BOX (main_vertical_box), img_struct->menubar, FALSE, TRUE, 0);

	menuitem1 = gtk_menu_item_new_with_mnemonic (_("_Slideshow"));
	gtk_container_add (GTK_CONTAINER (img_struct->menubar), menuitem1);

	menu1 = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem1), menu1);

	imagemenuitem1 = gtk_menu_item_new_with_mnemonic(_("_New"));
	gtk_container_add (GTK_CONTAINER (menu1), imagemenuitem1);
	gtk_widget_add_accelerator (imagemenuitem1,"activate", img_struct->accel_group,GDK_KEY_n,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (imagemenuitem1),"activate",G_CALLBACK (img_new_slideshow),img_struct);

	img_struct->open_menu = gtk_menu_item_new_with_mnemonic (_("_Open"));
	gtk_container_add (GTK_CONTAINER (menu1), img_struct->open_menu);
	gtk_widget_add_accelerator (img_struct->open_menu,"activate", img_struct->accel_group,GDK_KEY_o,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (img_struct->open_menu),"activate",G_CALLBACK (img_choose_slideshow_filename),img_struct);

	img_struct->open_recent = gtk_menu_item_new_with_mnemonic (_("Open recent"));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu1 ), img_struct->open_recent);
	
	img_struct->recent_slideshows = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (img_struct->open_recent), img_struct->recent_slideshows);
	
    img_struct->import_project_menu = gtk_menu_item_new_with_mnemonic (_("Import slideshow"));
    gtk_container_add (GTK_CONTAINER (menu1),img_struct->import_project_menu);
    g_signal_connect (G_OBJECT (img_struct->import_project_menu),"activate",G_CALLBACK (img_choose_slideshow_filename),img_struct);

	img_struct->save_menu = gtk_menu_item_new_with_mnemonic (_("_Save"));
	gtk_container_add (GTK_CONTAINER (menu1), img_struct->save_menu);
	gtk_widget_add_accelerator (img_struct->save_menu,"activate", img_struct->accel_group,GDK_KEY_s,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (img_struct->save_menu),"activate",G_CALLBACK (img_choose_slideshow_filename),img_struct);

	img_struct->save_as_menu = gtk_menu_item_new_with_mnemonic (_("Save _As"));
	gtk_container_add (GTK_CONTAINER (menu1), img_struct->save_as_menu);
	g_signal_connect (G_OBJECT (img_struct->save_as_menu),"activate",G_CALLBACK (img_choose_slideshow_filename),img_struct);

	img_struct->close_menu = gtk_menu_item_new_with_mnemonic (_("_Close"));
	gtk_container_add (GTK_CONTAINER (menu1), img_struct->close_menu);
	gtk_widget_add_accelerator (img_struct->close_menu,"activate", img_struct->accel_group,GDK_KEY_w,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (img_struct->close_menu),"activate",G_CALLBACK (img_close_slideshow),img_struct);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu1), separatormenuitem1);

	import_menu = gtk_menu_item_new_with_mnemonic (_("Import any med_ia"));
	gtk_container_add (GTK_CONTAINER (menu1),import_menu);
	gtk_widget_add_accelerator (import_menu,"activate",img_struct->accel_group,GDK_KEY_i,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (import_menu),"activate",G_CALLBACK (img_add_slides_thumbnails),img_struct);
	
	properties_menu = gtk_menu_item_new_with_mnemonic (_("_Properties"));
	gtk_container_add (GTK_CONTAINER (menu1), properties_menu);
	gtk_widget_add_accelerator (properties_menu,"activate",img_struct->accel_group,GDK_KEY_p,GDK_MOD1_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (properties_menu), "activate", G_CALLBACK (img_project_properties), img_struct);
	
	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu1), separatormenuitem1);

	preview_menu = gtk_menu_item_new_with_mnemonic (_("Preview"));
	gtk_container_add (GTK_CONTAINER (menu1), preview_menu);
	gtk_widget_add_accelerator (preview_menu, "activate",img_struct->accel_group, GDK_KEY_space, 0, GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (preview_menu),"activate",G_CALLBACK (img_start_stop_preview),img_struct);
	
	export_menu = gtk_menu_item_new_with_mnemonic (_("Ex_port"));
	gtk_container_add (GTK_CONTAINER (menu1), export_menu);
	gtk_widget_add_accelerator (export_menu, "activate",img_struct->accel_group,GDK_KEY_p,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (export_menu),"activate",G_CALLBACK (img_show_export_dialog),img_struct);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu1), separatormenuitem1);

	imagemenuitem5 = gtk_menu_item_new_with_mnemonic (_("_Quit"));
	gtk_container_add (GTK_CONTAINER (menu1), imagemenuitem5);
	gtk_widget_add_accelerator (imagemenuitem5, "activate",img_struct->accel_group,GDK_KEY_q,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (imagemenuitem5),"activate",G_CALLBACK (img_quit_menu),img_struct);

	/* Slide menu */
	menuitem2 = gtk_menu_item_new_with_mnemonic (_("_Slide"));
	gtk_container_add (GTK_CONTAINER (img_struct->menubar), menuitem2);

	slide_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem2), slide_menu);

	add_slide = gtk_menu_item_new_with_mnemonic (_("Add empt_y slide"));
	gtk_container_add (GTK_CONTAINER (slide_menu), add_slide);
	gtk_widget_add_accelerator( add_slide, "activate", img_struct->accel_group,	GDK_KEY_y, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE );
	g_signal_connect( G_OBJECT( add_slide ), "activate", G_CALLBACK( img_add_empty_slide ), img_struct );

	img_struct->edit_empty_slide = gtk_menu_item_new_with_mnemonic (_("Edit _empty slide"));
	gtk_container_add (GTK_CONTAINER (slide_menu), img_struct->edit_empty_slide);
	gtk_widget_set_sensitive(img_struct->edit_empty_slide, FALSE);
	gtk_widget_add_accelerator( img_struct->edit_empty_slide, "activate", img_struct->accel_group,	GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE );
	g_signal_connect( G_OBJECT( img_struct->edit_empty_slide ), "activate", G_CALLBACK( img_add_empty_slide ), img_struct );

	img_struct->remove_menu = gtk_menu_item_new_with_mnemonic (_("Dele_te"));
	gtk_container_add (GTK_CONTAINER (slide_menu), img_struct->remove_menu);
	gtk_widget_add_accelerator (img_struct->remove_menu,"activate",img_struct->accel_group, GDK_KEY_Delete,0,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (img_struct->remove_menu),"activate",G_CALLBACK (img_delete_selected_slides),img_struct);

	img_struct->report_menu = gtk_menu_item_new_with_mnemonic (_("Repor_t"));
	gtk_container_add (GTK_CONTAINER (slide_menu), img_struct->report_menu);
	gtk_widget_add_accelerator( img_struct->report_menu, "activate", img_struct->accel_group, GDK_KEY_t, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE );
	g_signal_connect (G_OBJECT (img_struct->report_menu),"activate",G_CALLBACK (img_show_slides_report_dialog),img_struct);

	flip_horizontally_menu = gtk_menu_item_new_with_mnemonic (_("_Flip horizontally"));
	gtk_container_add (GTK_CONTAINER (slide_menu),flip_horizontally_menu);
	gtk_widget_add_accelerator (flip_horizontally_menu,"activate",img_struct->accel_group, GDK_KEY_f,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect( G_OBJECT( flip_horizontally_menu ), "activate", G_CALLBACK( img_flip_horizontally), img_struct );

	rotate_left_menu = gtk_menu_item_new_with_mnemonic (_("Rotate co_unter-clockwise"));
	gtk_container_add (GTK_CONTAINER (slide_menu),rotate_left_menu);
	gtk_widget_add_accelerator (rotate_left_menu,"activate",img_struct->accel_group, GDK_KEY_u,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect( G_OBJECT( rotate_left_menu ), "activate", G_CALLBACK( img_rotate_slides_left), img_struct );

	rotate_right_menu = gtk_menu_item_new_with_mnemonic (_("_Rotate clockwise"));
	gtk_container_add (GTK_CONTAINER (slide_menu),rotate_right_menu);
	gtk_widget_add_accelerator (rotate_right_menu,"activate",img_struct->accel_group, GDK_KEY_r,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect( G_OBJECT( rotate_right_menu ), "activate", G_CALLBACK ( img_rotate_slides_right ), img_struct );

	img_struct->select_all_menu = gtk_menu_item_new_with_mnemonic(_("Select All"));
	gtk_container_add (GTK_CONTAINER (slide_menu),img_struct->select_all_menu);
	gtk_widget_add_accelerator (img_struct->select_all_menu,"activate",img_struct->accel_group,GDK_KEY_a,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (img_struct->select_all_menu),"activate",G_CALLBACK (img_select_all_thumbnails),img_struct);

	deselect_all_menu = gtk_menu_item_new_with_mnemonic (_("Un_select all"));
	gtk_container_add (GTK_CONTAINER (slide_menu),deselect_all_menu);
	gtk_widget_add_accelerator (deselect_all_menu,"activate",img_struct->accel_group,GDK_KEY_a,GDK_CONTROL_MASK | GDK_SHIFT_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (deselect_all_menu),"activate",G_CALLBACK (img_unselect_all_thumbnails),img_struct);

	/* Help menu */
	menuitem3 = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_container_add (GTK_CONTAINER (img_struct->menubar), menuitem3);
	menu3 = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem3), menu3);

	contents = gtk_menu_item_new_with_mnemonic (_("Contents"));
	gtk_container_add (GTK_CONTAINER (menu3),contents);
	gtk_widget_add_accelerator (contents,"activate",img_struct->accel_group,GDK_KEY_F1,0,GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (contents),"activate",G_CALLBACK (img_show_uri),img_struct);

	about = gtk_menu_item_new_with_mnemonic (_("About"));
	gtk_container_add (GTK_CONTAINER (menu3), about);
	gtk_widget_show_all (img_struct->menubar);
	g_signal_connect (G_OBJECT (about),"activate",G_CALLBACK (img_show_about_dialog),img_struct);

	//Create the sidebar with the buttons and the related widget area
	img_struct->main_horizontal_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (main_vertical_box), img_struct->main_horizontal_box, TRUE, TRUE, 0);
	
	img_struct->sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(img_struct->sidebar), 5);
    gtk_box_pack_start(GTK_BOX(img_struct->main_horizontal_box), img_struct->sidebar, FALSE, FALSE, 0);
	
	button1 = gtk_button_new();
	gtk_widget_set_tooltip_text(button1, _("Import any media"));
    GtkWidget *image1 = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(button1), image1);
    gtk_button_set_relief(GTK_BUTTON(button1), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), button1, FALSE, FALSE, 5);
	g_signal_connect(button1, "clicked", G_CALLBACK(img_add_any_media_callback), img_struct);	
	
	button2 = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(button2, _("Lists all the imported media"));
    image1 = gtk_image_new_from_icon_name("applications-multimedia", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(button2), image1);
    gtk_button_set_relief(GTK_BUTTON(button2), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), button2, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button2), TRUE);
       
    button3= gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(button3, _("Insert text"));
    image1 = gtk_image_new_from_icon_name("insert-text", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(button3), image1);
    gtk_button_set_relief(GTK_BUTTON(button3), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), button3, FALSE, FALSE, 5);

	button4 = gtk_button_new_with_label("\U0001F642");
	gtk_button_set_relief(GTK_BUTTON(button4), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), button4, FALSE, FALSE, 10);

	GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start(GTK_BOX(img_struct->main_horizontal_box), separator, FALSE, FALSE, 0);

    //Create the hidden notebook widget
	sidebar_notebook = gtk_notebook_new ();
	g_object_set (G_OBJECT (sidebar_notebook),"show-border", FALSE,"show-tabs", FALSE,"enable-popup",FALSE,NULL);
	gtk_widget_set_can_focus (sidebar_notebook, TRUE);
	gtk_box_pack_start (GTK_BOX(img_struct->main_horizontal_box), sidebar_notebook, FALSE, FALSE, 0);

	g_signal_connect(button2, "toggled", G_CALLBACK(img_toggle_button_callback), sidebar_notebook);
	g_signal_connect(button3, "toggled", G_CALLBACK(img_toggle_button_callback), sidebar_notebook);

	//Create the list media widget
	GtkWidget *media_iconview, *vbox, *hbox;

	img_struct->media_model = gtk_list_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);
	media_iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL(img_struct->media_model));
	g_object_unref (img_struct->media_model);

	scrollable_window = gtk_scrolled_window_new(NULL, NULL);
	g_object_set (G_OBJECT (scrollable_window),"hscrollbar-policy",GTK_POLICY_NEVER,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);
	gtk_container_add (GTK_CONTAINER (scrollable_window), media_iconview);

	gtk_icon_view_set_item_orientation (GTK_ICON_VIEW (media_iconview), GTK_ORIENTATION_VERTICAL);
	gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (media_iconview), 0);
	gtk_icon_view_set_columns(GTK_ICON_VIEW (media_iconview), 2);
	
	gtk_container_add (GTK_CONTAINER (sidebar_notebook), scrollable_window);
	gtk_icon_view_enable_model_drag_dest(GTK_ICON_VIEW(media_iconview), drop_targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE |  GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect( G_OBJECT( media_iconview ), "drag-data-received", G_CALLBACK( img_on_drag_data_received), img_struct );
	//gtk_icon_view_set_reorderable(GTK_ICON_VIEW (media_iconview),TRUE);
	GtkCellRenderer *cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT(media_iconview), cell, TRUE );
	gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT(media_iconview), cell,  "text", 1, NULL);
	g_object_set(cell, "ellipsize", PANGO_ELLIPSIZE_END, "xalign", 0.5, NULL);
	
	/*Create the text widget section */
	vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (sidebar_notebook), vbox);
	
	//1st row font selector and font color widgets
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 10);

	img_struct->sub_font = gtk_font_button_new();
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->sub_font, FALSE, FALSE, 5);
	
	img_struct->subtitle_font_color = gtk_button_new();
	gtk_widget_set_name(img_struct->subtitle_font_color , "font_color_button");
	gtk_widget_set_valign(img_struct->subtitle_font_color, GTK_ALIGN_CENTER);
	g_signal_connect( G_OBJECT( img_struct->subtitle_font_color ), "clicked", G_CALLBACK( img_font_color_clicked ), img_struct );
	gtk_box_pack_start( GTK_BOX(hbox), img_struct->subtitle_font_color, FALSE, FALSE, 5);
	gtk_widget_set_tooltip_text(img_struct->subtitle_font_color, _("Font color"));
	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, "#font_color_button {min-width:12px;min-height:12px;border-radius: 50%;background: white;} #font_button {padding: 0px;} button {padding: 5px}",  -1, NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    //2nd row style buttons
    hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

	img_struct->bold_style = gtk_button_new_with_label("<span size='large'><b>B</b></span>");
	gtk_widget_set_name(img_struct->bold_style , "font_button");
	GtkWidget *label = gtk_bin_get_child(GTK_BIN(img_struct->bold_style));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	g_signal_connect( G_OBJECT( img_struct->bold_style ), "clicked",	  G_CALLBACK( img_subtitle_style_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->bold_style, FALSE, FALSE, 5);

	img_struct->italic_style = gtk_button_new_with_label("<span size='x-large'><i>I</i></span>");
	label = gtk_bin_get_child(GTK_BIN(img_struct->italic_style));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	g_signal_connect( G_OBJECT( img_struct->italic_style ), "clicked",	  G_CALLBACK( img_subtitle_style_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->italic_style, FALSE, FALSE, 5);
	
	img_struct->underline_style = gtk_button_new_with_label("<span size='x-large'><u>U</u></span>");
	label = gtk_bin_get_child(GTK_BIN(img_struct->underline_style));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	g_signal_connect( G_OBJECT( img_struct->underline_style ), "clicked", G_CALLBACK( img_subtitle_style_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->underline_style, FALSE, FALSE, 5);

	//3rd row alignment buttons
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	
	img_struct->left_justify = gtk_button_new();
	gtk_widget_set_name(img_struct->left_justify , "font_button");
	g_signal_connect( G_OBJECT( img_struct->left_justify ), "clicked",		  G_CALLBACK( img_set_slide_text_align ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->left_justify, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("format-justify-left", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->left_justify), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->left_justify, _("Align left"));

	img_struct->fill_justify = gtk_button_new();
	gtk_widget_set_name(img_struct->fill_justify , "font_button");
	g_signal_connect( G_OBJECT( img_struct->fill_justify ), "clicked",			  G_CALLBACK( img_set_slide_text_align ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->fill_justify, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("format-justify-center", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->fill_justify), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->fill_justify, _("Align center"));

	img_struct->right_justify = gtk_button_new();
	gtk_widget_set_name(img_struct->right_justify , "font_button");
	g_signal_connect( G_OBJECT( img_struct->right_justify ), "clicked",		  G_CALLBACK( img_set_slide_text_align ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->right_justify, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("format-justify-right", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->right_justify), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->right_justify, _("Align right"));

	/* Create the image area and the other widgets */
	right_vertical_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX(img_struct->main_horizontal_box) , right_vertical_box, TRUE, TRUE, 10);

	img_struct->image_area = gtk_drawing_area_new();
	//gtk_widget_set_size_request(img_struct->image_area, 500, 150);
	gtk_box_pack_start(GTK_BOX(right_vertical_box), img_struct->image_area, TRUE, TRUE, 10);
	gtk_widget_add_events( img_struct->image_area, GDK_POINTER_MOTION_MASK  | GDK_BUTTON_PRESS_MASK  | GDK_BUTTON_RELEASE_MASK  | GDK_SCROLL_MASK );
	g_signal_connect( G_OBJECT( img_struct->image_area ), "draw",	  						G_CALLBACK( img_on_draw_event ), img_struct );
	g_signal_connect( G_OBJECT( img_struct->image_area ), "button-press-event",	G_CALLBACK( img_image_area_button_press ), img_struct );
	g_signal_connect( G_OBJECT( img_struct->image_area ), "button-release-event",G_CALLBACK( img_image_area_button_release ), img_struct );
	g_signal_connect( G_OBJECT( img_struct->image_area ), "motion-notify-event",  G_CALLBACK( img_image_area_motion ), img_struct );
	g_signal_connect( G_OBJECT( img_struct->image_area ), "scroll-event",				G_CALLBACK( img_image_area_scroll ), img_struct);

	//Preview buttons
	a_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(right_vertical_box), a_hbox, FALSE, FALSE, 10);
	gtk_widget_set_halign(GTK_WIDGET(a_hbox), GTK_ALIGN_CENTER);

	begining = gtk_button_new();
	gtk_box_pack_start(GTK_BOX(a_hbox), begining, FALSE, FALSE, 10);
	image_buttons = gtk_image_new_from_icon_name ("go-first", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_button_set_image(GTK_BUTTON(begining), image_buttons);
	
	img_struct->preview_button = gtk_button_new();
	gtk_box_pack_start(GTK_BOX(a_hbox), img_struct->preview_button, FALSE, FALSE, 10);
	image_buttons = gtk_image_new_from_icon_name ("media-playback-start", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_button_set_image(GTK_BUTTON(img_struct->preview_button), image_buttons);
	g_signal_connect (G_OBJECT (img_struct->preview_button),"clicked",G_CALLBACK (img_start_stop_preview),img_struct);

	end = gtk_button_new();
	gtk_box_pack_start(GTK_BOX(a_hbox), end, FALSE, FALSE, 10);
	image_buttons = gtk_image_new_from_icon_name ("go-last", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_button_set_image(GTK_BUTTON(end), image_buttons);

	/* Slide frame */
	frame1 = gtk_frame_new (NULL);
	//gtk_box_pack_start (GTK_BOX (vbox_frames), frame1, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_IN);

	frame_label = gtk_label_new (_("<b>Slide Settings</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame1), frame_label);
	gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);

	vbox_info_slide = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(frame1), vbox_info_slide);
	gtk_widget_set_halign(GTK_WIDGET(vbox_info_slide), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top(GTK_WIDGET(vbox_info_slide), 10);
	gtk_widget_set_margin_bottom(GTK_WIDGET(vbox_info_slide), 5);
	gtk_widget_set_margin_start(GTK_WIDGET(vbox_info_slide), 5);
	gtk_widget_set_margin_end(GTK_WIDGET(vbox_info_slide), 5);

	/* Transition types label */
	transition_label = gtk_label_new (_("Transition Type:"));
	//gtk_box_pack_start (GTK_BOX (vbox_info_slide), transition_label, FALSE, FALSE, 0);
	gtk_label_set_xalign(GTK_LABEL(transition_label), 0);
	gtk_label_set_yalign(GTK_LABEL(transition_label), -1);

	/* Slide selected, slide resolution, slide type and slide total duration */
	grid = gtk_grid_new();
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
	gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 5);
	//gtk_box_pack_start (GTK_BOX (vbox_info_slide), grid, TRUE, FALSE, 0);
	
	/* Transition type */
	img_struct->transition_type = _gtk_combo_box_new_text( TRUE );
	atk = gtk_widget_get_accessible(img_struct->transition_type);
	atk_object_set_description(atk, _("Transition type"));
	gtk_grid_attach (GTK_GRID (grid), img_struct->transition_type, 0, 0, 1, 1);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gtk_combo_box_get_model(GTK_COMBO_BOX(img_struct->transition_type))),
										1, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(gtk_combo_box_get_model(GTK_COMBO_BOX(img_struct->transition_type))),1, img_sort_none_before_other,NULL,NULL);

	gtk_widget_set_sensitive(img_struct->transition_type, FALSE);
	g_signal_connect (G_OBJECT (img_struct->transition_type), "changed",G_CALLBACK (img_combo_box_transition_type_changed),img_struct);

	img_struct->random_button = gtk_button_new_with_mnemonic (_("Random"));
	gtk_widget_set_tooltip_text(img_struct->random_button,_("Imagination randomly decides which transition to apply"));
	gtk_grid_attach (GTK_GRID (grid), img_struct->random_button, 1, 0, 1, 1);
	gtk_widget_set_sensitive(img_struct->random_button, FALSE);
	g_signal_connect (G_OBJECT (img_struct->random_button),"clicked",G_CALLBACK (img_random_button_clicked),img_struct);

	/* Slide duration */
	duration_label = gtk_label_new (_("Still slide duration in sec:"));
	gtk_grid_attach (GTK_GRID (grid), duration_label, 0, 3, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(duration_label), 0);
	gtk_label_set_yalign(GTK_LABEL(duration_label), 0.5);

	img_struct->duration = gtk_spin_button_new_with_range (1,60, 1);
	gtk_grid_attach (GTK_GRID (grid), img_struct->duration, 1, 3, 1, 1);
	gtk_widget_set_sensitive(img_struct->duration, FALSE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (img_struct->duration),TRUE);
	g_signal_connect (G_OBJECT (img_struct->duration),"value-changed",G_CALLBACK (img_spinbutton_value_changed),img_struct);

	/* Slide Total Duration */
	total_time = gtk_label_new (_("Slideshow length:"));
	gtk_grid_attach (GTK_GRID (grid), total_time, 0, 4, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(total_time), 0);
	gtk_label_set_yalign(GTK_LABEL(total_time), 0.5);

	img_struct->slideshow_duration = gtk_label_new ("");
	gtk_grid_attach (GTK_GRID (grid), img_struct->slideshow_duration, 1, 4, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(img_struct->slideshow_duration), 0);
	gtk_label_set_yalign(GTK_LABEL(img_struct->slideshow_duration), 0.5);

	/* Slide motion frame */
	frame2 = gtk_frame_new (NULL);
	//gtk_box_pack_start (GTK_BOX (vbox_frames), frame2, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);

	frame_label = gtk_label_new (_("<b>Slide Motion</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame2), frame_label);
	gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);
	gtk_widget_set_margin_start(frame_label, 2);
	gtk_widget_set_margin_top(frame_label, 2);

	vbox_slide_motion = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	//gtk_container_add(GTK_CONTAINER(frame2), vbox_slide_motion);
	gtk_widget_set_halign(GTK_WIDGET(vbox_slide_motion), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top(GTK_WIDGET(vbox_slide_motion), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(vbox_slide_motion), 2);
	gtk_widget_set_margin_start(GTK_WIDGET(vbox_slide_motion), 5);
	gtk_widget_set_margin_end(GTK_WIDGET(vbox_slide_motion), 5);

	hbox_stop_points = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	//gtk_box_pack_start (GTK_BOX (vbox_slide_motion), hbox_stop_points, TRUE, FALSE, 0);
	stop_points_label = gtk_label_new(_("Stop Point:"));
	//gtk_box_pack_start (GTK_BOX (hbox_stop_points), stop_points_label, TRUE, TRUE, 0);
	gtk_label_set_xalign(GTK_LABEL(stop_points_label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(stop_points_label), 0.5);

	img_struct->ken_left = gtk_button_new();
	g_signal_connect( G_OBJECT( img_struct->ken_left ), "clicked",
					  G_CALLBACK( img_goto_prev_point ), img_struct );
	//gtk_box_pack_start (GTK_BOX (hbox_stop_points), img_struct->ken_left, FALSE, TRUE, 0);
	image_buttons = gtk_image_new_from_icon_name ("go-previous", GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(img_struct->ken_left), image_buttons);

	img_struct->ken_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (img_struct->ken_entry), 2);
	gtk_entry_set_width_chars(GTK_ENTRY (img_struct->ken_entry), 2);
	{
		GObject *object = G_OBJECT( img_struct->ken_entry );
	
		g_signal_connect( object, "activate",
						  G_CALLBACK( img_goto_point ), img_struct );
		g_signal_connect( object, "insert-text",
						  G_CALLBACK( img_check_numeric_entry ), NULL );
	}
	//gtk_box_pack_start (GTK_BOX (hbox_stop_points), img_struct->ken_entry, FALSE, TRUE, 0);
	label_of = gtk_label_new(_(" of "));
	gtk_box_pack_start (GTK_BOX (hbox_stop_points), label_of, FALSE, FALSE, 0);
	img_struct->total_stop_points_label = gtk_label_new(NULL);
	gtk_box_pack_start (GTK_BOX (hbox_stop_points), img_struct->total_stop_points_label, FALSE, FALSE, 0);

	img_struct->ken_right = gtk_button_new();
	g_signal_connect( G_OBJECT( img_struct->ken_right ), "clicked",
					  G_CALLBACK( img_goto_next_point ), img_struct );
	//gtk_box_pack_start (GTK_BOX (hbox_stop_points), img_struct->ken_right, FALSE, TRUE, 0);
	image_buttons = gtk_image_new_from_icon_name ("go-next", GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(img_struct->ken_right), image_buttons);

	img_struct->ken_add = gtk_button_new();
	g_signal_connect( G_OBJECT( img_struct->ken_add ), "clicked",
					  G_CALLBACK( img_add_stop_point ), img_struct );
	//gtk_box_pack_start (GTK_BOX (hbox_stop_points), img_struct->ken_add, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->ken_add), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->ken_add,_("Add Stop point"));

	hbox_time_offset = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	//gtk_box_pack_start (GTK_BOX (vbox_slide_motion), hbox_time_offset, FALSE, FALSE, 0);
	time_offset_label = gtk_label_new(_("Motion duration in sec:"));
	//gtk_box_pack_start (GTK_BOX (hbox_time_offset), time_offset_label, TRUE, TRUE, 0);
	gtk_label_set_xalign(GTK_LABEL(time_offset_label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(time_offset_label), 0.5);
	img_struct->ken_duration = gtk_spin_button_new_with_range (1, 60, 1);
	gtk_entry_set_max_length(GTK_ENTRY(img_struct->ken_duration), 2);
	//gtk_box_pack_start (GTK_BOX (hbox_time_offset), img_struct->ken_duration, FALSE, FALSE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (img_struct->ken_duration),TRUE);
	g_signal_connect( G_OBJECT( img_struct->ken_duration ), "value-changed",
					  G_CALLBACK( img_update_stop_point ), img_struct );
	
	img_struct->ken_remove = gtk_button_new();
	g_signal_connect( G_OBJECT( img_struct->ken_remove ), "clicked",
					  G_CALLBACK( img_delete_stop_point ), img_struct );
	//gtk_box_pack_start (GTK_BOX (hbox_time_offset), img_struct->ken_remove, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("list-remove", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->ken_remove), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->ken_remove,_("Remove Stop point"));

	GtkWidget *hbox_zoom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	//gtk_box_pack_start (GTK_BOX (vbox_slide_motion), hbox_zoom, FALSE, FALSE, 0);

	label = gtk_label_new(_("Zoom: "));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_box_pack_start (GTK_BOX (hbox_zoom), label, FALSE, TRUE, 0);

	img_struct->ken_zoom = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
							1, 30,
							0.10000000000000001);
	gtk_scale_set_value_pos (GTK_SCALE(img_struct->ken_zoom), GTK_POS_LEFT);
	//gtk_box_pack_start (GTK_BOX (hbox_zoom), img_struct->ken_zoom, TRUE, TRUE, 0);
	g_signal_connect( G_OBJECT( img_struct->ken_zoom ), "value-changed",
					  G_CALLBACK( img_ken_burns_zoom_changed ), img_struct );
	
	/* Slide text frame */
	frame4 = gtk_frame_new (NULL);
	//gtk_box_pack_start (GTK_BOX (vbox_frames), frame4, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame4), GTK_SHADOW_OUT);

	frame_label = gtk_label_new (_("<b>Slide Text</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame4), frame_label);
	gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);
	gtk_widget_set_margin_start(frame_label, 2);
	gtk_widget_set_margin_top(frame_label, 2);

	vbox_slide_caption = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	//gtk_container_add(GTK_CONTAINER(frame4), vbox_slide_caption);
	gtk_widget_set_halign(GTK_WIDGET(vbox_slide_caption), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top(GTK_WIDGET(vbox_slide_caption), 5);
	gtk_widget_set_margin_bottom(GTK_WIDGET(vbox_slide_caption), 5);
	gtk_widget_set_margin_start(GTK_WIDGET(vbox_slide_caption), 5);
	gtk_widget_set_margin_end(GTK_WIDGET(vbox_slide_caption), 5);

	a_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	//gtk_box_pack_start (GTK_BOX (vbox_slide_caption), a_hbox, FALSE, FALSE, 0);
	
	hbox_textview = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	//gtk_box_pack_start (GTK_BOX (vbox_slide_caption), hbox_textview, FALSE, FALSE, 0);
	
	img_struct->sub_textview = gtk_text_view_new();
	gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(img_struct->sub_textview), FALSE);
	img_struct->slide_text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(img_struct->sub_textview));
	g_signal_connect( G_OBJECT( img_struct->slide_text_buffer ), "changed",
					  G_CALLBACK( img_queue_subtitle_update ), img_struct );
	/* Let's connect the focus-in and focus-out events to prevent the
	 * DEL key when pressed inside the textview delete the selected slide */
	g_signal_connect( G_OBJECT( img_struct->sub_textview ), "focus-in-event",
					  G_CALLBACK( img_sub_textview_focus_in ), img_struct );
	g_signal_connect( G_OBJECT( img_struct->sub_textview ), "focus-out-event",
					  G_CALLBACK( img_sub_textview_focus_out ), img_struct );
	img_struct->scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	g_object_set (G_OBJECT (img_struct->scrolled_win),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,NULL);
	//gtk_container_add(GTK_CONTAINER (img_struct->scrolled_win), img_struct->sub_textview);
	//gtk_box_pack_start (GTK_BOX (hbox_textview), img_struct->scrolled_win, TRUE, TRUE, 0);
	gtk_widget_set_size_request(img_struct->scrolled_win, -1, 70);

	text_animation_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	//gtk_box_pack_start( GTK_BOX( vbox_slide_caption ), text_animation_hbox, FALSE, FALSE, 0 );

	//This is to render the pattern button
	pixbuf = gtk_icon_theme_load_icon(img_struct->icon_theme,"image", 20, 0, NULL);
	tmp_image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);

	img_struct->pattern_image = GTK_WIDGET (gtk_tool_button_new (tmp_image,""));
	//gtk_box_pack_start (GTK_BOX (text_animation_hbox), img_struct->pattern_image, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(img_struct->pattern_image, _("Click to choose the text pattern") );
	g_signal_connect (	G_OBJECT (img_struct->pattern_image), "clicked", G_CALLBACK (img_pattern_clicked), img_struct);
   
	a_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	//gtk_box_pack_start (GTK_BOX (vbox_slide_caption), a_hbox, FALSE, FALSE, 0);

	a_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	//gtk_box_pack_start (GTK_BOX (vbox_slide_caption), a_hbox, FALSE, FALSE, 0);
	a_label = gtk_label_new(_("Animation:"));
	gtk_label_set_xalign(GTK_LABEL(a_label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(a_label), 0.5);
	//gtk_box_pack_start (GTK_BOX (a_hbox), a_label, TRUE, TRUE, 0);
	img_struct->sub_anim = img_create_subtitle_animation_combo();
	gtk_combo_box_set_active(GTK_COMBO_BOX(img_struct->sub_anim), 0);
	g_signal_connect( G_OBJECT( img_struct->sub_anim ), "changed",
					  G_CALLBACK( img_text_anim_set ), img_struct );
	//gtk_box_pack_start (GTK_BOX (a_hbox), img_struct->sub_anim, FALSE, FALSE, 0);

	a_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	//gtk_box_pack_start (GTK_BOX (vbox_slide_caption), a_hbox, FALSE, FALSE, 0);

	a_label = gtk_label_new( _("Animation Speed:") );
	gtk_label_set_xalign(GTK_LABEL(a_label), 0);
	gtk_label_set_yalign(GTK_LABEL(a_label), 0.5);
	//gtk_box_pack_start( GTK_BOX( a_hbox ), a_label, TRUE, TRUE, 0 );

	img_struct->sub_anim_duration = gtk_spin_button_new_with_range (1, 60, 1);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (img_struct->sub_anim_duration),TRUE);
	//gtk_box_pack_start( GTK_BOX( a_hbox ), img_struct->sub_anim_duration,						FALSE, FALSE, 0 );
	gtk_widget_set_size_request(img_struct->sub_anim_duration, 50, -1);
	gtk_entry_set_max_length(GTK_ENTRY(img_struct->sub_anim_duration), 2);
	
	gtk_widget_set_sensitive( img_struct->sub_anim_duration, FALSE );
	g_signal_connect( G_OBJECT( img_struct->sub_anim_duration ), "value-changed",
					  G_CALLBACK( img_combo_box_anim_speed_changed ), img_struct );

	frame4 = gtk_frame_new (NULL);
	//gtk_box_pack_start (GTK_BOX (vbox_frames), frame4, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame4), GTK_SHADOW_OUT);

	vbox_slide_position = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	//gtk_container_add (GTK_CONTAINER (frame4), vbox_slide_position);
	gtk_widget_set_halign(GTK_WIDGET(vbox_slide_position), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top(GTK_WIDGET(vbox_slide_position), 5);
	gtk_widget_set_margin_bottom(GTK_WIDGET(vbox_slide_position), 5);
	gtk_widget_set_margin_start(GTK_WIDGET(vbox_slide_position), 5);
	gtk_widget_set_margin_end(GTK_WIDGET(vbox_slide_position), 5);

	frame_label = gtk_label_new (_("<b>Slide Text Position</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame4), frame_label);
	gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);
	gtk_widget_set_margin_start(frame_label, 2);
	gtk_widget_set_margin_top(frame_label, 2);

	grid = gtk_grid_new();
	//gtk_box_pack_start (GTK_BOX (vbox_slide_position), grid, TRUE, TRUE, 0);
	gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 10);

	label = gtk_label_new(_("Horizontal Position: "));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

	img_struct->sub_posX_adj = (GtkAdjustment *) gtk_adjustment_new( 1.0, 1, -1, 1.0, 1.0, 0.0 );
	img_struct->sub_posX = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,
					     img_struct->sub_posX_adj);
	gtk_grid_attach (GTK_GRID (grid), img_struct->sub_posX, 1, 0, 1, 1);
	gtk_widget_set_size_request(img_struct->sub_posX, 200, -1);
	gtk_scale_set_draw_value( GTK_SCALE(img_struct->sub_posX), FALSE);
	g_signal_connect( G_OBJECT( img_struct->sub_posX ), "value-changed",
					  G_CALLBACK( img_text_pos_changed ), img_struct );

	img_struct->x_justify = gtk_button_new();
	g_signal_connect( G_OBJECT( img_struct->x_justify ), "clicked",
					  G_CALLBACK( img_align_text_horizontally_vertically ), img_struct );
	image_buttons = gtk_image_new_from_icon_name ("format-justify-center", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->x_justify), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->x_justify,_("Center the text horizontally"));
	gtk_grid_attach (GTK_GRID (grid), img_struct->x_justify, 2, 0, 1, 1);

	label = gtk_label_new(_("Vertical Position: "));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

	img_struct->sub_posY_adj = (GtkAdjustment *) gtk_adjustment_new( 1.0, 1, -1, 1.0, 1.0, 0.0 );
	img_struct->sub_posY = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,
					     img_struct->sub_posY_adj);
	gtk_grid_attach (GTK_GRID (grid), img_struct->sub_posY, 1, 1, 1, 1);
	//gtk_widget_set_size_request(img_struct->sub_posY, 200, -1);
	gtk_scale_set_draw_value( GTK_SCALE(img_struct->sub_posY), FALSE);
	g_signal_connect( G_OBJECT( img_struct->sub_posY ), "value-changed",
					  G_CALLBACK( img_text_pos_changed ), img_struct );

	img_struct->y_justify = gtk_button_new();
	g_signal_connect( G_OBJECT( img_struct->y_justify ), "clicked",
					  G_CALLBACK( img_align_text_horizontally_vertically ), img_struct );
	gtk_grid_attach (GTK_GRID (grid), img_struct->y_justify, 2, 1, 1, 1);
	image_buttons = gtk_image_new_from_icon_name ("format-justify-center", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->y_justify), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->y_justify,_("Center the text vertically"));

	label = gtk_label_new(_("Angle: "));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);

	img_struct->sub_angle = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
							 -90, 90, 1);
	gtk_grid_attach (GTK_GRID (grid), img_struct->sub_angle, 1, 2, 1, 1);
	gtk_scale_set_draw_value( GTK_SCALE(img_struct->sub_angle), FALSE);
	g_signal_connect( G_OBJECT( img_struct->sub_angle ), "value-changed",
					  G_CALLBACK( img_text_pos_changed ), img_struct );

	img_struct->reset_angle = gtk_button_new();
	g_signal_connect( G_OBJECT( img_struct->reset_angle ), "clicked",
					  G_CALLBACK( img_align_text_horizontally_vertically ), img_struct );
	gtk_grid_attach (GTK_GRID (grid), img_struct->reset_angle, 2, 2, 1, 1);
	image_buttons = gtk_image_new_from_icon_name ("edit-delete", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->reset_angle), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->reset_angle,_("Reset the angle"));
    
	/* Create the bottom slides thumbnail model */
	img_struct->thumbnail_model = gtk_list_store_new( 4,
                                                      GDK_TYPE_PIXBUF,  /* thumbnail */
                                                      G_TYPE_POINTER,   /* slide_info */
													  GDK_TYPE_PIXBUF,  /* transition thumbnail */
													  G_TYPE_BOOLEAN ); /* presence of a subtitle */

	/* Add wrapper for DnD */
	eventbox = gtk_event_box_new();
	img_struct->thum_root = eventbox;
	gtk_event_box_set_above_child( GTK_EVENT_BOX( eventbox ), FALSE );
	gtk_event_box_set_visible_window( GTK_EVENT_BOX( eventbox ), FALSE );
	gtk_drag_dest_set( GTK_WIDGET( eventbox ), GTK_DEST_DEFAULT_ALL,
					   drop_targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE |
					   GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect( G_OBJECT( eventbox ), "drag-data-received",
					  G_CALLBACK( img_on_drag_data_received), img_struct );
	gtk_box_pack_start( GTK_BOX( main_vertical_box ), eventbox, FALSE, TRUE, 0 );

	/* Create the thumbnail viewer */
	img_struct->thumb_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_size_request( img_struct->thumb_scrolledwindow, -1, 85 );
	g_signal_connect( G_OBJECT( img_struct->thumb_scrolledwindow ), "scroll-event",
					  G_CALLBACK( img_scroll_thumb ), img_struct );
	gtk_container_add( GTK_CONTAINER( eventbox ), img_struct->thumb_scrolledwindow );
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (img_struct->thumb_scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (img_struct->thumb_scrolledwindow), GTK_SHADOW_IN);

	img_struct->thumbnail_iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL (img_struct->thumbnail_model));
	gtk_container_add( GTK_CONTAINER( img_struct->thumb_scrolledwindow ), img_struct->thumbnail_iconview );
	
	/* This to avoid changing a lot of code after the deletion of the overview mode */
	img_struct->active_icon = img_struct->thumbnail_iconview;
	
	/* Create the cell layout */
	pixbuf_cell = img_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (img_struct->thumbnail_iconview), pixbuf_cell, FALSE);
	{
		gchar     *path;
		GdkPixbuf *text;

#if PLUGINS_INSTALLED
		path = g_strconcat( DATADIR,
							"/imagination/pixmaps/imagination-text.png",
							NULL );
#else
		path = g_strdup( "pixmaps/imagination-text.png" );
#endif
		text = gdk_pixbuf_new_from_file( path, NULL );
		g_free( path );
		g_object_set( G_OBJECT( pixbuf_cell ), "width", 110,
											   "ypad", 2,
											   "text-ico", text,
											   NULL );
		g_object_unref( G_OBJECT( text ) );
	}
	gtk_cell_layout_set_attributes(
			GTK_CELL_LAYOUT( img_struct->thumbnail_iconview ), pixbuf_cell,
			"pixbuf", 0,
			"transition", 2,
			"has-text", 3,
			NULL );

	/* Set some iconview properties */
	gtk_icon_view_set_text_column( GTK_ICON_VIEW( img_struct->thumbnail_iconview ), -1 );
	gtk_icon_view_set_reorderable(GTK_ICON_VIEW (img_struct->thumbnail_iconview),TRUE);
	gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (img_struct->thumbnail_iconview), GTK_SELECTION_MULTIPLE);
	gtk_icon_view_set_item_orientation (GTK_ICON_VIEW (img_struct->thumbnail_iconview), GTK_ORIENTATION_HORIZONTAL);
	gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (img_struct->thumbnail_iconview),0);
	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (img_struct->thumbnail_iconview),0);
	gtk_icon_view_set_item_padding (GTK_ICON_VIEW (img_struct->thumbnail_iconview), 0);
	gtk_widget_set_valign(img_struct->thumbnail_iconview, GTK_ALIGN_BASELINE);

	g_signal_connect (G_OBJECT (img_struct->thumbnail_iconview),"selection-changed",G_CALLBACK (img_iconview_selection_changed),img_struct);
	g_signal_connect (G_OBJECT (img_struct->thumbnail_iconview),"select-all",G_CALLBACK (img_iconview_selection_changed),img_struct);
	g_signal_connect (G_OBJECT (img_struct->thumbnail_iconview),"button-press-event",G_CALLBACK (img_iconview_selection_button_press),img_struct);
	gtk_widget_show_all( eventbox );

	/* Create the status bar */
	img_struct->statusbar = gtk_statusbar_new ();
	gtk_widget_show (img_struct->statusbar);
	g_object_set(img_struct->statusbar, "margin", 0, NULL);
	gtk_box_pack_start (GTK_BOX (main_vertical_box), img_struct->statusbar, FALSE, TRUE, 0);
	img_struct->context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (img_struct->statusbar), "statusbar");

	/* Create the progress bar */
	img_struct->progress_bar = gtk_progress_bar_new();
	gtk_widget_set_size_request (img_struct->progress_bar, -1, 15);
	{
		GtkWidget *vbox;

		vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_pack_start (GTK_BOX (img_struct->statusbar), vbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), img_struct->progress_bar, TRUE, FALSE, 0);
		gtk_widget_show (vbox);
	}
	gtk_widget_show_all( img_struct->main_horizontal_box );
	gtk_window_add_accel_group (GTK_WINDOW (img_struct->imagination_window), img_struct->accel_group);

	/* Disable all Ken Burns controls */
	img_ken_burns_update_sensitivity( img_struct, FALSE, 0 );

	/* Disable all subtitle controls */
	img_subtitle_update_sensitivity( img_struct, 0 );

	/* Load interface settings or apply default ones */
	if( ! img_load_window_settings( img_struct ) )
		img_set_window_default_settings( img_struct );

	return img_struct;
}

static GtkWidget *img_load_icon_from_theme(GtkIconTheme* icon_theme, gchar *name, gint size) {
	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(icon_theme, name, size,0,NULL);
	if (!pixbuf) 
	    pixbuf = gtk_icon_theme_load_icon(icon_theme, "image-missing", size, 0, NULL);
	GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);
	return image;
}

static gboolean img_sub_textview_focus_in(GtkWidget * UNUSED(widget), GdkEventFocus * UNUSED(event), img_window_struct *img)
{
	gtk_widget_remove_accelerator (img->select_all_menu, img->accel_group, GDK_KEY_a, GDK_CONTROL_MASK);
	gtk_widget_remove_accelerator (img->remove_menu,     img->accel_group, GDK_KEY_Delete, 0);
	return FALSE;
}


static gboolean img_sub_textview_focus_out(GtkWidget * UNUSED(widget), GdkEventFocus * UNUSED(event), img_window_struct *img)
{
	gtk_widget_add_accelerator (img->select_all_menu,"activate", img->accel_group, GDK_KEY_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator (img->remove_menu,    "activate", img->accel_group, GDK_KEY_Delete, 0, GTK_ACCEL_VISIBLE);
	return FALSE;
}

static void img_quit_menu(GtkMenuItem * UNUSED(menuitem), img_window_struct *img)
{
	if( ! img_quit_application( NULL, NULL, img ) )
		gtk_main_quit();
}

void img_iconview_selection_changed(GtkIconView *iconview, img_window_struct *img)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint nr_selected = 0;
	GList *selected = NULL;
	gchar *slide_info_msg = NULL, *selected_slide_nr = NULL;
	slide_struct 	*info_slide;

	if (img->export_is_running || img->preview_is_running)
		return;

	model = gtk_icon_view_get_model(iconview);

	selected = gtk_icon_view_get_selected_items(iconview);
	nr_selected = g_list_length(selected);
	
	if (selected == NULL)
	{
		img_set_statusbar_message(img,nr_selected);
		if( img->current_image )
		{
			cairo_surface_destroy( img->current_image );
			img->current_image = NULL;
		}
		img->current_slide = NULL;
		gtk_widget_queue_draw( img->image_area );

		/* Disable slide settings */
        img_disable_videotab(img);

		if (img->slides_nr == 0)
			gtk_label_set_text(GTK_LABEL (img->slideshow_duration),"");

		gtk_widget_set_sensitive(img->edit_empty_slide, FALSE);
		img->elapsed_time = 0;
		return;
	}

	gtk_widget_set_sensitive(img->duration,			TRUE);
	gtk_widget_set_sensitive(img->transition_type,	TRUE);
	gtk_widget_set_sensitive(img->random_button,	TRUE);

	img->cur_nr_of_selected_slide = gtk_tree_path_get_indices(selected->data)[0]+1;
	selected_slide_nr = g_strdup_printf("%d",img->cur_nr_of_selected_slide);
	//gtk_entry_set_text(GTK_ENTRY(img->slide_number_entry),selected_slide_nr);
	g_free(selected_slide_nr);

	/* Let's calculate the duration of all slides
	 * prior to the selected one only if the user
	 * has selected a slide other than the first */
	if (img->cur_nr_of_selected_slide - 1 > 0)
	{
		gtk_tree_model_get_iter(model,&iter,selected->data);

		while( gtk_tree_model_iter_previous( model, &iter ) )
		{
			gtk_tree_model_get( model, &iter, 1, &info_slide, -1 );
			if (info_slide->render)
				img->elapsed_time += 3;

			img->elapsed_time += info_slide->duration;
		}
	}
	gtk_tree_model_get_iter(model,&iter,selected->data);
	GList *node4;
	for(node4 = selected;node4 != NULL;node4 = node4->next)
		gtk_tree_path_free(node4->data);

	g_list_free (selected);
	gtk_tree_model_get(model,&iter,1,&info_slide,-1);
	img->current_slide = info_slide;

	/* Set the transition type */
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(img->transition_type));

	/* Block "changed" signal from model to avoid rewriting the same value back into current slide. */
	g_signal_handlers_block_by_func(img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);
	{
		GtkTreeIter   iter;
		GtkTreeModel *model;

		model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->transition_type ) );
		gtk_tree_model_get_iter_from_string( model, &iter, info_slide->path );
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(img->transition_type), &iter );
	}
	g_signal_handlers_unblock_by_func(img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);

	/* Set the transition duration */
	g_signal_handlers_block_by_func((gpointer)img->duration, (gpointer)img_spinbutton_value_changed, img);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(img->duration), info_slide->duration);
	g_signal_handlers_unblock_by_func((gpointer)img->duration, (gpointer)img_spinbutton_value_changed, img);

	/* Update Ken Burns display */
	img_update_stop_display( img, TRUE );

	/* Update subtitle widgets */
	img_update_subtitles_widgets( img );

	/* Enable/disable Edit empty slide menu item */
	if (info_slide->original_filename == NULL)
		gtk_widget_set_sensitive(img->edit_empty_slide, TRUE);
	else
		gtk_widget_set_sensitive(img->edit_empty_slide, FALSE);
	
	if (nr_selected > 1)
	{
		img_set_statusbar_message(img,nr_selected);
		img_ken_burns_update_sensitivity( img, FALSE, 0 );
		img_subtitle_update_sensitivity( img, 2 );
	}
	else
	{
		if (FALSE == info_slide->load_ok)
        {
            slide_info_msg = g_strdup_printf(_("File '%s' not found"), info_slide->original_filename);
            gtk_statusbar_push(GTK_STATUSBAR (img->statusbar), img->context_id, slide_info_msg);
            g_free (slide_info_msg);
            img_disable_videotab(img);
        }
        else
        {
            if (info_slide->o_filename != NULL)
            {
                slide_info_msg = g_strdup_printf("%s    %s: %s    %s: %s",info_slide->o_filename, _("Resolution"), info_slide->resolution, _("Type"), info_slide->type ? info_slide->type : "?");
                gtk_statusbar_push(GTK_STATUSBAR (img->statusbar), img->context_id, slide_info_msg);
                g_free(slide_info_msg);
            }
            img_ken_burns_update_sensitivity( img, TRUE, info_slide->no_points );
            img_subtitle_update_sensitivity( img, 1 );
        }
	}

	if( img->current_image )
	{
		cairo_surface_destroy( img->current_image );
		img->current_image = NULL;
	}

	if( ! info_slide->o_filename )
	{
		img_scale_empty_slide( info_slide->gradient, 
							info_slide->countdown,
							info_slide->g_start_point,
							info_slide->g_stop_point,
							info_slide->g_start_color,
							info_slide->g_stop_color,
							info_slide->countdown_color,
							0, -1,
							img->video_size[0],
							img->video_size[1], NULL,
							&img->current_image );
	}
	else
		/* Respect quality settings */
		img_scale_image( info_slide->p_filename,
							(gdouble)img->video_size[0] / img->video_size[1],
							0, img->video_size[1], img->distort_images,
							img->background_color, NULL, &img->current_image );
}

void img_combo_box_transition_type_changed (GtkComboBox *combo, img_window_struct *img)
{
	GList        *selected,
				 *bak;
	GtkTreeIter   iter;
	GtkTreeModel *model;
	gpointer      address;
	slide_struct *info_slide;
	gint          transition_id;
	GtkTreePath  *p;
	gchar        *path;
	GdkPixbuf    *pix;

	/* Check if anything is selected and return if nothing is */
	selected = gtk_icon_view_get_selected_items(
					GTK_ICON_VIEW( img->active_icon ) );
	if( selected == NULL )
		return;

	/* Get information about selected transition */
	model = gtk_combo_box_get_model( combo );
	gtk_combo_box_get_active_iter( combo, &iter );
	gtk_tree_model_get( model, &iter, 0, &pix,
									  2, &address,
									  3, &transition_id,
									  -1 );

	/* Get string representation of the path, which will be
	 * saved inside slide */
	p = gtk_tree_model_get_path( model, &iter );
	path = gtk_tree_path_to_string( p );
	gtk_tree_path_free( p );

	/* Update all selected slides */
	model = GTK_TREE_MODEL( img->thumbnail_model );
	bak = selected;
	while (selected)
	{
		gtk_tree_model_get_iter( model, &iter, selected->data );
		gtk_tree_model_get( model, &iter, 1, &info_slide, -1 );
		gtk_list_store_set( GTK_LIST_STORE( model ), &iter, 2, pix, -1 );
		info_slide->render = (ImgRender)address;
		info_slide->transition_id = transition_id;
		g_free( info_slide->path );
		info_slide->path = g_strdup( path );

		/* If this is first slide, we need to copy transition
		 * to the last pseudo-slide too. */
		if( gtk_tree_path_get_indices( selected->data )[0] == 0 && img->bye_bye_transition)
			img->final_transition.render = (ImgRender)address;

		selected = selected->next;
	}
	g_free( path );
	if( pix )
		g_object_unref( G_OBJECT( pix ) );
	img_taint_project(img);
	img_report_slides_transitions( img );
	img_set_total_slideshow_duration( img );
	GList *node5;
	for(node5 = bak;node5 != NULL;node5 = node5->next) {
		gtk_tree_path_free(node5->data);
	}
	g_list_free( bak );
}

static void img_random_button_clicked(GtkButton * UNUSED(button), img_window_struct *img)
{
	GList        *selected,
				 *bak;
	GtkTreeIter   iter;
	GtkTreeModel *model;
	slide_struct *info_slide;
	GdkPixbuf    *pixbuf;

	model = GTK_TREE_MODEL( img->thumbnail_model );
	selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW (img->active_icon));
	if (selected == NULL)
		return;

	/* Avoiding GList memory leak. */
	bak = selected;
	while (selected)
	{
		gtk_tree_model_get_iter(model, &iter,selected->data);
		gtk_tree_model_get(model, &iter,1,&info_slide,-1);
		pixbuf = img_set_random_transition(img, info_slide);
		gtk_list_store_set( GTK_LIST_STORE( model ), &iter, 2, pixbuf, -1 );
		if( pixbuf )
			g_object_unref( G_OBJECT( pixbuf ) );

		/* If this is first slide, copy transition to last
		 * pseudo-slide */
		if( gtk_tree_path_get_indices( selected->data )[0] == 0 && img->bye_bye_transition)
			img->final_transition.render = info_slide->render;

		selected = selected->next;
	}
	img_taint_project(img);
	GList *node6;
	for(node6 = bak;node6 != NULL;node6 = node6->next) {
		gtk_tree_path_free(node6->data);
	}
	g_list_free(bak);

	/* Update total slideshow duration */
	img_set_total_slideshow_duration(img);

	/* This fixes enable/disable issue */
	img_iconview_selection_changed(GTK_ICON_VIEW(img->active_icon), img );
}

static void img_font_color_clicked(GtkButton * UNUSED(button), img_window_struct *img)
{
	GtkWidget *chooser, *dialog;
	
	chooser = gtk_color_chooser_widget_new();
	g_object_set(G_OBJECT(chooser), "show-editor", TRUE, NULL);
	gtk_widget_show_all(chooser);
	dialog = gtk_dialog_new_with_buttons("Choose custom color", GTK_WINDOW (img->imagination_window),	GTK_DIALOG_MODAL, _("_Cancel"), GTK_RESPONSE_REJECT, _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	gtk_box_pack_start(GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), chooser, TRUE, TRUE, 5);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GdkRGBA color;

		gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (chooser), &color);
	}
	gtk_widget_destroy (dialog);
}

static GdkPixbuf *img_set_random_transition( img_window_struct *img,  slide_struct  *info_slide )
{
	gint          nr;
	gint          r1, r2;
	gpointer      address;
	gint          transition_id;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar         path[10];
	GdkPixbuf    *pix;

	/* Get tree store that holds transitions */
	model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->transition_type ) );

	/* Get number of top-levels (categories) and select one */
	nr = gtk_tree_model_iter_n_children( model, NULL );

	/* Fix crash if no modules are loaded */
	if( nr < 2 )
		return( NULL );

	r1 = g_random_int_range( 1, nr );
	g_snprintf( path, sizeof( path ), "%d", r1 );
	gtk_tree_model_get_iter_from_string( model, &iter, path );

	/* Get number of transitions in selected category and select one */
	nr = gtk_tree_model_iter_n_children( model, &iter );
	r2 = g_random_int_range( 0, nr );
	g_snprintf( path, sizeof( path ), "%d:%d", r1, r2 );
	gtk_tree_model_get_iter_from_string( model, &iter, path );

	gtk_tree_model_get( model, &iter, 0, &pix, 2, &address, 3, &transition_id, -1 );
	info_slide->transition_id = transition_id;
	info_slide->render = (ImgRender)address;

	/* Prevent leak here */
	if( info_slide->path )
		g_free( info_slide->path );
	info_slide->path = g_strdup( path );

	/* Select proper iter in transition model */
	g_signal_handlers_block_by_func((gpointer)img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);	
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(img->transition_type), &iter);

	/* Update the slide dialog report in real time */
	img_report_slides_transitions(img);
	g_signal_handlers_unblock_by_func((gpointer)img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);	

	return( pix );
}

static void img_select_all_thumbnails(GtkMenuItem * UNUSED(item), img_window_struct *img)
{
	gtk_icon_view_select_all(GTK_ICON_VIEW (img->active_icon));
}

static void img_unselect_all_thumbnails(GtkMenuItem * UNUSED(item), img_window_struct *img)
{
	gtk_icon_view_unselect_all(GTK_ICON_VIEW (img->active_icon));
}

static void img_goto_line_entry_activate(GtkWidget * UNUSED(entry), img_window_struct *img)
{
	gint slide;
	GtkTreePath *path;

	slide = strtol(gtk_entry_get_text(GTK_ENTRY(img->slide_number_entry)), NULL, 10);
	if (slide > 0 && slide <= img->slides_nr)
	{
		gtk_icon_view_unselect_all(GTK_ICON_VIEW (img->active_icon));
		path = gtk_tree_path_new_from_indices(slide-1,-1);
		gtk_icon_view_set_cursor (GTK_ICON_VIEW (img->active_icon), path, NULL, FALSE);
		gtk_icon_view_select_path (GTK_ICON_VIEW (img->active_icon), path);
		gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (img->active_icon), path, FALSE, 0, 0);
		gtk_tree_path_free (path);
	}
}

static gint img_sort_none_before_other(GtkTreeModel *model,GtkTreeIter *a,GtkTreeIter *b,gpointer UNUSED(data))
{
	gchar *name1, *name2;
	gint i;

	gtk_tree_model_get(model, a, 1, &name1, -1);
	gtk_tree_model_get(model, b, 1, &name2, -1);

	if (strcmp(name1,_("None")) == 0)
		i = -1;
	else if (strcmp(name2,_("None")) == 0)
		i = 1;
	else
		i = (g_strcmp0 (name1,name2));

	g_free(name1);
	g_free(name2);
	return i;	
}

static void img_check_numeric_entry (GtkEditable *entry, gchar *text, gint UNUSED(length), gint * UNUSED(position), gpointer  UNUSED(data))
{
	if(*text < '0' || *text > '9')
		g_signal_stop_emission_by_name( (gpointer)entry, "insert-text" );
}

/*
 * img_iconview_selection_button_press:
 *
 * This is a temporary hack that should do the job of unselecting slides if
 * single slide should be selected after select all.
 */
static gboolean
img_iconview_selection_button_press( GtkWidget         *widget,
									 GdkEventButton    *button,
									 img_window_struct * UNUSED(img) )
{
	if( ( button->button == 1 ) &&
		! ( button->state & ( GDK_SHIFT_MASK | GDK_CONTROL_MASK ) ) )
		gtk_icon_view_unselect_all( GTK_ICON_VIEW( widget ) );

	return( FALSE );
}

static gboolean
img_scroll_thumb( GtkWidget         *widget,
				  GdkEventScroll    *scroll,
				  img_window_struct * UNUSED(img) )
{
	GtkAdjustment *adj;
	gdouble        page, step, upper, value, offset;

	adj = gtk_scrolled_window_get_hadjustment( GTK_SCROLLED_WINDOW( widget ) );

	page  = gtk_adjustment_get_page_size( adj );
	step  = gtk_adjustment_get_step_increment( adj );
	upper = gtk_adjustment_get_upper( adj );
	value = gtk_adjustment_get_value( adj );


	switch (scroll->direction) {
	    case GDK_SCROLL_UP:
	    case GDK_SCROLL_LEFT:
		offset = - step;
		break;
	    case GDK_SCROLL_DOWN:
	    case GDK_SCROLL_RIGHT:
		offset = + step;
		break;
	    case GDK_SCROLL_SMOOTH:
		/* one of the ->delta is 0 depending on the direction */
		offset = step * (scroll->delta_x + scroll->delta_y);
	}

	gtk_adjustment_set_value( adj, CLAMP( value + offset, 0, upper - page ) );
	return( TRUE );
}

static void img_show_uri(GtkMenuItem * UNUSED(menuitem), img_window_struct *img)
{
	gchar *file = NULL;
	gchar *lang = NULL;
	
	lang = g_strndup(g_getenv("LANG"),2);
	file = g_strconcat("file://",DATADIR,"/doc/",PACKAGE,"/html/",lang,"/index.html",NULL);
	g_free(lang);
	img_message (img, file);

	/* If help is not localized yet, show default language (english) */
	if ( !gtk_show_uri_on_window(GTK_WINDOW(img->imagination_window), file, GDK_CURRENT_TIME, NULL))
	{
		g_free( file );
		file = g_strconcat("file://",DATADIR,"/doc/",PACKAGE,"/html/en/index.html",NULL);
		gtk_show_uri_on_window(GTK_WINDOW(img->imagination_window), file, GDK_CURRENT_TIME, NULL);
	}
	g_free(file);
}

void
img_queue_subtitle_update( GtkTextBuffer     * UNUSED(buffer),
						   img_window_struct *img )
{
	/* This queue enables us to avoid sensless copying and redrawing when typing
	 * relatively fast (limit is cca. 3 keypresses per second) */
	if( img->subtitle_update_id )
		g_source_remove( img->subtitle_update_id );

	img_taint_project(img);

	img->subtitle_update_id =
			g_timeout_add( 300, (GSourceFunc)img_subtitle_update, img );
}

static GtkWidget *
img_create_subtitle_animation_combo( void )
{
	GtkWidget       *combo;
	GtkListStore    *store;
	TextAnimation   *animations;
	gint             no_anims;
	register gint    i;
	GtkTreeIter      iter;
	GtkCellRenderer *cell;

	store = gtk_list_store_new( 3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT );

	no_anims = img_get_text_animation_list( &animations );
	for( i = 0; i < no_anims; i++ )
	{
		gtk_list_store_append( store, &iter );
		gtk_list_store_set( store, &iter, 0, animations[i].name,
										  1, animations[i].func,
										  2, animations[i].id,
										  -1 );
	}
	img_free_text_animation_list( no_anims, animations );

	combo = gtk_combo_box_new_with_model( GTK_TREE_MODEL( store ) );
	g_object_unref( G_OBJECT( store ) );

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell, TRUE );
	gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT( combo ), cell, "text", 0 );

	return( combo );
}

static gboolean img_subtitle_update( img_window_struct *img )
{
	gboolean     has_subtitle;
	GtkTreeIter	 iter;  
	GList       *list;

	if( img->current_slide->subtitle )
	{
		g_free( img->current_slide->subtitle );
		img->current_slide->subtitle = NULL;
	}
	has_subtitle = 1 < gtk_text_buffer_get_char_count( img->slide_text_buffer );
	if( has_subtitle )
	{	
		img_store_rtf_buffer_content(img);
	}
	list = gtk_icon_view_get_selected_items(
				GTK_ICON_VIEW( img->active_icon ) );
	gtk_tree_model_get_iter( GTK_TREE_MODEL( img->thumbnail_model ),
							 &iter, list->data );
	GList *node8;
	for(node8 = list;node8 != NULL;node8 = node8->next) {
		gtk_tree_path_free(node8->data);
	}
	g_list_free( list );
	gtk_list_store_set( GTK_LIST_STORE( img->thumbnail_model ), &iter,
						3, has_subtitle, -1 );

	/* Queue redraw */
	gtk_widget_queue_draw( img->image_area );

	/* Set source id to zero and remove itself from main context */
	img->subtitle_update_id = 0;

	return( FALSE );
}

void
img_text_font_set( GtkFontChooser     *button,
				   img_window_struct *img )
{
	const gchar *string;
	
	string = gtk_font_chooser_get_font( button );

	img_update_sub_properties( img, NULL, -1, -1, string, NULL, NULL, NULL, NULL,
								img->current_slide->top_border, img->current_slide->bottom_border, img->current_slide->alignment, -1);

	gtk_widget_queue_draw( img->image_area );
	img_taint_project(img);
}

void
img_text_anim_set( GtkComboBox       *combo,
				   img_window_struct *img )
{
	GtkTreeModel      *model;
	GtkTreeIter        iter;
	TextAnimationFunc  anim;
	gint               anim_id;

	model = gtk_combo_box_get_model( combo );
	gtk_combo_box_get_active_iter( combo, &iter );
	gtk_tree_model_get( model, &iter, 1, &anim, 2, &anim_id, -1 );

	img_update_sub_properties( img, anim, anim_id, -1, NULL, NULL, NULL, NULL, NULL, 
								img->current_slide->top_border, img->current_slide->bottom_border, img->current_slide->alignment, -1);

	/* Speed should be disabled when None is in effect */
	gtk_widget_set_sensitive( img->sub_anim_duration,
							  (gboolean)gtk_combo_box_get_active( combo ) );

	gtk_widget_queue_draw( img->image_area );
	img_taint_project(img);
}

void
img_font_color_changed( GtkColorChooser    *button, img_window_struct *img )
{
	GdkRGBA color;
	gchar	 *rgb;
	gdouble  font_color[4];
	gboolean 	selection;
	GtkTextIter start, end;
	GtkTextTag 	*tag;

	gtk_color_chooser_get_rgba( button, &color );

	selection = gtk_text_buffer_get_selection_bounds(img->slide_text_buffer, &start, &end);
	if (selection > 0)
	{
		rgb = gdk_rgba_to_string(&color);
		tag = gtk_text_tag_table_lookup(img->tag_table, "foreground");
		g_object_set(tag, "foreground", rgb, NULL);
		g_free(rgb);
		gtk_text_buffer_apply_tag(img->slide_text_buffer, tag, &start, &end);
		img_store_rtf_buffer_content(img);
	}
	else
	{
		font_color[0] = (gdouble)color.red;
		font_color[1] = (gdouble)color.green;
		font_color[2] = (gdouble)color.blue;
		font_color[3] = (gdouble)color.alpha;
 		img_update_sub_properties( img, NULL, -1, -1, NULL, font_color, NULL, NULL, NULL,
								img->current_slide->top_border, img->current_slide->bottom_border, img->current_slide->alignment, -1);
	}
	gtk_widget_queue_draw( img->image_area );
	img_taint_project(img);
}

void
img_font_brdr_color_changed( GtkColorChooser    *button,
                          img_window_struct *img )
{
    GdkRGBA color;
    gdouble  font_brdr_color[4];

    gtk_color_chooser_get_rgba( button, &color );
    
	font_brdr_color[0] = (gdouble)color.red;
	font_brdr_color[1] = (gdouble)color.green;
	font_brdr_color[2] = (gdouble)color.blue;
	font_brdr_color[3] = (gdouble)color.alpha;
   	img_update_sub_properties( img, NULL, -1, -1, NULL, NULL, font_brdr_color, NULL, NULL,
							img->current_slide->top_border, img->current_slide->bottom_border, img->current_slide->alignment, -1);

	gtk_widget_queue_draw( img->image_area );
	img_taint_project(img);
}

void
img_font_bg_color_changed( GtkColorChooser    *button,
                          img_window_struct *img )
{
    GdkRGBA color;
    gchar	 *rgb;
	gdouble  font_bgcolor[4];
	gboolean 	selection;
    GtkTextIter start, end;
	GtkTextTag 	*tag;

    gtk_color_chooser_get_rgba( button, &color );
    
	selection = gtk_text_buffer_get_selection_bounds(img->slide_text_buffer, &start, &end);
	if (selection > 0)
	{
		rgb = gdk_rgba_to_string(&color);
		tag = gtk_text_tag_table_lookup(img->tag_table, "background");
		g_object_set(tag, "background", rgb, NULL);
		g_free(rgb);
		gtk_text_buffer_apply_tag(img->slide_text_buffer, tag, &start, &end);
		img_store_rtf_buffer_content(img);
	}
	else
	{
		font_bgcolor[0] = (gdouble)color.red;
		font_bgcolor[1] = (gdouble)color.green;
		font_bgcolor[2] = (gdouble)color.blue;
		font_bgcolor[3] = (gdouble)color.alpha;
	
		img_update_sub_properties( img, NULL, -1, -1, NULL, NULL, NULL, font_bgcolor, NULL, 
								img->current_slide->top_border, img->current_slide->bottom_border, img->current_slide->alignment, -1);
	}
    gtk_widget_queue_draw( img->image_area );
    img_taint_project(img);
}

void
img_combo_box_anim_speed_changed( GtkSpinButton       *spinbutton,
								  img_window_struct *img )
{
	gint speed;

	speed = gtk_spin_button_get_value_as_int(spinbutton);
	img_update_sub_properties( img, NULL, -1, speed, NULL, NULL, NULL, NULL, NULL, 
								img->current_slide->top_border, img->current_slide->bottom_border, img->current_slide->alignment, -1);
	img_taint_project(img);
}

void img_sub_border_width_changed( GtkSpinButton       *spinbutton,
								  img_window_struct *img )
{
	gint width;

	width = gtk_spin_button_get_value_as_int(spinbutton);
	img_update_sub_properties( img, NULL, -1, -1, NULL, NULL, NULL, NULL, NULL, 
								img->current_slide->top_border, img->current_slide->bottom_border, -1, width);

	gtk_widget_queue_draw( img->image_area );
	img_taint_project(img);
}

void
img_disable_videotab (img_window_struct *img)
{
    gtk_widget_set_sensitive(img->random_button, FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(img->transition_type), 0);
    gtk_widget_set_sensitive(img->transition_type, FALSE);
    gtk_widget_set_sensitive(img->duration, FALSE);

    img_ken_burns_update_sensitivity (img, FALSE, 0);
    img_subtitle_update_sensitivity (img, 0);
}


void
img_ken_burns_update_sensitivity( img_window_struct *img,
								  gboolean           slide_selected,
								  gint               no_points )
{
	/* Modes of operation:
	 *   3 - disable all
	 *   2 - enable duration, zoom and add
	 *   1 - disable only navigation
	 *   0 - enable all
	 */
	gint mode = 3;

	if( slide_selected )
	{
		switch( no_points )
		{
			case 0:
				mode = 2;
				break;

			case 1:
				mode = 1;
				break;

			default:
				mode = 0;
				break;
		}
	}

	/* Disable all - this is starting state */
	gtk_widget_set_sensitive( img->ken_left,     FALSE );
	gtk_widget_set_sensitive( img->ken_entry,    FALSE );
	gtk_widget_set_sensitive( img->ken_right,    FALSE );
	gtk_widget_set_sensitive( img->ken_duration, FALSE );
	gtk_widget_set_sensitive( img->ken_zoom,     FALSE );
	gtk_widget_set_sensitive( img->ken_add,      FALSE );
	gtk_widget_set_sensitive( img->ken_remove,   FALSE );

	/* Enabler */
	switch( mode ) /* THIS SWITCH IS IN FALL-THROUGH MODE!! */
	{
		case 0: /* Enable all */
			gtk_widget_set_sensitive( img->ken_left,     TRUE );
			gtk_widget_set_sensitive( img->ken_entry,    TRUE );
			gtk_widget_set_sensitive( img->ken_right,    TRUE );
			// fall through
		case 1: /* Disable navigation only */
			gtk_widget_set_sensitive( img->ken_remove,   TRUE );
			// fall through
		case 2: /* Only adding is enabled */
			gtk_widget_set_sensitive( img->ken_add,      TRUE );
			gtk_widget_set_sensitive( img->ken_zoom,     TRUE );
			gtk_widget_set_sensitive( img->ken_duration, TRUE );
			// fall through
		case 3: /* Disable all */
			break;
	}
}

void
img_subtitle_update_sensitivity( img_window_struct *img,
								 gint               mode )
{
	/* Modes:
	 *  0 - disable all
	 *  1 - enable all
	 *  2 - enable all but text field
	 */

	/* Text view is special, since it cannot handle multiple slides */
	gtk_widget_set_sensitive( img->sub_textview,
							  ( mode == 2 ? FALSE : (gboolean)mode ) );

	/* Let's delete the textbuffer when no slide is selected */
	if( mode == 0 || mode == 2 )
	{
		g_signal_handlers_block_by_func( (gpointer)img->slide_text_buffer,
										 (gpointer)img_queue_subtitle_update,
										 img );
		g_object_set( G_OBJECT( img->slide_text_buffer ), "text", "", NULL );
		g_signal_handlers_unblock_by_func( (gpointer)img->slide_text_buffer,
										   (gpointer)img_queue_subtitle_update,
										   img );
	}

	/* Animation duration is also special, since it shoudl be disabled when None
	 * animation is selected. */
	if( gtk_combo_box_get_active( GTK_COMBO_BOX( img->sub_anim ) ) && mode )
		gtk_widget_set_sensitive( img->sub_anim_duration, TRUE );
	else
		gtk_widget_set_sensitive( img->sub_anim_duration, FALSE );

	//~ gtk_widget_set_sensitive( img->sub_font,    (gboolean)mode );

    //~ gtk_widget_set_sensitive( img->sub_brdr_color, (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->sub_bgcolor, (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->pattern_image, (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->sub_anim,    (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->sub_posX,     (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->sub_posY,     (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->sub_angle,    (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->x_justify,    (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->y_justify,    (gboolean)mode );
	//~ gtk_widget_set_sensitive( img->reset_angle,    (gboolean)mode );
}

void
img_update_sub_properties( img_window_struct *img,
						   TextAnimationFunc  UNUSED(anim),
						   gint               anim_id,
						   gdouble            anim_duration,
						   const gchar       *desc,
						   gdouble           *color,
						   gdouble           *brdr_color,
						   gdouble           *bg_color,
						   gdouble           *border_color,
						   gboolean			top_border,
						   gboolean			bottom_border,
						   gint	             border_width,
						   gint	             alignment)
{
	GList        *selected,
				 *tmp;
	GtkTreeIter   iter;
	GtkTreeModel *model;

	/* Get all selected slides */
	selected = gtk_icon_view_get_selected_items(
					GTK_ICON_VIEW( img->active_icon ) );
	if( ! selected )
		return;

	model = GTK_TREE_MODEL( img->thumbnail_model );

	for( tmp = selected; tmp; tmp = g_list_next( tmp ) )
	{
		slide_struct *slide;

		gtk_tree_model_get_iter( model, &iter, (GtkTreePath *)tmp->data );
		gtk_tree_model_get( model, &iter, 1, &slide, -1 );
		img_set_slide_text_info( slide, NULL, NULL, NULL, img->current_slide->pattern_filename,
								 anim_id, anim_duration,	img->current_slide->posX,
															img->current_slide->posY, 
															img->current_slide->subtitle_angle,
								 desc, color, brdr_color, bg_color, border_color, top_border, bottom_border, border_width, alignment, img );
	}

	GList *node9;
	for(node9 = selected;node9 != NULL;node9 = node9->next) {
		gtk_tree_path_free(node9->data);
	}
	g_list_free( selected );
}

static void img_report_slides_transitions(img_window_struct *img)
{
	static GtkWidget *viewport;
	GtkWidget        *label;
	GHashTable       *trans_hash;
	GList            *values,
					 *tmp;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gboolean          flag;
	gint              i;

#define GIP( val ) GINT_TO_POINTER( ( val ) )
#define GPI( val ) GPOINTER_TO_INT( ( val ) )

	if (img->report_dialog == NULL)
	{
		GtkWidget *vbox, *swindow;

		img->report_dialog = gtk_dialog_new_with_buttons(
					_("Slides Transitions Report Dialog"),
					GTK_WINDOW( img->imagination_window ),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					"_Close", GTK_RESPONSE_ACCEPT,
					NULL );
		gtk_container_set_border_width( GTK_CONTAINER( img->report_dialog ),
										10 );		
		gtk_window_set_default_size( GTK_WINDOW( img->report_dialog ),
									 480, 370 );
		gtk_window_set_modal( GTK_WINDOW( img->report_dialog ), FALSE );

		g_signal_connect( G_OBJECT( img->report_dialog ), "delete-event",
						  G_CALLBACK( gtk_widget_hide_on_delete ), NULL );
		g_signal_connect( G_OBJECT( img->report_dialog ), "response",
						  G_CALLBACK( gtk_widget_hide_on_delete ), NULL );

		vbox = gtk_dialog_get_content_area( GTK_DIALOG( img->report_dialog ) );
		swindow = gtk_scrolled_window_new( NULL, NULL );
		gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( swindow ),
										GTK_POLICY_AUTOMATIC,
										GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start( GTK_BOX( vbox ), swindow, TRUE, TRUE, 0 );

		viewport = gtk_viewport_new( NULL, NULL );
		gtk_viewport_set_shadow_type( GTK_VIEWPORT( viewport ),
									  GTK_SHADOW_NONE);
		gtk_container_add( GTK_CONTAINER( swindow ), viewport );
	}

	/* Delete previous shown rows */
	if( img->vbox_slide_report_rows )
	{
		gtk_widget_destroy( img->vbox_slide_report_rows );
		img->vbox_slide_report_rows = NULL;
	}

	model = GTK_TREE_MODEL(img->thumbnail_model);
	if( gtk_tree_model_get_iter_first( model, &iter ) == 0)
		return;

	/* Hash table is used only for quick way of accessing transition info.
	 * Information is stored inside array of 3 gpointers */
	trans_hash = g_hash_table_new( g_direct_hash, NULL );

	for( flag = TRUE, i = 0;
		 flag;
		 flag = gtk_tree_model_iter_next( model, &iter ), i++ )
	{
		slide_struct *slide;
		gpointer     *info;

		gtk_tree_model_get( model, &iter, 1, &slide, -1 );
		if( slide->transition_id < 1 )
			continue;

		info = g_hash_table_lookup( trans_hash, GIP( slide->transition_id ) );
		if( ! info )
		{
			/* Create new info element */
			info = g_slice_alloc0( sizeof( gpointer ) * 3 );
			info[0] = GIP( slide->transition_id );
			g_hash_table_insert( trans_hash, GIP( slide->transition_id ),
											 info );
		}

		/* Increment counter */
		info[1] = GIP( GPI( info[1] ) + 1 );

		/* Append another element to glist */
		info[2] = g_list_append( (GList *)info[2], GIP( i ) );
	}

	/* Set the vertical box container that was previously
	 * destroyed so to allow update in real time */
	img->vbox_slide_report_rows = gtk_box_new(GTK_ORIENTATION_VERTICAL,
						  15);
	gtk_container_add( GTK_CONTAINER( viewport ), img->vbox_slide_report_rows );

	label = gtk_label_new( _("\n<span weight='bold'>Note:</span>\n\n"
							 "Slides whose transition is applied only once are "
							 "not shown here.\n"
							 "Click on the slide to have Imagination "
							 "automatically select it." ) );
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_label_set_use_markup( GTK_LABEL( label ), TRUE );
	gtk_box_pack_start( GTK_BOX( img->vbox_slide_report_rows ), label,
						FALSE, FALSE, 0);

	/* Get information and free hash table */
	values = g_hash_table_get_values( trans_hash );
	g_hash_table_destroy( trans_hash );

	/* Sort values list here */
	values = g_list_sort( values, img_sort_report_transitions );

	/* Display results */
	for( tmp = values; tmp; tmp = g_list_next( tmp ) )
	{
		gpointer *info = tmp->data;

		if( GPI( info[1] ) > 1 )
		{
			GList     *tmp1;
			GtkWidget *hbox_rows,
					  *frame,
					  *image,
					  *nr_label;
			gchar     *filename,
					  *nr;

			hbox_rows = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,
						15);
			gtk_box_pack_start( GTK_BOX( img->vbox_slide_report_rows ),
								hbox_rows, FALSE, FALSE, 0 );

			frame = gtk_frame_new( NULL );
			gtk_frame_set_shadow_type( GTK_FRAME( frame ), GTK_SHADOW_NONE );
			gtk_box_pack_start( GTK_BOX( hbox_rows ), frame, FALSE, FALSE, 0 );

#if PLUGINS_INSTALLED
			filename =
				g_strdup_printf( "%s/imagination/pixmaps/imagination-%d.png",
								 DATADIR, GPI( info[0] ) );
#else /* PLUGINS_INSTALLED */
			filename =
				g_strdup_printf( "./pixmaps/imagination-%d.png",
								 GPI( info[0] ) );
#endif
			image = gtk_image_new_from_file( filename );
			g_free( filename );
			gtk_container_add( GTK_CONTAINER( frame ), image );

			nr = g_strdup_printf( "(%d)", GPI( info[1] ) );
			nr_label = gtk_label_new( nr );
			gtk_box_pack_start( GTK_BOX( hbox_rows ), nr_label,
								FALSE, FALSE, 0 );
			g_free( nr );

			for( tmp1 = (GList *)info[2]; tmp1; tmp1 = g_list_next( tmp1 ) )
			{
				GtkWidget   *button,
							*image;
				GdkPixbuf   *pixbuf;
				GtkTreePath *path;
				GtkTreeIter  iter;

				path = gtk_tree_path_new_from_indices( GPI( tmp1->data ), -1 );
				gtk_tree_model_get_iter( model, &iter, path );
				gtk_tree_path_free( path );

				gtk_tree_model_get( model, &iter, 0, &pixbuf, -1 );

				button = gtk_button_new();
				g_object_set_data( G_OBJECT( button ), "index", tmp1->data );
				g_signal_connect( G_OBJECT( button ), "clicked",
								  G_CALLBACK( img_select_slide_from_slide_report_dialog ), img );
				gtk_box_pack_start( GTK_BOX( hbox_rows ), button,
									FALSE, FALSE, 0 );

				image = gtk_image_new_from_pixbuf( pixbuf );
				g_object_unref( G_OBJECT( pixbuf ) );
				gtk_container_add( GTK_CONTAINER( button ), image );
			}
		}
		g_list_free( (GList *)info[2] );
		g_slice_free1( sizeof( gpointer ) * 3, info );
	}

	if( gtk_widget_get_visible( img->report_dialog ) )
		gtk_widget_show_all( img->report_dialog );

#undef GIP
#undef GPI
}

static void img_select_slide_from_slide_report_dialog(GtkButton *button, img_window_struct *img)
{
	GtkTreePath *path;
	gint slide = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "index"));

	gtk_icon_view_unselect_all(GTK_ICON_VIEW (img->active_icon));
	path = gtk_tree_path_new_from_indices(slide, -1);
	gtk_icon_view_set_cursor (GTK_ICON_VIEW (img->active_icon), path, NULL, FALSE);
	gtk_icon_view_select_path (GTK_ICON_VIEW (img->active_icon), path);
	gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (img->active_icon), path, FALSE, 0, 0);
	gtk_tree_path_free (path);
}

static void img_show_slides_report_dialog(GtkMenuItem * UNUSED(item), img_window_struct *img)
{
	img_report_slides_transitions(img);
	gtk_widget_show_all(img->report_dialog);
}

static gint
img_sort_report_transitions( gconstpointer a,
							 gconstpointer b )
{
	gint val_a = GPOINTER_TO_INT( ( (gpointer *)a )[1] ),
		 val_b = GPOINTER_TO_INT( ( (gpointer *)b )[1] );

	return( val_a - val_b );
}

static void
img_toggle_frame_rate( GtkCheckMenuItem  *item,
					   img_window_struct *img )
{
	gpointer tmp;

	if( ! gtk_check_menu_item_get_active( item ) )
		return;

	tmp = g_object_get_data( G_OBJECT( item ), "index" );
	img->preview_fps = GPOINTER_TO_INT( tmp );
}

