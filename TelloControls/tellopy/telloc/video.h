// Contains utility functions to decode h264 video streams for the telloc project
//
#ifndef TELLOC_VIDEO_H
#define TELLOC_VIDEO_H

// include the ffmpeg libraries
#include "libavcodec/avcodec.h"
#include <libswscale/swscale.h>

// include for codec advanced usage
#include "libavutil/opt.h"


// struct to hold the state of the video decoder
typedef struct {
    AVCodecContext* codec_context;
    const AVCodec* codec;
    AVPacket* packet;
    AVFrame* frame;
    AVFrame* frame_rgb;
    struct SwsContext* sws_context;
    int frame_ready;
    int frame_size;
    int frame_width;
    int frame_height;
    int frame_channels;
    int frame_buffer_size;
    unsigned char* frame_buffer;
} telloc_video_decoder;

// function to initialize the video decoder
int telloc_video_decoder_init(telloc_video_decoder* decoder);

// function to decode a video_stream frame
int telloc_video_decoder_decode(telloc_video_decoder* decoder, unsigned char* video_stream, unsigned int video_stream_length);

// function to check if a frame is a valid h264 start code
int telloc_video_decoder_is_start_code(const unsigned char* video_stream, unsigned int video_stream_length);

// function to free the video decoder
int telloc_video_decoder_free(telloc_video_decoder* decoder);

#endif //TELLOC_VIDEO_H
