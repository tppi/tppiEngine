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

#include "tppiByteStreamAACSource.hh"
#include "InputFile.hh"
#include "GroupsockHelper.hh"
#include "tppiConfig.hh"
#include "tppiUtil.hh"

////////// tppiByteStreamAACSource //////////

tppiByteStreamAACSource*
tppiByteStreamAACSource::createNew(UsageEnvironment& env, tppiByteStreamAACParameters params,
				unsigned preferredFrameSize,
				unsigned playTimePerFrame) {

  tppiByteStreamAACSource* newSource
    = new tppiByteStreamAACSource(env, params, preferredFrameSize, playTimePerFrame);
  
  return newSource;
}

tppiByteStreamAACSource::tppiByteStreamAACSource(UsageEnvironment& env, tppiByteStreamAACParameters params,
					   unsigned preferredFrameSize,
					   unsigned playTimePerFrame)
  : FramedSource(env), fPreferredFrameSize(preferredFrameSize),
    fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
    fHaveStartedReading(False), fLimitNumBytesToStream(False), 
    fNumBytesToStream(0), fParams(params), fuSecsPerFrame(1000000/params.fps) {

}

tppiByteStreamAACSource::~tppiByteStreamAACSource() {

}

void tppiByteStreamAACSource::doGetNextFrame() {
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

void tppiByteStreamAACSource::deliverFrame() {
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
   
    if((d.size > 0) && (d.type == AAC)) {
    u_int8_t* newFrameDataStart = (u_int8_t*)d.buf; //%%% TO BE WRITTEN %%%
    unsigned newFrameSize = d.size; //%%% TO BE WRITTEN %%%

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

void tppiByteStreamAACSource::doStopGettingFrames() {
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
}
