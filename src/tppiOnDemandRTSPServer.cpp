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
// A test program that demonstrates how to stream - via unicast RTP
// - various kinds of file on demand, using a built-in RTSP server.
// main program

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "InputFile.hh"

#include "tppiByteStreamH264VideoServerMediaSubsession.hh"
#include "tppiByteStreamADTSAudioServerMediaSubsession.hh"
#include "tppiByteStreamH264Source.hh"
#include "tppiByteStreamAACSource.hh"
#include "tppiAnnounceURL.hh"
#include "tppiConfig.hh"
#include "tppiUtil.hh"

#include <iostream>
#include <thread>

UsageEnvironment* env;

#if defined(ringbuf_cc)
CircularBuffer<data_t> q1(DATA_ITEM_NMAX);
CircularBuffer<data_t> q2(DATA_ITEM_NMAX);
#elif defined(ringbuf_c)
ringbuffer_t *q1;
ringbuffer_t *q2;
#else
#endif

void producer_h264_source()
{
    while(true)
    {       
        long start = getCurrentTime();

		static int n = 0;
		char str[128];
		sprintf(str, "../samples/_testsrc_rango_h264_nalu_frame_/test%d.264", n);
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

		if (++n > 299) n = 0;

		delete[] h264_raw; 

        long end = getCurrentTime();
        long duration = end - start;
        //std::cout << "producer duration : " << duration << std::endl; 
        usleep(1000 * (40 - duration));
    }
       
    return;
}

void producer_aac_source()
{
    while(true)
    {
        long start = getCurrentTime();

		static int n = 0;
		char str[128];
		sprintf(str, "../samples/_testsrc_rango_aac_adts_frame_/test%d.aac", n);
		FILE* fFid = OpenInputFile(*env, str);      
		unsigned FileSize =  GetFileSize(str, fFid);
		SeekFile64(fFid, 0, SEEK_SET);
		char *aac_raw = new char[FileSize];           
		size_t aac_size = 0;
		if ((aac_size = fread(aac_raw, 1, FileSize, fFid)) != FileSize) {
		    printf("buffer %s failed, expect=%dKB, actual=%dKB.\n", str,
		           (int)(FileSize / 1024), (int)(aac_size / 1024));
		}
		CloseInputFile(fFid);

		data_t d;
		if(aac_size < ITEM_BUFFER_SIZE) {
			memcpy(d.buf, aac_raw, aac_size);
			d.size = aac_size;
			d.type = AAC;
#if defined(ringbuf_cc)
		    q2.put(d);
#elif defined(ringbuf_c)
            rb_put(q2, &d);
#else
#endif
            fflush(stdout);
		}

		if (++n > 559) n = 0;

		delete[] aac_raw; 

        long end = getCurrentTime();
        long duration = end - start;
        //std::cout << "producer duration : " << duration << std::endl; 
        usleep(1000 * (20 - duration));
    }
       
    return;
}

int main(int argc, char** argv) {

  // To make the second and subsequent client for each stream reuse the same
  // input stream as the first client (rather than playing the file from the
  // start for each client), change the following "False" to "True":
  Boolean reuseFirstSource = True;

  Boolean avSynchronize = True;

#if defined(ringbuf_cc)
#elif defined(ringbuf_c)
    q1 = rb_create(DATA_ITEM_NMAX, sizeof(data_t));
    q2 = rb_create(DATA_ITEM_NMAX, sizeof(data_t));
#else
#endif
 
  std::thread t1(producer_h264_source);
  std::thread t2(producer_aac_source);

  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  OutPacketBuffer::maxSize = 500000;
  //OutPacketBuffer::maxSize = 2000000;

  // Create the RTSP server:
  RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554);
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  char const* descriptionString
    = "Session streamed by \"testOnDemandRTSPServer\"";

  // Set up each of the possible streams that can be served by the
  // RTSP server.  Each such stream is implemented using a
  // "ServerMediaSession" object, plus one or more
  // "ServerMediaSubsession" objects for each audio/video substream.

  // H.264 + AAC stream:
  // unicast
  {
    ServerMediaSession* sms
      = ServerMediaSession::createNew(*env, "live", NULL,
				      descriptionString);

    tppiByteStreamH264Parameters h264Params{q1, 25, avSynchronize};			            
    sms->addSubsession(tppiByteStreamH264VideoServerMediaSubsession
		       ::createNew(*env, h264Params, reuseFirstSource));
		       
	  tppiByteStreamAACParameters aacParams{q2, 2, 3, 2, 50, avSynchronize};       
    sms->addSubsession(tppiByteStreamADTSAudioServerMediaSubsession
		       ::createNew(*env, aacParams, reuseFirstSource));

    rtspServer->addServerMediaSession(sms);

    announceURL(rtspServer, sms);
  }

  // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
  // Try first with the default HTTP port (80), and then with the alternative HTTP
  // port numbers (8000 and 8080).

  if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
    *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
  } else {
    *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
  }

  env->taskScheduler().doEventLoop(); // does not return

#if defined(ringbuf_cc)
#elif defined(ringbuf_c)
    rb_destroy(q1);
    rb_destroy(q2);
#else
#endif

  return 0; // only to prevent compiler warning
}

