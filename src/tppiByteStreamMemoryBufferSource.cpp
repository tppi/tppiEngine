/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
HanXiChangLong, PanYu, GuangZhou, MA 510000 China
**********/

// Copyright (c) 2022-2032 zhoudd  All rights reserved
// A test program that reads a H.264 Elementary Stream video ringbuf
// and streams it using RTP
// main program

// NOTE: For this application to work, the H.264 Elementary Stream video file *must* contain SPS and PPS NAL units,
// ideally at or near the start of the file.  These SPS and PPS NAL units are used to specify 'configuration' information
// that is set in the output stream's SDP description (by the RTSP server that is built in to this application).
// Note also that - unlike some other "*Streamer" demo applications - the resulting stream can be received only using a
// RTSP client (such as "openRTSP")

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "tppiDeviceH264Source.hh"
#include "tppiByteStreamH264Source.hh"
#include "tppiAnnounceURL.hh"
#include "tppiConfig.hh"
#include "tppiUtil.hh"
#include "InputFile.hh"

#include <iostream>
#include <thread>

UsageEnvironment* env;
RTPSink* videoSink;

H264VideoStreamDiscreteFramer *discreVideoFramer = NULL;

void play(); // forward

#if defined(ringbuf_cc)
CircularBuffer<data_t> q1(DATA_ITEM_NMAX);
#elif defined(ringbuf_c)
ringbuffer_t *q1;
#else
#endif

void producer_h264_source()
{
    while(true)
    {
        long start = getCurrentTime();

		// static uint16_t n = 2040;
        static uint16_t n = 0;
		char str[128];
		// sprintf(str, "../samples/_testsrc_grgbanking_h264_nalu_frame_/test%d.264", n);
        // sprintf(str, "../samples/_testsrc_hi-target_h264_nalu_frame_/test%d.264", n); 
        sprintf(str, "../samples/_testsrc_lavfi_h264_3840x2160_nalu_frame_/test%d.264", n);        
		FILE* fFid = OpenInputFile(*env, str);      
		unsigned FileSize =  GetFileSize(str, fFid);
		SeekFile64(fFid, 0, SEEK_SET);
		char *h264_raw = new char[FileSize];           
		size_t h264_size = 0;
		if ((h264_size = fread(h264_raw, 1, FileSize, fFid)) != FileSize) {
		    printf("buffer %s failed, expect=%dKB, actual=%dKB.\n", str,
		           (int)(FileSize / 1024), (int)(h264_size / 1024));
		}
		CloseInputFile(fFid);

        // printf("consumer h264 payload %x %x %x %x %x %ld %d\n",h264_raw[0],h264_raw[1],
        //                              h264_raw[2],h264_raw[3],h264_raw[4],h264_size, n);

		data_t d{};
	    if(h264_size < ITEM_BUFFER_SIZE) {
		    memcpy(d.buf, h264_raw, h264_size);
		    d.size = h264_size;
		    d.type = H264;
#if defined(ringbuf_cc)
		    q1.put(d);
#elif defined(ringbuf_c)
            rb_put(q1, &d);
#else
#endif
            fflush(stdout);
	    }

		// if (++n > 2099) n = 2040;
        // if (++n > 203) n = 0;
        if (++n > 606) n = 0;

		delete[] h264_raw; 

        long end = getCurrentTime();
        long duration = end - start;
        // std::cout << "producer duration : " << duration << std::endl;

        // 1080p/25fps
        // usleep(1000 * (40 - duration));

        // 4k/60fps
        usleep(1000 * (15 - duration));
    }
       
    return;
}

int main(int argc, char** argv) {

#if defined(ringbuf_cc)
#elif defined(ringbuf_c)
    q1 = rb_create(DATA_ITEM_NMAX, sizeof(data_t));
#else
#endif
 
  std::thread t1(producer_h264_source);

  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  // Create 'groupsocks' for RTP and RTCP:
  // 注意：live555的groupsock网络部分，老版本和新本版有调整
#if 0
  // live.2014.07.25.tar.gz
  struct in_addr destinationAddress;
  destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*env);
#else
  // live.2022.04.26.tar.gz
  struct sockaddr_storage destinationAddress;
  destinationAddress.ss_family = AF_INET;
  ((struct sockaddr_in&)destinationAddress).sin_addr.s_addr = chooseRandomIPv4SSMAddress(*env);
#endif
  // Note: This is a multicast address.  If you wish instead to stream
  // using unicast, then you should use the "testOnDemandRTSPServer"
  // test program - not this test program - as a model.

  const unsigned short rtpPortNum = 18888;
  const unsigned short rtcpPortNum = rtpPortNum+1;
  const unsigned char ttl = 255;

  const Port rtpPort(rtpPortNum);
  const Port rtcpPort(rtcpPortNum);

  Groupsock rtpGroupsock(*env, destinationAddress, rtpPort, ttl);
  rtpGroupsock.multicastSendOnly(); // we're a SSM source
  Groupsock rtcpGroupsock(*env, destinationAddress, rtcpPort, ttl);
  rtcpGroupsock.multicastSendOnly(); // we're a SSM source

  // Create a 'H264 Video RTP' sink from the RTP 'groupsock':
  OutPacketBuffer::maxSize = 500000;
  //OutPacketBuffer::maxSize = 2000000;
  videoSink = H264VideoRTPSink::createNew(*env, &rtpGroupsock, 96);

  // Create (and start) a 'RTCP instance' for this RTP sink:
  const unsigned estimatedSessionBandwidth = 1500; // in kbps; for RTCP b/w share
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen+1];
  gethostname((char*)CNAME, maxCNAMElen);
  CNAME[maxCNAMElen] = '\0'; // just in case
  RTCPInstance* rtcp
  = RTCPInstance::createNew(*env, &rtcpGroupsock,
			    estimatedSessionBandwidth, CNAME,
			    videoSink, NULL /* we're a server */,
			    True /* we're a SSM source */);
  // Note: This starts RTCP running automatically

  RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554);
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }
  ServerMediaSession* sms
    = ServerMediaSession::createNew(*env, "live", NULL,
		   "Session streamed by \"testH264VideoStreamer\"",
					   True /*SSM*/);
  sms->addSubsession(PassiveServerMediaSubsession::createNew(*videoSink, rtcp));
  rtspServer->addServerMediaSession(sms);
  announceURL(rtspServer, sms);

  // Start the streaming:
  *env << "Beginning streaming...\n";
  play();

  env->taskScheduler().doEventLoop(); // does not return

#if defined(ringbuf_cc)
#elif defined(ringbuf_c)
    rb_destroy(q1);
#else
#endif

  return 0; // only to prevent compiler warning
}

void afterPlaying(void* /*clientData*/) {
  //*env << "...done reading from tppi hardware encoder\n";
  videoSink->stopPlaying();
  Medium::close(discreVideoFramer);
  // Note that this also closes the input file that this source read from.

  // Start playing once again:
  play();
}

void play() {
  // Open the tppi hardware encoder as a 'byte-stream live source':
  
  // tppiEngine提供三种方法来传输媒体流(其实都差不多，配置有些差异), 而且都支持4k/60fps的大视频
  // 请查看tppiVideoSource，tppiByteStreamH264Source，ByteStreamMemoryBufferSource类
  // 任选其一
  #if 0 // tppiDeviceH264Source
  // multicast
  Boolean avSynchronize = False;
  tppiDeviceH264Parameters params{q1, 60, avSynchronize};
  tppiDeviceH264Source *tppiVideoSource = tppiDeviceH264Source::createNew(*env, params);
  #endif

  #if 0 // tppiByteStreamH264Source
  // unicast
  Boolean avSynchronize = False;
  tppiByteStreamH264Parameters params{q1, 60, avSynchronize};
  tppiByteStreamH264Source *tppiVideoSource = tppiByteStreamH264Source::createNew(*env, params);
  #endif

  #if 1 // ByteStreamMemoryBufferSource
  // unicast
  ByteStreamMemoryBufferSource *tppiVideoSource = NULL;
  static int isFirstFrame = 0;
  
loop:
  { 
    data_t d{};       
    #if defined(ringbuf_cc) 
      if (q1.size() > 0 ) {
      q1.get(&d);
    #elif defined(ringbuf_c)
      if (rb_numitems(q1) > 0) {
      rb_get(q1, &d);
    #else
    #endif
      if((d.size > 0) && (d.type == H264)){

        // printf("consumer h264 payload %x %x %x %x %x %d\n",d.buf[0],d.buf[1],d.buf[2],d.buf[3],d.buf[4],d.size);

        /*
        帧属性布局
        SPS frame   00 00 00 01 67
        PPS frame   00 00 00 01 68
        SEI frame   00 00 00 01 66        
        I   frame   00 00 00 01 65 or 00 00 01 65
        P   slice   00 00 00 01 41

        帧属性分离
        SPS:     0x67    header & 0x1F = 7
        PPS:     0x68    header & 0x1F = 8
        SEI:     0x66    header & 0x1F = 6
        I Frame: 0x65    header & 0x1F = 5
        P Frame: 0x41    header & 0x1F = 1
        */

        // 注意：如果数据帧不干净，就需要用tppiH264ImageType()函数过滤一下！
        if(tppiH264ImageType(d.buf) == 0) {goto loop;}
        // 注意：如果数据帧首帧不是关键帧, 需要丢弃前面的非关键帧！
        if(isFirstFrame == 0)
        {
            if(((d.buf[4] & 0x1F) == 5 ) || ((d.buf[3] & 0x1F) == 5 )) {isFirstFrame = 1;}
            if(((d.buf[4] & 0x1F) == 6 ) || ((d.buf[3] & 0x1F) == 6 )) {isFirstFrame = 1;}
            if(((d.buf[4] & 0x1F) == 7 ) || ((d.buf[3] & 0x1F) == 7 )) {isFirstFrame = 1;}
            if(((d.buf[4] & 0x1F) == 1 ) || ((d.buf[3] & 0x1F) == 1 )) {goto loop;}                             
        }
        
        // 注意：如果数据帧是工整的可以设置为false, 不用判断，直接赋值，提高效率！
        #if true
        uint8_t* buffer = NULL; //%%% TO BE WRITTEN %%%
        uint64_t bufferSize = 0; //%%% TO BE WRITTEN %%%
        // start code:  00 00 00 01/
        if((d.buf[3] & 0x1F) == 1 ){	
	      buffer = (u_int8_t*)d.buf + 4; //%%% TO BE WRITTEN %%%
	      bufferSize = d.size - 4; //%%% TO BE WRITTEN %%%
        }
        // start code:  00 00 01
        else if((d.buf[2] & 0x1F) == 1 ){	
	      buffer = (u_int8_t*)d.buf + 3; //%%% TO BE WRITTEN %%%
	      bufferSize = d.size - 3; //%%% TO BE WRITTEN %%%
        }
        else{}
        #else
        uint8_t* buffer = (u_int8_t*)d.buf + 4; //%%% TO BE WRITTEN %%%
        uint64_t bufferSize = d.size - 4; //%%% TO BE WRITTEN %%%
        #endif

        unsigned fPreferredFrameSize = 0; //%%% TO BE WRITTEN %%%
        unsigned fPlayTimePerFrame = 0; //%%% TO BE WRITTEN %%%
        //std::cout << "Val popped : " << d.type << " " << d.size << std::endl;
        tppiVideoSource = ByteStreamMemoryBufferSource::createNew(*env, buffer, bufferSize, \
                                                        False, fPreferredFrameSize, fPlayTimePerFrame);
      }

      }else {usleep(1000); goto loop;}

  }

  #endif

  if (tppiVideoSource == NULL) {
    *env << "Unable to open file \"" << "tppiH264VideoStreamer \""
         << "\" as a byte-stream file source\n";
    exit(1);
  }

  // Create a framer for the Video Elementary Stream:
  discreVideoFramer =
      H264VideoStreamDiscreteFramer::createNew(*env, tppiVideoSource);

  // Finally, start playing:
  //*env << "Start Streaming from " << "tppiH264VideoStreamer \"" << "...\n";
  videoSink->startPlaying(*discreVideoFramer, afterPlaying, videoSink);
}
