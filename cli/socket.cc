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

/*

  gpsim sockets.

  The purpose of a socket interface into the simulator is to provide an
  external interface that only loosely depends upon gpsim's internals.

  Phase 1:

  receive commands over a socket and execute them
*/


#define PORT 		0x1234
#define BUFSIZE 	8192


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
  void Send();

  void receive();
  void respond();

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
  buffer[0] = 0;

  socklen_t addrlen = sizeof(addr);
  if ((client_socket = accept(my_socket, (struct sockaddr *)  &addr, &addrlen)) == -1) {
    perror("accept");
    exit(1);
  }

}

void Socket::Recv()
{

  memset(buffer, 0, sizeof(buffer));

  if (recv(client_socket, buffer, sizeof(buffer), 0) == -1) {
    perror("recv");
    exit(1);
  }

}

void Socket::Send()
{
  printf("socket-sending %s",buffer);
  if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
    perror("send");
    exit(1);
  }
}

void Socket::receive(void)
{
  Accept();
  Recv();
  if(strlen(buffer)) {
    printf("received %s\n",buffer);

    parse_string(buffer);
  }
}


void Socket::respond(void)
{
  Send();
}




void *server_thread(void *ignored)
{
  std::cout << "running....\n";

  Socket s;

  s.init();

  while ( true )
    {
      s.receive();
      s.respond();
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

#endif
