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

#ifndef _WIN32
#include <pthread.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

//#include "../src/processor.h"
#include "../src/symbol.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif


//******** E X P E R I M E N T A L   C O D E !!! *************************
// Here is an experimental attribute for testing the socket interface.
// The purpose is to isolate socket testing from the rest of the simulator.
//
class TestInt32Array : public symbol
{
public:
  TestInt32Array(const char *_name, int _sz)
    : symbol(_name)
  {
    set_description(" test array for testing sockets");

    size = _sz;
    array = new int[size];
  }
  ~TestInt32Array()
  {
    delete[] array;
  }

  virtual void set(const char *cP,int len=0)
  {
    std::cout << name() << " set\n";
  }
  virtual void get(char *, int len)
  {
    std::cout << name() << " get\n";
  }
  virtual string toString()
  {
    return name();
  }

private:
  int size;
  int *array;

};


//*************************  end of experimental code ********************


class SocketLink;

// in input.cc -- parse_string sends a string through the command parser
extern int parse_string(char * str);

#ifdef WIN32
#define snprintf  _snprintf
#define socklen_t int

bool server_started = false;


void psocketerror(const char *str)
{
  fprintf (stderr, "%s: %s\n", str, g_win32_error_message(WSAGetLastError()));
}


bool winsockets_init(void)
{
  WSADATA wsaData;
  int err;

  if (0 != (err = WSAStartup( MAKEWORD( 2, 2 ), &wsaData )))
    return false;

  /* Confirm that the WinSock DLL supports 2.2.*/
  /* Note that if the DLL supports versions greater    */
  /* than 2.2 in addition to 2.2, it will still return */
  /* 2.2 in wVersion since that is the version we      */
  /* requested.                                        */

  if (LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) {
    /* Tell the user that we could not find a usable */
    /* WinSock DLL.                                  */
    WSACleanup( );
    return false;
  }

  return true;
}
#else
#define psocketerror(s) perror(s)
#define closesocket(s)  close(s)
#define SOCKET          int
#define INVALID_SOCKET  -1
#endif


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

enum eGPSIMSocketCommands
  {

    GPSIM_OBJTYP_STRING    = 2,
    GPSIM_OBJTYP_INT32     = 3,

    GPSIM_CMD_CREATE_SOCKET_LINK    = 0xF0,
    GPSIM_CMD_REMOVE_SOCKET_LINK    = 0xF1,
    GPSIM_CMD_QUERY_SOCKET_LINK     = 0xF2,
    GPSIM_CMD_WRITE_TO_SOCKET_LINK  = 0xF3,

    GPSIM_CMD_QUERY_SYMBOL          = 0xF4,
    GPSIM_CMD_WRITE_TO_SYMBOL       = 0xF5,

  };


#if 0
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

    GPSIM_CMD_TEST_BINARY = 0xa0,
#endif

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
int ParseString(char **buffer, char *retStr, int maxLen)
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
int ParseSocketLink(char **buffer, SocketLink **);

//------------------------------------------------------------------------
// Socket wrapper class
// 
// This class is a simple wrapper around the standard BSD socket calls.
// 

// FIXME - separate the client_socket into it's own class. This will allow
// the one server to listen to many clients.

class SocketBase
{
public:
  SocketBase(SOCKET s);
  SOCKET getSocket();
  void Close();
private:
  SOCKET socket;
};

class Socket
{
public:


  Socket();
  ~Socket();


  void init();

  void Close(SocketBase**);
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
    return (client != 0);
  }


  //private:
  char     buffer[BUFSIZE];
  SocketBase *my_socket;
  SocketBase *client;
  struct   sockaddr_in addr;
};




class SocketLink
{
public:
  SocketLink(unsigned int _handle, Socket *);
  void receive(char *);
  void send(char *);

  virtual void set(char *s, int i=0)=0;
  virtual void get(char *, int)=0;
  unsigned int getHandle() { return handle; }
private:
  unsigned int handle;
  Socket *sock;
};

class AttributeLink : public SocketLink
{
public:
  AttributeLink(unsigned int _handle, Socket *, Value *);

  void set(char *s, int i=0);
  void get(char *, int);

private:
  Value *v;
};

#define nSOCKET_LINKS 16

SocketLink *links[nSOCKET_LINKS];

SocketLink *gCreateSocketLink(unsigned int, Socket *, char**);

unsigned int FindFreeHandle()
{
  unsigned int i;
  static unsigned int sequence = 0;

  for(i=0; i<nSOCKET_LINKS; i++)
    if(links[i] == 0)
      return i | (++sequence << 16);

  return 0xffff;
}
//========================================================================
SocketBase::SocketBase(SOCKET s)
  : socket(s)
{
}
SOCKET SocketBase::getSocket()
{
  return socket;
}

void SocketBase::Close()
{
  if(socket != INVALID_SOCKET)
    closesocket(socket);

  socket = INVALID_SOCKET;

}

//========================================================================
int ParseSocketLink(char **buffer, SocketLink **sl)
{
  if(!sl)
    return 0;

  char *b = *buffer;
  int ret = 0;

  unsigned int handle;

  ret += ParseInt(&b,handle);

  if(ret) {
    //unsigned int sequence = (handle>>16) & 0xffff;
    unsigned int index    = handle & 0xffff;

    *sl = links[index&0x0f];
    if( (*sl) && (*sl)->getHandle() != handle)
      *sl = 0;
  }

  return ret;

}

//========================================================================
void CloseSocketLink(SocketLink *sl)
{
  if(!sl)
    return;

  unsigned int handle = sl->getHandle();
  if(links[handle&0x000f] == sl)
    links[handle&0x000f] = 0;

}
//========================================================================
Socket::Socket()
{
  my_socket = 0;
  client = 0;

  for(int i=0; i<nSOCKET_LINKS; i++)
    links[i] = 0;
}


Socket::~Socket()
{
  Close(&my_socket);
  Close(&client);

}


void Socket::init(void)
{
  SOCKET new_socket;

  if ((new_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    psocketerror("socket");
    exit(1);
  }

  my_socket = new SocketBase(new_socket);

  int on = 1;
  if ( setsockopt ( new_socket, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) != 0 ) {
    psocketerror("setsockopt");
    exit(1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);

  Bind();
  Listen();
}


void Socket::Close(SocketBase **sock)
{
  if (sock && *sock)
    (*sock)->Close();

  *sock = 0;

}


void Socket::Bind()
{
  if(!my_socket)
    return;

  if (bind(my_socket->getSocket(), (struct sockaddr *) &addr, sizeof(addr)) != 0) {
    psocketerror("bind");
  }
}


void Socket::Listen()
{
  if(!my_socket)
    return;

  if (listen(my_socket->getSocket(), 5) != 0) {
    psocketerror("listen");
  }
}


void Socket::Accept()
{
  socklen_t addrlen = sizeof(addr);
  SOCKET client_socket = accept(my_socket->getSocket(),(struct sockaddr *)  &addr, &addrlen);

  if (client_socket == INVALID_SOCKET) {
    psocketerror("accept");
    exit(1);
  }

  client = new SocketBase(client_socket);
}


void Socket::Recv()
{

  memset(buffer, 0, sizeof(buffer));

  if(!client)
    return;

  int ret = recv(client->getSocket(), buffer, sizeof(buffer), 0);

  if (ret < 0) {
    psocketerror("recv");
    Close(&my_socket);
    Close(&client);
  }
  
  if (ret == 0) {
    printf("recv: 0 bytes received\n");
    Close(&client);
  }
}


void Socket::Send(char *b)
{
  if(!client)
    return;

  if (send(client->getSocket(), b, strlen(b), 0) < 0) {
    psocketerror("send");
    closesocket(client->getSocket());
    client = 0;
  }
}


void Socket::receive(void)
{
  if (!bHaveClient()) {
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
  if (bHaveClient() && buf)
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


  unsigned int ObjectType = ascii2uint(&b,2);
  buffer_len -= 2;

  switch (ObjectType) {

  case GPSIM_CMD_CREATE_SOCKET_LINK:
    {
      unsigned int handle = FindFreeHandle();

      links[handle&0x0f] = gCreateSocketLink(handle, this, &b);
    }
    break;

  case GPSIM_CMD_REMOVE_SOCKET_LINK:
    {
      SocketLink *sl=0;

      ParseSocketLink(&b, &sl);

      if(sl)
	CloseSocketLink(sl);
      Send("+");
    }
    break;

  case GPSIM_CMD_QUERY_SOCKET_LINK:
    {
      SocketLink *sl=0;

      ParseSocketLink(&b, &sl);

      if(sl) {
	char buf[256];
	buf[0]='+';
	sl->get(&buf[1], sizeof(buf)-1);
	sl->send(buf);
      }
    }
    break;

  case GPSIM_CMD_WRITE_TO_SOCKET_LINK:
    {
      SocketLink *sl=0;

      int bl = ParseSocketLink(&b, &sl);
      //buffer_len -= bl;
      b += bl;

      if(sl) {
	sl->set(b);
	sl->send("+");
      }

    }
    break;

  case GPSIM_CMD_QUERY_SYMBOL:
    {
      char tmp[256];
      int bl = ParseString(&b,tmp,256);
      if(bl) {
	buffer_len -= bl;
	printf("Symbol command with string %s\n",tmp);
	Value *sym = get_symbol_table().find(tmp);
	if(sym) {
	  tmp[0]='+';
	  sym->get(&tmp[1], sizeof(tmp));
	  //snprintf(tmp,sizeof(tmp),"$03%08x",i);
	  printf("responding with %s\n",tmp);
	  respond(tmp);
	} else
	  respond("-");
      }
    }
    break;


  case GPSIM_CMD_WRITE_TO_SYMBOL:
    {
      char tmp[256];
      int bl = ParseString(&b,tmp,256);
      if(bl) {
	buffer_len -= bl;
	printf("Symbol write command with string %s\n",tmp);
	Value *sym = get_symbol_table().find(tmp);

	if(sym) {

	  sym->set(b);

	  printf("writing %s to %s\n",b,tmp);
	  respond(tmp);
	} else
	  respond("-");
      }
    }
    break;


  default:
    printf("Invalid object type: %d\n",ObjectType);
    respond("-");

  }

}


//========================================================================

SocketLink::SocketLink(unsigned int _handle, Socket *s)
  : handle(_handle), sock(s)
{
  char  buf[256];

  sprintf(buf, "+%08X:LINK",handle);
  send(buf);
}

void SocketLink::receive(char *m)
{
  std::cout<< "SocketLink::receive:" << m;
}
void SocketLink::send(char *m)
{
  if(sock)
    sock->respond(m);
}

//========================================================================
AttributeLink::AttributeLink(unsigned int _handle, Socket *s, Value *_v)
  : SocketLink(_handle,s), v(_v)
{
}

void AttributeLink::set(char *b, int len)
{
  if(v)
    v->set(b,len);
}

void AttributeLink::get(char *b, int len)
{
  if(v)
    v->get(b,len);
}

//========================================================================

SocketLink *gCreateSocketLink(unsigned int handle, Socket *s, char **type)
{
  if(type && *type) {
    cout << "creating socket link of type:" << *type << endl;

    char tmp[256];
    char *b = *type;

    int bl = ParseString(&b,tmp,256);

    if(bl) {

      printf("Creating socket link for  %s\n",tmp);
      Value *sym = get_symbol_table().find(tmp);
      if(sym)
	return new AttributeLink(handle,s,sym);
      else
	s->respond("-symcmd failed");
    }
  }

  return 0; 
}

//////////////////////////////////////////////////////////////////////////

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

void start_server(void)
{
  std::cout << "starting server....\n";

  pthread_attr_init(&thAttribute);
  pthread_attr_setdetachstate(&thAttribute, PTHREAD_CREATE_JOINABLE);
  pthread_create(&thSocketServer, &thAttribute, server_thread, (void *)&something);

  std::cout << " started server\n";
}
#else   // if USE_THREADS


#if GLIB_MAJOR_VERSION >= 2

static void debugPrintChannelStatus(GIOStatus stat)
{
  switch (stat) {
  case G_IO_STATUS_ERROR:
    std::cout << "G_IO_STATUS_ERROR\n";
    break;
  case G_IO_STATUS_NORMAL:
    std::cout << "G_IO_STATUS_NORMAL\n";
    break;
  case G_IO_STATUS_EOF:
    std::cout << "G_IO_STATUS_EOF\n";
    break;
  case G_IO_STATUS_AGAIN:
    std::cout << "G_IO_STATUS_AGAIN\n";
    break;
  }
}

static void debugPrintCondition(GIOCondition cond)
{
  if(cond & G_IO_IN)
    std::cout << "  G_IO_IN\n";
  if(cond & G_IO_OUT)
    std::cout << "  G_IO_OUT\n";
  if(cond & G_IO_PRI)
    std::cout << "  G_IO_PRI\n";
  if(cond & G_IO_ERR)
    std::cout << "  G_IO_ERR\n";
  if(cond & G_IO_HUP)
    std::cout << "  G_IO_HUP\n";
  if(cond & G_IO_NVAL)
    std::cout << "  G_IO_NVAL\n";
}
#endif

gboolean server_callback(GIOChannel *channel, GIOCondition condition, void *d )
{
  //std::cout << " Server callback for condition: 0x" << hex  << condition <<endl;
  //debugPrintCondition(condition);

  Socket *s = (Socket *)d;

  if(condition & G_IO_HUP) {
    std::cout<< "client has gone away\n";

#if GLIB_MAJOR_VERSION >= 2
    GError *err=NULL;
    GIOStatus stat = g_io_channel_shutdown(channel, TRUE, &err);
    std::cout << "channel status " << hex << stat << "  " ;
    debugPrintChannelStatus(stat);
#endif
    return FALSE;
  }

  if(condition & G_IO_IN) {
    unsigned int bytes_read=0;

    memset(s->buffer, 0, 256);

#if GLIB_MAJOR_VERSION >= 2

    GError *err=NULL;

#if 0
    g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &err);
    GIOStatus stat;
    bytes_read = 0;
    do {
      unsigned int b;
      stat = g_io_channel_read_chars(channel, &s->buffer[bytes_read], 1, &b, &err);
      bytes_read++;
      printf("read: %c  ", *s->buffer);
      debugPrintChannelStatus(stat);
      debugPrintCondition(g_io_channel_get_buffer_condition (channel));

    } while(G_IO_STATUS_NORMAL == stat);
#else
    //GIOStatus stat = g_io_channel_read_chars(channel, s->buffer, BUFSIZE, &bytes_read, &err);
    g_io_channel_read(channel, s->buffer, BUFSIZE, &bytes_read);
#endif

    if(err) {
      std::cout << "GError:" << err->message << endl;
    }


#else
    g_io_channel_read(channel, s->buffer, BUFSIZE, &bytes_read);
#endif
    std::cout << "Read " << bytes_read << " bytes: " << s->buffer << endl;

    if(bytes_read) {
      if (*s->buffer == '$') {
	s->buffer[bytes_read-1] = 0;
	s->ParseObject(&s->buffer[1]);
      } else {
	parse_string(s->buffer);
	s->respond("ACK");
      }
    } else
      return FALSE;

    return TRUE;
  }

  return FALSE;
}


gboolean server_accept(GIOChannel *channel, GIOCondition condition, void *d )
{
  Socket *s = (Socket *)d;


  s->Accept();

  if(!s->client)
    return FALSE;

  GIOChannel *new_channel = g_io_channel_unix_new(s->client->getSocket());
  GIOCondition new_condition = (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR);

  GError *err = NULL;
  GIOStatus stat;

  stat = g_io_channel_set_encoding (channel, NULL, &err);
  //stat = g_io_channel_set_flags (channel, G_IO_FLAG_SET_MASK, &err);
  stat = g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &err);

  g_io_add_watch(new_channel, 
		 new_condition,
		 server_callback,
		 (void*)s);

  return TRUE;
}


void start_server(void)
{
  TestInt32Array *test = new TestInt32Array("test",16);
  symbol_table.add(test);

  static Socket s;

  std::cout << "starting server....\n";

#ifdef _WIN32
  if (!winsockets_init()) {
    fprintf(stderr, "Could not find a usable WinSock DLL, sockets are disabled.\n");
    server_started = false;
    return;
  }
  server_started = true;
#endif

  s.init();
 
  if(s.my_socket->getSocket() > 0) {

    GIOChannel *channel = g_io_channel_unix_new(s.my_socket->getSocket());
    GIOCondition condition = (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR);

    GError *err = NULL;
    GIOStatus stat;

    stat = g_io_channel_set_encoding (channel, NULL, &err);

    //int flags = g_io_channel_get_flags(channel);
    //flags |= G_IO_FLAG_NONBLOCK;
    stat = g_io_channel_set_flags (channel, G_IO_FLAG_SET_MASK, &err);


    g_io_add_watch(channel, 
		   condition,
		   server_accept,
		   (void*)&s);
  }

  std::cout << " started server\n";
}


void stop_server(void)
{
#ifdef _WIN32
  if (server_started)
    WSACleanup();
#endif
}
#endif   // if USE_THREADS
