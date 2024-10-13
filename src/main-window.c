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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
//#include <assert.h>

#include "main-window.h"

static GtkTargetEntry dest_target[] =
{
	{ "text/uri-list", 0, 0 }
};

 static GtkTargetEntry source_target[] =
 {
	{ "application/x-media-item", 0, 0 }
 };

 static GtkTargetEntry timeline_drop_targets[] =
 {
	{ "application/x-media-item", GTK_TARGET_SAME_APP, 0},
	{ "text/uri-list", 0, 0}
};

static GtkTargetEntry timeline_drag_targets[] =
{
    { "application/x-media-item", GTK_TARGET_SAME_APP, 0 }
};

static void img_random_button_clicked(GtkButton *, img_window_struct *);
static GdkPixbuf *img_set_random_transition(img_window_struct *, media_struct *);
static void img_transition_speed_changed (GtkRange *,  img_window_struct *);
static void img_clear_audio_files(GtkButton *, img_window_struct *);
static void img_quit_menu(GtkMenuItem *, img_window_struct *);
static gint img_sort_none_before_other(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, gpointer);
static void img_check_numeric_entry (GtkEditable *entry, gchar *text, gint lenght, gint *position, gpointer data);
static void img_open_documentation(GtkMenuItem *, img_window_struct *);
static void img_open_github_bug_report(GtkMenuItem *, img_window_struct *);
static void img_open_paypal_page(GtkMenuItem *, img_window_struct *);
static gboolean img_iconview_popup(GtkWidget *,  GdkEvent  *, img_window_struct *);       
static void img_media_model_remove_media(GtkWidget *, img_window_struct *);
static void img_media_show_properties(GtkWidget *, img_window_struct *);

GtkWidget *button1,  *button2, *button3, *toggle_button_slide_motion, *button5, *beginning, *end;

static gboolean img_leave_notify_event(GtkWidget *widget,  GdkEvent  *event, img_window_struct *img)
{
	if (img->textbox->cursor_source_id > 0)
	{
		g_source_remove(img->textbox->cursor_source_id);
		img->textbox->cursor_source_id = 0;
	}
	return FALSE;
}

static gboolean img_enter_notify_event(GtkWidget *widget,  GdkEvent  *event, img_window_struct *img)
{
	 if (!img->textbox->cursor_source_id)
		img->textbox->cursor_source_id = g_timeout_add(750, (GSourceFunc) blink_cursor, img);

	return FALSE;
}

static void img_toggle_button_callback(GtkToggleButton *button, img_window_struct *img)
{
    GtkWidget *buttons[] = {button2, button3, img->toggle_button_text, toggle_button_slide_motion};
    for (int i = 0; i < 4; i++)
    {
        if (buttons[i] != GTK_WIDGET(button))
        {
			g_signal_handlers_block_by_func( buttons[i],  img_toggle_button_callback, img );
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[i]), FALSE);
			g_signal_handlers_unblock_by_func( buttons[i],  img_toggle_button_callback, img );
		}
        else
        {
			gtk_notebook_set_current_page (GTK_NOTEBOOK(img->side_notebook), i);
			g_signal_handlers_block_by_func( buttons[i],  img_toggle_button_callback, img );
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[i]), TRUE);
			g_signal_handlers_unblock_by_func( buttons[i],  img_toggle_button_callback, img );
        }
	}
}

static GtkWidget *img_create_subtitle_animation_combo( void );

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
	GtkWidget *select_all_menu;
	GtkWidget *imagemenuitem1;
	GtkWidget *imagemenuitem5;
	GtkWidget *separatormenuitem1;
	GtkWidget *slide_menu;
	GtkWidget *deselect_all_menu;
	GtkWidget *menuitem3;
	GtkWidget *tmp_image;
	GtkWidget *menu3;
	GtkWidget *about;
	GtkWidget *donate;
	GtkWidget *report_bug;
	GtkWidget *contents;
	GtkWidget *add_slide;
	GtkWidget *label_of;
	GtkWidget *side_notebook;
	GtkWidget *scrollable_window;
	GtkWidget *vbox_frames;
	GtkWidget *transition_label;
	GtkWidget *vbox_slide_motion;
	GtkWidget *viewport;
	GtkWidget *grid;
	GtkWidget *duration_label;
	GtkWidget *hbox_motion, *stop_points_label;
	GtkWidget  *time_offset_label;
	GtkWidget *image_buttons;
	GtkCellRenderer *pixbuf_cell;
	GdkPixbuf *icon;
	GtkWidget  *main_horizontal_box;
	GtkWidget *vbox, *hbox, *right_horizontal_box;
	GtkWidget *content_area;
	GtkWidget *preview_menu;
	GtkWidget *import_menu;
	GtkWidget *properties_menu;
	GtkWidget *export_menu;
	GdkPixbuf *pixbuf;
	GtkWidget *flip_horizontally_menu;
	GtkWidget *flip_vertically_menu;
	GtkWidget *rotate_menu;
	AtkObject *atk;

	img_struct = g_new0(img_window_struct, 1);
	
	/* Set some default values */
	img_struct->background_color[0] = 0;
	img_struct->background_color[1] = 0;
	img_struct->background_color[2] = 0;
	img_struct->media_nr = 0;
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

	img_struct->textbox = g_new0(img_textbox, 1);
    img_struct->textbox->angle = 0;
    img_struct->textbox->text = g_string_new("");
    img_struct->textbox->x = 275;
    img_struct->textbox->y = 162.5;
    img_struct->textbox->width = 250;
    img_struct->textbox->height = 125;
    img_struct->textbox->corner = GDK_LEFT_PTR;
    img_struct->textbox->action = NONE;
    img_struct->textbox->visible = FALSE;
    img_struct->textbox->cursor_visible = FALSE;
    img_struct->textbox->cursor_pos = 0;
	img_struct->textbox->cursor_source_id = 0;
	img_struct->textbox->selection_start = -1;
	img_struct->textbox->selection_end = -1;
	img_struct->textbox->is_x_snapped = FALSE;
	img_struct->textbox->is_y_snapped = FALSE;
	img_struct->textbox->layout = pango_cairo_create_layout(cairo_create(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1)));
	//TODO: remove font_desc
    img_struct->textbox->font_desc = pango_font_description_from_string("Noto 15");
    img_struct->textbox->attr_list = pango_attr_list_new();
    
    pango_layout_set_wrap(img_struct->textbox->layout, PANGO_WRAP_CHAR);
    pango_layout_set_ellipsize(img_struct->textbox->layout, PANGO_ELLIPSIZE_NONE);
    pango_layout_set_font_description(img_struct->textbox->layout, img_struct->textbox->font_desc);
    pango_layout_get_size(img_struct->textbox->layout, &img_struct->textbox->lw, &img_struct->textbox->lh);
	
	img_struct->textbox->lw /= PANGO_SCALE;
	img_struct->textbox->lh /= PANGO_SCALE;
	
	/* GUI STUFF */
	img_struct->icon_theme = gtk_icon_theme_get_default();
	icon = gtk_icon_theme_load_icon(img_struct->icon_theme, "imagination", 24, 0, NULL);

	img_struct->imagination_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_icon (GTK_WINDOW(img_struct->imagination_window),icon);
	gtk_window_set_position (GTK_WINDOW(img_struct->imagination_window),GTK_WIN_POS_CENTER);
	img_refresh_window_title(img_struct);
	g_signal_connect (img_struct->imagination_window, "delete-event",			G_CALLBACK (img_quit_application),img_struct);
	g_signal_connect (img_struct->imagination_window, "destroy", 					G_CALLBACK (gtk_main_quit), NULL );
	g_signal_connect (img_struct->imagination_window, "key-press-event", 		G_CALLBACK(img_window_key_pressed), img_struct);
	g_signal_connect(img_struct->imagination_window, "leave-notify-event", 	G_CALLBACK(img_leave_notify_event), img_struct);
    g_signal_connect(img_struct->imagination_window, "enter-notify-event", 	G_CALLBACK(img_enter_notify_event), img_struct);
	
	img_struct->accel_group = gtk_accel_group_new();
	
	main_vertical_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (img_struct->imagination_window), main_vertical_box);
    
	/* Create the menu items */
	img_struct->menubar = gtk_menu_bar_new ();
	gtk_box_pack_start (GTK_BOX (main_vertical_box), img_struct->menubar, FALSE, FALSE, 0);

	menuitem1 = gtk_menu_item_new_with_mnemonic (_("Project"));
	gtk_container_add (GTK_CONTAINER (img_struct->menubar), menuitem1);

	menu1 = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem1), menu1);

	imagemenuitem1 = gtk_menu_item_new_with_mnemonic(_("_New"));
	gtk_container_add (GTK_CONTAINER (menu1), imagemenuitem1);
	gtk_widget_add_accelerator (imagemenuitem1,"activate", img_struct->accel_group,GDK_KEY_n,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (imagemenuitem1,"activate",G_CALLBACK (img_new_slideshow),img_struct);

	img_struct->open_menu = gtk_menu_item_new_with_mnemonic (_("_Open"));
	gtk_container_add (GTK_CONTAINER (menu1), img_struct->open_menu);
	gtk_widget_add_accelerator (img_struct->open_menu,"activate", img_struct->accel_group,GDK_KEY_o,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (img_struct->open_menu,"activate",G_CALLBACK (img_choose_slideshow_filename),img_struct);

	img_struct->open_recent = gtk_menu_item_new_with_mnemonic (_("Open recent"));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu1 ), img_struct->open_recent);
	
	img_struct->recent_slideshows = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (img_struct->open_recent), img_struct->recent_slideshows);
	
	img_struct->save_menu = gtk_menu_item_new_with_mnemonic (_("_Save"));
	gtk_container_add (GTK_CONTAINER (menu1), img_struct->save_menu);
	gtk_widget_add_accelerator (img_struct->save_menu,"activate", img_struct->accel_group,GDK_KEY_s,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (img_struct->save_menu,"activate",G_CALLBACK (img_choose_slideshow_filename),img_struct);

	img_struct->save_as_menu = gtk_menu_item_new_with_mnemonic (_("Save _As"));
	gtk_container_add (GTK_CONTAINER (menu1), img_struct->save_as_menu);
	g_signal_connect (img_struct->save_as_menu,"activate",G_CALLBACK (img_choose_slideshow_filename),img_struct);

	img_struct->close_menu = gtk_menu_item_new_with_mnemonic (_("_Close"));
	gtk_container_add (GTK_CONTAINER (menu1), img_struct->close_menu);
	gtk_widget_add_accelerator (img_struct->close_menu,"activate", img_struct->accel_group,GDK_KEY_w,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (img_struct->close_menu,"activate",G_CALLBACK (img_close_slideshow),img_struct);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu1), separatormenuitem1);

	import_menu = gtk_menu_item_new_with_mnemonic (_("Import any med_ia"));
	gtk_container_add (GTK_CONTAINER (menu1),import_menu);
	gtk_widget_add_accelerator (import_menu,"activate",img_struct->accel_group,GDK_KEY_i,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (import_menu,"activate",G_CALLBACK (img_add_media_items),img_struct);
	
	properties_menu = gtk_menu_item_new_with_mnemonic (_("_Properties"));
	gtk_container_add (GTK_CONTAINER (menu1), properties_menu);
	gtk_widget_add_accelerator (properties_menu,"activate",img_struct->accel_group,GDK_KEY_p,GDK_MOD1_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (properties_menu, "activate", G_CALLBACK (img_project_properties), img_struct);
	
	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu1), separatormenuitem1);

	preview_menu = gtk_menu_item_new_with_mnemonic (_("Preview"));
	gtk_container_add (GTK_CONTAINER (menu1), preview_menu);
	gtk_widget_add_accelerator (preview_menu, "activate",img_struct->accel_group, GDK_KEY_space, 0, GTK_ACCEL_VISIBLE);
	g_signal_connect (preview_menu,"activate",G_CALLBACK (img_start_stop_preview),img_struct);
	
	export_menu = gtk_menu_item_new_with_mnemonic (_("Ex_port"));
	gtk_container_add (GTK_CONTAINER (menu1), export_menu);
	gtk_widget_add_accelerator (export_menu, "activate",img_struct->accel_group,GDK_KEY_p,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (export_menu,"activate",G_CALLBACK (img_show_export_dialog),img_struct);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu1), separatormenuitem1);

	imagemenuitem5 = gtk_menu_item_new_with_mnemonic (_("_Quit"));
	gtk_container_add (GTK_CONTAINER (menu1), imagemenuitem5);
	gtk_widget_add_accelerator (imagemenuitem5, "activate",img_struct->accel_group,GDK_KEY_q,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (imagemenuitem5,"activate",G_CALLBACK (img_quit_menu),img_struct);

	/* Media menu */
	menuitem2 = gtk_menu_item_new_with_mnemonic (_("Media"));
	gtk_container_add (GTK_CONTAINER (img_struct->menubar), menuitem2);

	slide_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem2), slide_menu);

	add_slide = gtk_menu_item_new_with_mnemonic (_("Add empt_y media"));
	gtk_container_add (GTK_CONTAINER (slide_menu), add_slide);
	gtk_widget_add_accelerator( add_slide, "activate", img_struct->accel_group,	GDK_KEY_y, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE );
	g_signal_connect(add_slide, "activate", G_CALLBACK( img_add_empty_slide ), img_struct );

	img_struct->edit_empty_slide = gtk_menu_item_new_with_mnemonic (_("Edit _empty media"));
	gtk_container_add (GTK_CONTAINER (slide_menu), img_struct->edit_empty_slide);
	gtk_widget_set_sensitive(img_struct->edit_empty_slide, FALSE);
	gtk_widget_add_accelerator( img_struct->edit_empty_slide, "activate", img_struct->accel_group,	GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE );
	g_signal_connect( img_struct->edit_empty_slide, "activate", G_CALLBACK( img_add_empty_slide ), img_struct );

	img_struct->remove_menu = gtk_menu_item_new_with_mnemonic (_("Dele_te"));
	gtk_container_add (GTK_CONTAINER (slide_menu), img_struct->remove_menu);
	g_signal_connect (img_struct->remove_menu,"activate",G_CALLBACK (img_media_model_remove_media), img_struct);

	flip_horizontally_menu = gtk_menu_item_new_with_mnemonic (_("_Flip horizontally"));
	gtk_container_add (GTK_CONTAINER (slide_menu),flip_horizontally_menu);
	gtk_widget_add_accelerator (flip_horizontally_menu,"activate",img_struct->accel_group, GDK_KEY_f,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	//g_signal_connect(flip_horizontally_menu, "activate", G_CALLBACK( img_flip_horizontally), img_struct );

	flip_vertically_menu = gtk_menu_item_new_with_mnemonic (_("_Flip vertically"));
	gtk_container_add (GTK_CONTAINER (slide_menu),flip_vertically_menu);
	//gtk_widget_add_accelerator (flip_vertically_menu,"activate",img_struct->accel_group, GDK_KEY_f,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	//g_signal_connect(flip_vertically_menu, "activate", G_CALLBACK( img_flip_vertically_menu), img_struct );

	rotate_menu = gtk_menu_item_new_with_mnemonic (_("_Rotate clockwise"));
	gtk_container_add (GTK_CONTAINER (slide_menu),rotate_menu);
	gtk_widget_add_accelerator (rotate_menu,"activate",img_struct->accel_group, GDK_KEY_r,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	//g_signal_connect(rotate_menu, "activate", G_CALLBACK ( img_rotate_media ), img_struct );

	select_all_menu = gtk_menu_item_new_with_mnemonic(_("Select All"));
	gtk_container_add (GTK_CONTAINER (slide_menu), select_all_menu);
	gtk_widget_add_accelerator (select_all_menu,"activate",img_struct->accel_group,GDK_KEY_a,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (select_all_menu,"activate",G_CALLBACK (img_select_all_media),img_struct);

	deselect_all_menu = gtk_menu_item_new_with_mnemonic (_("Un_select all"));
	gtk_container_add (GTK_CONTAINER (slide_menu),deselect_all_menu);
	gtk_widget_add_accelerator (deselect_all_menu,"activate",img_struct->accel_group,GDK_KEY_a,GDK_CONTROL_MASK | GDK_SHIFT_MASK,GTK_ACCEL_VISIBLE);
	g_signal_connect (deselect_all_menu,"activate",G_CALLBACK (img_unselect_all_media),img_struct);

	/* Help menu */
	menuitem3 = gtk_menu_item_new_with_mnemonic (_("Help"));
	gtk_container_add (GTK_CONTAINER (img_struct->menubar), menuitem3);
	menu3 = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem3), menu3);

	contents = gtk_menu_item_new_with_mnemonic (_("Contents"));
	gtk_container_add (GTK_CONTAINER (menu3),contents);
	gtk_widget_add_accelerator (contents,"activate",img_struct->accel_group,GDK_KEY_F1,0,GTK_ACCEL_VISIBLE);
	g_signal_connect (contents,"activate",G_CALLBACK (img_open_documentation),img_struct);

	separatormenuitem1 = gtk_separator_menu_item_new();
	gtk_container_add (GTK_CONTAINER(menu3), separatormenuitem1);
	
	report_bug = gtk_menu_item_new_with_mnemonic (_("Report a bug..."));
	gtk_container_add (GTK_CONTAINER (menu3), report_bug);
	g_signal_connect (report_bug,"activate",G_CALLBACK (img_open_github_bug_report),img_struct);
	
	donate = gtk_menu_item_new_with_mnemonic (_("Donate..."));
	gtk_container_add (GTK_CONTAINER (menu3), donate);
	g_signal_connect (donate,"activate",G_CALLBACK (img_open_paypal_page),img_struct);

	about = gtk_menu_item_new_with_mnemonic (_("About"));
	gtk_container_add (GTK_CONTAINER (menu3), about);
	g_signal_connect (about,"activate",G_CALLBACK (img_show_about_dialog),img_struct);
	gtk_window_add_accel_group (GTK_WINDOW (img_struct->imagination_window), img_struct->accel_group);

	//Create the sidebar with the buttons
	main_horizontal_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(main_vertical_box), main_horizontal_box, TRUE, TRUE, 0);

	img_struct->sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_can_focus(img_struct->sidebar, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(img_struct->sidebar), 5);
    gtk_box_pack_start(GTK_BOX(main_horizontal_box), img_struct->sidebar, FALSE, FALSE, 0);
	
	button1 = gtk_button_new_from_icon_name("list-add", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_set_tooltip_text(button1, _("Import any media"));
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), button1, FALSE, FALSE, 5);
	g_signal_connect(button1, "clicked", G_CALLBACK(img_add_any_media_callback), img_struct);	
	gtk_widget_set_can_focus(button1, FALSE);
	
	button2 = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(button2, _("Media Library"));
    GtkWidget *image1 = gtk_image_new_from_icon_name("applications-multimedia", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(button2), image1);
    gtk_button_set_relief(GTK_BUTTON(button2), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), button2, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button2), TRUE);
    gtk_widget_set_can_focus(button2, FALSE);
     
    button3 = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(button3, _("Transitions effects"));
    image1 = gtk_image_new_from_icon_name("emblem-photos", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(button3), image1);
    gtk_button_set_relief(GTK_BUTTON(button3), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), button3, FALSE, FALSE, 5);
    gtk_widget_set_can_focus(button3, FALSE);
    
    img_struct->toggle_button_text = gtk_toggle_button_new_with_label("<span size='x-large'><b>T</b></span>");
	GtkWidget *label = gtk_bin_get_child(GTK_BIN(img_struct->toggle_button_text));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_set_tooltip_text(img_struct->toggle_button_text, _("Insert text"));
    gtk_button_set_relief(GTK_BUTTON(img_struct->toggle_button_text), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), img_struct->toggle_button_text, FALSE, FALSE, 5);
	gtk_widget_set_can_focus(img_struct->toggle_button_text, FALSE);
	
	toggle_button_slide_motion = gtk_toggle_button_new();
	gtk_widget_set_tooltip_text(toggle_button_slide_motion, _("Ken Burns effect"));
	image1 = gtk_image_new_from_icon_name("emblem-shared", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(toggle_button_slide_motion), image1);
	gtk_button_set_relief(GTK_BUTTON(toggle_button_slide_motion), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), toggle_button_slide_motion, FALSE, FALSE, 5);

	button5 = gtk_button_new_with_label("\U0001F642");
	gtk_button_set_relief(GTK_BUTTON(button5), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(img_struct->sidebar), button5, FALSE, FALSE, 5);
	gtk_widget_set_can_focus(button5, FALSE);
	
	content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX(main_horizontal_box), content_area, TRUE, TRUE, 0);
	
	//Create the vertical paned widget to allow the timeline to resize upwards
	img_struct->vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (content_area), img_struct->vpaned, TRUE, TRUE, 0);
	g_signal_connect (img_struct->vpaned , "notify::position", G_CALLBACK (img_change_image_area_size),img_struct);
	
	right_horizontal_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_paned_pack1(GTK_PANED( img_struct->vpaned ), right_horizontal_box, TRUE, FALSE);

   //Create the hidden notebook widget
	img_struct->side_notebook = gtk_notebook_new();
	gtk_widget_set_can_focus(img_struct->side_notebook, FALSE);
	g_object_set (G_OBJECT (img_struct->side_notebook),"show-border", FALSE,"show-tabs", FALSE,"enable-popup",FALSE,NULL);
	gtk_box_pack_start (GTK_BOX(right_horizontal_box), img_struct->side_notebook, FALSE, FALSE, 0);

	g_signal_connect(button2, "toggled", G_CALLBACK(img_toggle_button_callback), img_struct);
	g_signal_connect(button3, "toggled", G_CALLBACK(img_toggle_button_callback), img_struct);
	g_signal_connect(img_struct->toggle_button_text, "toggled", G_CALLBACK(img_toggle_button_callback), img_struct);
	g_signal_connect(toggle_button_slide_motion, "toggled", G_CALLBACK(img_toggle_button_callback), img_struct);

	//Create the Media Library
	img_struct->media_model = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
	img_struct->media_iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL(img_struct->media_model));
	gtk_widget_set_can_focus(img_struct->media_iconview, FALSE);
	g_object_unref (img_struct->media_model);

	img_struct->media_iconview_swindow = gtk_scrolled_window_new(NULL, NULL);
	g_object_set (G_OBJECT (	img_struct->media_iconview_swindow),"hscrollbar-policy",GTK_POLICY_NEVER,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);
	gtk_container_add (GTK_CONTAINER (	img_struct->media_iconview_swindow), img_struct->media_iconview);

	gtk_icon_view_set_item_orientation (GTK_ICON_VIEW (img_struct->media_iconview), GTK_ORIENTATION_VERTICAL);
	gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (img_struct->media_iconview), 0);
	gtk_icon_view_set_columns(GTK_ICON_VIEW (img_struct->media_iconview), 2);
	gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (img_struct->media_iconview),0);
	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (img_struct->media_iconview),0);
	
	gtk_container_add (GTK_CONTAINER (img_struct->side_notebook), 	img_struct->media_iconview_swindow);
	gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (img_struct->media_iconview), GTK_SELECTION_MULTIPLE);
	
	// Enable Dnd functionality
	gtk_icon_view_enable_model_drag_dest(GTK_ICON_VIEW(img_struct->media_iconview), dest_target, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE |  GDK_ACTION_LINK | GDK_ACTION_ASK);
	gtk_icon_view_enable_model_drag_source(GTK_ICON_VIEW(img_struct->media_iconview), GDK_BUTTON1_MASK, source_target, 1, GDK_ACTION_COPY);
	g_signal_connect(img_struct->media_iconview, "drag-data-received",	G_CALLBACK( img_media_widget_drag_data_received), img_struct );
	g_signal_connect(img_struct->media_iconview, "drag-data-get", 			G_CALLBACK( img_media_widget_drag_data_get), img_struct );
	g_signal_connect(img_struct->media_iconview, "button-press-event",	G_CALLBACK( img_iconview_popup), img_struct );
	//gtk_icon_view_set_reorderable(GTK_ICON_VIEW (media_iconview),TRUE);
	
	GtkCellRenderer *cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT(img_struct->media_iconview), cell, TRUE );
	gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT(img_struct->media_iconview), cell,  "text", 1, NULL);
	g_object_set(cell, "ellipsize", PANGO_ELLIPSIZE_END, "xalign", 0.5, NULL);

	/* Create the transition widget section */
	vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	g_object_set (vbox, "margin-right", 10, NULL);
	gtk_container_add (GTK_CONTAINER (img_struct->side_notebook), vbox);
	
	transition_label = gtk_label_new (_("Transition Type:"));
	gtk_box_pack_start (GTK_BOX (vbox), transition_label, FALSE, FALSE, 0);
	gtk_label_set_xalign(GTK_LABEL(transition_label), 0);
	gtk_label_set_yalign(GTK_LABEL(transition_label), -1);

	/* Slide selected, slide resolution, slide type and slide total duration */
	grid = gtk_grid_new();
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
	gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 5);
	gtk_box_pack_start (GTK_BOX (vbox), grid, TRUE, FALSE, 0);

	/* Transition type */
	img_struct->transition_type = _gtk_combo_box_new_text( TRUE );
	atk = gtk_widget_get_accessible(img_struct->transition_type);
	atk_object_set_description(atk, _("Transition type"));
	gtk_grid_attach (GTK_GRID (grid), img_struct->transition_type, 0, 0, 1, 1);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gtk_combo_box_get_model(GTK_COMBO_BOX(img_struct->transition_type))),
										1, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(gtk_combo_box_get_model(GTK_COMBO_BOX(img_struct->transition_type))),1, img_sort_none_before_other,NULL,NULL);

	gtk_widget_set_sensitive(img_struct->transition_type, FALSE);
	g_signal_connect (img_struct->transition_type, "changed",G_CALLBACK (img_combo_box_transition_type_changed),img_struct);

	img_struct->random_button = gtk_button_new_with_mnemonic (_("Random"));
	gtk_widget_set_tooltip_text(img_struct->random_button,_("Imagination randomly decides which transition to apply"));
	gtk_grid_attach (GTK_GRID (grid), img_struct->random_button, 1, 0, 1, 1);
	gtk_widget_set_sensitive(img_struct->random_button, FALSE);
	g_signal_connect (img_struct->random_button,"clicked",G_CALLBACK (img_random_button_clicked),img_struct);

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

	/*Create the text widget section */
	scrollable_window = gtk_scrolled_window_new(NULL, NULL);
	g_object_set (G_OBJECT (scrollable_window),"hscrollbar-policy",GTK_POLICY_NEVER,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);
	
	vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	g_object_set (vbox, "margin-right", 10, NULL);
	gtk_container_add (GTK_CONTAINER (scrollable_window), vbox);
	gtk_container_add (GTK_CONTAINER (img_struct->side_notebook), scrollable_window);
	
	//1st row: font selector and font color widgets
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 8);

	label = gtk_label_new(_("<span size='large'><b>Text:</b></span>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 8);

	img_struct->sub_font = gtk_font_button_new();
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->sub_font, TRUE, TRUE, 5);
	g_signal_connect(img_struct->sub_font, "font-set",	  G_CALLBACK( img_text_font_changed ), img_struct );

	img_struct->sub_font_color = gtk_button_new();
	gtk_widget_set_name(img_struct->sub_font_color , "font_color_button");
	gtk_widget_set_valign(img_struct->sub_font_color, GTK_ALIGN_CENTER);
	g_signal_connect(img_struct->sub_font_color, "clicked", G_CALLBACK( img_text_color_clicked ), img_struct );
	gtk_box_pack_start( GTK_BOX(hbox), img_struct->sub_font_color, FALSE, FALSE, 5);
	gtk_widget_set_tooltip_text(img_struct->sub_font_color, _("Font color"));
	GtkCssProvider *css = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css, "#font_color_button {min-width:12px;min-height:12px;border-radius: 50%;background: white;} \
																	#nav_button {padding:0px;margin:0px} \
																	#font_button {padding: 0px;} button {padding: 5px}",  -1, NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(css);

    //2nd row: style buttons and aligment
    hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

	img_struct->bold_style = gtk_button_new_with_label("<span size='x-large'><b>B</b></span>");
	gtk_widget_set_name(img_struct->bold_style , "font_button");
	gtk_widget_set_size_request(img_struct->bold_style, 26, -1);
	label = gtk_bin_get_child(GTK_BIN(img_struct->bold_style));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	g_signal_connect(img_struct->bold_style, "clicked",	  G_CALLBACK( img_text_style_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->bold_style, FALSE, FALSE, 5);

	img_struct->italic_style = gtk_button_new_with_label("<span size='x-large'><i>I</i></span>");
	gtk_widget_set_name(img_struct->italic_style , "font_button");
	gtk_widget_set_size_request(img_struct->italic_style, 26, -1);
	label = gtk_bin_get_child(GTK_BIN(img_struct->italic_style));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	g_signal_connect(img_struct->italic_style, "clicked",	  G_CALLBACK( img_text_style_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->italic_style, FALSE, FALSE, 5);

	img_struct->underline_style = gtk_button_new_with_label("<span size='x-large'><u>U</u></span>");
	gtk_widget_set_name(img_struct->underline_style , "font_button");
	gtk_widget_set_size_request(img_struct->underline_style, 26, -1);
	label = gtk_bin_get_child(GTK_BIN(img_struct->underline_style));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	g_signal_connect(img_struct->underline_style, "clicked", G_CALLBACK( img_text_style_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->underline_style, FALSE, FALSE, 5);

	img_struct->left_justify = gtk_button_new();
	gtk_widget_set_name(img_struct->left_justify , "font_button");
	g_signal_connect( img_struct->left_justify, "clicked",		  G_CALLBACK( img_text_align_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->left_justify, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("format-justify-left", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->left_justify), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->left_justify, _("Align left"));

	img_struct->fill_justify = gtk_button_new();
	gtk_widget_set_name(img_struct->fill_justify , "font_button");
	g_signal_connect(img_struct->fill_justify, "clicked",			  G_CALLBACK( img_text_align_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->fill_justify, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("format-justify-center", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->fill_justify), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->fill_justify, _("Align center"));

	img_struct->right_justify = gtk_button_new();
	gtk_widget_set_name(img_struct->right_justify , "font_button");
	g_signal_connect( img_struct->right_justify, "clicked",		  G_CALLBACK( img_text_align_changed ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->right_justify, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("format-justify-right", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->right_justify), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->right_justify, _("Align right"));
	
	//3rd row: font pattern
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 7);

	label = gtk_label_new(_("Font pattern:"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	pixbuf = gtk_icon_theme_load_icon(img_struct->icon_theme,"image", 32, 0, NULL);
	tmp_image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);

	img_struct->pattern_image = GTK_WIDGET (gtk_tool_button_new (tmp_image,""));
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->pattern_image, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(img_struct->pattern_image, _("Click to choose the font pattern") );
	g_signal_connect (img_struct->pattern_image, "clicked", G_CALLBACK (img_pattern_clicked), img_struct);
	
	//4th row: Animation
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 7);

	label = gtk_label_new(_("Animation:"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	img_struct->sub_anim = img_create_subtitle_animation_combo();
	gtk_combo_box_set_active(GTK_COMBO_BOX(img_struct->sub_anim), 0);
	g_signal_connect(img_struct->sub_anim, "changed",  G_CALLBACK( img_text_anim_set ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->sub_anim, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 7);

	label = gtk_label_new( _("Animation Speed:") );
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 5);

	img_struct->sub_anim_duration = gtk_spin_button_new_with_range (1, 60, 1);
	gtk_widget_set_can_focus(img_struct->sub_anim_duration, FALSE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (img_struct->sub_anim_duration),TRUE);
	gtk_box_pack_start( GTK_BOX( hbox ), img_struct->sub_anim_duration,						FALSE, FALSE, 0 );
	gtk_widget_set_size_request(img_struct->sub_anim_duration, 50, -1);
	gtk_entry_set_max_length(GTK_ENTRY(img_struct->sub_anim_duration), 2);
	g_signal_connect(img_struct->sub_anim_duration, "value-changed",   G_CALLBACK( img_combo_box_anim_speed_changed ), img_struct );
	
	//5th row: background
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 7);

	label = gtk_label_new(_("<span size='large'><b>Background:</b></span>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	
	label = gtk_label_new(_("Color"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	img_struct->sub_font_bg_color = gtk_button_new();
	gtk_widget_set_name(img_struct->sub_font_bg_color , "font_color_button");
	gtk_widget_set_valign(img_struct->sub_font_bg_color, GTK_ALIGN_CENTER);
	g_signal_connect( img_struct->sub_font_bg_color, "clicked", G_CALLBACK( img_text_color_clicked ), img_struct );
	gtk_box_pack_start( GTK_BOX(hbox), img_struct->sub_font_bg_color, FALSE, FALSE, 5);
	
	//6th row: background padding
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

	label = gtk_label_new(_("Padding:"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	img_struct->sub_font_bg_padding = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
	gtk_scale_set_value_pos (GTK_SCALE(img_struct->sub_font_bg_padding), GTK_POS_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->sub_font_bg_padding, TRUE, TRUE, 0);
	//g_signal_connect( G_OBJECT( img_struct->sub_font_bg_padding ), "value-changed", G_CALLBACK( img_spin_test ), img_struct );
	
	//7th row: background radius
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

	label = gtk_label_new(_("Corner radius:"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	img_struct->sub_font_bg_radius = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
	gtk_scale_set_value_pos (GTK_SCALE(img_struct->sub_font_bg_radius), GTK_POS_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->sub_font_bg_radius, TRUE, TRUE, 0);
	
	//8th row: Shadow
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

	label = gtk_label_new(_("<span size='large'><b>Shadow:</b></span>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	
	label = gtk_label_new(_("Color"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	img_struct->sub_font_shadow_color = gtk_button_new();
	gtk_widget_set_name(img_struct->sub_font_shadow_color , "font_color_button");
	gtk_widget_set_valign(img_struct->sub_font_shadow_color, GTK_ALIGN_CENTER);
	g_signal_connect( img_struct->sub_font_shadow_color, "clicked", G_CALLBACK( img_text_color_clicked ), img_struct );
	gtk_box_pack_start( GTK_BOX(hbox), img_struct->sub_font_shadow_color, FALSE, FALSE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	label = gtk_label_new(_("Distance"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	img_struct->sub_font_shadow_distance = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
	gtk_scale_set_value_pos (GTK_SCALE(img_struct->sub_font_shadow_distance), GTK_POS_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->sub_font_shadow_distance, TRUE, TRUE, 0);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	
	label = gtk_label_new(_("Angle"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	img_struct->sub_font_shadow_angle = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 360, 1);
	gtk_scale_set_value_pos (GTK_SCALE(img_struct->sub_font_shadow_angle), GTK_POS_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->sub_font_shadow_angle, TRUE, TRUE, 0);
	
	//9th row: Outline
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	
	label = gtk_label_new(_("<span size='large'><b>Outline:</b></span>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	
	label = gtk_label_new(_("Color"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	img_struct->sub_font_outline_color = gtk_button_new();
	gtk_widget_set_name(img_struct->sub_font_outline_color , "font_color_button");
	gtk_widget_set_valign(img_struct->sub_font_outline_color, GTK_ALIGN_CENTER);
	g_signal_connect(img_struct->sub_font_outline_color, "clicked", G_CALLBACK( img_text_color_clicked ), img_struct );
	gtk_box_pack_start( GTK_BOX(hbox), img_struct->sub_font_outline_color, FALSE, FALSE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	
	label = gtk_label_new(_("Size"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
	
	hbox =  gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
	img_struct->sub_font_outline_size = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
	gtk_scale_set_value_pos (GTK_SCALE(img_struct->sub_font_outline_size), GTK_POS_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), img_struct->sub_font_outline_size, TRUE, TRUE, 0);
	
	/* Slide motion widgets (Ken Burns) */
	vbox_slide_motion = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_container_add (GTK_CONTAINER (img_struct->side_notebook), vbox_slide_motion);
	gtk_widget_set_valign(vbox_slide_motion, GTK_ALIGN_START);
	gtk_widget_set_halign(vbox_slide_motion, GTK_ALIGN_END);
	
	label = gtk_label_new(_("<span size='large'><b>Slide Motion:</b></span>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start (GTK_BOX (vbox_slide_motion), label, TRUE, TRUE, 5);
	
	hbox_motion = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start (GTK_BOX (vbox_slide_motion), hbox_motion, TRUE, FALSE, 0);
	stop_points_label = gtk_label_new(_("Stop Point:"));
	gtk_box_pack_start (GTK_BOX (hbox_motion), stop_points_label, TRUE, TRUE, 0);
	gtk_label_set_xalign(GTK_LABEL(stop_points_label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(stop_points_label), 0.5);

	img_struct->ken_left = gtk_button_new();
	g_signal_connect(img_struct->ken_left, "clicked",	  G_CALLBACK( img_goto_prev_point ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox_motion), img_struct->ken_left, FALSE, TRUE, 0);
	image_buttons = gtk_image_new_from_icon_name ("go-previous", GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(img_struct->ken_left), image_buttons);

	img_struct->ken_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (img_struct->ken_entry), 2);
	gtk_entry_set_width_chars(GTK_ENTRY (img_struct->ken_entry), 2);
	g_signal_connect(img_struct->ken_entry, "activate",   	G_CALLBACK( img_goto_point ), img_struct );
	g_signal_connect(img_struct->ken_entry, "insert-text", G_CALLBACK( img_check_numeric_entry ), NULL );

	gtk_box_pack_start (GTK_BOX (hbox_motion), img_struct->ken_entry, FALSE, TRUE, 0);
	label_of = gtk_label_new(_(" of "));
	gtk_box_pack_start (GTK_BOX (hbox_motion), label_of, FALSE, FALSE, 0);
	img_struct->total_stop_points_label = gtk_label_new(NULL);
	gtk_box_pack_start (GTK_BOX (hbox_motion), img_struct->total_stop_points_label, FALSE, FALSE, 0);

	img_struct->ken_right = gtk_button_new();
	g_signal_connect( img_struct->ken_right, "clicked",   G_CALLBACK( img_goto_next_point ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox_motion), img_struct->ken_right, FALSE, TRUE, 0);
	image_buttons = gtk_image_new_from_icon_name ("go-next", GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(img_struct->ken_right), image_buttons);

	img_struct->ken_add = gtk_button_new();
	g_signal_connect( img_struct->ken_add, "clicked",  G_CALLBACK( img_add_stop_point ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox_motion), img_struct->ken_add, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->ken_add), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->ken_add,_("Add Stop point"));

	hbox_motion = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox_slide_motion), hbox_motion, FALSE, FALSE, 0);
	time_offset_label = gtk_label_new(_("Duration in sec:"));
	gtk_box_pack_start (GTK_BOX (hbox_motion), time_offset_label, TRUE, TRUE, 0);
	gtk_label_set_xalign(GTK_LABEL(time_offset_label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(time_offset_label), 0.5);
	img_struct->ken_duration = gtk_spin_button_new_with_range (1, 60, 1);
	gtk_entry_set_max_length(GTK_ENTRY(img_struct->ken_duration), 2);
	gtk_box_pack_start (GTK_BOX (hbox_motion), img_struct->ken_duration, FALSE, FALSE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (img_struct->ken_duration),TRUE);
	g_signal_connect(img_struct->ken_duration, "value-changed",  G_CALLBACK( img_update_stop_point ), img_struct );
	
	img_struct->ken_remove = gtk_button_new();
	g_signal_connect( img_struct->ken_remove, "clicked",  G_CALLBACK( img_delete_stop_point ), img_struct );
	gtk_box_pack_start (GTK_BOX (hbox_motion), img_struct->ken_remove, FALSE, FALSE, 5);
	image_buttons = gtk_image_new_from_icon_name ("list-remove", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(img_struct->ken_remove), image_buttons);
	gtk_widget_set_tooltip_text(img_struct->ken_remove,_("Remove Stop point"));

	hbox_motion = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox_slide_motion), hbox_motion, FALSE, FALSE, 0);

	label = gtk_label_new(_("Zoom: "));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_box_pack_start (GTK_BOX (hbox_motion), label, FALSE, TRUE, 0);

	img_struct->ken_zoom = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 30, 0.10000000000000001);
	gtk_scale_set_value_pos (GTK_SCALE(img_struct->ken_zoom), GTK_POS_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox_motion), img_struct->ken_zoom, TRUE, TRUE, 0);
	g_signal_connect( img_struct->ken_zoom, "value-changed",  G_CALLBACK( img_ken_burns_zoom_changed ), img_struct );

	/* Create the image area */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	g_object_set(G_OBJECT(vbox), "valign", GTK_ALIGN_FILL);
	gtk_box_pack_start (GTK_BOX (right_horizontal_box), vbox, TRUE, TRUE, 0);
	
	img_struct->image_area = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox), img_struct->image_area, TRUE, TRUE, 0);
	
	gtk_widget_set_hexpand(img_struct->image_area, FALSE);
	gtk_widget_set_vexpand(img_struct->image_area, FALSE);
	
	gtk_widget_set_halign(img_struct->image_area, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(img_struct->image_area, GTK_ALIGN_CENTER);
	gtk_widget_set_events(img_struct->image_area, 
										  GDK_KEY_PRESS_MASK
										| GDK_BUTTON_PRESS_MASK
										| GDK_BUTTON_RELEASE_MASK
										| GDK_POINTER_MOTION_MASK);

    gtk_widget_set_can_focus(img_struct->image_area, TRUE);
    gtk_widget_grab_focus(img_struct->image_area);
    
	g_signal_connect(img_struct->image_area , "draw",	  						G_CALLBACK( img_on_draw_event ), img_struct );
	g_signal_connect(img_struct->image_area , "button-press-event",	G_CALLBACK( img_image_area_button_press ), img_struct );
	g_signal_connect(img_struct->image_area , "button-release-event",G_CALLBACK( img_image_area_button_release ), img_struct );
	g_signal_connect(img_struct->image_area , "motion-notify-event",  G_CALLBACK( img_image_area_motion ), img_struct );
	g_signal_connect(img_struct->image_area , "key-press-event", 		G_CALLBACK(img_image_area_key_press), img_struct);
	//g_signal_connect(img_struct->image_area ), "scroll-event",				G_CALLBACK( img_image_area_scroll ), img_struct);

	//Timer labels and preview buttons
	img_struct->preview_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(vbox), img_struct->preview_hbox, TRUE, FALSE, 10);
	gtk_widget_set_halign(GTK_WIDGET(img_struct->preview_hbox), GTK_ALIGN_CENTER);

	img_struct->current_time = gtk_label_new("00:00:00");
	PangoFontDescription *new_size = pango_font_description_from_string("Sans 10");
	gtk_widget_override_font(img_struct->current_time, new_size);
	gtk_box_pack_start(GTK_BOX(img_struct->preview_hbox), img_struct->current_time, FALSE, FALSE, 10);
	
	beginning = gtk_button_new_from_icon_name("go-first", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(img_struct->preview_hbox), beginning, FALSE, FALSE, 5);
	gtk_widget_set_can_focus(beginning, FALSE);
	gtk_widget_set_name(beginning, "nav_button");
	
	img_struct->preview_button = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(img_struct->preview_hbox), img_struct->preview_button, FALSE, FALSE, 5);
	g_signal_connect (img_struct->preview_button,"clicked",G_CALLBACK (img_start_stop_preview),img_struct);
	gtk_widget_set_can_focus(img_struct->preview_button, FALSE);
	gtk_widget_set_name(img_struct->preview_button, "nav_button");
	
	end = gtk_button_new_from_icon_name("go-last", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(img_struct->preview_hbox), end, FALSE, FALSE, 5);
	gtk_widget_set_can_focus(end, FALSE);
	gtk_widget_set_name(end, "nav_button");
	
	img_struct->total_time = gtk_label_new("00:00:00");
	gtk_widget_override_font(img_struct->total_time, new_size);
	gtk_box_pack_start(GTK_BOX(img_struct->preview_hbox), img_struct->total_time, FALSE, FALSE, 10);

	/* FINALLY the Imagination timeline widget: Yuppieeeeeeeeeeeeee */
	img_struct->timeline = img_timeline_new();
	//gtk_drag_source_set	(img_struct->timeline, GDK_BUTTON1_MASK, timeline_drag_targets, 1, GDK_ACTION_COPY);
	gtk_drag_dest_set		(img_struct->timeline, GTK_DEST_DEFAULT_ALL, timeline_drop_targets, 2, GDK_ACTION_COPY);

	g_object_set(img_struct->timeline, "total_time",  36000, NULL); //10 hours max
	gtk_widget_set_size_request(img_struct->timeline, 36000, -1);
	
	gtk_widget_add_events(img_struct->timeline, 
                           GDK_POINTER_MOTION_MASK
                         | GDK_BUTTON_PRESS_MASK   
                         | GDK_BUTTON_RELEASE_MASK
                         | GDK_SCROLL_MASK
                         | GDK_KEY_PRESS_MASK );

	gtk_widget_set_can_focus(img_struct->timeline, TRUE);
	gtk_widget_grab_focus(img_struct->timeline);
	
	viewport = gtk_viewport_new(NULL,NULL);
	img_struct->timeline_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add (GTK_CONTAINER(viewport), img_struct->timeline);
	gtk_container_add (GTK_CONTAINER(img_struct->timeline_scrolled_window), viewport);
	gtk_paned_pack2(GTK_PANED( img_struct->vpaned ), img_struct->timeline_scrolled_window, TRUE, FALSE);

	g_signal_connect(img_struct->timeline, 	"drag-data-received",		G_CALLBACK(img_timeline_drag_data_received), img_struct);
	g_signal_connect(img_struct->timeline,	"drag-motion", 				G_CALLBACK(img_timeline_drag_motion), img_struct);
	g_signal_connect(img_struct->timeline, 	"drag-leave", 					G_CALLBACK(img_timeline_drag_leave), img_struct);
	//g_signal_connect(img_struct->timeline,	"drag-data-get",				G_CALLBACK(img_timeline_drag_data_get), img_struct->timeline); //
	g_signal_connect(img_struct->timeline, 	"button-press-event",		G_CALLBACK(img_timeline_mouse_button_press), img_struct);
	g_signal_connect(img_struct->timeline, 	"button-release-event",	G_CALLBACK(img_timeline_mouse_button_release), img_struct->timeline);
	g_signal_connect(img_struct->timeline,	"motion-notify-event",	G_CALLBACK(img_timeline_motion_notify), img_struct);
	g_signal_connect(img_struct->timeline,	"key-press-event",			G_CALLBACK(img_timeline_key_press), img_struct);
	g_signal_connect(img_struct->timeline,	"scroll-event",					G_CALLBACK(img_timeline_scroll_event), viewport);

	// Set up CSS styling
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        ".timeline-button { border-width: 0; outline-width: 0; background: #606060; padding: 0; margin: 0; }"
        ".timeline-button:focus { outline-width: 0; }"
        ".timeline-button { border-top: 2px solid transparent; border-bottom: 2px solid transparent; }"
        ".timeline-button:checked { border-top-color: #FFD700; border-bottom-color: #FFD700; }"
        "tooltip  { all: unset; background-color: #FAECC6;  font-weight: normal; border-radius: 5px; border: 1px solid #f4d27b; }"
        "tooltip * {  color: #000000;  padding: 0; margin: 0 }", -1, NULL);
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);
    
	/* Load interface settings or apply default ones */
	if( ! img_load_window_settings( img_struct ) )
		img_set_window_default_settings( img_struct );

	return img_struct;
}

static void img_quit_menu(GtkMenuItem *menuitem, img_window_struct *img)
{
	if( ! img_quit_application( NULL, NULL, img ) )
		gtk_main_quit();
}

void img_combo_box_transition_type_changed (GtkComboBox *combo, img_window_struct *img)
{
	GList        *selected,
				 *bak;
	GtkTreeIter   iter;
	GtkTreeModel *model;
	gpointer      address;
	media_struct *info_slide;
	gint          transition_id;
	GtkTreePath  *p;
	gchar        *path;
	GdkPixbuf    *pix;

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
	//~ model = GTK_TREE_MODEL( img->thumbnail_model );
	//~ bak = selected;
	//~ while (selected)
	//~ {
		//~ gtk_tree_model_get_iter( model, &iter, selected->data );
		//~ gtk_tree_model_get( model, &iter, 1, &info_slide, -1 );
		//~ gtk_list_store_set( GTK_LIST_STORE( model ), &iter, 2, pix, -1 );
		//~ info_slide->render = (ImgRender)address;
		//~ info_slide->transition_id = transition_id;
		//~ g_free( info_slide->path );
		//~ info_slide->path = g_strdup( path );

		//~ selected = selected->next;
	//~ }
	g_free( path );
	if( pix )
		g_object_unref( G_OBJECT( pix ) );
	img_taint_project(img);

	GList *node5;
	for(node5 = bak;node5 != NULL;node5 = node5->next) {
		gtk_tree_path_free(node5->data);
	}
	g_list_free( bak );
}

static void img_random_button_clicked(GtkButton *button, img_window_struct *img)
{
	GList        *selected,
				 *bak;
	GtkTreeIter   iter;
	GtkTreeModel *model;
	media_struct *info_slide;
	GdkPixbuf    *pixbuf;

	//~ model = GTK_TREE_MODEL( img->thumbnail_model );
	//~ selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW (img->thumbnail_iconview));
	//~ if (selected == NULL)
		//~ return;

	/* Avoiding GList memory leak. */
	bak = selected;
	//~ while (selected)
	//~ {
		//~ gtk_tree_model_get_iter(model, &iter,selected->data);
		//~ gtk_tree_model_get(model, &iter,1,&info_slide,-1);
		//~ pixbuf = img_set_random_transition(img, info_slide);
		//~ gtk_list_store_set( GTK_LIST_STORE( model ), &iter, 2, pixbuf, -1 );
		//~ if( pixbuf )
			//~ g_object_unref( G_OBJECT( pixbuf ) );

		//~ selected = selected->next;
	//~ }
	img_taint_project(img);
	GList *node6;
	for(node6 = bak;node6 != NULL;node6 = node6->next) {
		gtk_tree_path_free(node6->data);
	}
	g_list_free(bak);

}

static GdkPixbuf *img_set_random_transition( img_window_struct *img,  media_struct  *info_slide )
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

	g_signal_handlers_block_by_func((gpointer)img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);	
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(img->transition_type), &iter);
	g_signal_handlers_unblock_by_func((gpointer)img->transition_type, (gpointer)img_combo_box_transition_type_changed, img);	

	return( pix );
}

static gint img_sort_none_before_other(GtkTreeModel *model,GtkTreeIter *a,GtkTreeIter *b, gpointer data)
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

static void img_check_numeric_entry (GtkEditable *entry, gchar *text, gint length, gint *position, gpointer  data)
{
	if(*text < '0' || *text > '9')
		g_signal_stop_emission_by_name( (gpointer)entry, "insert-text" );
}

static void img_open_github_bug_report(GtkMenuItem *menuitem , img_window_struct *img)
{
	gtk_show_uri_on_window(GTK_WINDOW(img->imagination_window), "https://github.com/colossus73/imagination/issues", GDK_CURRENT_TIME, NULL);
}

static void img_open_paypal_page(GtkMenuItem *menuitem , img_window_struct *img)
{
	gtk_show_uri_on_window(GTK_WINDOW(img->imagination_window), "https://www.paypal.me/giutor73", GDK_CURRENT_TIME, NULL);
}

static void img_open_documentation(GtkMenuItem *menuitem , img_window_struct *img)
{
	gchar *file = NULL;
	gchar *lang = NULL;
	
	lang = g_strndup(g_getenv("LANG"), 2);
	file = g_strconcat("file://",DATADIR,"/doc/", PACKAGE, "/html/", lang, "/index.html", NULL);
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

void img_text_font_set( GtkFontChooser     *button,
				   img_window_struct *img )
{
	const gchar *string;
	
	string = gtk_font_chooser_get_font( button );

	//img_update_sub_properties( img, NULL, -1, -1, string, NULL, NULL, NULL, NULL, img->current_slide->alignment);

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

	//img_update_sub_properties( img, anim, anim_id, -1, NULL, NULL, NULL, NULL, NULL,  img->current_slide->alignment);

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

	gtk_color_chooser_get_rgba( button, &color );

	gtk_widget_queue_draw( img->image_area );
	img_taint_project(img);
}

void img_font_bg_color_changed( GtkColorChooser    *button,
                          img_window_struct *img )
{
    GdkRGBA color;
    gchar	 *rgb;
	gdouble  font_bgcolor[4];
	gboolean 	selection;
    GtkTextIter start, end;

    gtk_color_chooser_get_rgba( button, &color );
 
    gtk_widget_queue_draw( img->image_area );
    img_taint_project(img);
}

void img_combo_box_anim_speed_changed( GtkSpinButton       *spinbutton,
								  img_window_struct *img )
{
	gint speed;

	speed = gtk_spin_button_get_value_as_int(spinbutton);
	//img_update_sub_properties( img, NULL, -1, speed, NULL, NULL, NULL, NULL, NULL, img->current_slide->alignment);
	img_taint_project(img);
}

static gboolean img_iconview_popup(GtkWidget *widget,  GdkEvent  *event, img_window_struct *img)
{
	GtkWidget *popover, *vbox, *item;
	GdkRectangle rect;
	GtkTreePath *path;
	GtkTreeIter iter;
	double vadj;

	//When the vertical adjustment is greater than 0, therefore the widget content is scrolled down, the popup doesn't show up so I keep it into account
	vadj = gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(img->media_iconview_swindow)));
	rect.x = event->button.x;
	rect.y = event->button.y - vadj;
	rect.width = rect.height = 1; 

	if (event->button.button == 3)
	{
		path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW(widget), event->button.x, event->button.y);
		if (path == NULL)
			goto end;
		
		gtk_tree_model_get_iter(GTK_TREE_MODEL(img->media_model), &iter, path );
		img->popup_iter = iter;
		popover = gtk_popover_new(widget);
		vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		g_object_set (vbox, "margin", 10, NULL);
		item = g_object_new (GTK_TYPE_MODEL_BUTTON, "text", _("Delete"),  NULL);
		g_signal_connect( item, "clicked", G_CALLBACK( img_media_model_remove_media ), img );
		gtk_container_add (GTK_CONTAINER (vbox), item);

		item = g_object_new (GTK_TYPE_MODEL_BUTTON, "visible", TRUE,  "text", _("Properties"),  NULL);
		g_signal_connect( item, "clicked", G_CALLBACK( img_media_show_properties ), img );
		gtk_container_add (GTK_CONTAINER (vbox), item);
		gtk_widget_show (vbox);

		gtk_container_add (GTK_CONTAINER (popover), vbox);
		gtk_popover_set_relative_to(GTK_POPOVER(popover), widget);
		gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
		gtk_widget_show_all(popover);
		gtk_popover_popup(GTK_POPOVER(popover));
		gtk_tree_path_free(path);
		return TRUE;
	}
	end:
	return FALSE;
}

static void img_media_model_remove_media(GtkWidget *widget, img_window_struct *img)
{
    ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *selected, *current;
    gint deletion_count = 0;
    media_struct *media;
    media_timeline *item;
    Track *track;

    model = GTK_TREE_MODEL(img->media_model);
    selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(img->media_iconview));
    if (selected == NULL || img->preview_is_running)
        return;

    // First pass: mark items for deletion and ask for confirmation
    for (current = selected; current != NULL; current = current->next)
    {
        gtk_tree_model_get_iter(model, &iter, current->data);
        gtk_tree_model_get(model, &iter, 2, &media, -1);
        
        gboolean on_timeline = FALSE;
        for (gint i = 0; i < priv->tracks->len; i++)
        {
            track = &g_array_index(priv->tracks, Track, i);
            if (track->items)
            {
                for (gint q = 0; q < track->items->len; q++)
                {
                    item = g_array_index(track->items, media_timeline *, q);
                    if (item->id == media->id)
                    {
                        on_timeline = TRUE;
                        break;
                    }
                }
                if (on_timeline) break;
            }
        }

        if (on_timeline)
        {
            gchar *msg = g_strdup_printf(_("The media <b>%s</b> is currently placed on the timeline. Deleting it will remove all of its instances from the timeline. " \
                "Are you sure you want to do this?"), media->full_path);
            int response = img_ask_user_confirmation(img, msg);
            g_free(msg);
            
            media->to_be_deleted = (response == GTK_RESPONSE_OK);
        }
        else
            media->to_be_deleted = TRUE;
    }
    // Second pass: perform deletions
    current = selected;
    while (current)
    {
        gtk_tree_model_get_iter(model, &iter, current->data);
        gtk_tree_model_get(model, &iter, 2, &media, -1);

        if (media->to_be_deleted)
        {
            for (gint i = 0; i < priv->tracks->len; i++)
            {
                track = &g_array_index(priv->tracks, Track, i);
                if (track->items)
                {
                    for (gint q = track->items->len - 1; q >= 0; q--)
                    {
                        item = g_array_index(track->items, media_timeline *, q);
                        if (item->id == media->id)
                        {
                            gtk_widget_destroy(item->button);
                            g_array_remove_index(track->items, q);
                            g_free(item);
                        }
                    }
                }
            }
            img_free_media_struct(media);
            img->media_nr--;
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            img_taint_project(img);
            
            gtk_tree_path_free(current->data);
		}
        current = current->next;
    }
		g_list_free(selected);
}

static void img_media_show_properties(GtkWidget *widget, img_window_struct *img)
{
	media_struct *media;
	gchar *string, *_string;
	GtkWidget *media_property_dialog, *table1, *label, *scrolled_win, *textview;
	GtkTextBuffer *buffer;
	guint64 filesize;
	GFileInfo *file_info;
	GFile		*file;
	
	//Get the media struct the user clicked on
	gtk_tree_model_get( GTK_TREE_MODEL(img->media_model), &img->popup_iter, 2, &media, -1 );
	
	media_property_dialog = gtk_dialog_new_with_buttons (_("Media Properties"),
									GTK_WINDOW (img->imagination_window),  GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL, NULL);

	g_signal_connect(media_property_dialog,"response",G_CALLBACK(gtk_widget_destroy), 	  NULL);
	g_signal_connect(media_property_dialog,"delete-event",G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_container_set_border_width (GTK_CONTAINER (media_property_dialog), 5);
	gtk_window_set_position (GTK_WINDOW (media_property_dialog),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (media_property_dialog),GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_resizable (GTK_WINDOW (media_property_dialog),FALSE);
	gtk_window_set_modal (GTK_WINDOW (media_property_dialog),TRUE);

	table1 = gtk_grid_new();
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (media_property_dialog))), table1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 6);
	gtk_grid_set_row_spacing (GTK_GRID (table1), 6);
	gtk_grid_set_column_spacing (GTK_GRID (table1), 12);

	label = gtk_label_new (NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Filename:</b>"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_grid_attach (GTK_GRID(table1), label, 0 , 0 , 1 ,1);
	
	label = gtk_label_new(NULL);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	if (media->media_type == 0)
		gtk_label_set_markup(GTK_LABEL(label), _("<b>Size:</b>"));
	else if (media->media_type == 1 || media->media_type == 2)
		gtk_label_set_markup(GTK_LABEL(label), _("<b>Duration:</b>"));

	gtk_grid_attach (GTK_GRID(table1), label, 0 , 1 , 1 ,1);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Type:</b>"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_grid_attach (GTK_GRID(table1), label, 0 , 2 , 1 ,1);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>File size:</b>"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_grid_attach (GTK_GRID(table1), label, 0 , 3 , 1 ,1);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Folder:</b>"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_grid_attach (GTK_GRID(table1), label, 0 , 4 , 1 ,1);

	if (media->media_type == 1)
	{
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), _("<b>Bitrate:</b>"));
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		gtk_grid_attach (GTK_GRID(table1), label, 0 , 5 , 1 ,1);
		
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), _("<b>Sample rate:</b>"));
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		gtk_grid_attach (GTK_GRID(table1), label, 0 , 6 , 1 ,1);
	}
	if (media->media_type == 1 || media->media_type == 2)
	{
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), _("<b>Metadata:</b>"));
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		gtk_label_set_yalign(GTK_LABEL(label), 0.0);
		gtk_grid_attach (GTK_GRID(table1), label, 0 , 7 , 1 ,1);
	}
	
	//Set the values
	string = g_path_get_basename(media->full_path);
	label = gtk_label_new (string);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	g_free(string);
	gtk_grid_attach (GTK_GRID(table1), label, 1 , 0 , 1 ,1);
	
	if (media->media_type == 0)
		string = g_strdup_printf("%d x %d", media->width, media->height);
	else
		string = g_strdup(media->audio_duration);
	label = gtk_label_new (string);
	gtk_grid_attach (GTK_GRID(table1), label, 1 , 1 , 1 ,1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	g_free(string);
	
	if (media->media_type == 0)
		label = gtk_label_new (media->image_type);
	else
		label = gtk_label_new (media->audio_type);
	gtk_grid_attach (GTK_GRID(table1), label, 1 , 2 , 1 ,1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	
	//Get the filesize
	file = g_file_new_for_path (media->full_path);
	file_info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
	filesize = g_file_info_get_size(file_info);
	
	string = g_format_size(filesize);
	label = gtk_label_new (string);
	gtk_grid_attach (GTK_GRID(table1), label, 1 , 3 , 1 ,1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	g_free(string);	
	g_object_unref(file_info);
	g_object_unref(file);

	//Folder
	string = g_path_get_dirname(media->full_path);
	_string = g_strconcat("<a href=\"file://", string, "\">", string, "</a>", NULL);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _string);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_label_set_track_visited_links (GTK_LABEL(label), FALSE);
	g_free(string);
	g_free(_string);
	gtk_grid_attach (GTK_GRID(table1), label, 1 , 4 , 1 ,1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	
	//Bitrate amd sample rate for audio/video files
	if (media->media_type == 1)
	{
		string = 	g_strdup_printf("%ld kbps", media->bitrate);
		label = gtk_label_new (string);
		gtk_grid_attach (GTK_GRID(table1), label, 1 , 5 , 1 ,1);
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		g_free(string);	
		
		string = 	g_strdup_printf("%d Hz", media->sample_rate);
		label = gtk_label_new (string);
		gtk_grid_attach (GTK_GRID(table1), label, 1 , 6 , 1 ,1);
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		g_free(string);	
	}
	// Metadata
	if (media->media_type == 1 || media->media_type == 2)
	{
		textview = gtk_text_view_new();
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		scrolled_win = gtk_scrolled_window_new(NULL, NULL);
		g_object_set (G_OBJECT (scrolled_win),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,NULL);
		gtk_container_add(GTK_CONTAINER (scrolled_win), textview);
		gtk_text_buffer_set_text(buffer, media->metadata, -1);
		gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);
		gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(textview), FALSE);
		gtk_widget_set_size_request(scrolled_win, 250, 200);
		gtk_grid_attach (GTK_GRID(table1), scrolled_win, 1 , 7 , 1 ,1);
	}
	gtk_widget_show_all(media_property_dialog);
}

gboolean img_change_image_area_size (GtkPaned *widget, GtkScrollType scroll_type,  img_window_struct *img)
{
	GtkAllocation allocation;
	GtkWidget *vbox;

	// This is a dirty hack to avoid this code to be executed at startup when there is no project
	// The image area is resized too small
	 
	if (img->media_nr == 0)
		return FALSE;
	
	vbox = gtk_widget_get_parent(GTK_WIDGET(img->image_area));
	gtk_widget_get_allocation(vbox, &allocation);
	
	gint last_pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(img->vpaned), "last-position"));
	gint actual_pos = gtk_paned_get_position(GTK_PANED(img->vpaned));
    gint paned_height = gtk_widget_get_allocated_height(GTK_WIDGET(img->vpaned));

	  // Calculate zoom change based on paned movement relative to its total height
    gdouble zoom_change = (gdouble)(actual_pos - last_pos) / paned_height;
    
    // Adjust zoom factor
    img->image_area_zoom += zoom_change;

    // Constrain zoom factor to reasonable limits (e.g., between 0.1 and 1.0)
    img->image_area_zoom = CLAMP(img->image_area_zoom, 0.1, 1.0);

    // Calculate new dimensions while maintaining aspect ratio
    gint new_width = img->video_size[0] * img->image_area_zoom;
    gint new_height = img->video_size[1] * img->image_area_zoom;
    
    // Get the available space in the window
	gint max_width = allocation.width * 0.85; 
    gint max_height = allocation.height * 0.85;
    
    // Scale down if necessary to fit within the window
    if (new_width > max_width || new_height > max_height)
    {
        gdouble scale = MIN((gdouble)max_width / new_width, (gdouble)max_height / new_height);
        new_width *= scale;
        new_height *= scale;
        img->image_area_zoom *= scale;
    }
    
    gtk_widget_set_size_request(img->image_area, new_width, new_height);
	g_object_set_data(G_OBJECT(widget), "last-position", GINT_TO_POINTER(actual_pos));

	return FALSE;
}
