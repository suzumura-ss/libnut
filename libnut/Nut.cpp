//
//  Nut.cpp
//
//  Created by Toshiyuki Suzumura.
//  Copyright (c) 2018 smoche.info. All rights reserved.
//

#pragma once

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <errno.h>
#include "Nut.hxx"



#pragma mark -- NutContext

Nut::NutHandle::NutHandle()
{
    _fp = nullptr;
}



Nut::NutHandle::~NutHandle()
{
    try {
        close();
    } catch (...) {}
}



bool Nut::NutHandle::open(HANDLE handle, bool write)
{
    close();
    int fd = _open_osfhandle((intptr_t)handle, write ? 0 : _O_RDONLY);
    if (fd < 0) {
        return false;
    }
    _fp = _fdopen(fd, write ? "wb" : "rb");
    if (!_fp) {
        ::_close(fd);
        return false;
    }
    return true;
}



void Nut::NutHandle::close()
{
    if (_fp) {
        fclose(_fp);
        _fp = nullptr;
    }
}




#pragma mark -- Muxer

Nut::Muxer::Muxer()
{
    _context = nullptr;
    memset(&_packet, 0, sizeof(_packet));
    memset(_streamHeaders, 0, sizeof(_streamHeaders));
    _streamHeaders[1].type = -1;
}



Nut::Muxer::~Muxer()
{
    try {
        close();
    } catch (...) {}
}



bool Nut::Muxer::open(HANDLE hWrite, const std::string& fourcc, const TimeBase& timeBase, int width, int height, int fixedFps, int colorSpace)
{
    close();
    if (!NutHandle::open(hWrite, true)) {
        return false;
    }
    _fourcc = fourcc;

    ::nut_muxer_opts_tt mopts;
    memset(&mopts, 0, sizeof(mopts));
    mopts.output.priv = (void*)_fp;
    mopts.write_index = 1;
    mopts.max_distance = 32768;

    _streamHeader.type = ::NUT_VIDEO_CLASS;
    _streamHeader.fourcc_len = _fourcc.length();
    _streamHeader.fourcc = (uint8_t*)_fourcc.c_str();
    _streamHeader.time_base = timeBase;
    _streamHeader.fixed_fps = fixedFps;
    _streamHeader.colorspace_type = colorSpace;
    _streamHeader.width = width;
    _streamHeader.height = height;

    _context = ::nut_muxer_init(&mopts, _streamHeaders, nullptr);

    if (!_context) {
        close();
        return false;
    }
    return true;
}



void Nut::Muxer::close()
{
    if (_context) {
        ::nut_muxer_uninit(_context);
        _context = nullptr;
    }
    NutHandle::close();
}



int Nut::Muxer::send(int64_t size, const uint8_t* buffer, uint64_t timestamp)
{
    _packet.len = size;
    _packet.stream = 0;
    _packet.pts = timestamp;
    _packet.flags = ::NUT_FLAG_KEY;
    _packet.next_pts = 0;
    nut_write_frame(_context, &_packet, buffer);
    return errno;
}



#pragma mark -- Demuxer

Nut::Demuxer::Demuxer()
{
    _context = nullptr;
    _streamHeader = nullptr;
    _packetInfo = nullptr;
    memset(&_packet, 0, sizeof(_packet));
    memset(&_stubHeader, 0, sizeof(_stubHeader));
    memset(&_stubInfo, 0, sizeof(_stubInfo));
}



Nut::Demuxer::~Demuxer()
{
    try {
        close();
    } catch (...) {}
}



bool Nut::Demuxer::open(HANDLE hRead)
{
    auto demux_info_callback = [](void* priv, ::nut_info_packet_tt* info)->void {
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
    };

    close();
    if (!NutHandle::open(hRead, false)) {
        return false;
    }

    nut_demuxer_opts_tt dopts;
    memset(&dopts, 0, sizeof(dopts));
    dopts.input.priv = _fp;
    dopts.new_info = demux_info_callback;

    _context = ::nut_demuxer_init(&dopts);
    if (!_context) {
        close();
        return false;
    }

    int64_t err = ::nut_read_headers(_context, &_streamHeader, &_packetInfo);
    if (err) {
        printf("nut_read_headers failed - %s(%lld)\n", ::nut_error(err), err);
        close();
        return false;
    }
    return true;
}



void Nut::Demuxer::close()
{
    if (_context) {
        ::nut_demuxer_uninit(_context);
        _context = nullptr;
        _streamHeader = nullptr;    // dealloc in nut_demuxer_uninit()
        _packetInfo = nullptr;      // dealloc in nut_demuxer_uninit()
    }
    NutHandle::close();
}



int Nut::Demuxer::recv(int64_t& size, uint8_t*& buffer, uint64_t& timestamp, bool& eof)
{
    memset(&_packet, 0, sizeof(_packet));
    int64_t err = ::nut_read_next_packet(_context, &_packet);
    if (err) {
        printf("nut_read_next_packet failed - %s(%lld)\n", nut_error(err), err);
        return (int)err;
    }
    if (size == 0) {
        size = _packet.len;
        buffer = new uint8_t[(size_t)_packet.len];
    }
    int64_t e = ::nut_read_frame(_context, &size, buffer);
    eof = (e) ? true : false;
    return 0;
}
