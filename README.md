## Introduction

tppiEngine is a lightweight, efficient and complete digital media engine based on live555. \
tppiEngine will working on Implementing such as RTSP/GB28181 Video Monitor, \
SIP Video Intercom(Push to talk over Cellular), Internet Live Streaming, etc in only four lines.

tppiEngine(躺平派引擎)是一个基于live555实现的轻量, 高效, 完整的数字媒体引擎. \
tppiEngine致力于仅用4行代码来实现安防监控, 可视对讲, 互联网直播等音视频类应用.

兄弟们用tppiEngine轻松的完成工作，少加班就是对本项目最大的支持！

## Building

**0. live.2022.04.26**  
```
tar xvf package/live.2022.04.26.tar.gz -C ./
chmod 755 -R live
cd live
./genMakefiles linux-no-openssl
make
```
**1. tppi**  
```
cd src
cp ../live/config.linux-no-openssl ./
./genMakefiles linux-no-openssl
make
```

## Features

- [x] 1. tppiH264VideoStreamer.cpp
- [x] 2. tppiByteStreamMemoryBufferSource.cpp
- [x] 3. tppiOnDemandRTSPServer.cpp
- [ ] 4. tppiGB28181Endpoint.cpp
- [ ] 5. tppiGB28181Gateway.cpp
- [ ] 6. tppiPoC.cpp
- [ ] 7. tppiIMS.cpp
- [x] 8. tppiEngineDeviceMain.cpp
- [x] 9. tppiEnginePlatformMain.cpp

## Usage
```
int main()
{   
  // tppiEngineDeviceMain
  {
    Queue<data_t> audio(16), video(16);
		
    tppi* t1 = new tppi(producer_h264_source, 
                        producer_aac_source, 
                        consumer_rtsp_server, 
                        consumer_rtmp_publish,
                        consumer_gb28181_publish,
                        consumer_poc,
                        audio, 
                        video);
			
    t1->doneLoop();

    delete t1;
  }

  // tppiEnginePlatformMain
  {
    Queue<data_t> audio(16), video(16);
		
    tppi* t1 = new tppi(consumer_gb28181_gateway, 
                        consumer_ims, 
                        audio, 
                        video);
			
    t1->doneLoop();

    delete t1;
  }
  
  return 0;
}
```

## Reference
[1] http://www.live555.com/liveMedia/  
[2] http://www.live555.com/Elphel/  
[3] http://live555.com/liveMedia/public/  
[4] http://download.videolan.org/pub/contrib/live555/  
[5] http://live555.com/liveMedia/faq.html#jpeg-streaming  
[6] https://github.com/SonsOfTone/RaspberrIP-Camera  
[7] https://live-devel.live555.narkive.com/vvYX3wuu/how-to-sync-h264-and-aac-timestamp-from-live-streaming  
[8] https://live-devel.live.narkive.com/WoWe4rQI/how-to-set-h264-and-aac-live-frame-timestamp  
[9] https://live-devel.live555.narkive.com/06yLo1YX/true-push-devicesource  
[10] https://thecandcppclub.com/the-c-and-c-club-at-a-glance/  
[11] https://github.com/embeddedartistry/embedded-resources  
