/*
   Copyright (C) 2004 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

//
// protocol.h
//


#ifndef __PROTCOL_H__
#define __PROTCOL_H__

/// gpsim protocol
///
/// gpsim's protocol interface is designed to provide a way for clients 
/// that are not linked with gpsim to interface with gpsim. 


/// Basic types
/// These are the fundamental types the protocol interface
/// supports.

enum eGPSIMObjectTypes
  {
    eGPSIM_TYPE_CHAR = 1,
    eGPSIM_TYPE_STRING,
    eGPSIM_TYPE_UINT32,
    eGPSIM_TYPE_UCHAR,
    eGPSIM_TYPE_BOOLEAN,
    eGPSIM_TYPE_INT32,
    eGPSIM_TYPE_INT64,
    eGPSIM_TYPE_UINT64,
    eGPSIM_TYPE_FLOAT,
    eGPSIM_TYPE_DOUBLE,
    eGPSIM_TYPE_OBJECT,
    eGPSIM_TYPE_CUSTOM,
  };



/// PacketBuffer
/// A packet buffer is an area of memory that gpsim and a client
/// use to exchange information. The buffer consists of a sequence
/// encoded GPSIMObjectTypes. Member functions for encoding and 
/// decoding each type.

class PacketBuffer
{
public:
  PacketBuffer(unsigned int _size);
  ~PacketBuffer();


  char * getBuffer()
  {
    return &buffer[index];
  }

  unsigned int getSize()
  {
    return size;
  }

  void putc(char c)
  {
    if(index < size)
      buffer[index++] = c;
  }

  void puts(const char *, int);

  //private:
  char          *buffer;
  unsigned int   index;
  unsigned int   size;

};

class Packet
{
public:
  Packet(unsigned int rxsize, unsigned int txsize);

  bool DecodeHeader();
  bool DecodeObjectType(unsigned int &);
  bool DecodeUInt32(unsigned int &);
  bool DecodeUInt64(unsigned long long &);
  bool DecodeString(char *, int);
  bool DecodeBool(bool &);
  bool DecodeFloat(double &);

  bool EncodeUInt32(unsigned int);
  bool EncodeObjectType(unsigned int);
  bool EncodeString(const char *str, int len=-1);

  char *rxBuff()
  {
    return rxBuffer->getBuffer();
  }
  unsigned int rxSize()
  {
    return rxBuffer->size;
  }

  char *txBuff()
  {
    return txBuffer->buffer;
  }
  unsigned int txBytesBuffered()
  {
    return txBuffer->index;
  }

  void prepare()
  {
    rxBuffer->index = 0;
    txBuffer->index = 0;
  }

private:
  PacketBuffer *rxBuffer;
  PacketBuffer *txBuffer;

};

extern unsigned int a2i(char b);
extern unsigned int ascii2uint(char **buffer, int digits);

#endif
