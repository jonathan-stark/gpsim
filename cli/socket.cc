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
#include "../src/protocol.h"
#include "../src/gpsim_interface.h"
#include "../src/breakpoints.h"

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
class Socket;

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


//

/*

  gpsim sockets.

  The purpose of a socket interface into the simulator is to provide an
  external interface that only loosely depends upon gpsim's internals.
*/


#define PORT 		0x1234
#define BUFSIZE 	8192
//------------------------------------------------------------------------
bool ParseSocketLink(Packet *buffer, SocketLink **);


//------------------------------------------------------------------------
//------------------------------------------------------------------------
//

class SocketInterface : public Interface
{
private:
  Socket *sock;

public:

  virtual void SimulationHasStopped (gpointer object);
  virtual void Update  (gpointer object);

  virtual ~SocketInterface();

  SocketInterface(Socket *);
};


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

//------------------------------------------------------------------------
// Socket class

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
  void Service();

  void ParseObject();

  void respond(char *);

  bool bHaveClient()
  {
    return (client != 0);
  }



  Packet     *packet;

  SocketBase *my_socket;
  SocketBase *client;
  struct   sockaddr_in addr;
};




class SocketLink
{
public:
  SocketLink(unsigned int _handle);
  //  void send(char *);

  virtual void set(Packet &)=0;
  virtual void get(Packet &)=0;
  unsigned int getHandle() { return handle; }
private:
  unsigned int handle;
};

class AttributeLink : public SocketLink
{
public:
  AttributeLink(unsigned int _handle, Value *);

  void set(Packet &);
  void get(Packet &);

private:
  Value *v;
};

#define nSOCKET_LINKS 16

SocketLink *links[nSOCKET_LINKS];

SocketLink *gCreateSocketLink(unsigned int, Packet &);

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
bool ParseSocketLink(Packet *buffer, SocketLink **sl)
{
  if(!sl)
    return 0;

  unsigned int handle;

  if(buffer->DecodeUInt32(handle)) {

    unsigned int index    = handle & 0xffff;

    *sl = links[index&0x0f];
    if( (*sl) && (*sl)->getHandle() != handle)
      *sl = 0;

    return true;
  }

  return false;

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

  packet = new Packet(BUFSIZE,BUFSIZE);

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

  char *buffer = packet->rxBuff();
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

void Socket::ParseObject()
{

  unsigned int ObjectType;
  if(!packet->DecodeObjectType(ObjectType))
    return;

  switch (ObjectType) {

  case GPSIM_CMD_CREATE_SOCKET_LINK:
    {
      unsigned int handle = FindFreeHandle();
      SocketLink *sl = gCreateSocketLink(handle, *packet);


      if(sl) {
	links[handle&0x0f] = sl;
	packet->EncodeHeader();
	packet->EncodeUInt32(handle);
	packet->txTerminate();
	respond(packet->txBuff());
      }
    }
    break;

  case GPSIM_CMD_REMOVE_SOCKET_LINK:
    {
      SocketLink *sl=0;

      ParseSocketLink(packet, &sl);

      if(sl)
	CloseSocketLink(sl);
      Send("$");
    }
    break;

  case GPSIM_CMD_QUERY_SOCKET_LINK:
    {
      SocketLink *sl=0;

      ParseSocketLink(packet, &sl);

      if(sl) {
	packet->EncodeHeader();
	sl->get(*packet);
	packet->txTerminate();
	respond(packet->txBuff());
      }
    }
    break;

  case GPSIM_CMD_WRITE_TO_SOCKET_LINK:
    {
      SocketLink *sl=0;

      ParseSocketLink(packet, &sl);

      if(sl) {
	sl->set(*packet);
	respond("$");
      }

    }
    break;

  case GPSIM_CMD_QUERY_SYMBOL:
    {
      char tmp[256];

      if(packet->DecodeString(tmp,256)) {

	Value *sym = get_symbol_table().find(tmp);
	if(sym) {
	  packet->EncodeHeader();
	  sym->get(*packet);
	  packet->txTerminate();
	  respond(packet->txBuff());

	} else
	  respond("-");
      }
    }
    break;


  case GPSIM_CMD_WRITE_TO_SYMBOL:
    {

      // The client has requested to write directly to a symbol
      // (without using a SocketLink). So, we'll extract the symbol
      // name from the packet and look it up in the symbol table.
      // If the symbol is found then, it will get assigned the 
      // value that's stored in the packet.

      char tmp[256];

      if(packet->DecodeString(tmp,256)) {

	Value *sym = get_symbol_table().find(tmp);

	if(sym) {

	  // We'll define the response header before we decode
	  // the rest of the packet. The reason for this is that
	  // some symbols may wish to generate a response.

	  packet->EncodeHeader();
	  sym->set(*packet);
	  packet->txTerminate();

	  respond(packet->txBuff());

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
//------------------------------------------------------------------------
//
void Socket::Service()
{
  if(packet->brxHasData()) {

    if (packet->DecodeHeader()) {
      ParseObject();
    } else {
      if(parse_string(packet->rxBuff()) >= 0)
	respond("+ACK");
      else
	respond("+BUSY");
    }
  }

}

//========================================================================
// Socket Interface

SocketInterface::SocketInterface(Socket *new_socket)
  : sock(new_socket)
{
}
void SocketInterface::SimulationHasStopped (gpointer object)
{
}
void SocketInterface::Update  (gpointer object)
{
  if(sock)
    sock->Service();
  printf("socket update\n");
}

SocketInterface::~SocketInterface()
{
  if(sock)
    sock->Service();
}

//========================================================================

SocketLink::SocketLink(unsigned int _handle)
  : handle(_handle)
{
}
/*
void SocketLink::send(char *m)
{
  if(sock)
    sock->respond(m);
}
*/
//========================================================================
AttributeLink::AttributeLink(unsigned int _handle, Value *_v)
  : SocketLink(_handle), v(_v)
{
}

void AttributeLink::set(Packet &p)
{
  if(v)
    v->set(p);
}

void AttributeLink::get(Packet &p)
{
  if(v)
    v->get(p);
}

//========================================================================

SocketLink *gCreateSocketLink(unsigned int handle, Packet &p)
{

  char tmp[256];

  if(p.DecodeString(tmp,256)) {

    Value *sym = get_symbol_table().find(tmp);
    if(sym)
      return new AttributeLink(handle,sym);
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

//gboolean service_socket()
static gboolean server_callback(GIOChannel *channel, GIOCondition condition, void *d )
{
  std::cout << " Server callback for condition: 0x" << hex  << condition <<endl;
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
    gsize bytes_read=0;

    s->packet->prepare();
    memset(s->packet->rxBuff(), 0, 256);

#if GLIB_MAJOR_VERSION >= 2

    GError *err=NULL;
    GIOStatus stat;
    gsize b;

    g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &err);
    stat = g_io_channel_read_chars(channel, 
				   s->packet->rxBuff(), 
				   s->packet->rxSize(), 
				   &b, 
				   &err);
    bytes_read = b;
    s->packet->rxAdvance(bytes_read);
 
    if(err) {
      std::cout << "GError:" << err->message << endl;
    }


#else
    g_io_channel_read(channel, s->packet->rxBuff(), BUFSIZE, &bytes_read);
#endif

    if(bytes_read) {
      if(get_interface().bSimulating()) {
	std::cout << "setting a socket break point because sim is running \n";
	get_bp().set_socket_break();
      } else
	s->Service();
    } else
      return FALSE;

    return TRUE;
  }

  return FALSE;
}


static gboolean server_accept(GIOChannel *channel, GIOCondition condition, void *d )
{
  Socket *s = (Socket *)d;


  s->Accept();

  if(!s->client)
    return FALSE;

  GIOChannel *new_channel = g_io_channel_unix_new(s->client->getSocket());
  GIOCondition new_condition = (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR);

#if GLIB_MAJOR_VERSION >= 2
  GError *err = NULL;
  GIOStatus stat;

  stat = g_io_channel_set_encoding (channel, NULL, &err);
  //stat = g_io_channel_set_flags (channel, G_IO_FLAG_SET_MASK, &err);
  stat = g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &err);
#endif

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

#if GLIB_MAJOR_VERSION >= 2
    GError *err = NULL;
    GIOStatus stat;

    stat = g_io_channel_set_encoding (channel, NULL, &err);

    //int flags = g_io_channel_get_flags(channel);
    //flags |= G_IO_FLAG_NONBLOCK;
    stat = g_io_channel_set_flags (channel, G_IO_FLAG_SET_MASK, &err);
#endif

    g_io_add_watch(channel, 
		   condition,
		   server_accept,
		   (void*)&s);

    
    gi.add_interface(new SocketInterface(&s));

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
