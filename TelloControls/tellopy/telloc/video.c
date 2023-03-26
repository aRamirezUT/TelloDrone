// Contains the implementation for the video decoder for the telloc library
//
#include "video.h"

#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>


// function to initialize the video decoder
int telloc_video_decoder_init(telloc_video_decoder* decoder) {
    // tell ffmpeg not log anything except panic
    av_log_set_level(AV_LOG_PANIC);

    // Initialize state to NULL
    decoder->codec = NULL;
    decoder->codec_context = NULL;
    decoder->sws_context = NULL;
    decoder->frame = NULL;
    decoder->frame_rgb = NULL;
    decoder->packet = NULL;
    decoder->frame_ready = 0;

    // initialize the ffmpeg state
    decoder->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!decoder->codec) {
        return 1;
    }
    decoder->codec_context = avcodec_alloc_context3(decoder->codec);
    if (!decoder->codec_context) {
        return 1;
    }
    // set the codec context options
    av_opt_set(decoder->codec_context->priv_data, "preset", "ultrafast", 0);
    av_opt_set(decoder->codec_context->priv_data, "tune", "zerolatency", 0);
    decoder->codec_context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    decoder->codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
    decoder->codec_context->thread_count = 1;
    decoder->codec_context->error_concealment = 3;
    decoder->codec_context->workaround_bugs = FF_BUG_AUTODETECT;
    decoder->codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    decoder->codec_context->width = 960;
    decoder->codec_context->height = 720;
    decoder->codec_context->gop_size = 0;

    // initialize sws context
    decoder->sws_context = sws_getContext(decoder->codec_context->width, decoder->codec_context->height, decoder->codec_context->pix_fmt, decoder->codec_context->width, decoder->codec_context->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

    if (avcodec_open2(decoder->codec_context, decoder->codec, NULL) < 0) {
        return 1;
    }
    decoder->frame = av_frame_alloc();
    if (!decoder->frame) {
        return 1;
    }
    decoder->frame_rgb = av_frame_alloc();
    if (!decoder->frame_rgb) {
        return 1;
    }
    decoder->packet = av_packet_alloc();
    if (!decoder->packet) {
        return 1;
    }

    av_image_alloc(decoder->frame_rgb->data, decoder->frame_rgb->linesize, decoder->codec_context->width, decoder->codec_context->height, AV_PIX_FMT_RGB24, 1);

    decoder->frame_ready = 0;
    decoder->frame_size = 0;
    decoder->frame_width = 0;
    decoder->frame_height = 0;
    decoder->frame_channels = 0;
    decoder->frame_buffer_size = 0;
    decoder->frame_buffer = NULL;
    return 0;
}

// function to attempt to decode an h264 video frame
int telloc_video_decoder_decode(telloc_video_decoder* decoder, unsigned char* video_stream, unsigned int video_stream_length) {
    // decode the video frame
    av_packet_unref(decoder->packet);
    decoder->packet = av_packet_alloc();
    decoder->packet->data = video_stream;
    decoder->packet->size = (int) video_stream_length;
    avcodec_send_packet(decoder->codec_context, decoder->packet);

    // check if the frame is ready
    if (avcodec_receive_frame(decoder->codec_context, decoder->frame)) {
        // frame not ready
        return 1;
    }

    // frame is ready

    // convert the image to rgb24 and save in PIX_FMT_RGB24
    sws_scale(decoder->sws_context, (const uint8_t* const*)decoder->frame->data, decoder->frame->linesize, 0, decoder->codec_context->height, decoder->frame_rgb->data, decoder->frame_rgb->linesize);

    // copy the frame data to the frame buffer
    decoder->frame_width = decoder->codec_context->width;
    decoder->frame_height = decoder->codec_context->height;
    decoder->frame_channels = 3;
    decoder->frame_size = decoder->frame_width*decoder->frame_height*decoder->frame_channels;
    if (decoder->frame_buffer_size < decoder->frame_size) {
        decoder->frame_buffer_size = decoder->frame_size*2;
        decoder->frame_buffer = realloc(decoder->frame_buffer, decoder->frame_buffer_size);
    }

    av_image_copy_to_buffer(decoder->frame_buffer, decoder->frame_buffer_size, (const uint8_t* const*)decoder->frame_rgb->data, (const int*)decoder->frame_rgb->linesize, AV_PIX_FMT_RGB24, decoder->codec_context->width, decoder->codec_context->height, 1);

    // set the frame ready flag
    decoder->frame_ready = 1;

    return 0;
}

// function to check if a frame is a valid h264 start code
int telloc_video_decoder_is_start_code(const unsigned char* video_stream, unsigned int video_stream_length) {
    if (video_stream_length < 4) {
        return 0;
    }
    if (video_stream[0] == 0x00 && video_stream[1] == 0x00 && video_stream[2] == 0x00 && video_stream[3] == 0x01) {
        return 1;
    }
    return 0;
}

// function to free the video decoder
int telloc_video_decoder_free(telloc_video_decoder* decoder) {
    // free the ffmpeg state
    if(decoder->codec_context) {
        avcodec_free_context(&decoder->codec_context);
    }
    if (decoder->frame) {
        av_frame_free(&decoder->frame);
    }
    if(decoder->frame_rgb){
        av_frame_free(&decoder->frame_rgb);
    }
    av_packet_free(&decoder->packet);
    av_freep(&decoder->frame_buffer);
    sws_freeContext(decoder->sws_context);
    return 0;
}


