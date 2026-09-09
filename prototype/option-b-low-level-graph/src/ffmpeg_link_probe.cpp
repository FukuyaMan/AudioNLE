extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include <iostream>

int main() {
  if (avcodec_version() == 0 || avformat_version() == 0 || avutil_version() == 0 || swresample_version() == 0) return 1;
  std::cout << "FFmpeg link probe: avcodec=" << avcodec_version()
            << " avformat=" << avformat_version()
            << " avutil=" << avutil_version()
            << " swresample=" << swresample_version() << '\n';
}
