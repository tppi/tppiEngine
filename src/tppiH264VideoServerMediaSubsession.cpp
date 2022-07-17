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
// on demand, from a H264 video ringbuf.
// Implementation

#include <liveMedia.hh>
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamFramer.hh"
#include "tppiH264VideoServerMediaSubsession.hh"
#include "tppiDeviceH264Source.hh"

tppiH264VideoServerMediaSubsession*
tppiH264VideoServerMediaSubsession::createNew(UsageEnvironment& env, 
						  tppiDeviceH264Parameters params,
					      Boolean reuseFirstSource) {
  return new tppiH264VideoServerMediaSubsession(env, params, reuseFirstSource);
}

tppiH264VideoServerMediaSubsession::tppiH264VideoServerMediaSubsession(UsageEnvironment& env,
										tppiDeviceH264Parameters params,
								        Boolean reuseFirstSource)
  : OnDemandServerMediaSubsession(env, reuseFirstSource),
    fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), fParams(params) {
}

tppiH264VideoServerMediaSubsession::~tppiH264VideoServerMediaSubsession() {
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  tppiH264VideoServerMediaSubsession* subsess = (tppiH264VideoServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void tppiH264VideoServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  tppiH264VideoServerMediaSubsession* subsess = (tppiH264VideoServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void tppiH264VideoServerMediaSubsession::checkForAuxSDPLine1() {
  nextTask() = NULL;

  char const* dasl;
  if (fAuxSDPLine != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (!fDoneFlag) {
    // try again after a brief delay:
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			      (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* tppiH264VideoServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
    // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
    // and we need to start reading data from our file until this changes.
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }

  envir().taskScheduler().doEventLoop(&fDoneFlag);

  return fAuxSDPLine;
}

FramedSource* tppiH264VideoServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 500; // kbps, estimate

  // Create the video source:
  // Open the tppi hardware encoder as a 'byte-stream live source':
  //tppiDeviceH264Parameters params;
  tppiDeviceH264Source *tppiVideoSource = tppiDeviceH264Source::createNew(envir(), fParams);
  if (tppiVideoSource == NULL) {
    envir() << "Unable to open file \"" << "tppiH264VideoStreamer \""
         << "\" as a byte-stream file source\n";
    exit(1);
  }

  // Create a framer for the Video Elementary Stream:
  return H264VideoStreamDiscreteFramer::createNew(envir(), tppiVideoSource);
}

RTPSink* tppiH264VideoServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
