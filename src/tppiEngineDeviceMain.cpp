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

// Copyright (c) 2022-2032, zhoudd  All rights reserved
// tppiEngine is a lightweight, efficient and complete digital media engine 
// based on live555, that solved application scenarios such as RTSP/GB28181 
// Video Monitor, SIP Video Intercom(Push to talk over Cellular), 
// Internet Live Streaming, etc in only four lines.

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

#if defined(ringbuf_cc)

using namespace std;

UsageEnvironment* env;

void producer_h264_source(CircularBuffer<data_t>& audio, CircularBuffer<data_t>& video)
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

		data_t d;
		if(h264_size < ITEM_BUFFER_SIZE) {
			memcpy(d.buf, h264_raw, h264_size);
			d.size = h264_size;
			d.type = H264;
			video.put(d);
            fflush(stdout);

            //std::cout << "Val popped : " << d.type << " " << d.size << std::endl;
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

void producer_aac_source(CircularBuffer<data_t>& audio, CircularBuffer<data_t>& video)
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
			audio.put(d);
            fflush(stdout);

            //std::cout << "Val popped : " << d.type << " " << d.size << std::endl;
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

void consumer_rtsp_server(CircularBuffer<data_t>& audio, CircularBuffer<data_t>& video)
{
#if 0
    while(true)
    {
    	data_t d{};
    	
        video.get(&d);
        if(d.size > 0){
        	std::cout << "Val popped : " << d.type << " " << d.size << std::endl; 
        } 

        audio.get(&d);
        if(d.size > 0){
        	std::cout << "Val popped : " << d.type << " " << d.size << std::endl; 
        }    
    }

#else

	//UsageEnvironment* env;
    OutPacketBuffer::maxSize = 500000;
	//OutPacketBuffer::maxSize = 2000000;

	// To make the second and subsequent client for each stream reuse the same
	// input stream as the first client (rather than playing the file from the
	// start for each client), change the following "False" to "True":
	Boolean reuseFirstSource = True;

  Boolean avSynchronize = True;

	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

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
	
    tppiByteStreamH264Parameters h264Params{video, 25, avSynchronize};			            
    sms->addSubsession(tppiByteStreamH264VideoServerMediaSubsession
		       ::createNew(*env, h264Params, reuseFirstSource));
		       
	  tppiByteStreamAACParameters aacParams{audio, 2, 3, 2, 50, avSynchronize};       
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

#endif
    
    return;
}

void consumer_rtmp_publish(CircularBuffer<data_t>& audio, CircularBuffer<data_t>& video){}
void consumer_gb28181_publish(CircularBuffer<data_t>& audio, CircularBuffer<data_t>& video){}
void consumer_poc(CircularBuffer<data_t>& audio, CircularBuffer<data_t>& video){}


class tppi
{	
private:
	void (*fun1)(CircularBuffer<data_t> &, CircularBuffer<data_t> &);
	thread t1;
	void (*fun2)(CircularBuffer<data_t> &, CircularBuffer<data_t> &);
	thread t2;
	void (*fun3)(CircularBuffer<data_t> &, CircularBuffer<data_t> &);
	thread t3;
	void (*fun4)(CircularBuffer<data_t> &, CircularBuffer<data_t> &);
	thread t4;
	void (*fun5)(CircularBuffer<data_t> &, CircularBuffer<data_t> &);
	thread t5;
	void (*fun6)(CircularBuffer<data_t> &, CircularBuffer<data_t> &);
	thread t6;			
	CircularBuffer<data_t>& q1;
	CircularBuffer<data_t>& q2;

public:
    tppi(void (*function1)(CircularBuffer<data_t> &, CircularBuffer<data_t> &), 
    	void (*function2)(CircularBuffer<data_t> &, CircularBuffer<data_t> &), 
    	void (*function3)(CircularBuffer<data_t> &, CircularBuffer<data_t> &),
    	void (*function4)(CircularBuffer<data_t> &, CircularBuffer<data_t> &), 
    	void (*function5)(CircularBuffer<data_t> &, CircularBuffer<data_t> &), 
    	void (*function6)(CircularBuffer<data_t> &, CircularBuffer<data_t> &), 
    	CircularBuffer<data_t>& q1,
    	CircularBuffer<data_t>& q2):
    	 
    	fun1{function1},
    	t1(fun1,ref(q1),ref(q2)),
    	fun2{function2},
    	t2(fun2,ref(q1),ref(q2)),
    	fun3{function3},
    	t3(fun3,ref(q1),ref(q2)), 
    	fun4{function4},
    	t4(fun4,ref(q1),ref(q2)),
    	fun5{function5},
    	t5(fun5,ref(q1),ref(q2)), 
    	fun6{function6},
    	t6(fun6,ref(q1),ref(q2)),     	    	   	
    	q1{q1},
    	q2{q2}
    	
    {
    	// add your code			
    }
    
    ~tppi()
    {
		t1.join();
		t2.join();
		t3.join();
		t4.join();
		t5.join();
		t6.join();		
    }
    
    void doneLoop()
    { 
       // add your code    
    }
    
    // delete any copy/move operations
    tppi(const tppi&) =delete;
    tppi(tppi&&) =delete;
    tppi& operator=(const tppi&) =delete;
    tppi& operator=(tppi&&) =delete;
};

int main()
{ 
	CircularBuffer<data_t> audio(16), video(16);
		
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
   
    return 0;
}

#elif defined(ringbuf_c)

int main()
{
    return 0;
}

#else

#endif
