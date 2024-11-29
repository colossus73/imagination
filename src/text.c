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
	g_print("Prima if %p\n",img->current_item->text);
	if (img->current_item->text)
		if ( ! img->current_item->text->visible)
			return FALSE;
g_print("Dopo if %p\n",img->current_item->text);
    img->current_item->text->cursor_visible = ! img->current_item->text->cursor_visible;
    gtk_widget_queue_draw(img->image_area);
    return TRUE;
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
        //~ if (item->action != IS_ROTATING)
            //~ item->visible = FALSE;
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
	gchar *font_name;
	PangoFontDescription *desc;
	
	font_name = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(button));
	desc = pango_font_description_from_string(font_name);
	
	img->current_item->text->attr = pango_attr_font_desc_new(desc);
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
	pango_attr_list_insert(img->current_item->text->attr_list, img->current_item->text->attr);
	pango_font_description_free(desc);
	g_free(font_name);
}

void img_text_color_clicked(GtkWidget *widget, img_window_struct *img)
{
	GtkWidget *chooser, *dialog;
	GdkRGBA color;

	chooser = gtk_color_chooser_widget_new();
	g_object_set(G_OBJECT(chooser), "show-editor", TRUE, NULL);
	gtk_widget_show_all(chooser);
	dialog = gtk_dialog_new_with_buttons(_("Choose custom color"), GTK_WINDOW (img->imagination_window),	GTK_DIALOG_MODAL, _("_Cancel"), GTK_RESPONSE_REJECT, _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	gtk_box_pack_start(GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), chooser, TRUE, TRUE, 5);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (chooser), &color);
		gtk_widget_override_background_color(widget, GTK_STATE_FLAG_NORMAL, &color);
	}
	gtk_widget_destroy (dialog);

	if (GTK_WIDGET(widget) == img->sub_font_color)
		img->current_item->text->attr = pango_attr_foreground_new(color.red * 65535, color.green * 65535, color.blue * 65535);
  //~ else if (GTK_WIDGET(button) == img->italic_style)
		//~ choice = 1;

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
	pango_attr_list_insert(img->current_item->text->attr_list, img->current_item->text->attr);
}

void img_text_style_changed(GtkButton *button, img_window_struct *img)
{
	gint choice;

	img_taint_project(img);

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
		img->current_item->text->attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
		break;
		
		case 1:
		img->current_item->text->attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
		break;

		case 2:
		img->current_item->text->attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
		break;
	}
	
	img->current_item->text->attr->start_index = MIN(img->current_item->text->selection_start, img->current_item->text->selection_end);
	img->current_item->text->attr->end_index 	= MAX(img->current_item->text->selection_start, img->current_item->text->selection_end);

    pango_attr_list_insert(img->current_item->text->attr_list, img->current_item->text->attr);
}

void img_text_align_changed(GtkButton *button, img_window_struct *img)
{
	gint alignment = 0;

	if (img->current_item->text == NULL)
		return;

	if (GTK_WIDGET(button) == img->left_justify)
		alignment = PANGO_ALIGN_LEFT;
	else if (GTK_WIDGET(button) == img->fill_justify)
		alignment = PANGO_ALIGN_CENTER;
	else if (GTK_WIDGET(button) == img->right_justify)
		alignment = PANGO_ALIGN_RIGHT;

	pango_layout_set_alignment (img->current_item->text->layout, alignment);
	
	img_taint_project(img);
}

gboolean img_is_style_applied(PangoLayout *layout, PangoAttrType attr_type, int start, int end)
{
    PangoAttrList *attr_list = pango_layout_get_attributes(layout);
    if (!attr_list) return FALSE;
    
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
                      gdouble		*progress_font_color,
                      gdouble		*progress_font_brdr_color,
					  gdouble	    *progress_font_bgcolor,
					  gdouble		*progress_border_color);

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

void img_render_subtitle( img_window_struct 	  *img,
					 cairo_t			*cr,
					 gdouble			zoom,
					 gint				posx,
					 gint				posy,
					 gint				angle,
					 gint				alignment,
					 gdouble			factor,
					 gdouble			offx,
					 gdouble			offy,
                     gboolean			centerX,
                     gboolean			centerY,
					 gdouble			progress)
{
	gint		 lw,     /* Layout width */
				 lh;     /* Layout height */

	PangoLayout *layout;
	gchar		*text, *string, *dummy;
	GdkColor 	*color;
	GtkTextTag	*tag;

	if ( ! img->current_item->text->text)
		return;

	/* Save cairo state */
	cairo_save( cr );
	cairo_scale( cr, zoom, zoom );

	/* Create pango layout and measure it */
	layout = pango_cairo_create_layout( cr );
	pango_layout_set_font_description( layout, img->current_item->text->font_desc );

	pango_layout_set_text( layout, (gchar*)img->current_item->text->text, -1 );

	pango_layout_get_size( layout, &lw, &lh );
	lw /= PANGO_SCALE;
	lh /= PANGO_SCALE;

	/* Do animation */
	if( img->current_item->text->anim )
		(*img->current_item->text->anim)( cr, layout, img->video_size[0], img->video_size[1], lw, lh, posx, posy, angle, progress, img->current_item->text);
	else
	{
		/* No animation renderer */
        img_text_draw_layout(cr, layout, posx, posy, angle, img->current_item->text);
	}

	/* Destroy layout */
	g_object_unref( G_OBJECT( layout ) );

	/* Restore cairo */
	cairo_restore( cr );
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
    gdouble  progress_font_color[4], progress_font_bgcolor[4], progress_font_shadow_color[4], progress_font_outline_color[4];

	/* Calculate colors */
    progress_font_color[0] = current_media->font_color[0];
    progress_font_color[1] = current_media->font_color[1];
    progress_font_color[2] = current_media->font_color[2];
    progress_font_color[3] = current_media->font_color[3] * progress;

	progress_font_bgcolor[0] = current_media->font_bg_color[0];
    progress_font_bgcolor[1] = current_media->font_bg_color[1];
    progress_font_bgcolor[2] = current_media->font_bg_color[2];
    progress_font_bgcolor[3] = current_media->font_bg_color[3] * pow(progress, 6);

    progress_font_shadow_color[0] = current_media->font_shadow_color[0];
    progress_font_shadow_color[1] = current_media->font_shadow_color[1];
    progress_font_shadow_color[2] = current_media->font_shadow_color[2];
    progress_font_shadow_color[3] = current_media->font_shadow_color[3] * pow(progress, 6);
    
    progress_font_outline_color[0] = current_media->font_outline_color[0];
    progress_font_outline_color[1] = current_media->font_outline_color[1];
    progress_font_outline_color[2] = current_media->font_outline_color[2];
    progress_font_outline_color[3] = current_media->font_outline_color[3];

    /* Paint text */
    img_text_draw_layout_fade(cr, layout, posx, posy, angle, current_media, progress_font_color, progress_font_bgcolor,
						progress_font_shadow_color, progress_font_outline_color);
}

//~ void
//~ img_set_slide_text_info( media_struct      *slide,
						 //~ GtkListStore      *store,
						 //~ GtkTreeIter       *iter,
						 //~ guint8		       *text,
						 //~ gchar				*pattern_filename,
						 //~ gint	            anim_id,
						 //~ gint               anim_duration,
						 //~ gint               posx,
						 //~ gint               posy,
						 //~ gint               angle,
						 //~ const gchar       *font_desc,
						 //~ gdouble           *font_color,
                         //~ gdouble           *font_bg_color,
                         //~ gdouble           *font_shadow_color,
                         //~ gdouble           *font_outline_color,
                         //~ gint	           alignment,
						 //~ img_window_struct *img )
//~ {
	//~ if (pattern_filename)
		//~ slide->pattern_filename = pattern_filename;

	//~ /* Set the slide text info parameters */
	//~ if( ( anim_id > -1 ) && ( anim_id != slide->anim_id ) )
	//~ {
		//~ GtkTreeModel *model;
		//~ gchar        *path;
		//~ GtkTreeIter   iter;

		//~ path = g_strdup_printf( "%d", anim_id );
		//~ model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->sub_anim ) );
		//~ gtk_tree_model_get_iter_from_string( model, &iter, path );
		//~ g_free( path );

		//~ slide->anim_id = anim_id;
		//~ gtk_tree_model_get( model, &iter, 1, &slide->anim, -1 );

		//~ /* Sync timings */
		//~ img_sync_timings( slide, img );
	//~ }

	//~ if( ( anim_duration > 0 ) && ( anim_duration != slide->anim_duration ) )
	//~ {
		//~ slide->anim_duration = anim_duration;

		//~ /* Synchronize timings */
		//~ img_sync_timings( slide, img );
	//~ }

	//~ if (posx > -1 || posy > -1)
	//~ {
		//~ slide->posX = posx;
		//~ slide->posY = posy;
	//~ }
	
	//~ slide->subtitle_angle = angle;

	//~ if( font_desc )
	//~ {
		//~ if( slide->font_desc )
			//~ pango_font_description_free( slide->font_desc );
		//~ slide->font_desc = pango_font_description_from_string( font_desc );
	//~ }

	//~ if( font_color )
	//~ {
		//~ slide->font_color[0] = font_color[0];
		//~ slide->font_color[1] = font_color[1];
		//~ slide->font_color[2] = font_color[2];
		//~ slide->font_color[3] = font_color[3];
	//~ }

    //~ if( font_bg_color )
    //~ {
        //~ slide->font_bg_color[0] = font_bg_color[0];
        //~ slide->font_bg_color[1] = font_bg_color[1];
        //~ slide->font_bg_color[2] = font_bg_color[2];
        //~ slide->font_bg_color[3] = font_bg_color[3];
    //~ }
    
    //~ if( font_shadow_color )
    //~ {
        //~ slide->font_shadow_color[0] = font_shadow_color[0];
        //~ slide->font_shadow_color[1] = font_shadow_color[1];
        //~ slide->font_shadow_color[2] = font_shadow_color[2];
        //~ slide->font_shadow_color[3] = font_shadow_color[3];
    //~ }
    
    //~ if( font_outline_color )
    //~ {
        //~ slide->font_outline_color[0] = font_outline_color[0];
        //~ slide->font_outline_color[1] = font_outline_color[1];
        //~ slide->font_outline_color[2] = font_outline_color[2];
        //~ slide->font_outline_color[3] = font_outline_color[3];
    //~ }
    	
	//~ if (alignment > 0)
		//~ slide->alignment = alignment;
//~ }								

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
		cairo_set_source_rgba( cr, current_media->font_color[0],
								current_media->font_color[1],
								current_media->font_color[2],
								current_media->font_color[3] );
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
                      gdouble		*progress_font_color,
                      gdouble		*progress_font_bg_color,
					  gdouble	    *progress_font_shadow_color,
					  gdouble		*progress_font_outline_color)
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
		cairo_set_source_rgba( cr, progress_font_color[0],
								progress_font_color[1],
								progress_font_color[2],
								progress_font_color[3] );
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
	gtk_widget_get_allocation(img->image_area, &allocation);

	cairo_save(cr);
	if (media->text->visible)
	{
		// Apply rotation for the entire textbox
		cairo_translate(cr, media->text->x + media->text->width / 2, media->text->y + media->text->height / 2);
		cairo_rotate(cr, media->text->angle);
		cairo_translate(cr, -(media->text->x + media->text->width / 2), -(media->text->y + media->text->height / 2));
		
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
	}
		
	// Draw the text
	if (media->text)
	{
		cairo_set_source_rgb(cr, 0, 0, 0);
		pango_layout_set_attributes(media->text->layout, media->text->attr_list);
		cairo_move_to(cr, media->text->x, media->text->y);	
		pango_layout_context_changed(media->text->layout); //This for having the PangoAttr to work
		pango_cairo_show_layout(cr, media->text->layout);
	}
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
	item->duration = (priv->rubber_band_end_x - priv->rubber_band_start_x) / priv->pixels_per_second;//gtk_spin_button_get_value(GTK_SPIN_BUTTON(img->media_duration));
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
	text->font_color[0] = 0;
	text->font_color[1] = 0;
	text->font_color[2] = 0;
	text->font_color[3] = 1;

	text->font_shadow_color[0] = 1;
	text->font_shadow_color[1] = 1;
	text->font_shadow_color[2] = 1;
	text->font_shadow_color[3] = 1;

	text->font_bg_color[0] = 1;
	text->font_bg_color[1] = 1;
	text->font_bg_color[2] = 1;
	text->font_bg_color[3] = 0;

	text->font_outline_color[0] = 1;
	text->font_outline_color[1] = 1;
	text->font_outline_color[2] = 1;
	text->font_outline_color[3] = 1;

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
