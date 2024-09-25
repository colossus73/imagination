/*
 *  Copyright (c) 2009-2020 Giuseppe Torelli <colossus73@gmail.com>
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

#include "new_slideshow.h"

GtkWidget		*width;
GtkWidget		*height;

void img_new_slideshow_settings_dialog(img_window_struct *img, gboolean property)
{
	GtkListStore	*liststore;
	GtkTreeIter 	iter;
	GtkWidget		*dialog1;
	GtkWidget		*dialog_vbox1;
	GtkWidget		*vbox1;
	GtkWidget		*iconview;
	GtkWidget		*grid;
	GtkWidget		*ex_hbox;
	GtkWidget		*distort_button;
	GtkWidget		*bg_button;
	GtkWidget		*fourk_button;
	GtkWidget		*eightk_button;
	GtkWidget		*fullhd_button;
	GtkWidget		*hd_button;
	GtkWidget		*label;
	GtkWidget		*image;
	GdkRGBA   		color;
	GdkPixbuf		*icon_pixbuf;
	gint       		response;
	gchar			*title;
    gboolean		c_color;

    if (property)
		title = _("<b><span font='13'>Edit the slideshow</span></b>");
	else
		title = _("<b><span font='13'>Create a new slideshow</span></b>");
	
	dialog1 = gtk_dialog_new_with_buttons(_("Slideshow settings"),
										GTK_WINDOW(img->imagination_window),
										GTK_DIALOG_DESTROY_WITH_PARENT,
										"_Cancel", GTK_RESPONSE_CANCEL,
										"_Ok", GTK_RESPONSE_ACCEPT, NULL);
	dialog_vbox1 = gtk_dialog_get_content_area( GTK_DIALOG( dialog1 ) );
	gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox1), 10);
	
	liststore = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_list_store_append (liststore,&iter);
	image = gtk_image_new_from_file("./icons/imagination.png");
	icon_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
	gtk_list_store_set (liststore, &iter, 0, icon_pixbuf, 1, title, -1);
	if(icon_pixbuf)
		g_object_unref (icon_pixbuf);

	iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL(liststore));
    g_object_unref (liststore);	
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(iconview), GTK_SELECTION_NONE);
	gtk_icon_view_set_item_orientation (GTK_ICON_VIEW (iconview), GTK_ORIENTATION_HORIZONTAL);
	gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (iconview), 0);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW (iconview),1);
	gtk_icon_view_set_markup_column(GTK_ICON_VIEW (iconview), 1);
	gtk_icon_view_set_item_padding(GTK_ICON_VIEW (iconview), 0);
	gtk_box_pack_start(GTK_BOX( dialog_vbox1 ), iconview, FALSE, FALSE, 0);
	
	vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);
 
    gtk_widget_set_margin_top(vbox1, 15);
    gtk_widget_set_margin_bottom(vbox1, 5);
    gtk_widget_set_margin_start(vbox1, 5);
    gtk_widget_set_margin_end(vbox1, 5);
	
	grid = gtk_grid_new();
	gtk_box_pack_start( GTK_BOX( vbox1 ), grid, FALSE, FALSE, 0 );
	gtk_grid_set_row_spacing(GTK_GRID(grid), 7);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

	label = gtk_label_new( _("Width:") );
	gtk_grid_attach( GTK_GRID(grid), label, 0, 0, 1, 1);
	
	width = gtk_spin_button_new_with_range (300, 9999, 10);
	gtk_grid_attach( GTK_GRID(grid), width, 1, 0, 1, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(width), img->video_size[0]);
	
	fourk_button = gtk_button_new_with_label("");
	label = gtk_bin_get_child(GTK_BIN(fourk_button));
	gtk_label_set_markup(GTK_LABEL(label), "8K <b>7680x4320</b>");
	gtk_grid_attach( GTK_GRID(grid), fourk_button, 2, 0, 1, 1);

	eightk_button = gtk_button_new_with_label("");
	label = gtk_bin_get_child(GTK_BIN(eightk_button));
	gtk_label_set_markup(GTK_LABEL(label), "4K <b>4096x2160</b>");
	gtk_grid_attach( GTK_GRID(grid), eightk_button, 3, 0, 1, 1);
	
	label = gtk_label_new( _("Height:") );
	gtk_grid_attach( GTK_GRID(grid), label, 0, 1, 1, 1);

	height = gtk_spin_button_new_with_range(300, 9999, 10);
	gtk_grid_attach( GTK_GRID(grid), height, 1, 1, 1, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(height), img->video_size[1]);
	
	fullhd_button = gtk_button_new_with_label("");
	label = gtk_bin_get_child(GTK_BIN(fullhd_button));
	gtk_label_set_markup(GTK_LABEL(label), "FULL HD <b>1920x1080</b>");
	gtk_grid_attach( GTK_GRID(grid), fullhd_button, 2, 1, 1, 1);

	hd_button = gtk_button_new_with_label("");
	label = gtk_bin_get_child(GTK_BIN(hd_button));
	gtk_label_set_markup(GTK_LABEL(label), "HD <b>1280x720</b>");
	gtk_grid_attach( GTK_GRID(grid), hd_button, 3, 1, 1, 1);
    
    g_signal_connect(G_OBJECT (fourk_button), "clicked", G_CALLBACK(img_new_slideshow_button_clicked), (gpointer)0);
    g_signal_connect(G_OBJECT (eightk_button),"clicked", G_CALLBACK(img_new_slideshow_button_clicked), (gpointer)1);
    g_signal_connect(G_OBJECT (fullhd_button),"clicked", G_CALLBACK(img_new_slideshow_button_clicked), (gpointer)2);
    g_signal_connect(G_OBJECT (hd_button),	  "clicked", G_CALLBACK(img_new_slideshow_button_clicked), (gpointer)3);

	ex_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start( GTK_BOX( vbox1 ), ex_hbox, FALSE, FALSE, 5 );
	gtk_widget_set_margin_top(GTK_WIDGET(ex_hbox), 15);

	distort_button = gtk_switch_new();
	gtk_box_pack_start( GTK_BOX( ex_hbox ), distort_button, FALSE, FALSE, 0 );
	label = gtk_label_new(_("Distort images to fill the whole screen") );
	gtk_box_pack_start( GTK_BOX( ex_hbox ), label, FALSE, FALSE, 0);
	
	ex_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start( GTK_BOX( vbox1 ), ex_hbox, FALSE, FALSE, 0 );
	
	color.red   = img->background_color[0];
	color.green = img->background_color[1];
	color.blue  = img->background_color[2];
	color.alpha = 1.0;

	bg_button = gtk_color_button_new_with_rgba( &color );
	gtk_box_pack_start( GTK_BOX( ex_hbox ), bg_button, FALSE, FALSE, 0 );
	gtk_widget_show_all(dialog_vbox1);

	/* Set parameters */
	gtk_switch_set_active( GTK_SWITCH( distort_button ), img->distort_images );

	response = gtk_dialog_run(GTK_DIALOG(dialog1));

	if (response == GTK_RESPONSE_ACCEPT)
	{
		if (! property)
			img_close_slideshow(NULL, img);
		
		img->video_size[0] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(width));
		img->video_size[1] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(height));
		img->video_ratio = (gdouble)img->video_size[0] / img->video_size[1];

		GdkRGBA new;

		/* Get distorsion settings */
		img->distort_images = gtk_switch_get_active(GTK_SWITCH(distort_button));

		/* Get color settings */
		gtk_color_chooser_get_rgba( GTK_COLOR_CHOOSER(bg_button), &new);
		img->background_color[0] = (gdouble)new.red;
		img->background_color[1] = (gdouble)new.green;
		img->background_color[2] = (gdouble)new.blue;
		c_color = ( color.red   != new.red   ) ||
				  ( color.green != new.green ) ||
				  ( color.blue  != new.blue  );

	}
	gtk_widget_destroy(dialog1);
}

static void img_update_current_slide( img_window_struct *img )
{
	if( ! img->current_slide )
		return;

	cairo_surface_destroy( img->current_image );
	img_scale_image( img->current_slide->full_path, img->video_ratio,
					 0, img->video_size[1], img->distort_images,
					 img->background_color, NULL, &img->current_image );
	gtk_widget_queue_draw( img->image_area );
}

void img_new_slideshow_button_clicked(GtkWidget *button, gpointer video_size)
{
	gint w,h;

	switch (GPOINTER_TO_INT(video_size))
	{
		case 0:
		w = 7680;
		h = 4320;
		break;

		case 1:
		w = 4096;
		h = 2160;
		break;

		case 2:
		w = 1920;
		h = 1080;
		break;

		case 3:
		w = 1280;
		h = 720;
		break;
	}
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(width), (gdouble)w);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(height),(gdouble)h);
}
