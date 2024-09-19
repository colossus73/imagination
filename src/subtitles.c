/*
** Copyright (C) 2009-2024 Giuseppe Torelli <colossus73@gmail.com>
** Copyright (C) 2009 Tadej Borovšak <tadeboro@gmail.com>
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

#include "subtitles.h"
#include "support.h"

gboolean blink_cursor(img_window_struct *img)
{
	if ( ! img->textbox->draw_rect)
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

    if (img->textbox->draw_rect && 
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
        img->textbox->draw_rect = TRUE;
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
            img->textbox->draw_rect = FALSE;
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
		img->textbox->cursor_pos = index + trailing;
		        
	gtk_widget_queue_draw(img->image_area);
}

void img_draw_textbox(cairo_t *cr, img_window_struct *img)
{
	int width, height;
	
	width  = gtk_widget_get_allocated_width(img->image_area);
	height = gtk_widget_get_allocated_height(img->image_area);

	//cairo_save(cr);
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

		//Draw the horizontal centering line
		if (img->textbox->draw_horizontal_line)
		{
			cairo_save(cr);
			cairo_matrix_t identity;
			cairo_matrix_init_identity(&identity);
			cairo_set_matrix(cr, &identity);

			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_set_line_width(cr, 1.0);
			cairo_move_to(cr, 0, height - 2);
			cairo_line_to(cr, width*2, height - 2);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0.8, 0.7, 0.3);
			cairo_move_to(cr, 0, height);
			cairo_line_to(cr, width*2, height);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_move_to(cr, 0, height + 2);
			cairo_line_to(cr, width*2, height + 2);
			cairo_stroke(cr);
			cairo_restore(cr);
		}
		
		//~ // Draw the vertical centering line
		//~ if (img->textbox->draw_vertical_line)
		//~ {
			//~ cairo_save(cr);
			//~ cairo_matrix_t identity;
			//~ cairo_matrix_init_identity(&identity);
			//~ cairo_set_matrix(cr, &identity);
			
			//~ cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			//~ cairo_set_line_width(cr, 1.0);
			//~ cairo_move_to(cr, width-2, 0);
			//~ cairo_line_to(cr, width-2, height*2);
			//~ cairo_stroke(cr);
			//~ cairo_set_source_rgb(cr, 0.8, 0.7, 0.3);
			//~ cairo_move_to(cr, width, 0);
			//~ cairo_line_to(cr, width, height*2);
			//~ cairo_stroke(cr);
			//~ cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			//~ cairo_move_to(cr, width+2, 0);
			//~ cairo_line_to(cr, width+2, height*2);
			//~ cairo_stroke(cr);
			//~ cairo_restore(cr);
		//~ }
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
	
	//cairo_restore(cr);
	//gtk_widget_queue_draw(img->image_area);
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
				   slide_struct *current_slide);

static void
img_text_draw_layout( cairo_t     *cr,
                      PangoLayout 	*layout,
                      gint			posx,
                      gint 			posy,
                      gint 			angle,
                      slide_struct *);

static void
img_text_draw_layout_fade( cairo_t     *cr,
                      PangoLayout *layout,
                      gint         posx,
                      gint         posy,
                      gint         angle,
                      slide_struct *current_slide,
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
 					slide_struct *slide);

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
					slide_struct *current_slide);

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
 					slide_struct *slide);


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
 					slide_struct *slide);

                
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
 					slide_struct *slide);


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
 				slide_struct *slide);


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
 					slide_struct *slide);


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
 					slide_struct *slide);

                      
/* ****************************************************************************
 * Function definitions
 * ************************************************************************* */

/*
 * img_get_text_animation_list:
 * @animations: location to put list of available text animations
 *
 * This function is here to simplify accessing all available animations.
 *
 * Any newly added exporters should be listed in array returned by this function
 * or Imagination WILL NOT create combo box entries for them.
 *
 * List that is placed in exporters parameter should be considered read-only and
 * freed after usage with img_free_text_animation_list. If @animations is NULL,
 * only number of available animations is returned.
 *
 * Return value: Size of list in animations.
 */
gint
img_get_text_animation_list( TextAnimation **animations )
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

/*
 * img_free_text_animation_list:
 * @no_animations: number of animations in @animations
 * @animations: array of TextAnimation structs
 *
 * This function takes care of freeing any memory allocated by
 * img_get_text_animation_list function.
 */
void
img_free_text_animation_list( gint           no_animations,
							  TextAnimation *animations )
{
	register gint i;

	for( i = 0; i < no_animations; i++ )
		g_free( animations[i].name );

	g_slice_free1( sizeof( TextAnimation ) * no_animations, animations );
}

void
img_render_subtitle( img_window_struct 	  *img,
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

	if ( ! img->current_slide->subtitle)
		return;

	/* Save cairo state */
	cairo_save( cr );
	cairo_scale( cr, zoom, zoom );

	/* Create pango layout and measure it */
	layout = pango_cairo_create_layout( cr );
	pango_layout_set_font_description( layout, img->current_slide->font_desc );

	pango_layout_set_text( layout, (gchar*)img->current_slide->subtitle, -1 );

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

/* ****************************************************************************
 * Text animation renderers
 * ************************************************************************* */
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
				   slide_struct *current_slide)
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

void
img_set_slide_text_info( slide_struct      *slide,
						 GtkListStore      *store,
						 GtkTreeIter       *iter,
						 guint8		       *subtitle,
						 gchar				*pattern_filename,
						 gint	            anim_id,
						 gint               anim_duration,
						 gint               posx,
						 gint               posy,
						 gint               angle,
						 const gchar       *font_desc,
						 gdouble           *font_color,
                         gdouble           *font_bg_color,
                         gdouble           *font_shadow_color,
                         gdouble           *font_outline_color,
                         gint	           alignment,
						 img_window_struct *img )
{
	if (pattern_filename)
		slide->pattern_filename = pattern_filename;

	/* Set the slide text info parameters */
	if( ( anim_id > -1 ) && ( anim_id != slide->anim_id ) )
	{
		GtkTreeModel *model;
		gchar        *path;
		GtkTreeIter   iter;

		path = g_strdup_printf( "%d", anim_id );
		model = gtk_combo_box_get_model( GTK_COMBO_BOX( img->sub_anim ) );
		gtk_tree_model_get_iter_from_string( model, &iter, path );
		g_free( path );

		slide->anim_id = anim_id;
		gtk_tree_model_get( model, &iter, 1, &slide->anim, -1 );

		/* Sync timings */
		img_sync_timings( slide, img );
	}

	if( ( anim_duration > 0 ) && ( anim_duration != slide->anim_duration ) )
	{
		slide->anim_duration = anim_duration;

		/* Synchronize timings */
		img_sync_timings( slide, img );
	}

	if (posx > -1 || posy > -1)
	{
		slide->posX = posx;
		slide->posY = posy;
	}
	
	slide->subtitle_angle = angle;

	if( font_desc )
	{
		if( slide->font_desc )
			pango_font_description_free( slide->font_desc );
		slide->font_desc = pango_font_description_from_string( font_desc );
	}

	if( font_color )
	{
		slide->font_color[0] = font_color[0];
		slide->font_color[1] = font_color[1];
		slide->font_color[2] = font_color[2];
		slide->font_color[3] = font_color[3];
	}

    if( font_bg_color )
    {
        slide->font_bg_color[0] = font_bg_color[0];
        slide->font_bg_color[1] = font_bg_color[1];
        slide->font_bg_color[2] = font_bg_color[2];
        slide->font_bg_color[3] = font_bg_color[3];
    }
    
    if( font_shadow_color )
    {
        slide->font_shadow_color[0] = font_shadow_color[0];
        slide->font_shadow_color[1] = font_shadow_color[1];
        slide->font_shadow_color[2] = font_shadow_color[2];
        slide->font_shadow_color[3] = font_shadow_color[3];
    }
    
    if( font_outline_color )
    {
        slide->font_outline_color[0] = font_outline_color[0];
        slide->font_outline_color[1] = font_outline_color[1];
        slide->font_outline_color[2] = font_outline_color[2];
        slide->font_outline_color[3] = font_outline_color[3];
    }
    	
	if (alignment > 0)
		slide->alignment = alignment;
}								

static void
img_text_draw_layout( cairo_t     *cr,
                      PangoLayout *layout,
                      gint         posx,
                      gint         posy,
                      gint         angle,
                      slide_struct *current_slide)
{
	cairo_pattern_t  *font_pattern = NULL;
    gint x,y,w,h;
	gdouble cairo_factor;

	pango_layout_get_pixel_size (layout, &w, &h );
	pango_layout_set_alignment( layout, current_slide->alignment );

	/* Subtitle angle */
	cairo_translate (cr, posx + (w / 2), posy + (h / 2) );
	cairo_rotate (cr, angle * G_PI / 180.0);
	cairo_translate (cr, -(posx + (w / 2)), -(posy + (h / 2)) );
	pango_cairo_update_layout (cr, layout);
	
	/* Set the user chosen pattern
	 * to draw the subtitle */
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
		/* Draw the subtitle */
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
 					slide_struct *current_slide)
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
                     slide_struct *current_slide)
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
				   slide_struct *current_slide)
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
  					 slide_struct *current_slide)
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
			  slide_struct *current_slide)
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
				   slide_struct *current_slide)
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
				   slide_struct *current_slide)
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
				   slide_struct *current_slide)
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
                      slide_struct *current_slide,
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

	/* Subtitle angle */
	cairo_translate (cr, posx + (w / 2), posy + (h / 2) );
	cairo_rotate (cr, angle * G_PI / 180.0);
	cairo_translate (cr, -(posx + (w / 2)), -(posy + (h / 2)) );
	pango_cairo_update_layout (cr, layout);

	/* Set the user chosen pattern to draw the subtitle */
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
		/* Draw the subtitle */
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
