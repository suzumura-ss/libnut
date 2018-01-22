#define _CRT_SECURE_NO_WARNINGS
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include "libnut.h"

int muxer_main(int argc, char* argv[])
{
#ifdef WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    nut_muxer_opts_tt mopts;
    memset(&mopts, 0, sizeof(mopts));
    mopts.output = (nut_output_stream_tt) { .priv = stdout, .write = NULL };
    mopts.write_index = 1;
    mopts.realtime_stream = 0;
    mopts.fti = NULL;
    mopts.max_distance = 32768;
    mopts.alloc.malloc = NULL;

    nut_stream_header_tt nut_streams[2];
    memset(nut_streams, 0, sizeof(nut_streams));
    nut_streams[0].type = NUT_VIDEO_CLASS;
    nut_streams[0].fourcc_len = 4;
    nut_streams[0].fourcc = (uint8_t*)"NV12";
    nut_streams[0].time_base = (nut_timebase_tt) { .num = 1, .den = 15 };
    nut_streams[0].fixed_fps = 15;
    nut_streams[0].decode_delay = 0;
    //nut_streams[0].codec_specific_len = 0;
    //nut_streams[0].codec_specific = NULL;
    //nut_streams[0].max_pts = 0;
    nut_streams[0].width  = 32;
    nut_streams[0].height = 32;
    //nut_streams[0].sample_width = 0;
    //nut_streams[0].sample_height = 0;
    nut_streams[0].colorspace_type = 0;
    //[A] nut_streams[0].samplerate_num = 0;
    //[A] nut_streams[0].samplerate_denom = 0;
    //[A] nut_streams[0].channel_count = 0;
    nut_streams[1].type = -1;

    nut_context_tt * nut = nut_muxer_init(&mopts, nut_streams, NULL);

    for (int64_t i = 0; i < 256; ++i) {
        uint8_t buf[32 * 32 * 3 / 2];
        nut_packet_tt packet = (nut_packet_tt) {
            .len = sizeof(buf),
            .stream = 0,
            .pts = i,
            .flags = NUT_FLAG_KEY,
            .next_pts = 0
        };
        memset(buf, 127, sizeof(buf));
        memset(buf, (int)i, 32 * 32);
        nut_write_frame(nut, &packet, buf);
    }

    nut_muxer_uninit(nut);

    return 0;
}

//---------------------------------------------------------------------------------------


void nut_demux_info_callback(void* priv, nut_info_packet_tt* info)
{
    printf("-----------------\n");
    printf("     stream id+1: %lld\n", info->stream_id_plus1);
    printf("      chapter id: %lld\n", info->chapter_id);
    printf("chapter timebaae: %f\n", (float)info->chapter_tb.num / info->chapter_tb.den);
    printf("   chapter start: %lld\n", info->chapter_start);
    printf("  chapter length: %lld\n", info->chapter_len);
    printf("     info fields: %lld\n", info->count);
    for (int64_t i = 0; i < info->count; ++i) {
        printf("[%lld] type: %s\n", i, info->fields[i].type);
        printf("[%lld] name: %s\n", i, info->fields[i].name);
        printf("[%lld]  val: %lld\n", i, info->fields[i].val);
        printf("[%lld]  den: %lld\n", i, info->fields[i].den);
        printf("[%lld]   tb: %lld/%lld\n", i, info->fields[i].tb.num, info->fields[i].tb.den);
        printf("[%lld] data: %p\n", i, info->fields[i].data);
    }
    printf("-----------------\n");
}


int demux_main(int argc, char* argv[])
{
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    int64_t err;
    nut_demuxer_opts_tt dopts;
    memset(&dopts, 0, sizeof(dopts));
    dopts.input.priv = stdin;
    dopts.new_info = nut_demux_info_callback;

    nut_context_tt * nut = nut_demuxer_init(&dopts);
    if (nut) {
        nut_stream_header_tt* headers = NULL;   // nut_demuxer_uninit() が解放.
        nut_info_packet_tt* info = NULL;        // nut_demuxer_uninit() が解放.
        if ((err = nut_read_headers(nut, &headers, &info)) != 0) {
            printf("nut_read_headers failed - %s(%lld)\n", nut_error(err), err);
            goto exit_demuxer;
        }
        printf("            type: %lld\n", headers->type);
        printf("          fourcc: ");
        for (int64_t i = 0; i < headers->fourcc_len; ++i) {
            putchar(headers->fourcc[i]);
        }
        printf(" (%lld)\n", headers->fourcc_len);
        printf("       time_base: %lld/%lld\n", headers->time_base.num, headers->time_base.den);
        printf("       fixed_fps: %lld\n", headers->fixed_fps);
        printf("  codec_specific: ");
        for (int64_t i = 0; i < headers->codec_specific_len; ++i) {
            putchar(headers->codec_specific[i]);
        }
        printf(" (%lld)\n", headers->codec_specific_len);
        printf("           width: %lld\n", headers->width);
        printf("          height: %lld\n", headers->height);
        printf("    sample_width: %lld\n", headers->sample_width);
        printf("   sample_height: %lld\n", headers->sample_height);
        printf(" colorspace_type: %lld\n", headers->colorspace_type);
        printf("  samplerate_num: %lld\n", headers->samplerate_num);
        printf("samplerate_denom: %lld\n", headers->samplerate_denom);
        printf("   channel_count: %lld\n", headers->channel_count);

        int64_t frameIndex = 0;
        int64_t eof = 0;
        while (eof == 0) {
            nut_packet_tt packet;
            memset(&packet, 0, sizeof(packet));
            if ((err = nut_read_next_packet(nut, &packet)) != 0) {
                printf("nut_read_next_packet failed - %s(%lld)\n", nut_error(err), err);
                goto exit_demuxer;
            }

            printf("packet %lld: stream:%lld, pts:%lld, flag:%08llx, length: %lld ...", frameIndex, packet.stream, packet.pts, packet.flags, packet.len);
            int64_t length = packet.len;
            uint8_t* buf = malloc((size_t)length);
            eof = nut_read_frame(nut, &length, buf);
            for (int64_t i = 0; i < 8 && i < packet.len; ++i) {
                printf(" %02X", buf[i]);
            }
            printf(" ... ");
            for (int64_t i = packet.len - 8; i < packet.len; ++i) {
                printf(" %02X", buf[i]);
            }
            printf("\n");
            free(buf);
            frameIndex++;
        }
    }

exit_demuxer:
    nut_demuxer_uninit(nut);

    return 0;
}


int nut_main(int argc, char* argv[])
{
    if (argc == 2 && argv[1][0] == 'm') {
        return muxer_main(argc, argv);
    }
    if (argc == 2 && argv[1][0] == 'd') {
        return demux_main(argc, argv);
    }
    printf("%s <m|d>\n", argv[0]);
    return 1;
}
