/*
   Copyright (C) 2001 Ralf Forsberg

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


#ifndef __PARAFACE_H__
#define __PARAFACE_H__

#include <../src/stimuli.h>
#include <../src/ioports.h>
#include <../src/symbol.h>
#include <../src/trace.h>

#include <gtk/gtk.h>

class Paraface;

// Create a few classes from which an PARAFACE may be constructed

// Paraface_Port is base for InputPort and OutputPort
class Paraface_Port : public IOPORT
{
public:

    Paraface *paraface;
    virtual void trace_register_write(void);
    virtual void setbit(unsigned int bit_number, bool new_value);

    Paraface_Port (unsigned int _num_iopins=8);

};

class InputPort : public Paraface_Port, public BreakCallBack
{
public:
    unsigned int direction;

    virtual void put(unsigned int new_value);
    InputPort (unsigned int _num_iopins=5);
    virtual void callback(void);
    void update_pin_directions(unsigned int );
};

class OutputPort : public Paraface_Port
{
public:

    virtual void put(unsigned int new_value);
    OutputPort (unsigned int _num_iopins=8);

};

// Create a class derived from the IO_input class that
// will allow us to intercept when the I/O input is being
// driven. (This isn't done for PIC I/O pins because the
// logic for handling I/O pin changes resides in the IOPORT
// class.)

class Paraface_Input : public IO_input
{
public:

    virtual void put_node_state( int new_state);

    Paraface_Input (IOPORT *i, unsigned int b, char *opt_name=NULL) : IO_input(i,b,opt_name) { };

};

class Paraface : public ExternalModule
{
public:
    int fd; // file descriptor to parallel port

    unsigned char status, control, data; // last known parallel port state

    InputPort *input_port;
    OutputPort *output_port;

    Paraface(void);
    ~Paraface(void);


    void test(void);
    void update(void);

    // Inheritances from the Package class
    virtual void create_iopin_map(void);

    // Inheritance from Module class
    const virtual char *type(void) { return ("paraface"); };
    static ExternalModule *construct(char *new_name);

#ifdef linux
    int open_parallel_port(char *device="/dev/parport0");
#endif // linux
#ifdef __FreeBSD__
    int open_parallel_port(char *device="/dev/ppi0");
#endif // __FreeBSD__
#if !defined(linux) && !defined(__FreeBSD__)
    int open_parallel_port(char *device="/dev/bogus-device");
#endif
    int read_parallel_status(void);
    int write_parallel_data(int data);


};

#endif //  __PARAFACE_H__
