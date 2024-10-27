/*
 *  Copyright (c) 2024 Giuseppe Torelli <colossus73@gmail.com>
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
 
 #ifndef IMG_MEDIA_AUDIO_BUTTON_H
#define IMG_MEDIA_AUDIO_BUTTON_H

#include <gtk/gtk.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include "imagination.h"

G_BEGIN_DECLS

typedef enum {
    SAMPLE_FORMAT_UINT8,
    SAMPLE_FORMAT_INT16,
    SAMPLE_FORMAT_INT32,
    SAMPLE_FORMAT_FLOAT,
    SAMPLE_FORMAT_DOUBLE
} SampleFormat;

typedef struct
{
    // FFmpeg context fields
    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int audio_stream_index;
    
    // Audio format information
    SampleFormat format;
    int sample_size;
    int channels;
    int sample_rate;
    
    // Sample data
    float *samples;         			// Normalized float samples (-1.0 to 1.0)
    uint8_t *raw_samples;     	// Original raw sample data
    int num_samples;          	// Number of samples
    int size;               		  		// Size of raw_samples buffer in bytes
    int current_time;
   volatile gint is_playing;
} AudioData;

struct _ImgMediaAudioButton
{
  /*< private >*/
  GtkToggleButton da;
};

typedef struct _ImgMediaAudioButtonPrivate
{
	gchar					*filename;
	AudioData		*audio_data;
	cairo_surface_t 	*waveform_cache;
	gboolean 			cache_valid;
	gdouble 				last_width;
	gdouble 				last_height;
} ImgMediaAudioButtonPrivate;

#define IMG_TYPE_MEDIA_AUDIO_BUTTON (img_media_audio_button_get_type())
G_DECLARE_FINAL_TYPE(ImgMediaAudioButton, img_media_audio_button, IMG, MEDIA_AUDIO_BUTTON, GtkToggleButton)

GtkWidget *img_media_audio_button_new();
ImgMediaAudioButtonPrivate *img_media_audio_button_get_private_struct(ImgMediaAudioButton *);
gboolean img_load_audio_file(ImgMediaAudioButton *, const char *);
int img_play_audio_alsa(AudioData *);
G_END_DECLS

#endif
