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

/*
  Scripting

  client.cc - a program to demonstrate gpsim scripting.

  The purpose of this program is to demonstrate how one might write a
  script to perform regression testing on an algorithm. In this
  example, the algorithm is one that squares an 8-bit number. It is
  implemented in the PIC source code gensquares.asm. 

  The scripting mechanism is implemented on top of a socket
  interface. When gpsim is started, it will act as a server and began
  listening to clients. This client will initiate the communication
  link. It probably goes with out saying, but gpsim needs to be
  running before this client script will even work!

  Once this client establishs a communication link, it will proceed to
  set up the simulation environment and perform a regression test.

  Technical details:

  The scripting interface is implemented with a gpsim specific
  protocol. This protocol is somewhat analogous to gdb's
  protocol. gpsim's protocol operates in two modes. In one mode,
  character strings are sent across the socket interface and sent
  through the command line parser. Thus, it's possible to issue any
  gpsim command through a socket that can be typed at the command
  line. As of this writing, this interface is only uni-directional
  though. In other words, the client can only issue commands, but
  can't see the response. However, the other mode of the protocol
  allows bi-directional communication. This other mode ties directly
  into gpsim symbols and attributes. 

  In the attribute mode of the protocol, there are two ways to
  interface to the symbols. The first is by symbol name and the second
  is by a 'handle'. For either one of these, it's possible to read and
  write symbols. Handles are more efficient since they only have to be
  processed only one. Accessing by name on the other hand, requires
  string processing and symbol table accesses. 

*/


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#ifdef putc 
#undef putc
#endif

#include <gpsim/protocol.h>



/*
  The makefile in this directory should build the executable just
  fine, however here are the commands you'd enter from the command
  line:

  g++ -c  client.cc `pkg-config --cflags glib-2.0`
  g++ -o client client.o protocol.o -lstdc++ `pkg-config --libs glib-2.0`

*/

#define PORT        0x1234

int Send(int sd, const char *msg, int msgLength,
	 char *response, int respLength)
{

  if(!msg || !msgLength ||  !response || !respLength)
    return 0;

  if (send(sd, msg, msgLength, 0) == -1) {
    perror("send");
    exit(1);
  }

  int bytes = recv(sd, response, respLength, 0);
  if (bytes == -1) {
    perror("recv");
    exit(1);
  }

  return bytes;
}

//========================================================================
bool SendCmd(int sock, const char *cmd)
{
  if(cmd) {

    char response[256];

    int len = Send(sock, cmd, strlen(cmd), response, sizeof(response));

    if(len && len <sizeof(response)) {
      response[len] = 0;
    }


    return true;
  }

  return false;
}
//========================================================================
int Send(int sd, Packet &p)
{

  //printf("Send packet sending %s\n", p.txBuff());
  if (send(sd, p.txBuff(), p.txBytesBuffered(), 0) == -1) {
    perror("send");
    exit(1);
  }
  int bytes= recv(sd, p.rxBuff(), p.rxSize(), 0);
  if (bytes  == -1) {
    perror("recv");
    exit(1);
  }
  p.rxTerminate(bytes);
  //printf("Send packet response %s\n", p.rxBuff());

}

//========================================================================
int CreateSocket(const char *hostname)
{

  struct  sockaddr_in sin;
  struct  sockaddr_in pin;
  struct  hostent *hp;
  int	  sd;

  char        hname_buff[256];
  const char *hname;

  if(!hostname) {
    gethostname(hname_buff, sizeof(hname_buff));
    hname = hname_buff;
  } else
    hname = hostname;

  if ((hp = gethostbyname(hname)) == 0) {
    perror("gethostbyname");
    exit(1);
  }

  /* fill in the socket structure with host information */
  memset(&pin, 0, sizeof(pin));
  pin.sin_family = AF_INET;
  pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
  pin.sin_port = htons(PORT);

  /* grab an Internet domain socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  /* connect to PORT on HOST */
  if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
    perror("connect");
    exit(1);
  }

  return sd;
}

//========================================================================
//
unsigned int CreateLink(int sock, const char *sym_name, Packet &p)
{
  p.prepare();

  p.EncodeObjectType(GPSIM_CMD_CREATE_SOCKET_LINK);
  p.EncodeString(sym_name);

  Send(sock, p);

  unsigned int handle=0;

  if(p.DecodeHeader() && p.DecodeUInt32(handle) )
    return handle;

  return 0;
}

void RemoveLink(int sock, unsigned int  handle, Packet &p)
{
  p.prepare();

  p.EncodeObjectType(GPSIM_CMD_REMOVE_SOCKET_LINK);
  p.EncodeUInt32(handle);

  Send(sock,p);


}

void QueryLink(int sock, unsigned int  handle, Packet &p)
{
  p.prepare();

  p.EncodeObjectType(GPSIM_CMD_QUERY_SOCKET_LINK);
  p.EncodeUInt32(handle);

  Send(sock,p);
}

void WriteToLink(int sock, unsigned int  handle, Packet &p, unsigned int val)
{
  p.prepare();
  p.EncodeObjectType(GPSIM_CMD_WRITE_TO_SOCKET_LINK);
  p.EncodeUInt32(handle);
  p.EncodeUInt32(val);
}

bool QuerySymbolUInt32(int sock, const char *sym_name, Packet &p, unsigned int &i)
{
  p.prepare();
  p.EncodeObjectType(GPSIM_CMD_QUERY_SYMBOL);
  p.EncodeString(sym_name);
  p.txTerminate();
  Send(sock,p);

  if(p.DecodeHeader() && p.DecodeUInt32(i))
    return true;

  return false;
}

bool WriteSymbolUInt32(int sock, const char *sym_name, Packet &p, unsigned int v)
{
  p.prepare();
  p.EncodeObjectType(GPSIM_CMD_WRITE_TO_SYMBOL);
  p.EncodeString(sym_name);
  p.EncodeUInt32(v);
  p.txTerminate();
  Send(sock,p);
}

//========================================================================
//
// 
//
int main(int argc, char **argv)
{

  bool bPassed = true;  // regression test results - assume we pass

  int	  sd;
  sd = CreateSocket( argc>2 ? argv[2] : 0);

  Packet p(8192,8192);


  /*
    Set up.

    Since we contrrlling the simulator entirely from a script, there really is
    not much need for the command line output it provides. So we'll turn it off
    by setting the simulator attribute 'sim.verbosity' to zero.

  */

  WriteSymbolUInt32(sd, "sim.verbosity",p,0);


  /*
    The simulator's command line parser is still available for us to use.
    So we'll use it to load the source code and to set a break point.
  */
  SendCmd(sd,"load s gensquares.cod\n");
  SendCmd(sd,"break e start\n");

  /*
    There's a break point set at the label 'start'. Run to it.
  */
  SendCmd(sd,"run\n");


  /*
    Now, we'll establish links to some of the internal symbols we're
    interested in. These links are called 'handles'.
  */

  unsigned int h_count = CreateLink(sd,"count",p);
  unsigned int h_x_lo = CreateLink(sd,"x_lo",p);
  unsigned int h_x_hi = CreateLink(sd,"x_hi",p);

  if(!h_count || !h_x_lo || !h_x_hi) {
    printf("unable to create handles\n");
    close(sd);
    exit(1);
  }


  /*
    
  */

  unsigned int count;
  unsigned int x_lo=0;
  unsigned int x_hi=0;

  for(count=0; count < 128; count++) {

    WriteToLink(sd,h_count, p, count);

    Send(sd,p);

    SendCmd(sd,"run\n");

    QueryLink(sd,h_x_lo,p);
    if(! (p.DecodeHeader() && p.DecodeUInt32(x_lo))) {
      printf("failed to decode x_lo");
    }

    QueryLink(sd,h_x_hi,p);
    if(! (p.DecodeHeader() && p.DecodeUInt32(x_hi))) {
      printf("failed to decode x_hi");
    }

    /*
      Check the results.
    */
    if((count*count) != ((x_hi<<8) + x_lo)) {
      unsigned int a = count *count;
      unsigned int b = (x_hi<<8) + x_lo;
      printf("%x  %x  ", a, b);
      bPassed = false;
      printf("failed sent: 0x%02X  received 0x%02X%02X\n",count, x_hi,x_lo);
    }
  }

  /*
    release the handles created above.
  */

  RemoveLink(sd,h_count,p);
  RemoveLink(sd,h_x_lo,p);
  RemoveLink(sd,h_x_hi,p);

  
  WriteSymbolUInt32(sd, "sim.verbosity",p,1);

  close(sd);

  if(bPassed) 
    printf("The simulation passed!\n");
  else
    printf("The simulation failed\n");

  return 0;
}

 
