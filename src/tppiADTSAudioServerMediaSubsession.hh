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
// on demand, from an AAC audio ringbuf in ADTS format
// C++ header

#ifndef _TPPI_ADTS_AUDIO_SERVER_MEDIA_SUBSESSION_HH
#define _TPPI_ADTS_AUDIO_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAN_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif

#include "tppiDeviceAACSource.hh"

class tppiADTSAudioServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
  static tppiADTSAudioServerMediaSubsession* createNew(UsageEnvironment& env, 
  														tppiDeviceAACParameters params,
  														Boolean reuseFirstSource);

  unsigned samplingFrequency() const { return fSamplingFrequency; }
  unsigned numChannels() const { return fNumChannels; }
  char const* configStr() const { return fConfigStr; }
      // returns the 'AudioSpecificConfig' for this stream (in ASCII form)

protected:
  tppiADTSAudioServerMediaSubsession(UsageEnvironment& env, tppiDeviceAACParameters params, Boolean reuseFirstSource);
      // called only by createNew();
  virtual ~tppiADTSAudioServerMediaSubsession();

protected: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

private:
  unsigned fSamplingFrequency;
  unsigned fNumChannels;
  unsigned fuSecsPerFrame;
  char fConfigStr[5];
  tppiDeviceAACParameters fParams;
};
#endif
