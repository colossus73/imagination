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
#include <alsa/asoundlib.h>

#define comment_string \
	"Imagination 4.0 Slideshow Project - http://imagination.sf.net"

typedef struct _media_timeline media_timeline;
typedef struct _media_text media_text;

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
								   media_text * );

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
	*/
	
	gint			media_type;
	gint			id;						// This is needed to link the same media when placed on the timeline
	gint			bitrate;
	gint			channels;
	gint			sample_rate;
	gint			width;
	gint			height;
	gchar		*full_path;
	gchar 		*image_type;
	gchar 		*video_type;
	gchar 		*audio_type;
	gchar		*video_duration;
	gchar		*audio_duration;
	gchar		metadata[1024];
    gboolean  to_be_deleted;
	/* Fields that are filled if we create slide in memory */
	gint  gradient;         			/* Gradient type */
	gint 	countdown;					/* Slide countdown */
	gdouble g_start_color[3]; /* RGB start color */
	gdouble g_stop_color[3];  /* RGB stop color */
	gdouble countdown_color[3];  /* RGB countdown color */
	gdouble countdown_angle;  /* Countdown angle being used in the preview and export of such empty slide */
	gdouble g_start_point[2]; /* x, y coordinates of start point */
	gdouble g_stop_point[2];  /* x, y coordinates of stop point */

	/* Ken Burns effect variables */
	GList *points;    /* List with stop points */
	gint   no_points; /* Number of stop points in list */
	gint   cur_point; /* Currently active stop point */
};

/* Textbox related variables */
	enum action
{
    NONE = 0,
    IS_ROTATING,
    IS_DRAGGING,
    IS_RESIZING
};

typedef struct _img_window_struct img_window_struct;
struct _img_window_struct
{
	/* Main GUI related variables */
	GtkAccelGroup *accel_group;
	GtkWidget	*imagination_window;
	GtkWidget 	*menubar;
	GtkWidget 	*sidebar;
	GtkWidget 	*side_notebook;
	GtkWidget 	*toggle_button_image_options;
	GtkWidget 	*toggle_button_text;
	GtkWidget	*open_menu;
	GtkWidget	*open_recent;
	GtkWidget	*no_recent_item_menu;
	GtkWidget	*recent_slideshows;
    GtkWidget   *close_menu;
	GtkWidget	*save_menu;
	GtkWidget	*save_as_menu;
	GtkWidget	*edit_empty_slide;
	GtkWidget	*remove_menu;
	GtkWidget	*timeline;
	GtkWidget 	*preview_hbox;
	GtkWidget	*current_time;
	GtkWidget	*preview_button;
	GtkWidget	*total_time_label;
	GtkWidget	*image_hbox;
	GtkWidget	*transition_type;
	GtkWidget	*random_button;
	GtkWidget	*media_duration;
	GtkWidget 	*effect_combobox;
	GtkWidget	*media_opacity;
	GtkWidget	*media_volume;
	GtkWidget	*timeline_scrolled_window;
  	GtkWidget	*media_option_popover;
  	GtkWidget	*image_area;
  	GtkListStore *media_model;
  	GtkTreeModelFilter *media_model_filter;
  	GtkWidget	*media_library_filter;
  	GtkWidget 	*media_iconview_swindow;
  	GtkWidget 	*media_iconview;
  	GtkTreeIter 	popup_iter;
  	GtkIconTheme *icon_theme;
  	gchar			*current_dir;
  	GdkCursor 	*cursor;										/* Cursor to be stored before going fullscreen */
	GtkWidget   *vpaned;										/* Widget to allow timeline to be shrinked */
	GtkWidget   *hpaned;										/* Widget to allow media library to be shrinked */
	GtkWidget	*transition_hbox;
	GtkWidget	*opacity_hbox;
	GtkWidget	*volume_hbox;

	/* Ken Burns related controls */
	GtkWidget *ken_left;     /* Go to left stop point button */
	GtkWidget *ken_entry;    /* Jump to stop point entry */
	GtkWidget *ken_right;    /* Go to right stop point button */
	GtkWidget *ken_duration; /* Stop point duration spin button */
	GtkWidget *ken_zoom;     /* Zoom slider */
	GtkWidget *ken_add;      /* Add stop point button */
	GtkWidget *ken_remove;   /* Remove stop point button */

	/* Text related controls */
	GtkWidget	*sub_font;
	GtkWidget	*sub_font_color;
	GtkWidget	*sub_font_bg_color;
	GtkWidget	*sub_font_bg_radius;
	GtkWidget	*sub_font_bg_padding;
	GtkWidget	*sub_font_shadow_color; 
	GtkWidget	*sub_font_shadow_distance; 
	GtkWidget	*sub_font_shadow_angle; 
	GtkWidget	*sub_font_outline_color; 
	GtkWidget	*sub_font_outline_size; 
	GtkWidget	*bold_style;
    GtkWidget	*italic_style;
    GtkWidget	*underline_style;
    GtkWidget	*left_justify;
    GtkWidget	*fill_justify;
    GtkWidget	*right_justify;
    GtkWidget	*pattern_image;	  		/* Font Pattern */
	GtkWidget	*sub_anim;         		/* Animation combo box */
	GtkWidget	*sub_anim_duration;	/* Animation duration spin button */
	//GtkWidget *reset_angle;       		/* Button to reset the angle to 0 */
	//GtkWidget *sub_angle;          	/* Text angle hscale range */
	
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
  	media_struct 		*current_media;
  	media_timeline 	*current_item;

	/* Renderers and module stuff */
  	gint				nr_transitions_loaded;
  	gint				next_id;						// This is the id counter when new media are added
  	GSList			*plugin_list;

	/* Project related variables */
	gchar       	*project_filename;		// project name for saving
	gchar       	*slideshow_filename;	// exported slideshow movie
	gchar       	*project_current_dir;
	gboolean		project_is_modified;
	gboolean		relative_filenames;
	gboolean		flip_aspect_ratio;
    gint        		video_size[2];
	gint        		frame_rate;
	gint				total_time;
	gint        		sample_rate;
	gint        		bitrate;
	gdouble   	video_ratio;
    gdouble   	background_color[3];
  	gint				media_nr;

	/* Variables common to export and preview functions */
	GtkWidget		*container_menu;	/* Container combo box in the export dialog */
	GtkWidget		*vcodec_menu;		/* Video codec combo box in the export dialog */
	GtkWidget		*acodec_menu;		/* Audio codec combo box in the export dialog */
	GtkWidget		*video_quality;		/* Combo box to store the quality CRF when enconding */
	GtkWidget		*quality_label;		/* label to be changed when selecting formats which don't require CRF */
	GtkWidget		*file_po;					/* Popover to notify user to choose a slideshow filename */
	GHashTable		*cached_preview_surfaces;	/* GHashTable to store the cached surfaces during the preview */
	cairo_surface_t *current_image;  		/* Image in preview area */
	cairo_surface_t *exported_image; 	/* Image being exported */
	ImgStopPoint    *point1;        			/* Last stop point of image1 */
	ImgStopPoint    *point2;        			/* First stop point of image2 */
  	guint		    	source_id;
  	gboolean	    	gradient_slide;				 /* Flag to allow the hack when transitioning from an empty slide with fade gradient */
	gdouble			transition_progress;
	gdouble			g_stop_color[3]; 			/* Backup stop color to allow the transition from image_from painted with the second color set in the empty slide fade gradient */

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
	gboolean		window_is_fullscreen;
  	gboolean		preview_is_running;
  	GtkWidget	*import_slide_chooser;
	GtkWidget	*total_stop_points_label;
	GtkWidget	*slide_number_entry;

	/* Export dialog related stuff */
	gint        		export_is_running;
	gint 				current_timeline_index;
	gint		     	export_fps;        				/* Frame rate for exported video */
	GtkWidget   *export_pbar1;
	GtkWidget   *export_pbar2;
	GtkWidget   *export_label;
	GtkWidget	*elapsed_time_label;
	GtkWidget   *export_dialog;
	GtkWidget   *export_cancel_button;
	GtkWidget   *export_pause_button;
	gdouble      	elapsed_time;      			/* Elapsed time during export */
	guint        		export_slide;					/* Number of slide being exported */
	GSourceFunc  export_idle_func;			/* Stored procedure for pause */
	GTimer			 *elapsed_timer;				/* GTimer for the elapsed time */

	/* AV library stuff */
	AVFrame 				*video_frame;
	AVFrame 				*audio_frame;
	AVStream				*video_stream;
	AVCodecContext		*codec_context;
	AVPacket					*video_packet;
	AVPacket					*audio_packet;
	AVFormatContext	*video_format_context;
	struct SwsContext	*sws_ctx;
	struct SwrContext	*swr_ctx;
	
	/* Alsa library stuff */
	snd_pcm_t 	*pcm_handle;
	GSList		*media_playing;

	/* Application related stuff */
	gdouble  	image_area_zoom; 	/* Zoom to be applied to image area */
	gint    		preview_fps;     			/* Preview frame rate */
	gboolean	dark_theme;

	/* Clipboard related stuff */
	ImgClipboardMode	clipboard_mode;
};

#endif
