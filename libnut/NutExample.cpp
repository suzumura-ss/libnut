#include <stdio.h>
#include "Nut.hxx"



int muxer_main()
{
    int width = 1920;
    int height = 1080;
    Nut::Muxer muxer;
    HANDLE handle = CreateFileA("1920x1080.nut", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    muxer.open(handle, "NV12", Nut::TimeBase(1, 15), width, height, 15, 2);

    size_t length = width * height * 3 / 2;
    uint8_t* buf = new uint8_t[length];

    for (int64_t i = 0; i < 256; ++i) {
        memset(buf, 127, length);
        memset(buf, (uint8_t)i, width * height);
        muxer.send(length, buf, i);
    }

    muxer.close();
    delete[] buf;
    return 0;
}



int demux_main()
{
    Nut::Demuxer demuxer;
    HANDLE handle = CreateFileA("1920x1080.nut", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    std::string fourcc;
    Nut::TimeBase timeBase;
    demuxer.open(handle);

    int64_t bufferSize = 0;
    uint8_t* buffer = nullptr;
    uint64_t timeStamp;
    int64_t frameIndex = 0;
    bool eof;
    while (!demuxer.recv(bufferSize, buffer, timeStamp, eof)) {
        printf("packet %lld: stream:%lld, pts:%lld, flag:%08llx, length: %lld ...", frameIndex, demuxer.lastPacket().stream, demuxer.lastPacket().pts, demuxer.lastPacket().flags, demuxer.lastPacket().len);
        for (int64_t i = 0; i < 8 && i < demuxer.lastPacket().len; ++i) {
            printf(" %02X", buffer[i]);
        }
        printf(" ... ");
        for (int64_t i = bufferSize - 8; i < bufferSize; ++i) {
            printf(" %02X", buffer[i]);
        }
        printf("\n");
        frameIndex++;
        bufferSize = 0;
        delete[] buffer;
        if (eof) break;
    }
    demuxer.close();
    return 0;
}



int Nut_main(int argc, char* argv[])
{
    if (argc == 2 && argv[1][0] == 'm') {
        return muxer_main();
    }
    if (argc == 2 && argv[1][0] == 'd') {
        return demux_main();
    }
    printf("%s <m|d>\n", argv[0]);
    return 1;
}
