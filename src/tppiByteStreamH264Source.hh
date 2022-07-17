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
// C++ header

#ifndef _TPPI_BYTE_STREAM_H264_SOURCE_HH
#define _TPPI_BYTE_STREAM_H264_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

#include "tppiConfig.hh"

// The following class can be used to define specific encoder parameters
class tppiByteStreamH264Parameters {
  //%%% TO BE WRITTEN %%%
public:

#if defined(ringbuf_cc)
  CircularBuffer<data_t>& q{q};
#elif defined(ringbuf_c)
  ringbuffer_t *q;
#else
#endif

  u_int8_t fps;    
  Boolean avSynchronize;
};

class tppiByteStreamH264Source: public FramedSource {
public:
  static tppiByteStreamH264Source* createNew(UsageEnvironment& env,
					 tppiByteStreamH264Parameters params,
					 unsigned preferredFrameSize = 0,
					 unsigned playTimePerFrame = 0);
  // "preferredFrameSize" == 0 means 'no preference'
  // "playTimePerFrame" is in microseconds


protected:
  tppiByteStreamH264Source(UsageEnvironment& env,
		       tppiByteStreamH264Parameters params,
		       unsigned preferredFrameSize,
		       unsigned playTimePerFrame);
	// called only by createNew()

  virtual ~tppiByteStreamH264Source();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void deliverFrame();
  virtual void doStopGettingFrames();

protected:

private:
  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  Boolean fFidIsSeekable;
  unsigned fLastPlayTime;
  Boolean fHaveStartedReading;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
  tppiByteStreamH264Parameters fParams;
  unsigned fuSecsPerFrame;
  unsigned isFirstFrame;
};

#endif
