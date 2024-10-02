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
	if ( ! img->textbox->visible)
		return TRUE;

    img->textbox->cursor_visible = ! img->textbox->cursor_visible;
    gtk_widget_queue_draw(img->image_area);
    return TRUE;
}

void img_textbox_button_pressed(GdkEventButton *event, img_window_struct *img)
{
	double x, y;
	int index, trailing;
	
    transform_coords(img->textbox, event->x, event->y, &x, &y);

    if (img->textbox->visible && 
        x >= img->textbox->x + (img->textbox->width / 2) - 5 && 
        x <= (img->textbox->x + (img->textbox->width / 2)) + 5 && 
        y >= img->textbox->y + img->textbox->height + 10 && 
        y <= img->textbox->y + img->textbox->height + 20)
    {
        img->textbox->button_pressed = TRUE;
        img->textbox->action = IS_ROTATING;
    }
    //If the left mouse click occurred inside the img->textbox set to true its boolean value
    else if (x >= img->textbox->x && x <= img->textbox->x + img->textbox->width &&
             y >= img->textbox->y && y <= img->textbox->y + img->textbox->height)
    {
        img->textbox->button_pressed = TRUE;
        img->textbox->visible = TRUE;
        img->textbox->cursor_visible = TRUE;
        if (!img->textbox->cursor_source_id)
            img->textbox->cursor_source_id = g_timeout_add(750, (GSourceFunc) blink_cursor, img);
    }
    else
    {
        img->textbox->cursor_visible = FALSE;
         img->textbox->selection_start = img->textbox->selection_end = 0;
        if (img->textbox->cursor_source_id)
        {
            g_source_remove(img->textbox->cursor_source_id);
            img->textbox->cursor_source_id = 0;
        }
        if (img->textbox->action != IS_ROTATING)
            img->textbox->visible = FALSE;
    }

    img->textbox->orig_x = x;
    img->textbox->orig_y = y;
    img->textbox->orig_width = img->textbox->width;
    img->textbox->orig_height = img->textbox->height;
    img->textbox->dragx = x - img->textbox->x;
    img->textbox->dragy = y - img->textbox->y;

   transform_coords(img->textbox, event->x, event->y, &x, &y);
    x -= img->textbox->x;
    y -= img->textbox->y;

    pango_layout_xy_to_index(img->textbox->layout, 
                             (int)(x * PANGO_SCALE), 
                             (int)(y * PANGO_SCALE), 
                             &index, &trailing);
                         
	// Double-click: select word
	if (event->type == GDK_2BUTTON_PRESS)
		select_word_at_position(img->textbox, index); 
    else
	{	
			if (img->textbox->selection_start != -1 && img->textbox->selection_end != -1)
				img->textbox->selection_start = img->textbox->selection_end = -1;
		img->textbox->cursor_pos = index + trailing;
	 }      
	gtk_widget_queue_draw(img->image_area);
}

void img_text_font_changed(GtkFontButton *button, img_window_struct *img )
{
	gchar *font_name;
	PangoFontDescription *desc;
	
	font_name = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(button));
	desc = pango_font_description_from_string(font_name);
	
	img->textbox->attr = pango_attr_font_desc_new(desc);
	if (img->textbox->selection_start == -1 && img->textbox->selection_end == -1)
	{
		img->textbox->attr->start_index = 0;
		img->textbox->attr->end_index = -1;
	}
	else
	{
		img->textbox->attr->start_index = MIN(img->textbox->selection_start, img->textbox->selection_end);
		img->textbox->attr->end_index 	= MAX(img->textbox->selection_start, img->textbox->selection_end);
	}
	pango_attr_list_insert(img->textbox->attr_list, img->textbox->attr);
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
		img->textbox->attr = pango_attr_foreground_new(color.red * 65535, color.green * 65535, color.blue * 65535);
  //~ else if (GTK_WIDGET(button) == img->italic_style)
		//~ choice = 1;

	if (img->textbox->selection_start == -1 && img->textbox->selection_end == -1)
	{
		img->textbox->attr->start_index = 0;
		img->textbox->attr->end_index = -1;
	}
	else
	{
		img->textbox->attr->start_index = MIN(img->textbox->selection_start, img->textbox->selection_end);
		img->textbox->attr->end_index 	= MAX(img->textbox->selection_start, img->textbox->selection_end);
	}
	pango_attr_list_insert(img->textbox->attr_list, img->textbox->attr);
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
		img->textbox->attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
		break;
		
		case 1:
		img->textbox->attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
		break;

		case 2:
		img->textbox->attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
		break;
	}
	
	img->textbox->attr->start_index = MIN(img->textbox->selection_start, img->textbox->selection_end);
	img->textbox->attr->end_index 	= MAX(img->textbox->selection_start, img->textbox->selection_end);

    pango_attr_list_insert(img->textbox->attr_list, img->textbox->attr);
}

void img_text_align_changed(GtkButton *button, img_window_struct *img)
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

	pango_layout_set_alignment (img->textbox->layout, alignment);
	
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
				   media_struct *current_slide);

static void
img_text_draw_layout( cairo_t     *cr,
                      PangoLayout 	*layout,
                      gint			posx,
                      gint 			posy,
                      gint 			angle,
                      media_struct *);

static void
img_text_draw_layout_fade( cairo_t     *cr,
                      PangoLayout *layout,
                      gint         posx,
                      gint         posy,
                      gint         angle,
                      media_struct *current_slide,
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
 					media_struct *slide);

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
					media_struct *current_slide);

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
 					media_struct *slide);


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
 					media_struct *slide);

                
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
 					media_struct *slide);


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
 				media_struct *slide);


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
 					media_struct *slide);


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
 					media_struct *slide);

                      
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

	if ( ! img->current_slide->text)
		return;

	/* Save cairo state */
	cairo_save( cr );
	cairo_scale( cr, zoom, zoom );

	/* Create pango layout and measure it */
	layout = pango_cairo_create_layout( cr );
	pango_layout_set_font_description( layout, img->current_slide->font_desc );

	pango_layout_set_text( layout, (gchar*)img->current_slide->text, -1 );

	pango_layout_get_size( layout, &lw, &lh );
	lw /= PANGO_SCALE;
	lh /= PANGO_SCALE;

	/* Do animation */
	if( img->current_slide->anim )
		(*img->current_slide->anim)( cr, layout, img->video_size[0], img->video_size[1], lw, lh, posx, posy, angle, progress, img->current_slide);
	else
	{
		/* No animation renderer */
        img_text_draw_layout(cr, layout, posx, posy, angle, img->current_slide);
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
				   media_struct *current_slide)
{
    gdouble  progress_font_color[4], progress_font_bgcolor[4], progress_font_shadow_color[4], progress_font_outline_color[4];

	/* Calculate colors */
    progress_font_color[0] = current_slide->font_color[0];
    progress_font_color[1] = current_slide->font_color[1];
    progress_font_color[2] = current_slide->font_color[2];
    progress_font_color[3] = current_slide->font_color[3] * progress;

	progress_font_bgcolor[0] = current_slide->font_bg_color[0];
    progress_font_bgcolor[1] = current_slide->font_bg_color[1];
    progress_font_bgcolor[2] = current_slide->font_bg_color[2];
    progress_font_bgcolor[3] = current_slide->font_bg_color[3] * pow(progress, 6);

    progress_font_shadow_color[0] = current_slide->font_shadow_color[0];
    progress_font_shadow_color[1] = current_slide->font_shadow_color[1];
    progress_font_shadow_color[2] = current_slide->font_shadow_color[2];
    progress_font_shadow_color[3] = current_slide->font_shadow_color[3] * pow(progress, 6);
    
    progress_font_outline_color[0] = current_slide->font_outline_color[0];
    progress_font_outline_color[1] = current_slide->font_outline_color[1];
    progress_font_outline_color[2] = current_slide->font_outline_color[2];
    progress_font_outline_color[3] = current_slide->font_outline_color[3];

    /* Paint text */
    img_text_draw_layout_fade(cr, layout, posx, posy, angle, current_slide, progress_font_color, progress_font_bgcolor,
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
                      media_struct *current_slide)
{
	cairo_pattern_t  *font_pattern = NULL;
    gint x,y,w,h;
	gdouble cairo_factor;

	pango_layout_get_pixel_size (layout, &w, &h );
	pango_layout_set_alignment( layout, current_slide->alignment );

	/* text angle */
	cairo_translate (cr, posx + (w / 2), posy + (h / 2) );
	cairo_rotate (cr, angle * G_PI / 180.0);
	cairo_translate (cr, -(posx + (w / 2)), -(posy + (h / 2)) );
	pango_cairo_update_layout (cr, layout);
	
	/* Set the user chosen pattern
	 * to draw the text */
	if (current_slide->pattern_filename)
	{
		cairo_surface_t  *tmp_surf;
		cairo_matrix_t	matrix;

		tmp_surf = cairo_image_surface_create_from_png(current_slide->pattern_filename);
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
		cairo_set_source_rgba( cr, current_slide->font_color[0],
								current_slide->font_color[1],
								current_slide->font_color[2],
								current_slide->font_color[3] );
	}
    /* Move to proper place and paint text */
	cairo_move_to( cr, posx, posy );
	pango_cairo_show_layout( cr, layout );
	
	if (current_slide->pattern_filename)
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
 					media_struct *current_slide)
{
    img_text_draw_layout(cr, layout,
                         posx * progress - lw * ( 1 - progress ),
                         posy,
                         angle,
                         current_slide);
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
                     media_struct *current_slide)
{
    img_text_draw_layout(cr, layout,
                         posx * progress + sw * ( 1 - progress ),
                         posy,
                         angle,
                        current_slide);
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
				   media_struct *current_slide)
{
    img_text_draw_layout(cr, layout,
                         posx,
                         posy * progress - lh * ( 1 - progress ),
                         angle,
                         current_slide);
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
  					 media_struct *current_slide)
{
    img_text_draw_layout(cr, layout,
                         posx,
                         posy * progress + sh * ( 1 - progress ),
                         angle,
                        current_slide);
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
			  media_struct *current_slide)
{
	cairo_translate( cr, posx + lw * 0.5, posy + lh * 0.5 );
	cairo_scale( cr, exp(log(progress)), exp(log(progress)) );

    img_text_draw_layout(cr, layout,
                         - lw * 0.5,
                         - lh * 0.5,
                         angle,
                         current_slide);
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
				   media_struct *current_slide)
{
    img_text_draw_layout(cr, layout,
                         posx,
                         sh * (1 - progress) - lh * progress,
                         angle,
                         current_slide);
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
				   media_struct *current_slide)
{
    img_text_draw_layout(cr, layout,
                         sw * (1 - progress) - lw * progress,
                         posy, angle,
                         current_slide);
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
				   media_struct *current_slide)
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
                         current_slide);
}

static void
img_text_draw_layout_fade( cairo_t     *cr,
                      PangoLayout *layout,
                      gint         posx,
                      gint         posy,
                      gint         angle,
                      media_struct *current_slide,
                      gdouble		*progress_font_color,
                      gdouble		*progress_font_bg_color,
					  gdouble	    *progress_font_shadow_color,
					  gdouble		*progress_font_outline_color)
{
	cairo_pattern_t  *font_pattern = NULL;
    gint x,y,w,h;
	gdouble cairo_factor;

	pango_layout_get_pixel_size (layout, &w, &h );
	pango_layout_set_alignment( layout, current_slide->alignment );

	/* text angle */
	cairo_translate (cr, posx + (w / 2), posy + (h / 2) );
	cairo_rotate (cr, angle * G_PI / 180.0);
	cairo_translate (cr, -(posx + (w / 2)), -(posy + (h / 2)) );
	pango_cairo_update_layout (cr, layout);

	/* Set the user chosen pattern to draw the text */
	if (current_slide->pattern_filename)
	{
		cairo_surface_t  *tmp_surf;
		cairo_matrix_t	matrix;

		tmp_surf = cairo_image_surface_create_from_png(current_slide->pattern_filename);
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
	
	if (current_slide->pattern_filename)
		cairo_pattern_destroy(font_pattern);
}
