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
 
#include "audio.h"

G_DEFINE_TYPE_WITH_PRIVATE(ImgMediaAudioButton, img_media_audio_button, GTK_TYPE_TOGGLE_BUTTON)

static void img_free_audio_data(AudioData *data)
{
    if (data)
    {
        g_free(data->raw_samples);
        g_free(data->samples);
        
        if (data->decoder_context)
            avcodec_free_context(&data->decoder_context);

        if (data->format_context)
            avformat_close_input(&data->format_context);
        
        g_free(data);
    }
}

static void img_media_audio_button_dispose(GObject *object)
{
	ImgMediaAudioButton *button = IMG_MEDIA_AUDIO_BUTTON(object);
    ImgMediaAudioButtonPrivate *priv = img_media_audio_button_get_instance_private(button);
 
	g_clear_pointer(&priv->filename, g_free);
    if (priv->audio_data)
    {
        img_free_audio_data(priv->audio_data);
        priv->audio_data = NULL;
    }
    
    G_OBJECT_CLASS(img_media_audio_button_parent_class)->dispose(object);
}

static void img_draw_waveform(ImgMediaAudioButton *button, cairo_t *cr, gint width, gint height)
{
	ImgMediaAudioButtonPrivate *priv = img_media_audio_button_get_instance_private(button);
    
    if (!priv->audio_data || !priv->audio_data->samples)
        return;

	// Calculate samples per pixel
	double samples_per_pixel = priv->audio_data->num_samples / (double)width;
    
	// Add padding
	int padding = (int)(height * 0.15);  // 15% padding top and bottom
	int track_height = height - (padding * 2);
    
	// Set up clipping to ensure waveform stays within button
	cairo_save(cr);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_clip(cr);
    
	// Set waveform color
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	else
		cairo_set_source_rgb(cr, 0.4, 0.4, 0.8);
  
	cairo_set_line_width(cr, 1.0);

	if (samples_per_pixel < 1.0)
	{
		// Draw individual samples when zoomed in
        cairo_move_to(cr, 0, height/2);
        for (int i = 0; i < width && (i * samples_per_pixel) < priv->audio_data->num_samples; i++) 
		{
            int sample_idx = (int)(i * samples_per_pixel);
            float sample = priv->audio_data->samples[sample_idx];
            int y = padding + (int)(((1.0 - sample) / 2.0) * track_height);
            cairo_line_to(cr, i, y);
		}
	}
    else
    {
        // Draw min/max ranges when zoomed out
        for (int i = 0; i < width; i++)
        {
            int start_idx = (int)(i * samples_per_pixel);
            int end_idx = (int)((i + 1) * samples_per_pixel);
            
            if (start_idx >= priv->audio_data->num_samples) break;
            
            float min = 1.0f;
            float max = -1.0f;
            
            // Find min/max in sample range
            for (int j = start_idx; j < end_idx && j < priv->audio_data->num_samples; j++)
            {
                float sample = priv->audio_data->samples[j];
                if (sample < min) min = sample;
                if (sample > max) max = sample;
            }
            
            int y_min = padding + (int)(((1.0 - min) / 2.0) * track_height);
            int y_max = padding + (int)(((1.0 - max) / 2.0) * track_height);
            
            cairo_move_to(cr, i + 0.5, y_max);
            cairo_line_to(cr, i + 0.5, y_min);
        }
    }
    
    cairo_stroke(cr);
    cairo_restore(cr);
}

static gboolean img_media_audio_button_draw(GtkWidget *widget, cairo_t *cr)
{
	ImgMediaAudioButtonPrivate *priv = img_media_audio_button_get_instance_private((ImgMediaAudioButton *) widget);

    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		cairo_set_source_rgb(cr, 0.208, 0.518, 0.894);
	else
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		
    // Draw background
    cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
    cairo_fill(cr);
    
    // Draw waveform
    img_draw_waveform((ImgMediaAudioButton*) widget, cr, allocation.width, allocation.height);
    
    // Draw button frame
    gtk_render_frame(context, cr, 0, 0, allocation.width, allocation.height);
    
    // Draw audio name
    if (priv->filename)
    {
        PangoLayout *layout = gtk_widget_create_pango_layout(widget, priv->filename);
        gint text_width, text_height;
        pango_layout_get_pixel_size(layout, &text_width, &text_height);
        
        // Draw text background for better visibility
        cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
        cairo_rectangle(cr, 2, 2, text_width + 4, text_height + 2);
        cairo_fill(cr);
        
        // Draw text
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_move_to(cr, 4, 2);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
    }

    return TRUE;
}

static void img_media_audio_button_class_init(ImgMediaAudioButtonClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    
    object_class->dispose = img_media_audio_button_dispose;
    widget_class->draw = img_media_audio_button_draw;
}

static void img_media_audio_button_init(ImgMediaAudioButton *button)
{
	 ImgMediaAudioButtonPrivate *priv = img_media_audio_button_get_instance_private(button);
	//Nothing to do here at the moment
}

GtkWidget *img_media_audio_button_new()
{
    return GTK_WIDGET(g_object_new(img_media_audio_button_get_type(), NULL));
}

static AudioData *img_create_audio_data_struct(AVFormatContext *pFormatContext, AVCodecContext *pDecoderContext, int audio_stream_index)
{
    AudioData *data = g_new0(AudioData, 1);
    
    data->format_context = pFormatContext;
    data->decoder_context = pDecoderContext;
    data->format = pDecoderContext->sample_fmt;
    data->sample_size = (int) av_get_bytes_per_sample(pDecoderContext->sample_fmt);
    data->channels = pDecoderContext->ch_layout.nb_channels;
    data->audio_stream_index = audio_stream_index;

    switch (pDecoderContext->sample_fmt)
    {
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
			data->format = SAMPLE_FORMAT_UINT8;
		break;
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
			data->format = SAMPLE_FORMAT_INT16;
		break;
		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
			data->format = SAMPLE_FORMAT_INT32;
		break;
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
			data->format = SAMPLE_FORMAT_FLOAT;
		break;
		case AV_SAMPLE_FMT_DBL:
		case AV_SAMPLE_FMT_DBLP:
			data->format = SAMPLE_FORMAT_DOUBLE;
		break;
		default:
			g_warning("Bad format: %s", av_get_sample_fmt_name(pDecoderContext->sample_fmt));
			img_free_audio_data(data);
		return NULL;
    }

    return data;
}

static void img_read_raw_audio_data(AudioData *data)
{
    AVPacket *packet = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();

    double duration = data->format_context->duration / (double) AV_TIME_BASE;
    int raw_sample_rate = 0;
    int is_planar = av_sample_fmt_is_planar(data->decoder_context->sample_fmt);
    int total_size = 0;
    int allocated_buffer_size = (data->format_context->bit_rate / 8) * duration;
    
    data->raw_samples = g_malloc(allocated_buffer_size);

    while (av_read_frame(data->format_context, packet) >= 0)
    {
        if (packet->stream_index == data->audio_stream_index)
        {
            int ret = avcodec_send_packet(data->decoder_context, packet);
            if (ret < 0)
            {
                g_warning("Error sending packet for decoding");
                break;
            }

            while (ret >= 0) 
            {
                ret = avcodec_receive_frame(data->decoder_context, pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break;
                } else if (ret < 0)
                {
                    g_warning("Error during decoding");
                    goto end;
                }

                int data_size = av_samples_get_buffer_size(
                    is_planar ? &pFrame->linesize[0] : NULL,
                    data->channels,
                    pFrame->nb_samples,
                    data->decoder_context->sample_fmt,
                    1
                );

                if (raw_sample_rate == 0)
                    raw_sample_rate = pFrame->sample_rate;

                if (total_size + data_size > allocated_buffer_size)
                {
                    allocated_buffer_size = allocated_buffer_size * 1.25;
                    data->raw_samples = g_realloc(data->raw_samples, allocated_buffer_size);
                }

                if (is_planar)
                {
                    for (int i = 0; i < data_size / data->channels; i += data->sample_size)
                    {
                        for (int c = 0; c < data->channels; c++)
                        {
                            memcpy(data->raw_samples + total_size, 
                                   pFrame->extended_data[c] + i, 
                                   data->sample_size);
                            total_size += data->sample_size;
                        }
                    }
                } else {
                    memcpy(data->raw_samples + total_size, pFrame->data[0], data_size);
                    total_size += data_size;
                }
            }
        }
        av_packet_unref(packet);
    }

end:
    data->size = total_size;
    data->sample_rate = raw_sample_rate;
    data->num_samples = total_size / data->sample_size;
    data->duration = (data->size * 8.0) / 
                    (raw_sample_rate * data->sample_size * 8.0 * data->channels);

    // Allocate and convert to normalized float samples
    data->samples = g_new(float, data->num_samples);
    for (int i = 0; i < data->num_samples; i++) {
        float sample = 0;
        switch (data->format) {
            case SAMPLE_FORMAT_UINT8:
                sample = (((uint8_t*)data->raw_samples)[i] / 255.0f) * 2 - 1;
                break;
            case SAMPLE_FORMAT_INT16:
                sample = ((int16_t*)data->raw_samples)[i] / 32768.0f;
                break;
            case SAMPLE_FORMAT_INT32:
                sample = ((int32_t*)data->raw_samples)[i] / 2147483648.0f;
                break;
            case SAMPLE_FORMAT_FLOAT:
                sample = ((float*)data->raw_samples)[i];
                break;
            case SAMPLE_FORMAT_DOUBLE:
                sample = ((double*)data->raw_samples)[i];
                break;
        }
        data->samples[i] = sample;
    }

    av_frame_free(&pFrame);
    av_packet_free(&packet);
}

gboolean img_load_audio_file(ImgMediaAudioButton *button, const char *filename)
{
	ImgMediaAudioButtonPrivate *priv = img_media_audio_button_get_instance_private(button);

	AudioData *audio_data = NULL;
    AVFormatContext *format_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    int audio_stream_index = -1;

	const char *base_name = g_path_get_basename(filename);
    priv->filename = g_strdup(base_name);
    
    if (avformat_open_input(&format_ctx, filename, NULL, NULL) != 0) {
        g_warning("Could not open file %s", filename);
        return FALSE;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        g_printerr("Could not find stream information\n");
        avformat_close_input(&format_ctx);
        return FALSE;
    }

    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }

    if (audio_stream_index == -1) {
        g_printerr("Could not find audio stream\n");
        avformat_close_input(&format_ctx);
        return FALSE;
    }

    const AVCodec *codec = avcodec_find_decoder(format_ctx->streams[audio_stream_index]->codecpar->codec_id);
    if (!codec) {
        g_printerr("Unsupported codec\n");
        avformat_close_input(&format_ctx);
        return FALSE;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        g_printerr("Could not allocate audio codec context\n");
        avformat_close_input(&format_ctx);
        return FALSE;
    }

    if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[audio_stream_index]->codecpar) < 0) {
        g_printerr("Could not copy codec parameters to decoder context\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return FALSE;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        g_printerr("Could not open codec\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return FALSE;
    }

   audio_data = img_create_audio_data_struct(format_ctx, codec_ctx, audio_stream_index);
    if (!audio_data) {
        g_warning("Could not create audio data struct");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return FALSE;
    }

    img_read_raw_audio_data(audio_data);

    if (audio_data->size == 0) {
        g_warning("No samples were loaded");
        img_free_audio_data(audio_data);
        return FALSE;
    }

    //~ g_debug("Loaded %d samples", audio_data->num_samples);
    //~ g_debug("Sample rate: %d", audio_data->sample_rate);
    //~ g_debug("Channels: %d", audio_data->channels);
    //~ g_debug("Duration: %.2f seconds", audio_data->duration);

	priv->audio_data = audio_data;
	priv->duration = priv->audio_data->duration;
	
	return TRUE;
}
