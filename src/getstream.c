#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "libavformat/avformat.h"
#include "libavutil/time.h"
#include "libavutil/mathematics.h"

#define RTMP_ADDR "rtmp://192.168.2.122/live/password"

#define RESOLUTION_X 800
#define RESOLUTION_Y 480

#define LOOP_RUN 1
#define LOOP_STOP 0

int loop_state_flag;

void signal_handler(int sig) {
    if(sig == SIGINT) { /* no need? */
        loop_state_flag = LOOP_STOP;
    }
}

void getstream(unsigned char *mem) {

    /* open stream and get head info */
    AVFormatContext *ifmt_ctx = NULL;
    if (avformat_open_input(&ifmt_ctx, RTMP_ADDR, NULL, NULL) < 0) {
        printf("Error: Connect to the server failed!Please check the url!\r\n");
        goto fail_open_stream;
    }
    printf("Connect to the server succeed!\r\n");

    /* get extra stream info */
    if (avformat_find_stream_info(ifmt_ctx, NULL) < 0) {
        printf("Error: Get extra info failed!\r\n");
        goto fail_get_streaminfo;
    }
    printf("Get extra info succeed!\r\n");

    /* print some input info */
    av_dump_format(ifmt_ctx, 0, RTMP_ADDR, 0);

    /* find best stream */
    int video_idx = -1; 
    video_idx = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_idx < 0) {
        printf("Error: Find video stream failed!\r\n");
        goto fail_find_videostream;
    }else {
        printf("Find video stream succeed!\r\n");
    }

    /* set h264 decoder */
    const AVCodec *codec;
    codec = avcodec_find_decoder(ifmt_ctx->streams[video_idx]->codecpar->codec_id);
    if(!codec){
        printf("Error: Find codec failed!\r\n");
        goto fail_find_codec;
    }
    printf("Find codec succeed!\r\n");

    AVCodecContext  *codec_ctx;
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        printf("Error: Allocate video codec context failed!\r\n");
        goto fail_find_codec_ctx;
    }
    printf("Allocate video codec context succeed!\r\n");

    if(avcodec_parameters_to_context(codec_ctx, ifmt_ctx->streams[video_idx]->codecpar) < 0) {
        printf("Error: Par 2 ctx failed!\r\n");
        goto fail_par_ctx;
    }
    printf("Par 2 ctx succeed!\r\n");

    if(avcodec_open2(codec_ctx, codec, NULL) < 0) {
        printf("Error: Open codec failed!\r\n");
        goto fail_open_codec;
    }
    printf("Open codec succeed!\r\n");

    AVFrame *frame = NULL;
    frame = av_frame_alloc();
    if(!frame)
    {
        printf("Error: Init frame failed!\r\n");
        goto fail_init_frame;
    }
    printf("Allocate video frame succeed!\r\n");
    /* set the info for sws_getContext() init */
    frame->height = ifmt_ctx->streams[video_idx]->codecpar->height;
    frame->width = ifmt_ctx->streams[video_idx]->codecpar->width;
    frame->format = ifmt_ctx->streams[video_idx]->codecpar->format;

    int i;

    AVFrame *rgbframe = NULL;
    rgbframe = av_frame_alloc();
    if(!rgbframe)
    {
        printf("Error: Init rgb frame failed!\r\n");
        goto fail_init_rgbframe;
    }
    printf("Allocate rgb frame succeed!\r\n");
    /* set the info for sws_getContext() and av_frame_get_buffer() */
    rgbframe->height = RESOLUTION_Y;
    rgbframe->width = RESOLUTION_X;
    rgbframe->format = AV_PIX_FMT_RGB32;
    /* av_frame_get_buffer() is nessary or goes "bad dst image pointers" */
    if (av_frame_get_buffer(rgbframe, 0) < 0) {
        printf("Error: Allocate rgb frame data failed!\r\n");
        goto fail_allocate_rgbframedata;
    }

    struct SwsContext *sws_ctx;
    sws_ctx = sws_getContext(frame->width, frame->height, AV_PIX_FMT_YUV420P, 
            rgbframe->width, rgbframe->height, AV_PIX_FMT_RGB32, 0, NULL, NULL, NULL);

    AVPacket *packet = av_packet_alloc();

    int ret;
    loop_state_flag = LOOP_RUN;

    printf("Start play!\r\n");

    while (loop_state_flag == LOOP_RUN) {
        if(av_read_frame(ifmt_ctx, packet) < 0) {
            printf("Error: Read packet failed!\r\n");
            break;
        }

        /* decode */
        if (packet->stream_index == video_idx) {
            /* is a video frame */
            ret = avcodec_send_packet(codec_ctx, packet);
            if(ret < 0) {
                printf("Error: Send packet to the decoder failed!\r\n");
                av_packet_unref(packet); /* free memory */
                break;
            }

            /* send to decoder success */
            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    // av_packet_unref(packet); /* free memory */
                    break;
                }else if(ret < 0){
                    printf("Error: Send packet to the decoder failed!\r\n");
                    av_packet_unref(packet); /* free memory */
                    goto loop_error;
                }

                if(ret >= 0){

                    /* YUV420 2 RGB32 */
                    sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, rgbframe->data, rgbframe->linesize);

                    memcpy(mem, rgbframe->data[0], RESOLUTION_X * RESOLUTION_Y * 4); /* dont forget [0] */

                    av_packet_unref(packet); /* free memory */
                }
            }
        }else {
            /* is not a video frame */
            av_packet_unref(packet); /* free memory */
        }
    }

loop_error:
    sws_freeContext(sws_ctx);
    av_packet_free(&packet);
fail_allocate_rgbframedata:
    av_frame_free(&rgbframe);
    rgbframe = NULL;
fail_init_rgbframe:
    av_frame_free(&frame);
    frame = NULL;
fail_init_frame:
fail_open_codec:
    avcodec_free_context(&codec_ctx);
fail_par_ctx:
fail_find_codec_ctx:
fail_find_codec:
fail_find_videostream:
fail_get_streaminfo:
    avformat_close_input(&ifmt_ctx);
    ifmt_ctx = NULL;
fail_open_stream:
    ;
}
