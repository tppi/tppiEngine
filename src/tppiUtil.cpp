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

#include <sys/time.h>
#include <cstddef>
#include "tppiUtil.hh"

char tppiH264ImageType(char* rawBuffer) {
  if ((rawBuffer[0] == 0x00) && (rawBuffer[1] == 0x00) && (rawBuffer[2] == 0x00) &&
      (rawBuffer[3] == 0x01) && ((rawBuffer[4] & 0x1F) == 7))
    return 's'; // SPS Image Start Code
  else if ((rawBuffer[0] == 0x00) && (rawBuffer[1] == 0x00) && (rawBuffer[2] == 0x00) &&
           (rawBuffer[3] == 0x01) && ((rawBuffer[4] & 0x1F) == 8))
    return 'p'; // PPS Image Start Code
  else if ((rawBuffer[0] == 0x00) && (rawBuffer[1] == 0x00) && (rawBuffer[2] == 0x00) &&
           (rawBuffer[3] == 0x01) && ((rawBuffer[4] & 0x1F) == 5))
    return 'I'; // I Image Start Code
  else if ((rawBuffer[0] == 0x00) && (rawBuffer[1] == 0x00) && (rawBuffer[2] == 0x01)){
    return 'I'; // I Image Start Code
  }
  else if ((rawBuffer[0] == 0x00) && (rawBuffer[1] == 0x00) && (rawBuffer[2] == 0x00) &&
           (rawBuffer[3] == 0x01) && ((rawBuffer[4] & 0x1F) == 1))
    return 'P'; // P Image Start Code
  else
    return 0;
}

long getCurrentTime(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

