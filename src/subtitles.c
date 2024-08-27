/*
** Copyright (C) 2009-2020 Giuseppe Torelli <colossus73@gmail.com>
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

/* ****************************************************************************
 * Local declarations
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
					 gdouble			UNUSED(factor),
					 gdouble			UNUSED(offx),
					 gdouble			UNUSED(offy),
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

	if (strstr((const gchar*)img->current_slide->subtitle, "GTKTEXTBUFFERCONTENTS-0001"))
	{

		/* Convert the GTK Rich Text Buffer
		 * tags to Pango language markup */
		img->current_slide->subtitle[26] =  32;
		img->current_slide->subtitle[27] =  32;
		img->current_slide->subtitle[28] =  32;
		img->current_slide->subtitle[29] =  32;

		string = g_strdup(strstr((gchar*)img->current_slide->subtitle,"<text>"));
		
		img->current_slide->subtitle[26] =  ((img->current_slide->subtitle_length - 30) >> 24) & 0xFF;
		img->current_slide->subtitle[27] =  ((img->current_slide->subtitle_length - 30) >> 16) & 0xFF;
		img->current_slide->subtitle[28] =  ((img->current_slide->subtitle_length - 30) >> 8 ) & 0xFF;
		img->current_slide->subtitle[29] =  (img->current_slide->subtitle_length - 30) >> 0;

		str_replace(string, "<text>", "");
		str_replace(string, "</text>", "");
		str_replace(string, "</text_view_markup>", "");
		
		str_replace(string, "<apply_tag name=\"underline\"", "<span underline=\"single\"");
		str_replace(string, "<apply_tag name=\"bold\"", "<span font_weight=\"bold\"");
		str_replace(string, "<apply_tag name=\"italic\"", "<span font_style=\"italic\"");
		if (strstr(string, "foreground"))
		{
			str_replace(string, "<apply_tag name=\"foreground\"", "<span foreground=\"rrrrggggbbbb\"");
			tag = gtk_text_tag_table_lookup(img->tag_table, "foreground");
			g_object_get(tag, "foreground-gdk", &color, NULL);
			dummy = gdk_color_to_string(color);
			str_replace(string, "rrrrggggbbbb", dummy);
			g_free(dummy);
		}
		if (strstr(string, "background"))
		{
			str_replace(string, "<apply_tag name=\"background\"", "<span background=\"rrrrggggbbbb\"");
			tag = gtk_text_tag_table_lookup(img->tag_table, "background");
			g_object_get(tag, "background-gdk", &color, NULL);
			dummy = gdk_color_to_string(color);
			str_replace(string, "rrrrggggbbbb", dummy);
			g_free(dummy);
		}
		str_replace(string, "</apply_tag>", "</span>");
		string[strlen(string) - 2] = '\0';

		pango_parse_markup(string, strlen(string), 0, NULL, &text, NULL, NULL);
		pango_layout_set_markup(layout, string, strlen(string));
		pango_layout_set_alignment(layout, alignment);
		pango_layout_set_text( layout, text, -1 );
		g_free(string);
	}
	else
		pango_layout_set_text( layout, (gchar*)img->current_slide->subtitle, -1 );

	pango_layout_get_size( layout, &lw, &lh );
	lw /= PANGO_SCALE;
	lh /= PANGO_SCALE;

	if (centerX)
	{
		posx = (img->video_size[0] - lw) /2;
		img->current_slide->posX = posx;
		gtk_range_set_value( GTK_RANGE(img->sub_posX), (gdouble) img->current_slide->posX);
	}
	if (centerY)
	{
		posy = (img->video_size[1] - lh) /2;
		img->current_slide->posY = posy;
		gtk_range_set_value( GTK_RANGE(img->sub_posY), (gdouble) img->current_slide->posY);
	}

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
				   gint         UNUSED(sw),
				   gint         UNUSED(sh),
				   gint         UNUSED(lw),
				   gint         UNUSED(lh),
				   gint         posx,
				   gint         posy,
				   gint         angle,
				   gdouble      progress,
				   slide_struct *current_slide)
{
    gdouble  progress_font_color[4], progress_font_brdr_color[4], progress_font_bgcolor[4], progress_border_color[4];

	/* Calculate colors */
    progress_font_color[0] = current_slide->font_color[0];
    progress_font_color[1] = current_slide->font_color[1];
    progress_font_color[2] = current_slide->font_color[2];
    progress_font_color[3] = current_slide->font_color[3] * progress;

	progress_font_brdr_color[0] = current_slide->font_brdr_color[0];
    progress_font_brdr_color[1] = current_slide->font_brdr_color[1];
    progress_font_brdr_color[2] = current_slide->font_brdr_color[2];
    progress_font_brdr_color[3] = current_slide->font_brdr_color[3] * pow(progress, 6);

    progress_font_bgcolor[0] = current_slide->font_bg_color[0];
    progress_font_bgcolor[1] = current_slide->font_bg_color[1];
    progress_font_bgcolor[2] = current_slide->font_bg_color[2];
    progress_font_bgcolor[3] = current_slide->font_bg_color[3] * pow(progress, 6);
    
    progress_border_color[0] = current_slide->border_color[0];
    progress_border_color[1] = current_slide->border_color[1];
    progress_border_color[2] = current_slide->border_color[2];
    progress_border_color[3] = current_slide->border_color[3];

    /* Paint text */
    img_text_draw_layout_fade(cr, layout, posx, posy, angle, current_slide, progress_font_color, progress_font_brdr_color,
						progress_font_bgcolor, progress_border_color);
}

void
img_set_slide_text_info( slide_struct      *slide,
						 GtkListStore      * UNUSED(store),
						 GtkTreeIter       * UNUSED(iter),
						 guint8		       * UNUSED(subtitle),
						 gchar				*pattern_filename,
						 gint	            anim_id,
						 gint               anim_duration,
						 gint               posx,
						 gint               posy,
						 gint               angle,
						 const gchar       *font_desc,
						 gdouble           *font_color,
                         gdouble           *font_brdr_color,
                         gdouble           *font_bg_color,
                         gdouble           *border_color,
                         gboolean			top_border,
						 gboolean			bottom_border,
                         gint	           alignment,
						 gint	           border_width,
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

    if( font_brdr_color )
    {
        slide->font_brdr_color[0] = font_brdr_color[0];
        slide->font_brdr_color[1] = font_brdr_color[1];
        slide->font_brdr_color[2] = font_brdr_color[2];
        slide->font_brdr_color[3] = font_brdr_color[3];
    }
    
    if( font_bg_color )
    {
        slide->font_bg_color[0] = font_bg_color[0];
        slide->font_bg_color[1] = font_bg_color[1];
        slide->font_bg_color[2] = font_bg_color[2];
        slide->font_bg_color[3] = font_bg_color[3];
    }
    
    if( border_color )
    {
        slide->border_color[0] = border_color[0];
        slide->border_color[1] = border_color[1];
        slide->border_color[2] = border_color[2];
        slide->border_color[3] = border_color[3];
    }
    
    slide->top_border = top_border;
    slide->bottom_border = bottom_border;
    
	if( border_width > 0)
		slide->border_width = border_width;
	
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

	if (current_slide->top_border || current_slide->bottom_border)
	{
		cairo_set_line_width(cr, (gdouble) current_slide->border_width);
		cairo_set_source_rgba(cr, current_slide->border_color[0],
							  current_slide->border_color[1],
                              current_slide->border_color[2],
                              current_slide->border_color[3]);
        
        if (current_slide->border_width % 2 == 0)
			cairo_factor = 0;
		else
			cairo_factor = 0.5;

        if (current_slide->top_border)
		{
			cairo_move_to(cr, posx - 5, posy + cairo_factor);  
			cairo_line_to(cr, (posx + 3) + w + 4, posy + cairo_factor);
		}
		if (current_slide->bottom_border)
		{
			cairo_move_to(cr, posx - 5, posy + h + 5 + cairo_factor);  
			cairo_line_to(cr, (posx + 3) + w + 4, posy + h + 5 + cairo_factor);
		}
		cairo_stroke(cr);
	}

	/* Draw the border only if the user
	 * chose an alpha value greater than 0 */
	if (current_slide->font_brdr_color[3] > 0)
	{
		cairo_set_source_rgba(cr, current_slide->font_brdr_color[0],
								  current_slide->font_brdr_color[1],
								  current_slide->font_brdr_color[2],
								  current_slide->font_brdr_color[3] );
		for (x=-1; x <=1; x++)
		{
			for (y=-1; y<=1; y++)
			{
				cairo_move_to( cr, posx + x, posy + y );
				pango_cairo_show_layout( cr, layout );
			}
		}
	}
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
 					gint         UNUSED(sw),
 					gint         UNUSED(sh),
 					gint         lw,
 					gint         UNUSED(lh),
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
 					 gint         UNUSED(sh),
 					 gint         UNUSED(lw),
 					 gint         UNUSED(lh),
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
				   gint         UNUSED(sw),
				   gint         UNUSED(sh),
				   gint         UNUSED(lw),
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
					  gint         UNUSED(sw),
					  gint         sh,
  					  gint         UNUSED(lw),
  					  gint         UNUSED(lh),
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
			   gint         UNUSED(sw),
			   gint         UNUSED(sh),
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
				   gint         UNUSED(sw),
				   gint         sh,
				   gint         UNUSED(lw),
				   gint         lh,
				   gint         posx,
				   gint         UNUSED(posy),
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
				   gint         UNUSED(sh),
				   gint         lw,
				   gint         UNUSED(lh),
				   gint         UNUSED(posx),
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
				   gint         UNUSED(sw),
				   gint         UNUSED(sh),
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
                      gdouble		*progress_font_brdr_color,
					  gdouble	    *progress_font_bgcolor,
					  gdouble		*progress_border_color)
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

	if (current_slide->top_border || current_slide->bottom_border)
	{
		cairo_set_line_width(cr, (gdouble) current_slide->border_width);
		cairo_set_source_rgba(cr, current_slide->border_color[0],
							  current_slide->border_color[1],
                              current_slide->border_color[2],
                              current_slide->border_color[3]);
        
        if (current_slide->border_width % 2 == 0)
			cairo_factor = 0;
		else
			cairo_factor = 0.5;

        if (current_slide->top_border)
		{
			cairo_move_to(cr, posx - 5, posy + cairo_factor);  
			cairo_line_to(cr, (posx + 3) + w + 4, posy + cairo_factor);
		}
		if (current_slide->bottom_border)
		{
			cairo_move_to(cr, posx - 5, posy + h + 5 + cairo_factor);  
			cairo_line_to(cr, (posx + 3) + w + 4, posy + h + 5 + cairo_factor);
		}
		cairo_stroke(cr);
	}

	/* Draw the border only if the user
	 * chose an alpha value greater than 0 */
	if (progress_font_brdr_color[3] > 0)
	{
		cairo_set_source_rgba(cr, progress_font_brdr_color[0],
								  progress_font_brdr_color[1],
								  progress_font_brdr_color[2],
								  progress_font_brdr_color[3] );
		for (x=-1; x <=1; x++)
		{
			for (y=-1; y<=1; y++)
			{
				cairo_move_to( cr, posx + x, posy + y );
				pango_cairo_show_layout( cr, layout );
			}
		}
	}
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
