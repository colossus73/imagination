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
 
 #include <gtk/gtk.h>
 #include "imagination.h"
 #include "callbacks.h"
 #include "export.h"
 
/* Internal structure, used for creating empty slide */
typedef struct _ImgEmptySlide ImgEmptySlide;
struct _ImgEmptySlide
{
	/* Values */
	gdouble c_start[3];  /* Start color */
	gdouble c_stop[3];   /* Stop color */
	gdouble countdown_color[3];   /* Countdown color */
	gdouble pl_start[2]; /* Linear start point */
	gdouble pl_stop[2];  /* Linear stop point */
	gdouble pr_start[2]; /* Radial start point */
	gdouble pr_stop[2];  /* Radial stop point */
	gint    drag;        /* Are we draging point:
							  0 - no
							  1 - start point
							  2 - stop point */
	gint    gradient;    /* Gradient type:
						      0 - solid color
							  1 - linear
							  2 - radial
							  3 - gradient fading animation 
							  4 - old movie countdown */
	gint countdown;
	gdouble angle;

	/* Widgets */
	GtkWidget *color2;
	GtkWidget *color3;
	GtkWidget *preview;
	GtkWidget *label_countdown;
	GtkWidget *range_countdown;
	GtkWidget *radio[5];
	gdouble		alpha;
	
	/* GSource */
	gint		source;
};

gboolean img_fade_countdown(ImgEmptySlide *slide);
gboolean img_empty_slide_countdown_preview(img_window_struct *img);

static void img_countdown_value_changed (GtkSpinButton *spinbutton, ImgEmptySlide *slide);

static void img_gradient_toggled( GtkToggleButton *button,
					  ImgEmptySlide   *slide );

static void img_gradient_color_set( GtkColorChooser *button,
						ImgEmptySlide  *slide );

gboolean img_gradient_draw( GtkWidget      *widget, cairo_t *cr,  ImgEmptySlide  *slide );

static gboolean img_gradient_press( GtkWidget      *widget,
					GdkEventButton *button,
					ImgEmptySlide  *slide );

static gboolean img_gradient_release( GtkWidget      *widget,
					  GdkEventButton *button,
					  ImgEmptySlide  *slide );

static gboolean img_gradient_move( GtkWidget      *widget,
				   GdkEventMotion *motion,
				   ImgEmptySlide  *slide );
