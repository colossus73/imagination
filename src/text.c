/*
** Copyright (C) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "text.h"
#include "support.h"

gboolean blink_cursor(img_window_struct *img)
{
	if (img->current_item->text)
	{
		if ( ! img->current_item->text->visible)
			return G_SOURCE_REMOVE;

		img->current_item->text->cursor_visible = ! img->current_item->text->cursor_visible;
		gtk_widget_queue_draw(img->image_area);
		return G_SOURCE_CONTINUE;
	}
	else
		return G_SOURCE_REMOVE;
}

void img_textbox_button_pressed(GdkEventButton *event, img_window_struct *img)
{
	media_text *item;
	gdouble x, y;
	gint index, trailing;

	item = img->current_item->text;
    transform_coords(item, event->x, event->y, &x, &y);

	if (item->visible && 
		x >= item->x + (item->width / 2) - 5 && 
		x <= (item->x + (item->width / 2)) + 5 && 
		y >= item->y + item->height + 10 && 
		y <= item->y + item->height + 20)
	{
		item->button_pressed = TRUE;
		item->action = IS_ROTATING;
	}
	//If the left mouse click occurred inside the item set to true its boolean value
	else if (x >= item->x && x <= item->x + item->width &&
			y >= item->y && y <= item->y + item->height)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(img->current_item->button), TRUE);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(img->side_notebook), 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(img->toggle_button_text), TRUE);
		
		img_text_item_set_widget_settings(img);
		item->button_pressed = TRUE;
		item->visible = TRUE;
		item->cursor_visible = TRUE;
		if (!item->cursor_source_id)
			item->cursor_source_id = g_timeout_add(750, (GSourceFunc) blink_cursor, img);
	}
	else
	{
		item->visible = FALSE;
		item->cursor_visible = FALSE;
		item->selection_start = item->selection_end = 0;
		if (item->cursor_source_id)
		{
			g_source_remove(item->cursor_source_id);
			item->cursor_source_id = 0;
        }
    }

    item->orig_x = x;
    item->orig_y = y;
    item->orig_width = item->width;
    item->orig_height = item->height;
    item->dragx = x - item->x;
    item->dragy = y - item->y;

   //transform_coords(item, event->x, event->y, &x, &y);
    x -= item->x;
    y -= item->y;

    pango_layout_xy_to_index(item->layout, 
                             (int)(x * PANGO_SCALE), 
                             (int)(y * PANGO_SCALE), 
                             &index, &trailing);
                         
	// Double-click: select word
	if (event->type == GDK_2BUTTON_PRESS)
		select_word_at_position(item, index); 
    else
	{	
			if (item->selection_start != -1 && item->selection_end != -1)
				item->selection_start = item->selection_end = -1;
		item->cursor_pos = index + trailing;
	 }      
	gtk_widget_queue_draw(img->image_area);
}

void img_text_font_changed(GtkFontButton *button, img_window_struct *img )
{
	media_text *text;
	gchar *font_name;
	PangoFontDescription *desc;

	text = img->current_item->text;
	if (text == NULL)
		return;
	
	font_name = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(button));
	desc = pango_font_description_from_string(font_name);
	
	text->attr = pango_attr_font_desc_new(desc);
	if (text->selection_start == -1 && text->selection_end == -1)
	{
		text->attr->start_index = 0;
		text->attr->end_index = -1;
	}
	else
	{
		text->attr->start_index = MIN(text->selection_start, text->selection_end);
		text->attr->end_index 	= MAX(text->selection_start, text->selection_end);
	}
	pango_attr_list_insert(text->attr_list, text->attr);
	pango_font_description_free(desc);
	g_free(font_name);
}

void img_text_color_clicked(GtkWidget *widget, img_window_struct *img)
{
	GtkWidget *chooser, *dialog;
	GdkRGBA color;
	gchar *title = NULL;

	if (img->current_item->media_type != 3)
		return;

	if (widget == img->sub_font_color)
		title = _("Please choose the font color");
	else if (widget == img->sub_font_bg_color)
		title = _("Please choose the background color");
	else if (widget == img->sub_font_shadow_color)
		title = _("Please choose the font shadow color");
	else if (widget == img->sub_font_outline_color)
		title = _("Please choose the font outline color");

	chooser = gtk_color_chooser_widget_new();
	g_object_set(G_OBJECT(chooser), "show-editor", TRUE, NULL);
	gtk_widget_show_all(chooser);
	dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW (img->imagination_window),	GTK_DIALOG_MODAL, _("_Cancel"), GTK_RESPONSE_REJECT, _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	gtk_box_pack_start(GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), chooser, TRUE, TRUE, 5);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (chooser), &color);
		gtk_widget_override_background_color(widget, GTK_STATE_FLAG_NORMAL, &color);
	}
	gtk_widget_destroy (dialog);

	if (GTK_WIDGET(widget) == img->sub_font_color)
	{
		img->current_item->text->attr = pango_attr_foreground_new(color.red * 65535, color.green * 65535, color.blue * 65535);
 
		if (img->current_item->text->selection_start == -1 && img->current_item->text->selection_end == -1)
		{
			img->current_item->text->attr->start_index = 0;
			img->current_item->text->attr->end_index = -1;
		}
		else
		{
			img->current_item->text->attr->start_index = MIN(img->current_item->text->selection_start, img->current_item->text->selection_end);
			img->current_item->text->attr->end_index 	= MAX(img->current_item->text->selection_start, img->current_item->text->selection_end);
		}
		img->current_item->text->font_color.red = color.red;
		img->current_item->text->font_color.green = color.green;
		img->current_item->text->font_color.blue = color.blue;
		img->current_item->text->font_color.alpha = color.alpha;
		pango_attr_list_insert(img->current_item->text->attr_list, img->current_item->text->attr);
	}
	else if (GTK_WIDGET(widget) == img->sub_font_bg_color)
	{
		img->current_item->text->font_bg_color.red = color.red;
		img->current_item->text->font_bg_color.green = color.green;
		img->current_item->text->font_bg_color.blue = color.blue;
		img->current_item->text->font_bg_color.alpha = color.alpha;
	}
	else if (GTK_WIDGET(widget) == img->sub_font_shadow_color)
	{
		img->current_item->text->font_shadow_color.red = color.red;
		img->current_item->text->font_shadow_color.green = color.green;
		img->current_item->text->font_shadow_color.blue = color.blue;
		img->current_item->text->font_shadow_color.alpha = color.alpha;
	}
	else if (GTK_WIDGET(widget) == img->sub_font_outline_color)
	{
		img->current_item->text->font_outline_color.red = color.red;
		img->current_item->text->font_outline_color.green = color.green;
		img->current_item->text->font_outline_color.blue = color.blue;
		img->current_item->text->font_outline_color.alpha = color.alpha;
	}
}

void img_text_style_changed(GtkButton *button, img_window_struct *img)
{
	gint choice;
	media_text *text;
	
	text = img->current_item->text;
	if (text == NULL)
		return;

	/* Which button did the user press? */
	if (GTK_WIDGET(button) == img->bold_style)
		choice = 0;
	else if (GTK_WIDGET(button) == img->italic_style)
		choice = 1;
	else if (GTK_WIDGET(button) == img->underline_style)
		choice = 2;
	else if (GTK_WIDGET(button) == img->left_justify)
		choice = 3;
	else if (GTK_WIDGET(button) == img->fill_justify)
		choice = 4;
	else if (GTK_WIDGET(button) == img->right_justify)
		choice = 5;
	
	switch (choice)
	{
		case 0:
		text->attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
		break;
		
		case 1:
		text->attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
		break;

		case 2:
		text->attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
		break;
	}
	
	text->attr->start_index = MIN(text->selection_start, text->selection_end);
	text->attr->end_index 	= MAX(text->selection_start, text->selection_end);

    pango_attr_list_insert(text->attr_list, text->attr);
    
    img_taint_project(img);
}

void img_text_align_changed(GtkButton *button, img_window_struct *img)
{
	media_text *text;
	
	text = img->current_item->text;
	if (text == NULL)
		return;

	if (GTK_WIDGET(button) == img->left_justify)
		text->alignment = PANGO_ALIGN_LEFT;
	else if (GTK_WIDGET(button) == img->fill_justify)
		text->alignment = PANGO_ALIGN_CENTER;
	else if (GTK_WIDGET(button) == img->right_justify)
		text->alignment = PANGO_ALIGN_RIGHT;

	pango_layout_set_alignment (text->layout, text->alignment);
	gtk_widget_queue_draw(img->image_area);
	img_taint_project(img);
}

void img_text_slider_changed(GtkRange *range, img_window_struct *img)	
{
	media_text *text;
	
	text = img->current_item->text;
	if (text == NULL)
		return;
	
	if (range == GTK_RANGE(img->sub_font_bg_padding))
		text->bg_padding = gtk_range_get_value(GTK_RANGE(img->sub_font_bg_padding));
	else if (range == GTK_RANGE(img->sub_font_bg_radius))
		text->bg_radius = gtk_range_get_value(GTK_RANGE(img->sub_font_bg_radius));
	else if (range == GTK_RANGE(img->sub_font_shadow_distance))
		text->shadow_distance = gtk_range_get_value(GTK_RANGE(img->sub_font_shadow_distance));
	else if (range == GTK_RANGE(img->sub_font_shadow_angle))
		text->shadow_angle = gtk_range_get_value(GTK_RANGE(img->sub_font_shadow_angle));
	else if (range ==GTK_RANGE(img->sub_font_outline_size))
		text->outline_size = gtk_range_get_value(GTK_RANGE(img->sub_font_outline_size));

	gtk_widget_queue_draw(img->image_area);
}

gboolean img_is_style_applied(PangoLayout *layout, PangoAttrType attr_type, int start, int end)
{
    PangoAttrList *attr_list = pango_layout_get_attributes(layout);
    if (!attr_list)
		return FALSE;
    
    PangoAttrIterator *iter = pango_attr_list_get_iterator(attr_list);
    gboolean style_applied = FALSE;
    
    do
    {
        int iter_start, iter_end;
        pango_attr_iterator_range(iter, &iter_start, &iter_end);
        
        if ((iter_start >= start && iter_start < end) ||
            (iter_end > start && iter_end <= end) ||
            (iter_start <= start && iter_end >= end))
            {
				PangoAttribute *attr = pango_attr_iterator_get(iter, attr_type);
				if (attr)
				{
					style_applied = TRUE;
					break;
				}
			}
    } while (pango_attr_iterator_next(iter));
    
    pango_attr_iterator_destroy(iter);
    return style_applied;
}

static void
img_text_ani_fade( cairo_t     *cr,
				   PangoLayout *layout,
				   gint         sw,
				   gint         sh,
				   gint         lw,
				   gint         lh,
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble      progress,
				   media_text *current_media);

static void
img_text_draw_layout( cairo_t     *cr,
                      PangoLayout 	*layout,
                      gint			posx,
                      gint 			posy,
                      gint 			angle,
                      media_text *);

static void
img_text_draw_layout_fade( cairo_t     *cr,
                      PangoLayout *layout,
                      gint         posx,
                      gint         posy,
                      gint         angle,
                      media_text *current_media,
                      GdkRGBA		progress_font_color,
                      GdkRGBA		progress_font_brdr_color,
					  GdkRGBA	    progress_font_bgcolor,
					  GdkRGBA		progress_border_color);

static void
img_text_from_left( cairo_t     *cr,
					PangoLayout *layout,
 					gint         sw,
 					gint         sh,
 					gint         lw,
 					gint         lh,
 					gint         posx,
 					gint         posy,
 					gint         angle,
 					gdouble	progress,
 					media_text *slide);

static void
img_text_from_right( cairo_t     *cr,
					 PangoLayout *layout,
 					 gint         sw,
 					 gint         sh,
 					 gint         lw,
 					 gint         lh,
 					 gint         posx,
 					 gint         posy,
 					 gint         angle,
 					 gdouble	   progress,
					media_text *current_media);

static void
img_text_spin_grow( cairo_t     *cr,
				   PangoLayout *layout,
				   gint         sw,
				   gint         sh,
				   gint         lw,
				   gint         lh,
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble	progress,
 					media_text *slide);


static void
img_text_from_top( cairo_t     *cr,
				   PangoLayout *layout,
				   gint         sw,
				   gint         sh,
				   gint         lw,
				   gint         lh,
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble	progress,
 					media_text *slide);

                
static void
img_text_from_bottom( cairo_t     *cr,
					  PangoLayout *layout,
					  gint         sw,
					  gint         sh,
  					  gint         lw,
  					  gint         lh,
  					  gint         posx,
  					  gint         posy,
  					  gint         angle,
  					 gdouble	progress,
 					media_text *slide);


static void
img_text_grow( cairo_t     *cr,
			   PangoLayout *layout,
			   gint         sw,
			   gint         sh,
			   gint         lw,
			   gint         lh,
			   gint         posx,
			   gint         posy,
			   gint         angle,
				gdouble	progress,
 				media_text *slide);


static void
img_text_bottom_to_top( cairo_t     *cr,
					PangoLayout *layout,
 					gint         sw,
 					gint         sh,
 					gint         lw,
 					gint         lh,
 					gint         posx,
 					gint         posy,
 					gint         angle,
 					gdouble	progress,
 					media_text *slide);


static void
img_text_right_to_left( cairo_t     *cr,
					PangoLayout *layout,
 					gint         sw,
 					gint         sh,
 					gint         lw,
 					gint         lh,
 					gint         posx,
 					gint         posy,
 					gint         angle,
					gdouble	progress,
 					media_text *slide);

                      
gint img_get_text_animation_list( TextAnimation **animations )
{
	TextAnimation *list;              /* List of all animations */
	gint           no_animations = 10; /* Number of animations */
	gint           i = 0;

	if( animations )
	{
		/* Populate list */
		/* DO NOT SHUFFLE THIS LIST! ONLY ADD NEW ANIMATIONS AT THE END OF THE
		 * LIST OR LOADING OF OLD PROJECTS WON'T WORK PROPERLY!!!! */
		list = g_slice_alloc( sizeof( TextAnimation ) * no_animations );

		/* No animation function (id = 0) */
		list[i].name   = g_strdup( _("None") );
		list[i].id     = i;
		list[i++].func = NULL;

		list[i].name   = g_strdup( _("Fade") );
		list[i].id     = i;
		list[i++].func = img_text_ani_fade;

		list[i].name   = g_strdup( _("Slide from left") );
		list[i].id     = i;
		list[i++].func = img_text_from_left;

		list[i].name   = g_strdup( _("Slide from right") );
		list[i].id     = i;
		list[i++].func = img_text_from_right;

		list[i].name   = g_strdup( _("Slide from top") );
		list[i].id     = i;
		list[i++].func = img_text_from_top;

		list[i].name   = g_strdup( _("Slide from bottom") );
		list[i].id     = i;
		list[i++].func = img_text_from_bottom;

		list[i].name   = g_strdup( _("Grow") );
		list[i].id     = i;
		list[i++].func = img_text_grow;

		list[i].name   = g_strdup( _("Spin & Grow") );
		list[i].id     = i;
		list[i++].func = img_text_spin_grow;

        list[i].name   = g_strdup( _("Slide bottom to top") );
        list[i].id     = i;
        list[i++].func = img_text_bottom_to_top;

        list[i].name   = g_strdup( _("Slide right to left") );
        list[i].id     = i;
        list[i++].func = img_text_right_to_left;

		/* FIXME: Add more animations here.
		 *
		 * DO NOT FORGET TO UPDATE no_animations VARIABLE AT THE TOP OF THIS
		 * FUNCTION WHEN ADDING NEW ANIMATIONS!! */
		
		*animations = list;
	}

	return( no_animations );
}

void img_free_text_animation_list( gint           no_animations,
							  TextAnimation *animations )
{
	register gint i;

	for( i = 0; i < no_animations; i++ )
		g_free( animations[i].name );

	g_slice_free1( sizeof( TextAnimation ) * no_animations, animations );
}

static void
img_text_ani_fade( cairo_t     *cr,
				   PangoLayout *layout,
				   gint         sw,
				   gint         sh,
				   gint         lw,
				   gint         lh,
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble      progress,
				   media_text *current_media)
{
    GdkRGBA  progress_font_color, progress_font_bgcolor, progress_font_shadow_color, progress_font_outline_color;

	/* Calculate colors */
    progress_font_color.red = current_media->font_color.red;
    progress_font_color.green = current_media->font_color.green;
    progress_font_color.blue = current_media->font_color.blue;
    progress_font_color.alpha = current_media->font_color.alpha * progress;

	progress_font_bgcolor.red = current_media->font_bg_color.red;
    progress_font_bgcolor.green = current_media->font_bg_color.green;
    progress_font_bgcolor.blue  = current_media->font_bg_color.blue;
    progress_font_bgcolor.alpha = current_media->font_bg_color.alpha * pow(progress, 6);

    progress_font_shadow_color.red = current_media->font_shadow_color.red;
    progress_font_shadow_color.green = current_media->font_shadow_color.green;
    progress_font_shadow_color.blue = current_media->font_shadow_color.blue;
    progress_font_shadow_color.alpha = current_media->font_shadow_color.alpha * pow(progress, 6);
    
    progress_font_outline_color.red = current_media->font_outline_color.red;
    progress_font_outline_color.green= current_media->font_outline_color.green;
    progress_font_outline_color.blue = current_media->font_outline_color.blue;
    progress_font_outline_color.alpha = current_media->font_outline_color.alpha;

    /* Paint text */
    img_text_draw_layout_fade(cr, layout, posx, posy, angle, current_media, progress_font_color, progress_font_bgcolor,
						progress_font_shadow_color, progress_font_outline_color);
}

static void img_text_draw_layout( cairo_t     *cr,
                      PangoLayout *layout,
                      gint         posx,
                      gint         posy,
                      gint         angle,
                      media_text *current_media)
{
	cairo_pattern_t  *font_pattern = NULL;
    gint x,y,w,h;
	gdouble cairo_factor;

	pango_layout_get_pixel_size (layout, &w, &h );
	pango_layout_set_alignment( layout, current_media->alignment );

	/* text angle */
	cairo_translate (cr, posx + (w / 2), posy + (h / 2) );
	cairo_rotate (cr, angle * G_PI / 180.0);
	cairo_translate (cr, -(posx + (w / 2)), -(posy + (h / 2)) );
	pango_cairo_update_layout (cr, layout);
	
	/* Set the user chosen pattern
	 * to draw the text */
	if (current_media->pattern_filename)
	{
		cairo_surface_t  *tmp_surf;
		cairo_matrix_t	matrix;

		tmp_surf = cairo_image_surface_create_from_png(current_media->pattern_filename);
		font_pattern = cairo_pattern_create_for_surface(tmp_surf);
		cairo_set_source(cr, font_pattern);
		cairo_pattern_set_extend (font_pattern, CAIRO_EXTEND_REPEAT);

		cairo_matrix_init_translate(&matrix, -posx, -posy);
		cairo_pattern_set_matrix (font_pattern, &matrix);
	}
	else
	{
		/* Draw the text */
		/* Set source color */
		cairo_set_source_rgba( cr, current_media->font_color.red, current_media->font_color.green, current_media->font_color.blue, current_media->font_color.alpha);
	}
    /* Move to proper place and paint text */
	cairo_move_to( cr, posx, posy );
	pango_cairo_show_layout( cr, layout );
	
	if (current_media->pattern_filename)
		cairo_pattern_destroy(font_pattern);
}

static void
img_text_from_left( cairo_t     *cr,
					PangoLayout *layout,
 					gint         sw,
 					gint         sh,
 					gint         lw,
 					gint         lh,
 					gint         posx,
 					gint         posy,
 					gint         angle,
 					gdouble	progress,
 					media_text*current_media)
{
    img_text_draw_layout(cr, layout,
                         posx * progress - lw * ( 1 - progress ),
                         posy,
                         angle,
                         current_media);
}

static void
img_text_from_right( cairo_t     *cr,
					 PangoLayout *layout,
 					 gint         sw,
 					 gint         sh,
 					 gint         lw,
 					 gint         lh,
 					 gint         posx,
 					 gint         posy,
 					 gint         angle,
 					 gdouble      progress,
                     media_text *current_media)
{
    img_text_draw_layout(cr, layout,
                         posx * progress + sw * ( 1 - progress ),
                         posy,
                         angle,
                        current_media);
}

static void
img_text_from_top( cairo_t     *cr,
				   PangoLayout *layout,
				   gint         sw,
				   gint         sh,
				   gint         lw,
				   gint         lh,
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble      progress,
				   media_text *current_media)
{
    img_text_draw_layout(cr, layout,
                         posx,
                         posy * progress - lh * ( 1 - progress ),
                         angle,
                         current_media);
}

static void
img_text_from_bottom( cairo_t     *cr,
					  PangoLayout *layout,
					  gint         sw,
					  gint         sh,
  					  gint         lw,
  					  gint         lh,
  					  gint         posx,
  					  gint         posy,
  					  gint         angle,
  					  gdouble      progress,
  					 media_text*current_media)
{
    img_text_draw_layout(cr, layout,
                         posx,
                         posy * progress + sh * ( 1 - progress ),
                         angle,
                        current_media);
}

static void
img_text_grow( cairo_t     *cr,
			   PangoLayout *layout,
			   gint         sw,
			   gint         sh,
			   gint         lw,
			   gint         lh,
			   gint         posx,
			   gint         posy,
			   gint         angle,
			   gdouble      progress,
			  media_text *current_media)
{
	cairo_translate( cr, posx + lw * 0.5, posy + lh * 0.5 );
	cairo_scale( cr, exp(log(progress)), exp(log(progress)) );

    img_text_draw_layout(cr, layout,
                         - lw * 0.5,
                         - lh * 0.5,
                         angle,
                         current_media);
}

static void
img_text_bottom_to_top( cairo_t     *cr,
				   PangoLayout *layout,
				   gint         sw,
				   gint         sh,
				   gint         lw,
				   gint         lh,
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble      progress,
				   media_text *current_media)
{
    img_text_draw_layout(cr, layout,
                         posx,
                         sh * (1 - progress) - lh * progress,
                         angle,
                         current_media);
}

static void
img_text_right_to_left( cairo_t     *cr,
				   PangoLayout *layout,
				   gint         sw,
				   gint         sh,
				   gint         lw,
				   gint         lh,
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble      progress,
				   media_text *current_media)
{
    img_text_draw_layout(cr, layout,
                         sw * (1 - progress) - lw * progress,
                         posy, angle,
                         current_media);
}

static void
img_text_spin_grow( cairo_t     *cr,
				   PangoLayout *layout,
				   gint         sw,
				   gint         sh,
				   gint         lw,
				   gint         lh,
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble      progress,
				   media_text *current_media)
{
	gint my_angle;
	my_angle = angle + 360 * exp(log(progress));

	cairo_translate( cr, posx + lw * 0.5, posy + lh * 0.5 );
	cairo_scale( cr, exp(log(progress)), exp(log(progress)) );
	cairo_rotate (cr, my_angle * G_PI / 180.0);

    img_text_draw_layout(cr, layout,
                         - lw * 0.5,
                         - lh * 0.5,
                         my_angle,
                         current_media);
}

static void
img_text_draw_layout_fade( cairo_t     *cr,
                      PangoLayout *layout,
                      gint         posx,
                      gint         posy,
                      gint         angle,
                      media_text *current_media,
                      GdkRGBA		progress_font_color,
                      GdkRGBA		progress_font_bg_color,
					  GdkRGBA	    progress_font_shadow_color,
					  GdkRGBA		progress_font_outline_color)
{
	cairo_pattern_t  *font_pattern = NULL;
    gint x,y,w,h;
	gdouble cairo_factor;

	pango_layout_get_pixel_size (layout, &w, &h );
	pango_layout_set_alignment( layout, current_media->alignment );

	/* text angle */
	cairo_translate (cr, posx + (w / 2), posy + (h / 2) );
	cairo_rotate (cr, angle * G_PI / 180.0);
	cairo_translate (cr, -(posx + (w / 2)), -(posy + (h / 2)) );
	pango_cairo_update_layout (cr, layout);

	/* Set the user chosen pattern to draw the text */
	if (current_media->pattern_filename)
	{
		cairo_surface_t  *tmp_surf;
		cairo_matrix_t	matrix;

		tmp_surf = cairo_image_surface_create_from_png(current_media->pattern_filename);
		font_pattern = cairo_pattern_create_for_surface(tmp_surf);
		cairo_set_source(cr, font_pattern);
		cairo_pattern_set_extend (font_pattern, CAIRO_EXTEND_REPEAT);

		cairo_matrix_init_translate(&matrix, -posx, -posy);
		cairo_pattern_set_matrix (font_pattern, &matrix);
	}
	else
	{
		/* Draw the text */
		/* Set source color */
		cairo_set_source_rgba( cr, progress_font_color.red, progress_font_color.green, progress_font_color.blue, progress_font_color.alpha);
	}
    /* Move to proper place and paint text */
	cairo_move_to( cr, posx, posy );
	pango_cairo_show_layout( cr, layout );
	
	if (current_media->pattern_filename)
		cairo_pattern_destroy(font_pattern);
}

void img_render_textbox(img_window_struct *img, cairo_t *cr, media_timeline *media)
{
	GtkAllocation allocation;
	GtkStyleContext *style_context;
	GdkRGBA color;
	media_text *text;
	
	gtk_widget_get_allocation(img->image_area, &allocation);
	text = img->current_item->text;

	// Apply rotation for the entire textbox
	cairo_save(cr);
	cairo_translate(cr, media->text->x + media->text->width / 2, media->text->y + media->text->height / 2);
	cairo_rotate(cr, media->text->angle);
	cairo_translate(cr, -(media->text->x + media->text->width / 2), -(media->text->y + media->text->height / 2));
	
	if (media->text->visible)
	{
		// Draw the angle
		if (media->text->action == IS_ROTATING && media->text->button_pressed)
			img_draw_rotation_angle(cr, media->text->x, media->text->width, media->text->y, media->text->height, media->text->angle);

		// Set the color to white for the outline effect
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_set_line_width(cr, 3.5);
		
		img_draw_rotating_handle(cr, media->text->x, media->text->width, media->text->y, media->text->height, 1.0, TRUE);
	
		// Draw the rectangle
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_rectangle(cr, media->text->x, media->text->y, media->text->width, media->text->height);
		cairo_stroke(cr);
		cairo_rectangle(cr, media->text->x , media->text->y , media->text->width , media->text->height );
		cairo_stroke_preserve(cr);
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_fill(cr);

		if (media->text->draw_horizontal_line || media->text->draw_vertical_line)
		{
			cairo_save(cr);
				cairo_translate(cr, media->text->x + media->text->width / 2, media->text->y + media->text->height / 2);
				cairo_rotate(cr, -media->text->angle);
				cairo_translate(cr, -(media->text->x + media->text->width / 2), -(media->text->y + media->text->height / 2));
				if (media->text->draw_horizontal_line)
					img_draw_horizontal_line(cr, &allocation);
				if (media->text->draw_vertical_line)
					img_draw_vertical_line(cr, &allocation);
			cairo_restore(cr);
		}
		cairo_set_line_width(cr, 2.5);
			
		// Draw the bottom right handle
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_arc(cr, media->text->x + media->text->width, media->text->y + media->text->height, 3, 0.0, 2 * G_PI);
		cairo_stroke_preserve(cr);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_fill(cr);
		
		// Draw selection highlight
		if (media->text->selection_start != media->text->selection_end)
		{
			int start_index = MIN(media->text->selection_start, media->text->selection_end);
			int end_index = MAX(media->text->selection_start, media->text->selection_end);
				
			PangoRectangle start_pos, end_pos;
			pango_layout_get_cursor_pos(media->text->layout, start_index, &start_pos, NULL);
			pango_layout_get_cursor_pos(media->text->layout, end_index, &end_pos, NULL);

			cairo_set_source_rgba(cr, 0.5, 0.5, 1.0, 0.6);  // Light blue, semi-transparent
			cairo_rectangle(cr, 
				media->text->x + 3 + start_pos.x / PANGO_SCALE, 
				media->text->y + start_pos.y / PANGO_SCALE,
				(end_pos.x - start_pos.x) / PANGO_SCALE, 
				(end_pos.y - start_pos.y + end_pos.height) / PANGO_SCALE);
			cairo_fill(cr);
		}
	}
	
	// Background
	gdouble degrees = G_PI / 180.0;
	if (media->text->bg_padding > 0 || media->text->bg_radius > 0)
	{
		gint width, height;
		PangoRectangle logical_rect, ink_rect;

		pango_layout_get_pixel_size(media->text->layout, &width, &height);
		pango_layout_get_extents(media->text->layout, &ink_rect, &logical_rect);

		// Convert Pango units to pixels
		ink_rect.x /= PANGO_SCALE;
		ink_rect.y /= PANGO_SCALE;
		ink_rect.width /= PANGO_SCALE;
		ink_rect.height /= PANGO_SCALE;

		gdouble total_width = ink_rect.width + (2 * media->text->bg_padding);
		gdouble total_height = ink_rect.height + (2 * media->text->bg_padding);

		cairo_set_source_rgba(cr, media->text->font_bg_color.red, media->text->font_bg_color.green, media->text->font_bg_color.blue, media->text->font_bg_color.alpha);

		cairo_new_sub_path(cr);
		cairo_arc(cr, media->text->x + ink_rect.x + total_width - media->text->bg_radius, media->text->y + ink_rect.y + media->text->bg_radius, media->text->bg_radius, -90 * degrees, 0 * degrees);
		cairo_arc(cr, media->text->x + ink_rect.x + total_width - media->text->bg_radius, media->text->y + ink_rect.y + total_height - media->text->bg_radius, media->text->bg_radius, 0 * degrees, 90 * degrees);
		cairo_arc(cr, media->text->x + ink_rect.x + media->text->bg_radius, media->text->y + ink_rect.y + total_height - media->text->bg_radius, media->text->bg_radius, 90 * degrees, 180 * degrees);
		cairo_arc(cr, media->text->x + ink_rect.x + media->text->bg_radius, media->text->y + ink_rect.y + media->text->bg_radius, media->text->bg_radius, 180 * degrees, 270 * degrees);
		cairo_close_path(cr);
		cairo_fill(cr);
	}
	// Shadow
	if (media->text->shadow_distance > 0 || media->text->shadow_angle > 0)
	{
		gdouble angle_rad = media->text->shadow_angle * (G_PI / 180.0);

		// Calculate x and y offsets
		gdouble offset_x = media->text->x + media->text->shadow_distance * cos(angle_rad);
		gdouble offset_y = media->text->y + media->text->shadow_distance * sin(angle_rad);
	
		gint blur_layers = 5.0 * 2 + 1;
		gdouble base_alpha = media->text->font_shadow_color.alpha / (blur_layers * 1.5);

		// Render blurred shadow layers
		for (int i = 0; i < blur_layers; i++)
		{
			cairo_save(cr);
			// Spread shadow layers
			double layer_spread = i - (blur_layers / 2);
			cairo_translate(cr, offset_x + layer_spread * 0.5, offset_y + layer_spread * 0.5);

			// Set shadow color with reduced alpha
			cairo_set_source_rgba(cr, media->text->font_shadow_color.red, media->text->font_shadow_color.green, media->text->font_shadow_color.blue, base_alpha);

			// Render shadow text
			pango_cairo_layout_path(cr, media->text->layout);
			cairo_fill(cr);

			cairo_restore(cr);
		}
	}

	// Outline
	if (media->text->outline_size > 0)
	{
		// First pass: draw outline
		cairo_set_source_rgba(cr, media->text->font_outline_color.red, media->text->font_outline_color.green, media->text->font_outline_color.blue, media->text->font_outline_color.alpha);
		cairo_set_line_width(cr, media->text->outline_size);

		// Create path for text
		cairo_move_to(cr, media->text->x, media->text->y);
		pango_cairo_layout_path(cr, media->text->layout);
		cairo_stroke(cr);
	}
	// Draw the cursor
	if (media->text->cursor_visible && media->text->cursor_source_id)
	{
		PangoRectangle strong_pos;
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); 
		pango_layout_get_cursor_pos(media->text->layout, media->text->cursor_pos, &strong_pos, NULL);
		cairo_move_to(cr, (double)strong_pos.x / PANGO_SCALE  + 2 + media->text->x, (double)strong_pos.y / PANGO_SCALE + 5 + media->text->y);
		cairo_line_to(cr, (double)strong_pos.x / PANGO_SCALE  + 2 + media->text->x, (double)(strong_pos.y + strong_pos.height) / PANGO_SCALE + 5 + media->text->y);
		cairo_stroke(cr);
	}
	// Draw the text
	cairo_set_source_rgb(cr, 0, 0, 0);
	pango_layout_set_attributes(media->text->layout, media->text->attr_list);
	cairo_move_to(cr, media->text->x + media->text->bg_padding, media->text->y + media->text->bg_padding);	
	pango_layout_context_changed(media->text->layout); //This to have the PangoAttr to work
	pango_cairo_show_layout(cr, media->text->layout);
	
	cairo_restore(cr);
}

media_text *img_create_text_item(img_window_struct *img, gint track_nr, gint track_pos_y)
{
	ImgTimelinePrivate *priv = img_timeline_get_private_struct(img->timeline);
	Track *track = NULL;
	media_timeline *item;
	media_text  *text;
	gint width;
	gdouble position_x;

	position_x = priv->rubber_band_start_x;
	track = g_array_index(priv->tracks, Track *, track_nr);
	
	// First the media_timeline struct
	item = g_new0( media_timeline, 1 );
	item->id = -1;
	item->media_type = 3; 	//Text type
	item->duration = (priv->rubber_band_end_x - priv->rubber_band_start_x) / priv->pixels_per_second;
	item->timeline_y = track_pos_y;
	item->transition_id = -1;
	width = item->duration;
	img_timeline_create_toggle_button(item, item->media_type, NULL, img);
	width *= priv->pixels_per_second;

	item->old_x = position_x;
    item->start_time = position_x / (BASE_SCALE * priv->zoom_scale);
    
   	gtk_widget_set_size_request(item->button, width, 50);
	gtk_layout_move(GTK_LAYOUT(img->timeline), item->button, item->start_time * BASE_SCALE * priv->zoom_scale, item->timeline_y);
	track->last_media_posX += width;

	// Then the media_text struct
	text = g_new0( media_text, 1 );
	text->anim_duration = 1;
	text->posX = 0;
	text->posY = 1;
	text->font_desc = pango_font_description_from_string( "Sans 12" );
	text->alignment = PANGO_ALIGN_LEFT;
	text->font_color.red = 0.0;
	text->font_color.green = 0.0;
	text->font_color.blue = 0.0;
	text->font_color.alpha = 1.0;

	text->font_shadow_color.red = 0.0;
	text->font_shadow_color.green = 0.0;
	text->font_shadow_color.blue = 0.0;
	text->font_shadow_color.alpha = 1.0;

	text->font_bg_color.red = 0.0;
	text->font_bg_color.green = 0.0;
	text->font_bg_color.blue = 0.0;
	text->font_bg_color.alpha = 1.0;

	text->font_outline_color.red = 0.0;
	text->font_outline_color.green = 0.0;
	text->font_outline_color.blue = 0.0;
	text->font_outline_color.alpha = 1.0;

	// Textbox stuff
    text->angle = 0;
    text->text = g_string_new("Sample Text");
    text->x = 275;
    text->y = 162.5;
    text->width = 250;
    text->height = 125;
    text->corner = GDK_LEFT_PTR;
    text->action = NONE;
    text->cursor_pos = 0;
    text->visible = TRUE;
    text->cursor_visible = FALSE;
	text->cursor_source_id = 0;
	text->selection_start = -1;
	text->selection_end = -1;
	text->is_x_snapped = FALSE;
	text->is_y_snapped = FALSE;
	text->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    text->cr = cairo_create(text->surface);
    text->layout = pango_cairo_create_layout(text->cr);
	text->attr_list = pango_attr_list_new();

    pango_layout_set_wrap(text->layout, PANGO_WRAP_CHAR);
    pango_layout_set_ellipsize(text->layout, PANGO_ELLIPSIZE_NONE);
    pango_layout_set_font_description(text->layout, text->font_desc);
    pango_layout_get_size(text->layout, &text->lw, &text->lh);

	text->lw /= PANGO_SCALE;
	text->lh /= PANGO_SCALE;

	// Link the media_text structure to the media_timeline one
	item->text = text;
	img->current_item = item;
	g_array_append_val(track->items, item);

	img_taint_project(img);
	pango_layout_set_text(text->layout, text->text->str, -1);
	gtk_widget_queue_draw(img->image_area);
	return text;
}

void img_text_item_set_widget_settings(img_window_struct *img)
{
	media_text* text;
	
	text = img->current_item->text;

	// Set colors for the color buttons
	gtk_widget_override_background_color(img->sub_font_color, GTK_STATE_FLAG_NORMAL, &text->font_color);
	gtk_widget_override_background_color(img->sub_font_bg_color, GTK_STATE_FLAG_NORMAL, &text->font_bg_color);
	gtk_widget_override_background_color(img->sub_font_shadow_color, GTK_STATE_FLAG_NORMAL, &text->font_shadow_color);
	gtk_widget_override_background_color(img->sub_font_outline_color, GTK_STATE_FLAG_NORMAL, &text->font_outline_color);

	// Background padding
	g_signal_handlers_block_by_func(img->sub_font_bg_padding, img_text_slider_changed, img);  
	gtk_range_set_value(GTK_RANGE(img->sub_font_bg_padding), text->bg_padding);
	g_signal_handlers_unblock_by_func(img->sub_font_bg_padding, img_text_slider_changed, img);
	
	// Background radius
	g_signal_handlers_block_by_func(img->sub_font_bg_radius, img_text_slider_changed, img);  
	gtk_range_set_value(GTK_RANGE(img->sub_font_bg_radius), text->bg_radius);
	g_signal_handlers_unblock_by_func(img->sub_font_bg_radius, img_text_slider_changed, img);
	
	// Shadow distance
	g_signal_handlers_block_by_func(img->sub_font_shadow_distance, img_text_slider_changed, img);  
	gtk_range_set_value(GTK_RANGE(img->sub_font_shadow_distance), text->shadow_distance);
	g_signal_handlers_unblock_by_func(img->sub_font_shadow_distance, img_text_slider_changed, img);
	
	// Shadow angle
	g_signal_handlers_block_by_func(img->sub_font_shadow_angle, img_text_slider_changed, img);  
	gtk_range_set_value(GTK_RANGE(img->sub_font_shadow_angle), text->shadow_angle);
	g_signal_handlers_unblock_by_func(img->sub_font_shadow_angle, img_text_slider_changed, img);
	
	// Outline size
	g_signal_handlers_block_by_func(img->sub_font_outline_size, img_text_slider_changed, img);  
	gtk_range_set_value(GTK_RANGE(img->sub_font_outline_size), text->outline_size);
	g_signal_handlers_unblock_by_func(img->sub_font_outline_size, img_text_slider_changed, img);
}
