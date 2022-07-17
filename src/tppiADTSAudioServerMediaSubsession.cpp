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
// Implementation

#include <liveMedia.hh>
#include "MPEG4GenericRTPSink.hh"
#include "tppiADTSAudioServerMediaSubsession.hh"
#include "tppiDeviceAACSource.hh"

static unsigned const samplingFrequencyTable[16] = {
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};

tppiADTSAudioServerMediaSubsession*
tppiADTSAudioServerMediaSubsession::createNew(UsageEnvironment& env, tppiDeviceAACParameters params, Boolean reuseFirstSource) {
  return new tppiADTSAudioServerMediaSubsession(env, params, reuseFirstSource);
}

tppiADTSAudioServerMediaSubsession
::tppiADTSAudioServerMediaSubsession(UsageEnvironment& env, tppiDeviceAACParameters params, Boolean reuseFirstSource)
  : OnDemandServerMediaSubsession(env, reuseFirstSource), fParams(params) {

  fSamplingFrequency = samplingFrequencyTable[params.samplingFrequencyIndex];
  fNumChannels = params.channelConfiguration == 0 ? 2 : params.channelConfiguration;
  fuSecsPerFrame
    = (1024/*samples-per-frame*/*1000000) / fSamplingFrequency/*samples-per-second*/;

  // Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
  unsigned char audioSpecificConfig[2];
  u_int8_t const audioObjectType = params.profile + 1;
  audioSpecificConfig[0] = (audioObjectType<<3) | (params.samplingFrequencyIndex>>1);
  audioSpecificConfig[1] = (params.samplingFrequencyIndex<<7) | (params.channelConfiguration<<3);
  sprintf(fConfigStr, "%02X%02X", audioSpecificConfig[0], audioSpecificConfig[1]);

}

tppiADTSAudioServerMediaSubsession
::~tppiADTSAudioServerMediaSubsession() {
}

FramedSource* tppiADTSAudioServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 1; // kbps, estimate

  // Create the video source:
  // Open the tppi hardware encoder as a 'byte-stream live source':
  //tppiDeviceAACParameters params;
  tppiDeviceAACSource *tppiAudioSource = tppiDeviceAACSource::createNew(envir(), fParams);
  if (tppiAudioSource == NULL) {
    envir() << "Unable to open file \"" << "tppiAACAudioStreamer \""
         << "\" as a byte-stream file source\n";
    exit(1);
  }

  // Create a framer for the Video Elementary Stream:
  return ADTSAudioStreamDiscreteFramer::createNew(envir(), tppiAudioSource, configStr());
}

RTPSink* tppiADTSAudioServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* inputSource) {

  return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					rtpPayloadTypeIfDynamic,
					samplingFrequency(),
					"audio", "AAC-hbr", configStr(),
					numChannels());
}
