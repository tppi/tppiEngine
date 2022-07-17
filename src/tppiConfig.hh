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

#ifndef TPPI_CONFIG_CPP_H
#define TPPI_CONFIG_CPP_H

#define ringbuf_cc
//#define ringbuf_c

#if defined(ringbuf_cc)
#include "tppiCircularBuffer.hh"
#elif defined(ringbuf_c)
#include "tppiRingBuffer.hh"
#else
#endif

// payload data buffer
#define ITEM_BUFFER_SIZE       (1024*500)
#define DATA_ITEM_NMAX         (16)
#define MTU                    (1452)

// payload data type
enum payload_type
{
    H264 = 1, 
    H265,
    AAC, 
    G711A,
    G711U 
};

// ringbuf data item
typedef struct{
    char buf[ITEM_BUFFER_SIZE];
    int type;
    int size;
} data_t;

#endif
