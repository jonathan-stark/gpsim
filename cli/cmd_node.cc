/*
   Copyright (C) 1999 T. Scott Dattalo

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


#include <iostream>
#include <iomanip>
#include <string>

#include "command.h"
#include "cmd_node.h"

cmd_node c_node;

extern void dump_node_list(void);
extern void add_node(char *node_name);

static cmd_options cmd_node_options[] =
{
  NULL,0,0
};


cmd_node::cmd_node(void)
{ 
  name = "node";

  brief_doc = string("Add or display stimulus nodes");

  long_doc = string ("node [new_node1 new_node2 ...]\
\n\t If no new_node is specified then all of the nodes that have been\
\n\tdefined are displayed. If a new_node is specified then it will be\
\n\tadded to the node list. See the \"attach\" and \"stimulus\" commands\
\n\tto see how stimuli are added to the nodes.\
\n\n\texamples:
\n\n\tnode              // display the node list\
\n\tnode n1 n2 n3     // create and add 3 new nodes to the list\
\n");

  op = cmd_node_options; 
}


void cmd_node::list_nodes(void)
{

  dump_node_list();
}



void cmd_node::add_nodes(char_list *nodes)
{


  while(nodes)
    {
      if(verbose)
        cout << "new node : " << nodes->name << '\n';
      add_node(nodes->name);
      nodes = nodes->next;

    }
}

