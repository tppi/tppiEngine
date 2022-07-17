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

#include "tppiH264VideoServerMediaSubsession.hh"
#include "tppiADTSAudioServerMediaSubsession.hh"
#include "tppiAnnounceURL.hh"
#include "tppiDeviceH264Source.hh"
#include "tppiDeviceAACSource.hh"
#include "tppiConfig.hh"

#include <iostream>
#include <thread>

using namespace std;

void consumer_gb28181_gateway(CircularBuffer<data_t> &, CircularBuffer<data_t> &){}
void consumer_ims(CircularBuffer<data_t> &, CircularBuffer<data_t> &){}


class tppi
{	
private:
	void (*fun1)(CircularBuffer<data_t> &, CircularBuffer<data_t> &);
	thread t1;
	void (*fun2)(CircularBuffer<data_t> &, CircularBuffer<data_t> &);
	thread t2;			
	CircularBuffer<data_t>& q1;
    CircularBuffer<data_t>& q2;

public:
    tppi(void (*function1)(CircularBuffer<data_t> &, CircularBuffer<data_t> &), 
    	void (*function2)(CircularBuffer<data_t> &, CircularBuffer<data_t> &), 
    	CircularBuffer<data_t>& q1,
        CircularBuffer<data_t>& q2):
    	 
    	fun1{function1},
    	t1(fun1,ref(q1),ref(q2)),
    	fun2{function2},
    	t2(fun2,ref(q1),ref(q2)),    	    	   	
    	q1{q1},
        q2{q2}
    	
    {
    	// add your code 			
    }
    
    ~tppi()
    {
		t1.join();
		t2.join();	
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
		
	tppi* t1 = new tppi(consumer_gb28181_gateway, 
					   consumer_ims, 
					   audio, 
                       video);
			
	t1->doneLoop();

	delete t1;
   
    return 0;
}
