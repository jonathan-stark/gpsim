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


#include <string>
#include <iostream>

#include <pthread.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>


#include "../src/breakpoints.h"
#include "../src/processor.h"

// in input.cc -- parse_string sends a string through the command parser
extern int parse_string(char * str);

#ifndef _WIN32

unsigned int a2i(char b)
{

  if( b>='0'  && b<='9')
    return b-'0';

  if( b>='A' && b<='F')
    return b-'A'+10;

  if( b>='a' && b<='f')
    return b-'a'+10;

  return 0;

}

unsigned int ascii2uint(char **buffer, int digits)
{

  unsigned int ret = 0;
  char *b = *buffer;

  for(int i=0; i<digits; i++)
    ret = (ret << 4) + a2i(*b++);

  *buffer = b;

  return ret;
}
//

/*

  gpsim sockets.

  The purpose of a socket interface into the simulator is to provide an
  external interface that only loosely depends upon gpsim's internals.

  Phase 1:

  receive commands over a socket and execute them
*/


#define PORT 		0x1234
#define BUFSIZE 	8192

enum eGPSIMObjectTypes
  {

    // Basic types

    GPSIM_OBJTYP_CONTAINER = 1,
    GPSIM_OBJTYP_STRING    = 2,
    GPSIM_OBJTYP_INT32     = 3,
    GPSIM_OBJTYP_COMMAND   = 4,

    // commands:

    GPSIM_CMD_BREAK              = 0x80, // Query # of breaks
    GPSIM_CMD_BREAK_EXEC         = 0x81, // Set execution break
    GPSIM_CMD_BREAK_REGWRITE     = 0x82, // Set reg write break
    GPSIM_CMD_BREAK_REGREAD      = 0x83, // Set reg read break
    GPSIM_CMD_BREAK_REGWRITE_VAL = 0x84, // Set reg write val break
    GPSIM_CMD_BREAK_REGREAD_VAL  = 0x85, // Set reg read val break
    GPSIM_CMD_BREAK_STKOV        = 0x86, // Set break on stack overflow
    GPSIM_CMD_BREAK_STKUN        = 0x87, // Set break on stack underflow
    GPSIM_CMD_BREAK_WDT          = 0x88, // Set break on Watch Dog timer

    GPSIM_CMD_CLEAR       = 0x90,
    GPSIM_CMD_EXAMINE_RAM = 0x91,
    GPSIM_CMD_EXAMINE_ROM = 0x92,
    GPSIM_CMD_STEPOVER   = 0x93,
    GPSIM_CMD_RUN        = 0x94,
    GPSIM_CMD_SET        = 0x95,
    GPSIM_CMD_STEP       = 0x96,
    GPSIM_CMD_SYMBOL     = 0x97, // Query the value of a symbol
    GPSIM_CMD_VERSION    = 0x98,
    GPSIM_CMD_ASSIGN_RAM = 0x99,


  };


//--------------------
int ParseInt(char **buffer, unsigned int &retInt)
{

  char *b = *buffer;

  if(ascii2uint(&b,2) == GPSIM_OBJTYP_INT32) {
    retInt = ascii2uint(&b,8);
    *buffer = b;
    return 2+8;
  }

  return 0;
}

//--------------------
bool ParseString(char **buffer, char *retStr, int maxLen)
{

  char *b = *buffer;

  if(ascii2uint(&b,2) == GPSIM_OBJTYP_STRING) {
    int length = ascii2uint(&b,2);

    maxLen--;   // reserve space for a terminating 0.

    length = (maxLen < length) ? maxLen  : length;

    strncpy(retStr, b, length);
    retStr[length] = 0;

    *buffer = b + length;
    return 2+2+length;
  }

  return 0;
}

//------------------------------------------------------------------------
// Socket wrapper class
// 
// This class is a simple wrapper around the standard BSD socket calls.
// 

class Socket
{
public:


  Socket();
  ~Socket();


  void init();

  void Close(int &);
  void Bind();
  void Listen();
  void Accept();
  void Recv();
  void Send(char *);

  void ParseObject(char *);

  void receive();
  void respond(char *);

  bool bHaveClient()
  {
    return (client_socket != -1);
  }

  //private:
  char     buffer[BUFSIZE];
  int 	   my_socket, client_socket;
  struct   sockaddr_in addr;
};







Socket::Socket()
{
  my_socket = -1;
  client_socket = -1;

}

Socket::~Socket()
{
  Close(my_socket);
  Close(client_socket);

}


void Socket::init(void)
{
 
  if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  std::cout << "initializing socket\n";

  int on = 1;
  if ( setsockopt ( my_socket, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 ) {
    perror("setsockopt");
    exit(1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);

  Bind();
  Listen();
}

void Socket::Close(int &sock)
{
  if(sock != -1)
    close(sock);

  sock = -1;

}

void Socket::Bind()
{
  if (bind(my_socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    perror("bind");
  }
}

void Socket::Listen()
{
  if (listen(my_socket, 5) == -1) {
    perror("listen");
  }
}

void Socket::Accept()
{
  socklen_t addrlen = sizeof(addr);
  if ((client_socket = accept(my_socket, (struct sockaddr *)  &addr, &addrlen)) == -1) {
    perror("accept");
    exit(1);
  }

}

void Socket::Recv()
{

  memset(buffer, 0, sizeof(buffer));

  int ret = recv(client_socket, buffer, sizeof(buffer), 0);
  if (ret == -1) {
    perror("recv");
    close(my_socket);
    close(client_socket);
    client_socket = -1;
    my_socket = -1;
  }
  
  if (ret == 0) {
    printf("recv: 0 bytes received\n");
    close(my_socket);
    close(client_socket);
  }
}

void Socket::Send(char *b)
{
  //printf("socket-sending %s",buffer);
  if (send(client_socket, b, strlen(b), 0) == -1) {
    perror("send");
    close(my_socket);
    close(client_socket);
  }
}



void Socket::receive(void)
{
  if(!bHaveClient()) {
    init();
    Accept();
  }

  Recv();

  // Process the string that was just received.

  if(strlen(buffer)) {
    printf("received %s\n",buffer);

    if (*buffer == '$')
      ParseObject(&buffer[1]);
    else {
      parse_string(buffer);
      respond("ACK");
    }
  }
}


void Socket::respond(char *buf)
{
  if(bHaveClient() && buf)
    Send(buf);
}

//------------------------------------------------------------------------
// ParseObject
//
// Given a pointer to a string, this routine will extract gpsim 
// objects from it. These objects essentially tell gpsim how to
//
//   * * * W A R N I N G * * *
//
// This code will change. 
//

void Socket::ParseObject(char *buffer)
{

  int buffer_len = strlen(buffer);

  char *b = buffer;

  while(buffer_len > 0) {

    unsigned int ObjectType = ascii2uint(&b,2);
    buffer_len -= 2;

    switch (ObjectType) {

    case GPSIM_OBJTYP_CONTAINER:
      printf("Container of %d objects\n",ascii2uint(&b,2));
      buffer_len -= 2;
      respond("Container");
      
      break;

    case GPSIM_OBJTYP_STRING:
      {
	char tmp[256];
	
	int length = ascii2uint(&b,2);

  	length = (255 < length) ? 255  : length;

	strncpy(tmp, b, length);
	tmp[length] = 0;
	
	printf("String %s\n",tmp);
	buffer_len -= (length+2);
	respond("String");
       

      }
      break;

    case GPSIM_OBJTYP_INT32:
      {
	int i = ascii2uint(&b,8);
	printf("Integer %d\n",i);
	buffer_len -= 8;
	respond("Integer");
      }
      break;

    case GPSIM_CMD_BREAK:
    case GPSIM_CMD_BREAK_EXEC:
    case GPSIM_CMD_BREAK_REGWRITE:
    case GPSIM_CMD_BREAK_REGREAD:
    case GPSIM_CMD_BREAK_REGWRITE_VAL:
    case GPSIM_CMD_BREAK_REGREAD_VAL:
    case GPSIM_CMD_BREAK_STKOV:
    case GPSIM_CMD_BREAK_STKUN:
    case GPSIM_CMD_BREAK_WDT:


    case GPSIM_CMD_CLEAR:
    case GPSIM_CMD_EXAMINE_ROM:
    case GPSIM_CMD_STEPOVER:
    case GPSIM_CMD_RUN:
    case GPSIM_CMD_SET:
    case GPSIM_CMD_STEP:
    case GPSIM_CMD_VERSION:

      {
	printf("Command\n");
	respond("Command");

      }
      break;
    case GPSIM_CMD_SYMBOL:
      {
	char tmp[256];
	int bl = ParseString(&b,tmp,256);
	if(bl) {
	  buffer_len -= bl;
	  printf("Symbol command with string %s\n",tmp);
	  symbol *sym = get_symbol_table().find(tmp);
	  if(sym) {
	    snprintf(tmp,sizeof(tmp),"$03%08x",sym->get_value());
	    printf("responding with %s\n",tmp);
	    respond(tmp);
	  } else
	    respond("symcmd");
	}
      }
      break;

    case GPSIM_CMD_EXAMINE_RAM:
      {
	unsigned int ram_address;
	int bl = ParseInt(&b,ram_address);
	if(bl) {
	  buffer_len -= bl;

	  char tmp[256];
	  Processor *cpu = get_active_cpu();

	  if(cpu) {
	    Register *reg = cpu->rma.get_register(ram_address);
	    snprintf(tmp,sizeof(tmp),"$03%08x",reg->get_value());
	    respond(tmp);
	  } else
	    respond("no cpu");
	} else 
	  respond("examinecmd");
 
      }
      break;

    case GPSIM_CMD_ASSIGN_RAM:
      {
	unsigned int ram_address;
	unsigned int ram_value;
	int bl =0;

	bl += ParseInt(&b,ram_address);
	bl += ParseInt(&b,ram_value);

	if(bl) {
	  buffer_len -= bl;

	  Processor *cpu = get_active_cpu();

	  if(cpu) {
	    Register *reg = cpu->rma.get_register(ram_address);
	    reg->put_value(ram_value);
	    respond("ACK");
	  } else
	    respond("no cpu");
	} else 
	  respond("assigncmd");
 
      }
      break;

    default:
      printf("Invalid object type: %d\n",ObjectType);
    }

  }
}

#ifdef USE_THREADS_BUT_NOT_RECOMMENDED_BECAUSE_OF_CROSS_PLATFORM_ISSUES
void *server_thread(void *ignored)
{
  std::cout << "running....\n";

  Socket s;

  while ( true )
    {
      s.receive();
    }

  return 0;
}



pthread_t thSocketServer;
pthread_attr_t thAttribute;
int something=1;

int start_server(void)
{
  std::cout << "starting server....\n";

  pthread_attr_init(&thAttribute);
  pthread_attr_setdetachstate(&thAttribute, PTHREAD_CREATE_JOINABLE);
  pthread_create(&thSocketServer, &thAttribute, server_thread, (void *)&something);

  std::cout << " started server\n";
  return 0;
}
#else   // if USE_THREADS


gboolean server_callback(GIOChannel *channel, GIOCondition condition, void *d )
{
  std::cout << " Server callback\n";

  Socket *s = (Socket *)d;

  switch(condition) {
  case G_IO_IN:
    {
      unsigned int bytes_read=0;

      memset(s->buffer, 0, 256);

      std::cout << "Reading bytes\n";
      g_io_channel_read(channel, s->buffer, BUFSIZE, &bytes_read);
      std::cout << "Read " << bytes_read << " bytes: " << s->buffer << endl;

      if(bytes_read) {
	if (*s->buffer == '$')
	  s->ParseObject(&s->buffer[1]);
	else {
	  parse_string(s->buffer);
	  s->respond("ACK");
	}
      } else
	return FALSE;

      return TRUE;
    }
    break;
  case G_IO_HUP:
    std::cout << "client sent HUP\n";
    return FALSE;
  case G_IO_OUT:
    std::cout << "OUT\n";
    break;
  case G_IO_PRI:
    std::cout << "PRI\n";
    break;
  case G_IO_ERR:
    std::cout << "ERR\n";
    break;
  case G_IO_NVAL:
    std::cout << "NVAL\n";
    break;
  default:
    std::cout << "Unrecognized condition\n";
  }

  return FALSE;
}

gboolean server_accept(GIOChannel *channel, GIOCondition condition, void *d )
{
  Socket *s = (Socket *)d;


  s->Accept();


  GIOChannel *new_channel = g_io_channel_unix_new(s->client_socket);
  GIOCondition new_condition = (GIOCondition)( G_IO_IN | G_IO_HUP |
					   G_IO_ERR );



  g_io_add_watch(new_channel, 
		 new_condition,
		 server_callback,
		 (void*)s);

  return true;
}

int start_server(void)
{

  static Socket s;

  std::cout << "starting server....\n";

  s.init();

  if(s.my_socket > 0) {

    GIOChannel *channel = g_io_channel_unix_new(s.my_socket);
    GIOCondition condition = (GIOCondition)( G_IO_IN | G_IO_HUP |
					     G_IO_ERR );



    g_io_add_watch(channel, 
		   condition,
		   server_accept,
		   (void*)&s);
  }

  std::cout << " started server\n";
  return 0;

}

#endif   // if USE_THREADS
#endif   // if !_WIN32
