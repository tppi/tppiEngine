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
// "tppiEngine"
// Copyright (c) 2022-2032 zhoudd  All rights reserved.
// A ringbuf source that is a plain byte stream (rather than frames)
// Implementation

#include "tppiByteStreamH264Source.hh"
#include "InputFile.hh"
#include "GroupsockHelper.hh"
#include "tppiConfig.hh"
#include "tppiUtil.hh"
#include <liveMedia.hh>

////////// tppiByteStreamH264Source //////////

tppiByteStreamH264Source*
tppiByteStreamH264Source::createNew(UsageEnvironment& env, tppiByteStreamH264Parameters params,
				unsigned preferredFrameSize,
				unsigned playTimePerFrame) {

  tppiByteStreamH264Source* newSource
    = new tppiByteStreamH264Source(env, params, preferredFrameSize, playTimePerFrame);
  
  return newSource;
}

tppiByteStreamH264Source::tppiByteStreamH264Source(UsageEnvironment& env, tppiByteStreamH264Parameters params,
					   unsigned preferredFrameSize,
					   unsigned playTimePerFrame)
  : FramedSource(env), fPreferredFrameSize(preferredFrameSize),
    fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
    fHaveStartedReading(False), fLimitNumBytesToStream(False), 
    fNumBytesToStream(0), fParams(params), fuSecsPerFrame(1000000/params.fps), isFirstFrame(0) {

}

tppiByteStreamH264Source::~tppiByteStreamH264Source() {

}

void tppiByteStreamH264Source::doGetNextFrame() {
  if (fLimitNumBytesToStream && fNumBytesToStream == 0) {
    handleClosure();
    return;
  }

  if (!isCurrentlyAwaitingData()) {
    doStopGettingFrames(); // we're not ready for the data yet
    return;
  }

  // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
  if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
    fMaxSize = (unsigned)fNumBytesToStream;
  }
  if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
    fMaxSize = fPreferredFrameSize;
  }

  #if defined(ringbuf_cc)
  if (fParams.q.size() > 0 ) {
  #elif defined(ringbuf_c)
  if (rb_numitems(fParams.q) > 0 ) {
  #else
  #endif
    deliverFrame(); 
  } 

  // To avoid possible infinite recursion, we need to return to the event loop to do this:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
				(TaskFunc*)FramedSource::afterGetting, this);

  // After delivering the data, inform the reader that it is now available:
  //FramedSource::afterGetting(this);

}

void tppiByteStreamH264Source::deliverFrame() {
  // This function is called when new frame data is available from the device.
  // We deliver this data by copying it to the 'downstream' object, using the following parameters (class members):
  // 'in' parameters (these should *not* be modified by this function):
  //     fTo: The frame data is copied to this address.
  //         (Note that the variable "fTo" is *not* modified.  Instead,
  //          the frame data is copied to the address pointed to by "fTo".)
  //     fMaxSize: This is the maximum number of bytes that can be copied
  //         (If the actual frame is larger than this, then it should
  //          be truncated, and "fNumTruncatedBytes" set accordingly.)
  // 'out' parameters (these are modified by this function):
  //     fFrameSize: Should be set to the delivered frame size (<= fMaxSize).
  //     fNumTruncatedBytes: Should be set iff the delivered frame would have been
  //         bigger than "fMaxSize", in which case it's set to the number of bytes
  //         that have been omitted.
  //     fPresentationTime: Should be set to the frame's presentation time
  //         (seconds, microseconds).  This time must be aligned with 'wall-clock time' - i.e., the time that you would get
  //         by calling "gettimeofday()".
  //     fDurationInMicroseconds: Should be set to the frame's duration, if known.
  //         If, however, the device is a 'live source' (e.g., encoded from a camera or microphone), then we probably don't need
  //         to set this variable, because - in this case - data will never arrive 'early'.
  // Note the code below.

// add by zhoudd 2022-06-11
  { 
    data_t d{};
    #if defined(ringbuf_cc)
      fParams.q.get(&d);
    #elif defined(ringbuf_c)
      rb_get(fParams.q, &d);
    #else
    #endif
   
    if((d.size > 0) && (d.type == H264)) {
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

    // printf("consumer h264 payload %x %x %x %x %x %d\n",d.buf[0],d.buf[1],d.buf[2],d.buf[3],d.buf[4],d.size);

    // 注意：如果数据帧不干净，就需要用tppiH264ImageType()函数过滤一下！
    if(tppiH264ImageType(d.buf) == 0) return;

    // 注意：如果数据帧首帧不是关键帧, 需要丢弃前面的非关键帧！
    if(isFirstFrame == 0)
    {
        if(((d.buf[4] & 0x1F) == 5 ) || ((d.buf[3] & 0x1F) == 5 )) {isFirstFrame = 1;}
        if(((d.buf[4] & 0x1F) == 6 ) || ((d.buf[3] & 0x1F) == 6 )) {isFirstFrame = 1;}
        if(((d.buf[4] & 0x1F) == 7 ) || ((d.buf[3] & 0x1F) == 7 )) {isFirstFrame = 1;}
        if(((d.buf[4] & 0x1F) == 1 ) || ((d.buf[3] & 0x1F) == 1 )) return;                            
    }

    // 注意：如果数据帧是工整的可以设置为false, 不用判断，直接赋值，提高效率！
    #if true

    u_int8_t* newFrameDataStart = NULL;
    unsigned newFrameSize = 0;

    // start code:  00 00 00 01
    if((d.buf[3] & 0x1F) == 1 ){	
	  newFrameDataStart = (u_int8_t*)d.buf + 4; //%%% TO BE WRITTEN %%%
	  newFrameSize = d.size - 4; //%%% TO BE WRITTEN %%%
    }
    // start code:  00 00 01
    else if((d.buf[2] & 0x1F) == 1 ){	
	  newFrameDataStart = (u_int8_t*)d.buf + 3; //%%% TO BE WRITTEN %%%
	  newFrameSize = d.size - 3; //%%% TO BE WRITTEN %%%
    }
    else{}

    #else

    u_int8_t* newFrameDataStart = (u_int8_t*)d.buf + 4; //%%% TO BE WRITTEN %%%
    unsigned newFrameSize = d.size - 4; //%%% TO BE WRITTEN %%%

    #endif

	  // Deliver the data here:
	  if (newFrameSize > fMaxSize) {
		  fFrameSize = fMaxSize;
		  fNumTruncatedBytes = newFrameSize - fMaxSize;
	  } else {
		  fFrameSize = newFrameSize;
	  }

#if 0
    // Set the 'presentation time':
    if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) {
      if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
        // This is the first frame, so use the current time:
        gettimeofday(&fPresentationTime, NULL);
      } else {
        // Increment by the play time of the previous data:
        unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
        fPresentationTime.tv_sec += uSeconds/1000000;
        fPresentationTime.tv_usec = uSeconds%1000000;
      }

      // Remember the play time of this data:
      fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
      fDurationInMicroseconds = fLastPlayTime;
    } else {
      // We don't know a specific play time duration for this data,
      // so just record the current time as being the 'presentation time':
      gettimeofday(&fPresentationTime, NULL);

      //fDurationInMicroseconds = fuSecsPerFrame;
    }

#else

	  //gettimeofday(&fPresentationTime, NULL); // If you have a more accurate time - e.g., from an encoder - then use that instead.
	  // If the device is *not* a 'live source' (e.g., it comes instead from a file or buffer), 
	  // then set "fDurationInMicroseconds" here.

    // Set the 'presentation time':
    if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
      // This is the first frame, so use the current time:
      gettimeofday(&fPresentationTime, NULL);
    } else {
      // Increment by the play time of the previous frame:
      unsigned uSeconds = fPresentationTime.tv_usec + fuSecsPerFrame;
      fPresentationTime.tv_sec += uSeconds/1000000;
      fPresentationTime.tv_usec = uSeconds%1000000;
    }

    if(fParams.avSynchronize) {
      fDurationInMicroseconds = fuSecsPerFrame;
    }

#endif

    memmove(fTo, newFrameDataStart, fFrameSize);

    }   
  }
}

void tppiByteStreamH264Source::doStopGettingFrames() {
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
}
