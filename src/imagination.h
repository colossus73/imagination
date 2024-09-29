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
 
#ifndef __IMAGINATION_H__
#define __IMAGINATION_H__

#include <stdlib.h>
#include <gtk/gtk.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <cairo.h>

#define comment_string \
	"Imagination 2.0 Slideshow Project - http://imagination.sf.net" //The version number is left to 2.0 to allow compatibility of project files generated with older versions

/* ****************************************************************************
 * Subtitles related definitions
 * ************************************************************************* */

typedef enum
{
	ANGLE_0 = 0,
	ANGLE_90,
	ANGLE_180,
	ANGLE_270
}
ImgAngle;

/*
 * TextAnimationFunc:
 * @cr: cairo context that should be used for drawing
 * @layout: PangoLayout to be rendered
 * @sw: surface width
 * @sh: surface height
 * @lw: layout width
 * @lh: layout height
 * @posx: final position (x coord)
 * @posy: final position (y coord)
 * @progress: progress of animation
 * @font_color: array of RGBA values
 *
 * This is prototype for subtitle animation function. It's task it to render
 * @layout to @cr according to  @progress, @posx and @posy.
 *
 * When @progress > 1, @layout should be drawn at (@posx, @posy) without any
 * scalling.
 */
 
typedef struct _media_struct media_struct;
 
typedef void (*TextAnimationFunc)( cairo_t     *cr,
								   PangoLayout *layout,
								   gint         sw,
								   gint         sh,
								   gint         lw,
								   gint         lh,
								   gint         posx,
								   gint         posy,
								   gint         angle,
								   gdouble  progress,
								   media_struct * );


/* ****************************************************************************
 * Transition related definitions
 * ************************************************************************* */
/* Prototype of transition renderer */
typedef void (*ImgRender)( cairo_t *,
						   cairo_surface_t *,
						   cairo_surface_t *,
						   gdouble );

/* ****************************************************************************
 * Ken Burns effect related definitions
 * ************************************************************************* */
typedef struct _ImgStopPoint ImgStopPoint;
struct _ImgStopPoint
{
	gint    time; /* Duration of this stop point */
	gdouble offx; /* X and Y offsets of zoomed image */
	gdouble offy;
	gdouble zoom; /* Zoom level */
};

#define IMG_CLIPBOARD (gdk_atom_intern_static_string ("IMAGINATIO_OWN_CLIPBOARD")) 
#define IMG_INFO_LIST (gdk_atom_intern_static_string ("application/imagination-info-list"))

typedef enum
{
	IMG_CLIPBOARD_CUT,
	IMG_CLIPBOARD_COPY
} ImgClipboardMode;

typedef struct _media_struct media_struct;
struct _media_struct
{
	/* Media type
	 * 0 - image
	 * 1 - audio
	 * 2 - video
	 * 3 - text
	 * 4 - transition */
	
	gint			media_type;
	gchar		*full_path;
	gchar 		*image_type;
	gchar 		*video_type;
	gchar 		*audio_type;
	gchar		*video_duration;
	gchar		*audio_duration;
	gint			bitrate;
	gint			sample_rate;
	gchar		metadata[1024];
	gint			width;
	gint			height;
	guint64	filesize;

	ImgAngle  angle;      /* Angle of rotated image */
    gboolean  flipped;    /* flag for flipped images */

	/* Fields that are filled if we create slide in memory */
	gint  gradient;         			/* Gradient type */
	gint 	countdown;					/* Slide countdown */
	gdouble g_start_color[3]; /* RGB start color */
	gdouble g_stop_color[3];  /* RGB stop color */
	gdouble countdown_color[3];  /* RGB countdown color */
	gdouble countdown_angle;  /* Countdown angle being used in the preview and export of such empty slide */
	gdouble g_start_point[2]; /* x, y coordinates of start point */
	gdouble g_stop_point[2];  /* x, y coordinates of stop point */

	/* Still part of the slide params */
	gdouble duration; /* Duration of still part */ /* NOTE: sub1 */

	/* Transition params */
	gchar     *path;          			/* Transition model path to transition */
	gint       transition_id; 		/* Transition id */
	ImgRender  render;        	/* Transition render function */

	/* Ken Burns effect variables */
	GList *points;    /* List with stop points */
	gint   no_points; /* Number of stop points in list */
	gint   cur_point; /* Currently active stop point */

	/* Text related variables */
	guint8				 *subtitle;        			/* Subtitle text */
	gsize				 subtitle_length; 			/* Subtitle length */
	gchar			 	 *pattern_filename;	/* Pattern image file */
	TextAnimationFunc     anim;           /* Animation functions */
	gint                  posX;       	   				/* subtitle X position */
	gint                  posY;        	   				/* subtitle Y position */
	gint                  subtitle_angle;  			/* subtitle rotation angle */
	gint                  anim_id;         				/* Animation id */
	gint                  anim_duration;   		/* Duration of animation */
	PangoFontDescription *font_desc;      /* Font description */
	gdouble           font_color[4];   			/* Font color (RGBA format) */
	gdouble           font_bg_color[4]; 		/* Font background color (RGBA format) */
    gdouble           font_shadow_color[4]; 	/* Font shadow color (RGBA format) */
    gdouble           font_outline_color[4]; 	/* Font outline color (RGBA format) */
    gint 					bg_border_radious;			/* text background border radious */
    gint 					bg_padding;				/* text background padding */
    gint               	alignment;
};

/* Textbox related variables */
	enum action
{
    NONE = 0,
    IS_ROTATING,
    IS_DRAGGING,
    IS_RESIZING
};

typedef struct _img_textbox img_textbox;
struct _img_textbox
{
    GtkWidget *drawing_area;
    GString *text;
    gdouble angle;
    gdouble x, y, width, height;
    gdouble orig_x, orig_y, orig_width, orig_height;
    gdouble dragx, dragy;
    gint corner, action, lw, lh;
    guint cursor_source_id;
    gboolean button_pressed;
    gboolean cursor_visible;
    gboolean visible;
    gboolean draw_horizontal_line;
    gboolean draw_vertical_line;
    gint cursor_pos;
    PangoLayout *layout;
    PangoFontDescription *font_desc;
	PangoAttrList *attr_list;
	PangoAttribute *attr;
    gint selection_start;
    gint selection_end;
    gboolean is_x_snapped;
    gboolean is_y_snapped;
};

typedef struct _img_window_struct img_window_struct;
struct _img_window_struct
{
	/* Main GUI related variables */
	GtkAccelGroup *accel_group;
	GtkWidget	*imagination_window;
	GtkWidget 	*menubar;
	GtkWidget *sidebar;
	GtkWidget *side_notebook;
	GtkWidget *toggle_button_text;
	GtkWidget	*open_menu;
	GtkWidget	*open_recent;
	GtkWidget	*no_recent_item_menu;
	GtkWidget	*recent_slideshows;
    GtkWidget   *close_menu;
    GtkWidget   *import_project_menu;
	GtkWidget	*save_menu;
	GtkWidget	*save_as_menu;
	GtkWidget	*edit_empty_slide;
	GtkWidget	*remove_menu;
	GtkWidget	*timeline;
	GtkWidget 	*preview_hbox;
	GtkWidget	*beginning_timer_label;
	GtkWidget	*preview_button;
	GtkWidget	*slideshow_duration;
	GtkWidget	*transition_type;
	GtkWidget	*random_button;
	GtkWidget	*duration;
	GtkWidget	*timeline_scrolled_window;
  	GtkWidget	*media_option_popover;
  	GtkWidget	*image_area;
  	GtkListStore *media_model;
  	GtkWidget 	*media_iconview_swindow;
  	GtkWidget 	*media_iconview;
  	GtkTreeIter popup_iter;
  	GtkIconTheme *icon_theme;
  	gchar		*current_dir;
  	GdkCursor 	*cursor;										/* Cursor to be stored before going fullscreen */
	GtkWidget   *vpaned;										/* Widget to allow timeline to be shrinked */

	/* Ken Burns related controls */
	GtkWidget *ken_left;     /* Go to left stop point button */
	GtkWidget *ken_entry;    /* Jump to stop point entry */
	GtkWidget *ken_right;    /* Go to right stop point button */
	GtkWidget *ken_duration; /* Stop point duration spin button */
	GtkWidget *ken_zoom;     /* Zoom slider */
	GtkWidget *ken_add;      /* Add stop point button */
	GtkWidget *ken_remove;   /* Remove stop point button */

	/* Text related controls */
	GtkWidget *sub_font;        			
	GtkWidget *sub_font_color;    
	GtkWidget *sub_font_bg_color;
	GtkWidget *sub_font_bg_radius;
	GtkWidget *sub_font_bg_padding;
	GtkWidget *sub_font_shadow_color; 
	GtkWidget *sub_font_shadow_distance; 
	GtkWidget *sub_font_shadow_angle; 
	GtkWidget *sub_font_outline_color; 
	GtkWidget *sub_font_outline_size; 
	GtkWidget *bold_style;
    GtkWidget *italic_style;
    GtkWidget *underline_style;
    GtkWidget *left_justify;
    GtkWidget *fill_justify;
    GtkWidget *right_justify;
    GtkWidget *pattern_image;	  /* Font Pattern */
	GtkWidget *sub_anim;          /* Animation combo box */
	GtkWidget *sub_anim_duration; /* Animation duration spin button */
	//GtkWidget *reset_angle;       /* Button to reset the angle to 0 */
	//GtkWidget *sub_angle;          /* Text angle hscale range */
	
	/* Import slides dialog variables */
	GtkWidget	*dim_label;
	GtkWidget	*size_label;
  	GtkWidget	*preview_image;
 
	/* Current image position parameters */
	gdouble       x;             /* Last button press coordinates */
	gdouble       y;
	gdouble       bak_offx;      /* Stored offset at button press */
	gdouble       bak_offy;
	gdouble       maxoffx;       /* Maximal offsets for current zoom */
	gdouble       maxoffy;
	ImgStopPoint  current_point; /* Data for rendering current image */
  	media_struct 		*current_slide;
	img_textbox 	*textbox;

	/* Renderers and module stuff */
  	gint		nr_transitions_loaded;
  	GSList		*plugin_list;

	/* Project related variables */
	gchar       *project_filename;		// project name for saving
	gchar       *slideshow_filename;	// exported slideshow movie
	gchar       *project_current_dir;
	gboolean	project_is_modified;
	gboolean	relative_filenames;
    gint        video_size[2];
	gint        frame_rate;
	//gint        video_quality;
	gint        sample_rate;
	gint        bitrate;
	gdouble     video_ratio;
    gdouble     background_color[3];
  	gdouble		total_secs;
	//gint		total_music_secs;
  	gint		media_nr;

	/* Variables common to export and preview functions */
	GtkWidget		*container_menu;	/* Container combo box in the export dialog */
	GtkWidget		*vcodec_menu;		/* Video codec combo box in the export dialog */
	GtkWidget		*acodec_menu;		/* Audio codec combo box in the export dialog */
	GtkWidget		*video_quality;		/* Combo box to store the quality CRF when enconding */
	GtkWidget		*quality_label;		/* label to be changed when selecting formats which don't require CRF */
	GtkWidget		*file_po;			/* Popover to notify user to choose a slideshow filename */
	cairo_surface_t *current_image;  	/* Image in preview area */
	cairo_surface_t *exported_image; 	/* Image being exported */
	cairo_surface_t *image1;         	/* Original images */
	cairo_surface_t *image2;
	cairo_surface_t *image_from;     	/* Images used in transition rendering */
	cairo_surface_t *image_to;
	ImgStopPoint    *point1;        	/* Last stop point of image1 */
	ImgStopPoint    *point2;        	/* First stop point of image2 */
  	guint		     source_id;
  	gboolean	     gradient_slide; /* Flag to allow the hack when transitioning
										from an empty slide with fade gradient */
	gdouble			g_stop_color[3]; /* Backup stop color to allow the transition
										from image_from painted with the second color
										set in the empty slide fade gradient */

	/* Counters that control animation flow */
	guint  total_nr_frames;    /* Total number of frames */
	guint  displayed_frame;    /* Current frame */
	guint  slide_nr_frames;    /* Number of frames fo current slide */
	guint  slide_cur_frame;    /* Current slide frame */
	guint  slide_trans_frames; /* Number of frames in transition */
	guint  slide_still_frames; /* Number of frames in still part */
	gdouble  next_slide_off;     /* Time offset of next slide */

	gint   still_counter; /* Currently displayed still frame */
	gint   still_max;     /* Number of frames per stop point */
	gint   still_offset;  /* Offset in seconds for next stop point */
	guint  still_cmlt;    /* Cumulative number of still frames */
	GList *cur_point;     /* Current stop point */

	gint   cur_text_frame; /* Current text frame being displayed */
	gint   no_text_frames; /* All text frames */

	/* Preview related variables */
	gboolean	window_is_fullscreen;
  	gboolean	preview_is_running;
  	GtkWidget	*import_slide_chooser;
	GtkWidget	*total_stop_points_label;
  	GtkWidget	*total_slide_number_label;
	GtkWidget	*slide_number_entry;

	/* Export dialog related stuff */
	gint        export_is_running;
	GtkWidget   *export_pbar1;
	GtkWidget   *export_pbar2;
	GtkWidget   *export_label;
	GtkWidget	*elapsed_time_label;
	GtkWidget   *export_dialog;
	GtkWidget   *export_cancel_button;
	GtkWidget   *export_pause_button;
	gdouble      export_fps;        /* Frame rate for exported video */
	gdouble      elapsed_time;      /* Elapsed time during export */
	guint        export_slide;		/* Number of slide being exported */
	GSourceFunc  export_idle_func;	/* Stored procedure for pause */
	GTimer		 *elapsed_timer;	/* GTimer for the elapsed time */

	/* AV library stuff */
	AVFrame 		*video_frame;
	AVFrame 		*audio_frame;
	AVStream		*video_stream;
	AVCodecContext	*codec_context;
	AVPacket		*video_packet;
	AVPacket		*audio_packet;
	AVFormatContext	*video_format_context;
	struct SwsContext *sws_ctx;

	/* Application related stuff */
	gdouble  image_area_zoom; /* Zoom to be applied to image area */
	gint     preview_fps;     /* Preview frame rate */

	/* Clipboard related stuff */
	GList				*selected_paths;
	ImgClipboardMode	clipboard_mode;
};

#endif
