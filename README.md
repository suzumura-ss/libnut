libnut for Win32/C++ forked from https://github.com/Distrotech/libnut
==============

https://github.com/Distrotech/libnut を元に以下を更新しました.

* VS2017によるコンパイルエラーを除去
* VS2017による型変換の警告をint64_tを基本にして書き換え
* ffmpegの出力するnutを読むために nut version 番号を `3` に更新、それに関連して ffmpeg の出力する nut を読むと落ちるのを修正
    * get_info_header で name, type を読み込む処理でassertするのを修正
    * nut_demuxer_uninit でSEGVするのを修正
* C++からのインタフェースを追加
