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
    GPSIM_OBJTYP_CONTAINER = 1,
    GPSIM_OBJTYP_STRING = 2,
    GPSIM_OBJTYP_INT32 = 3,
    GPSIM_OBJTYP_COMMAND = 4

  };

enum eGPSIMCommands
  {
    GPSIM_CMD_BREAK = 1,
    GPSIM_CMD_CLEAR = 2,
    GPSIM_CMD_EXAMINE = 3,
    GPSIM_CMD_STEPOVER = 4,
    GPSIM_CMD_RUN = 5,
    GPSIM_CMD_SET = 6,
    GPSIM_CMD_STEP = 7,
    GPSIM_CMD_VERSION = 8

  }

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

private:
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

    case GPSIM_OBJTYP_COMMAND:
      {
	int command = ascii2uint(&b,2);

	printf("Command %d\n", command);
	buffer_len -= 8;

      }
    default:
      printf("Invalid object type: %d\n",ObjectType);
    }

  }
}


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

/*
int start_server(void)
{

  static Socket s;

  s.init();


}
*/

#endif
