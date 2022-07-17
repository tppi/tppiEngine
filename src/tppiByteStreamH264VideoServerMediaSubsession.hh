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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a H264 Elementary Stream video ringbuf.
// C++ header

#ifndef _TPPI_ByteStream_H264_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#define _TPPI_ByteStream_H264_VIDEO_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAN_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif

#include "tppiByteStreamH264Source.hh"

class tppiByteStreamH264VideoServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
  static tppiByteStreamH264VideoServerMediaSubsession*
  createNew(UsageEnvironment& env, tppiByteStreamH264Parameters params, Boolean reuseFirstSource);

  // Used to implement "getAuxSDPLine()":
  void checkForAuxSDPLine1();
  void afterPlayingDummy1();

protected:
  tppiByteStreamH264VideoServerMediaSubsession(UsageEnvironment& env,
  					   tppiByteStreamH264Parameters params,
				       Boolean reuseFirstSource);
      // called only by createNew();
  virtual ~tppiByteStreamH264VideoServerMediaSubsession();

  void setDoneFlag() { fDoneFlag = ~0; }

protected: // redefined virtual functions
  virtual char const* getAuxSDPLine(RTPSink* rtpSink,
				    FramedSource* inputSource);
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

private:
  char* fAuxSDPLine;
  char fDoneFlag; // used when setting up "fAuxSDPLine"
  RTPSink* fDummyRTPSink; // ditto
  tppiByteStreamH264Parameters fParams;
};

#endif
