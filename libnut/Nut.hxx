//
//  Nut.hxx
//
//  Created by Toshiyuki Suzumura.
//  Copyright (c) 2018 smoche.info. All rights reserved.
//

#pragma once
#include <Windows.h>
#include <string>
extern "C" {
#include "libnut.h"
}



namespace Nut {
    struct TimeBase : public ::nut_timebase_tt {
        TimeBase(int64_t num_ = 0, int64_t den_ = 0)
        {
            num = num_;
            den = den_;
        }
    };

    class NutHandle {
    protected:
        FILE * _fp;
    public:
        NutHandle();
        virtual ~NutHandle();
        bool open(HANDLE handle, bool write);
        void close();
    };


    class Muxer : public NutHandle {
    protected:
        ::nut_context_tt* _context;
        ::nut_stream_header_tt _streamHeaders[2];
        ::nut_stream_header_tt& _streamHeader = _streamHeaders[0];
        ::nut_packet_tt _packet;
        std::string _fourcc;

    public:
        Muxer();
        virtual ~Muxer();
        bool open(HANDLE hWrite, const std::string& fourcc, const TimeBase& timeBase, int width, int height, int fixedFps, int colorSpace);
        void close();
        int send(int64_t size, const uint8_t* buffer, uint64_t timestamp);
    };



    class Demuxer : public NutHandle {
    protected:
        ::nut_context_tt* _context;
        ::nut_stream_header_tt* _streamHeader;
        ::nut_info_packet_tt* _packetInfo;
        ::nut_packet_tt _packet;
        ::nut_stream_header_tt _stubHeader;
        ::nut_info_packet_tt _stubInfo;

    public:
        Demuxer();
        virtual ~Demuxer();
        bool open(HANDLE hRead);
        void close();
        int recv(int64_t& size, uint8_t*& buffer, uint64_t& timestamp, bool& eof);
        inline const ::nut_stream_header_tt& streamHeader() const
        {
            if (_streamHeader) {
                return *_streamHeader;
            }
            return _stubHeader;
        }
        inline const ::nut_info_packet_tt& packetInfo() const
        {
            if (_packetInfo) {
                return *_packetInfo;
            }
            return _stubInfo;
        }
        inline const ::nut_packet_tt lastPacket() const
        {
            return _packet;
        }
    };
}
