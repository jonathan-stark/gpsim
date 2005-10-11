/*
   Copyright (C) 2000,2001,2002
   Ralf Forsberg

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

#define GTK_ENABLE_BROKEN

#include "../config.h"
#ifdef HAVE_GUI


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <math.h>
#include <assert.h>

#include <iostream>
#include <iomanip>


#include "../src/modules.h"
#include "../src/stimuli.h"
#include "../src/pic-processor.h"
#include "../src/symbol.h"
#include "../src/stimuli.h"
#include "../src/value.h"
#include "../src/errors.h"
#include "../src/packages.h"

#include <vector>

#include "gui.h"
#include "gui_breadboard.h"

#define PINLINEWIDTH 3
#define CASELINEWIDTH 4

#define CASEOFFSET (CASELINEWIDTH/2)

#define FOORADIUS (CASELINEWIDTH) // radius of center top milling

#define LABELPAD 0 // increase this so wide lines doesn't clutter labels

static GdkColor high_output_color;
static GdkColor low_output_color;
static GdkColor black_color;

#define PINLENGTH (4*PINLINEWIDTH)

static int pinspacing = PINLENGTH;

#define LAYOUTSIZE_X 800
#define LAYOUTSIZE_Y 800

#define STRING_SIZE 128

#define ROUTE_RES (2*PINLINEWIDTH) // grid spacing

static void treeselect_module(GtkItem *item, GuiModule *p);

#define XSIZE LAYOUTSIZE_X/ROUTE_RES // grid size
#define YSIZE LAYOUTSIZE_Y/ROUTE_RES


/* If HMASK is set in board_matrix, then this position
 is unavailable for horizontal track */
#define HMASK 1 
#define VMASK 2
/*
 board matrix contains information about how a track can be routed.
 */
static unsigned char board_matrix[XSIZE][YSIZE];


//========================================================================

class BreadBoardXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value)
  {
    Breadboard_Window *bbw  = (Breadboard_Window *) (parent_window);

    bbw->Update();

  }
};
//========================================================================

/* Check the flags in board_matrix to see if we are allowed to 
   route horizontally here */
static inline int allow_horiz(point &p)
{
  if(board_matrix[p.x][p.y] & HMASK)
    return FALSE;
  return TRUE;
}

/* Check the flags in board_matrix to see if we are allowed to 
   route vertically here */
static inline int allow_vert(point &p)
{
  if(board_matrix[p.x][p.y] & VMASK)
    return FALSE;

  return TRUE;
}

// Find the direction to go to get from s to e if there are no obstacles.
static inline route_direction calculate_route_direction(point s, point e)
{
    if(abs(s.x-e.x) > abs(s.y-e.y))
    {
	// Left or right
	if(s.x<e.x)
	    return R_RIGHT;
	return R_LEFT;
    }
    if(s.y<e.y)
	return R_UP;
    return R_DOWN;
}

// Return right/left/up/down if the endpoint is straight ahead in that dir.
static inline route_direction calculate_route_direction_exact(point s, point e)
{
    if(s.x!=e.x && s.y!=e.y)
        return R_NONE;
    if(s.y==e.y)
    {
	// Left or right
	if(s.x<e.x)
	    return R_RIGHT;
	return R_LEFT;
    }
    if(s.y<e.y)
	return R_UP;
    return R_DOWN;
}

// Put point p as first point in pat
static void inline prepend_point_to_path(path **pat, point p)
{
    path *new_point;
    route_direction dir=R_NONE;

/* This commented-out code were supposed to just move the closest
   point instead of adding points all the time. FIXME, bug. */
/*    int add_point=0;

    if(*pat!=0)
    {
	dir = calculate_route_direction_exact(p, (*pat)->p);
	if(dir==R_NONE)
	{
            // Both X and Y has changed.
	    add_point=1;
	}
	else if(*pat!=0 && (*pat)->next!=0)
	{
	    if((*pat)->p.x == p.x &&
	       (*pat)->next->p.x == p.x &&
	       (*pat)->dir==dir)
	    {
		// same x, just change y
		(*pat)->p.y=p.y;
	    }
	    else if((*pat)->p.y == p.y &&
		    (*pat)->next->p.y == p.y &&
		    (*pat)->dir==dir)
	    {
		// same y, just change x
		(*pat)->p.x=p.x;
	    }
	    else
	    {
                add_point=1;
	    }
	}
	else
	{
	    add_point=1;
	}
    }
    else
    {
        add_point=1;
    }

    if(add_point)*/
//    {
	// Lots and lots of mallocs, FIXME
	new_point = (path*)malloc(sizeof(path));
	new_point->p=p;
	new_point->next = *pat;
	if((*pat)!=0)
	{
	    dir = calculate_route_direction(p, (*pat)->p);
	    if((*pat)->dir==R_NONE)
		(*pat)->dir=dir;
	}
	new_point->dir=dir;
	*pat = new_point;
//    }
}

// Free all memory in pat
static void clear_path(path **pat)
{

    path *current_path, *next;

    if(*pat==0)
	return;

    current_path = *pat;

    *pat = 0;

    while(current_path!=0)
    {
	next = current_path->next;

	free(current_path);

	current_path = next;
    }
}

#if 0
/* failed attempt to remove unnessessary points in paths. FIXME bug*/
// Compress sequences with same x or y to one sequence
static void compress_path(path **pat)
{
    int x,y;

    path *current_path;

    current_path = *pat;

    x=current_path->p.x;
    y=current_path->p.y;

    while(current_path!=0)
    {
        path *next_path = current_path->next;
	path *next2_path = next_path->next;

	if(next_path==0 || next2_path==0)
            break;

	if(current_path->p.x==next_path->p.x &&
	   current_path->p.x==next2_path->p.x &&
	   current_path->dir==next_path->dir &&
	   current_path->dir==next2_path->dir)
	{
	    current_path->next=next2_path;
	    free(next_path);
            continue;
	}
	if(current_path->p.y==next_path->p.y &&
	   current_path->p.y==next2_path->p.y &&
	   current_path->dir==next_path->dir &&
	   current_path->dir==next2_path->dir)
	{
	    current_path->next=next2_path;
	    free(next_path);
            continue;
	}

	current_path = current_path->next;
    }
}
#endif

// mask_matrix is used by trace_two_poins to know where is has been, and
// how quickly it came here. (depth is stored here if lower)
static unsigned short mask_matrix[XSIZE][YSIZE];

// maxdepth is shortest path from start to end
static unsigned short maxdepth;

// Penalty for making a turn in a trace
#define turnq(a,b) (((a)!=(b))*10)

static unsigned long calls;

/*
This is an recursive routine that tries to find a path between p and end.
*/
static int trace_two_points(path **pat,   // Pointer to resulting path
			    point p,  // Where we are now
			    point end,    // Where we want to go
			    int depth,
			    route_direction lastdir)    // How deep in we are
{
    int retval;
    route_direction dir;
    point up, left, right, down;

    if(depth==0)
    {
	// Initialize mask_matrix and maxdepth on first call

	int x,y;
	// Initialize mask_matrix and maxdepth
	//maxdepth=500;
	for(x=0;x<XSIZE;x++)
	    for(y=0;y<YSIZE;y++)
		mask_matrix[x][y]=maxdepth;

	clear_path(pat);
        calls=0;
    }

    calls++;

    ////////////////////////////////////////
    // Recusion termination
    ////////////////////////////////////////
    if(depth>maxdepth)
	return FALSE;
    if(depth>mask_matrix[p.x][p.y])
	return FALSE;
    if(abs(p.x-end.x)+abs(p.y-end.y)+depth>maxdepth)
	return FALSE;
    if(p.x == end.x && p.y==end.y)
    {
	// We are at end point

	if(depth < maxdepth)
	{
	    // We found a new shortest path.

	    //printf("Found path with length %d\n",depth);

	    maxdepth = depth;
	    clear_path(pat);
	    prepend_point_to_path(pat, p);
	    return TRUE;
	}
	return FALSE;
    }

    // Store new (closer) depth in mask_matrix.
    mask_matrix[p.x][p.y]=depth;

    // Find the general direction we want to go
    dir = calculate_route_direction(p,end);

    // Recursion return value
    retval=0;

    // Convenience
    up=p;up.y++;
    down=p;down.y--;
    left=p;left.x--;
    right=p;right.x++;

    /* Depending on where we wish to go, do recursion so that we likely
     will quickly find at least some kind of path to end.

     If we don't do that maxdepth will stay large, and the traceing will
     likely take a long time.

     We use allow_vert and allow_horiz to determine if the movement is
     allowed, of if there is something in our way.

     */
    switch(dir)
    {
    case R_UP:
	if(allow_vert(up))
	    retval|=trace_two_points(pat,up,end,depth+1+turnq(lastdir,R_UP),R_UP);
	if(p.x<end.x)
	{
	    if(allow_horiz(right))
		retval|=trace_two_points(pat,right,end,depth+1+turnq(lastdir,R_RIGHT),R_RIGHT);
	    if(allow_horiz(left))
		retval|=trace_two_points(pat,left,end,depth+1+turnq(lastdir,R_LEFT),R_LEFT);
	}
	else
	{
	    if(allow_horiz(left))
		retval|=trace_two_points(pat,left,end,depth+1+turnq(lastdir,R_LEFT),R_LEFT);
	    if(allow_horiz(right))
		retval|=trace_two_points(pat,right,end,depth+1+turnq(lastdir,R_RIGHT),R_RIGHT);
	}
	if(allow_vert(down))
	    retval|=trace_two_points(pat,down,end,depth+1+turnq(lastdir,R_DOWN),R_DOWN);
	break;
    case R_DOWN:
	if(allow_vert(down))
	    retval|=trace_two_points(pat,down,end,depth+1+turnq(lastdir,R_DOWN),R_DOWN);
	if(p.x<end.x)
	{
	    if(allow_horiz(right))
		retval|=trace_two_points(pat,right,end,depth+1+turnq(lastdir,R_RIGHT),R_RIGHT);
	    if(allow_horiz(left))
		retval|=trace_two_points(pat,left,end,depth+1+turnq(lastdir,R_LEFT),R_LEFT);
            }
	else
	{
	    if(allow_horiz(left))
		retval|=trace_two_points(pat,left,end,depth+1+turnq(lastdir,R_LEFT),R_LEFT);
	    if(allow_horiz(right))
		retval|=trace_two_points(pat,right,end,depth+1+turnq(lastdir,R_RIGHT),R_RIGHT);
	}
	if(allow_vert(up))
	    retval|=trace_two_points(pat,up,end,depth+1+turnq(lastdir,R_UP),R_UP);
	break;
    case R_LEFT:
	if(allow_horiz(left))
	    retval|=trace_two_points(pat,left,end,depth+1+turnq(lastdir,R_LEFT),R_LEFT);
	if(p.y<end.y)
	{
	    if(allow_vert(up))
		retval|=trace_two_points(pat,up,end,depth+1+turnq(lastdir,R_UP),R_UP);
	    if(allow_vert(down))
		retval|=trace_two_points(pat,down,end,depth+1+turnq(lastdir,R_DOWN),R_DOWN);
	}
	else
	{
	    if(allow_vert(down))
		retval|=trace_two_points(pat,down,end,depth+1+turnq(lastdir,R_DOWN),R_DOWN);
	    if(allow_vert(up))
		retval|=trace_two_points(pat,up,end,depth+1+turnq(lastdir,R_UP),R_UP);
	}
	if(allow_horiz(right))
	    retval|=trace_two_points(pat,right,end,depth+1+turnq(lastdir,R_RIGHT),R_RIGHT);
	break;
    case R_RIGHT:
	if(allow_horiz(right))
	{
	    retval|=trace_two_points(pat,right,end,depth+1+turnq(lastdir,R_RIGHT),R_RIGHT);
	}
	if(p.y<end.y)
	{
	    if(allow_vert(up))
		retval|=trace_two_points(pat,up,end,depth+1+turnq(lastdir,R_UP),R_UP);
	    if(allow_vert(down))
		retval|=trace_two_points(pat,down,end,depth+1+turnq(lastdir,R_DOWN),R_DOWN);
	}
	else
	{
	    if(allow_vert(down))
		retval|=trace_two_points(pat,down,end,depth+1+turnq(lastdir,R_DOWN),R_DOWN);
	    if(allow_vert(up))
		retval|=trace_two_points(pat,up,end,depth+1+turnq(lastdir,R_UP),R_UP);
	}
	if(allow_horiz(left))
	{
	    retval|=trace_two_points(pat,left,end,depth+1+turnq(lastdir,R_LEFT),R_LEFT);
	}
	break;

    case R_NONE:
      break;
    }


    // Check if some of the recursive traces went well.
    if(retval==(int)TRUE)
    {
	// We found a path to end. Add point p to path.
/* bug ahead, FIXME:	if(*pat!=0 && (*pat)->next!=0)
	{
          // If there are point in pat already, then check if
            // we can use that and just change the coords there.
	    if((*pat)->p.x == p.x &&
	       (*pat)->next->p.x == p.x &&
	      (*pat)->dir==dir)
	    {
		// same x, just change y
		(*pat)->p.y=p.y;
	    }
	    else if((*pat)->p.y == p.y &&
	       (*pat)->next->p.y == p.y)
	    {
		// same y, just change x
		(*pat)->p.x=p.x;
	    }
	    else
	    {
                // This is a turn in the trace. We need another point.
		prepend_point_to_path(pat, p, dir);
	    }
	}
	else*/
	{
          // This is first or second point.
	    prepend_point_to_path(pat, p);
	}
//	if(depth==0)
//	{
//            printf("Successful trace with %ld steps\n",calls);
//	}
	return TRUE;
    }

//    if(depth==0)
//    {
//	printf("Unsuccessful trace with %ld steps\n",calls);
//    }
    return FALSE; 
}
#if 0 // enable for debug
// print huge ascii picture of the board_matrix for debugging
void print_matrix(void)
{
    int x,y;

    for(y=YSIZE-1;y>=0;y--)
    {
        for(x=0;x<XSIZE;x++)
        {
	    if(board_matrix[x][y]==0)
		putchar('.');
	    else if(board_matrix[x][y]==(HMASK|VMASK))
		putchar('X');
	    else if(board_matrix[x][y]==HMASK)
		putchar('-');
	    else if(board_matrix[x][y]==VMASK)
		putchar('|');
	    else// if(isalnum(board_matrix[x][y]))
		putchar(board_matrix[x][y]);
	    //			else
	    //				assert(0);
	}
	putchar('\r');
	putchar('\n');
    }
} 

// Debug. Draw routing constraints. FIXME draw from board_matrix instead.
static void draw_board_matrix(Breadboard_Window *bbw)
{
    int x,y, width, height;
    GList *mi;
    int i;

    // Loop all modules
    mi = bbw->modules;
    while(mi!=0)
    {
	GuiModule *p = static_cast<GuiModule*>(mi->data);
	if(p->bIsBuilt) {
        x=p->x;
	y=p->y/*-PINLENGTH*/;
	width=p->width/*+PINLENGTH*/;
	height=p->height/*+PINLENGTH*/;

        // Debug. This shows the boxes that limits traceing.
	gdk_draw_rectangle(bbw->layout_pixmap,
			   bbw->case_gc,0,
			   x,y,width,height);
	//printf("%dx%d @ %d,%d with %d pins\n",width,height,x,y,p->module->get_pin_count());

	// Draw barriers around pins so the tracker can only get in
        // straigt to the pin and not from the side.
	for(i=1;i<=p->module->get_pin_count();i++)
	{

	    GList *e;

	    e = g_list_nth(p->pins, i-1);

            GuiPin *gp = static_cast<GuiPin*>(e->data);

	    switch(gp->orientation)
	    {
	    case LEFT:
		y=p->y+gp->y;
		gdk_draw_line(bbw->layout_pixmap,
			      bbw->case_gc,
			      p->x+gp->x-PINLENGTH,y,
			      p->x+gp->x+gp->width,y);
		y=p->y+gp->y+gp->height;
		gdk_draw_line(bbw->layout_pixmap,
			      bbw->case_gc,
			      p->x+gp->x-PINLENGTH,y,
			      p->x+gp->x+gp->width,y);
		break;
	    case RIGHT:
		y=p->y+gp->y;
		gdk_draw_line(bbw->layout_pixmap,
			      bbw->case_gc,
			      p->x+gp->x,y,
			      p->x+gp->x+gp->width+PINLENGTH,y);
		y=p->y+gp->y+gp->height;
		gdk_draw_line(bbw->layout_pixmap,
			      bbw->case_gc,
			      p->x+gp->x,y,
			      p->x+gp->x+gp->width+PINLENGTH,y);
		break;
	    default:
                assert(0);
	    }
	}
	}
        mi=mi->next;
    }
}
#endif

static GList *nodepath_list;

// Draw nodes in nodepath_list to layout_pixmap
static void clear_nodes(Breadboard_Window *bbw)
{

    GList *iter;
    path *nodepath;

    iter = nodepath_list;
    while(iter!=0)
    {
	nodepath = (path*)iter->data;

	clear_path(&nodepath);

        nodepath_list = g_list_remove(nodepath_list, iter->data);

        iter=nodepath_list;
    }

}

static void layout_adj_changed(GtkWidget *widget, Breadboard_Window *bbw);

// Draw node in nodepath_list to layout_pixmap
static void draw_nodes(Breadboard_Window *bbw)
{

    GList *iter;

    gdk_draw_rectangle (bbw->layout_pixmap,
			bbw->window->style->bg_gc[GTK_WIDGET_STATE (bbw->window)],
			TRUE,
			0, 0,
			LAYOUTSIZE_X,
			LAYOUTSIZE_Y);

    iter = nodepath_list;

    while(iter!=0) {

      int last_x, last_y;
      path *current_path;


      path *nodepath;
      nodepath = (path*)iter->data;

      current_path = nodepath;

      last_x = current_path->p.x*ROUTE_RES;
      last_y = current_path->p.y*ROUTE_RES;

      current_path=current_path->next;

      gdk_gc_set_foreground(bbw->pinline_gc,&black_color);

      while(current_path!=0)
	{
	  int x,y;

	  x=current_path->p.x*ROUTE_RES;
	  y=current_path->p.y*ROUTE_RES;

	  gdk_draw_line(bbw->layout_pixmap,
			bbw->pinline_gc,
			last_x,last_y,
			x,y);

	  last_x=x;
	  last_y=y;

	  current_path = current_path->next;
	}
      iter=iter->next;
    }


    layout_adj_changed(0,bbw);
}



// Here we fill board_matrix with module packages, so that trace_two_points
// know not to trace over them.
static void update_board_matrix(Breadboard_Window *bbw)
{
    int x,y, width, height;
    GList *mi;
    int i;

    // Clear first.
    for(y=YSIZE-1;y>=0;y--)
    {
	for(x=0;x<XSIZE;x++)
	    board_matrix[x][y]=0;
    }

    // Mark board outline, so we limit traces here
    for(x=0;x<XSIZE;x++)
    {
	board_matrix[x][0]=(HMASK|VMASK);
	board_matrix[x][YSIZE-1]=(HMASK|VMASK);
    }
    for(y=0;y<YSIZE;y++)
    {
	board_matrix[0][y]=(HMASK|VMASK);
	board_matrix[XSIZE-1][y]=(HMASK|VMASK);
    }


    // Loop all modules, and put its package and pins to board_matrix
    mi = bbw->modules;
    while(mi!=0) {

      GuiModule *p = static_cast<GuiModule*>(mi->data);

      if(p && p->bIsBuilt) {
        x=p->x;
	y=p->y;
	width=p->width;
	height=p->height;
 
	for(; y<p->y+height && y/ROUTE_RES < YSIZE; y+=ROUTE_RES)
	  for(x=p->x; x<p->x+width && x/ROUTE_RES<XSIZE;x+=ROUTE_RES)
	    board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);

	// Draw barriers around pins so the tracker can only get in
        // straigt to the pin and not from the side.
	for(i=1;i<=p->pin_count;i++) {

	  GList *e;

	  e = g_list_nth(p->pins, i-1);

	  GuiPin *gp = static_cast<GuiPin*>(e->data);

	  switch(gp->orientation)
	    {
	    case LEFT:
	      y=p->y+gp->y;
	      for(x=p->x+gp->x-PINLENGTH;
		  x<p->x+gp->x+gp->width;
		  x+=ROUTE_RES)
		board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
	      y=p->y+gp->y+gp->height;
	      for(x=p->x+gp->x-PINLENGTH;
		  x<p->x+gp->x+gp->width;
		  x+=ROUTE_RES)
		board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
	      break;
	    case RIGHT:
	      y=p->y+gp->y;
	      for(x=p->x+gp->x;
		  x<p->x+gp->x+gp->width+PINLENGTH;
		  x+=ROUTE_RES)
		board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
	      y=p->y+gp->y+gp->height;
	      for(x=p->x+gp->x;
		  x<p->x+gp->x+gp->width+PINLENGTH;
		  x+=ROUTE_RES)
		board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
	      break;
	    default:
	      assert(0);
	    }
	}
      }
      mi=mi->next;
    }

    clear_nodes(bbw);
    draw_nodes(bbw);
}

// Add path to board_matrix. This will make trace_two_point to not trace
// at its place. It can trace over it when in straight angle.
static void add_path_to_matrix(path *pat)
{
    int x=-1, y=-1;
    if(pat!=0)
    {
	x=pat->p.x;
	y=pat->p.y;
	pat=pat->next;
    }
    while(pat!=0)
    {
	if(pat->dir==R_LEFT || pat->dir==R_RIGHT)
	    board_matrix[x][y]|=HMASK;
	if(pat->dir==R_DOWN || pat->dir==R_UP)
	    board_matrix[x][y]|=VMASK;
	while(x!=pat->p.x || y!=pat->p.y)
	{
	    if(x<pat->p.x)
		x++;
	    if(x>pat->p.x)
		x--;
	    if(y<pat->p.y)
		y++;
	    if(y>pat->p.y)
                y--;
	    if(pat->dir==R_LEFT || pat->dir==R_RIGHT)
		board_matrix[x][y]|=HMASK;
	    if(pat->dir==R_DOWN || pat->dir==R_UP)
		board_matrix[x][y]|=VMASK;
	}

	pat = pat->next;
    }
}

static GuiPin *find_gui_pin(Breadboard_Window *bbw, stimulus *pin);

#define MAX_PATHS 32

static path *shortest_path[100][100];//[MAX_PATHS]=0;
static int pathlen[100][100];

static int *permutations;
static int *shortest_permutation;

#include <algorithm>

static void reverse_path(path **pat)
{
    path *next, *last=0;

    while(*pat != 0)
    {
	// Keep a pointer to next
	next = (*pat)->next;

	// New next poins to last (reversing the list)
	(*pat)->next = last;

	last = *pat;
	*pat = next;
    }
    *pat = last;
}

static void reverse_path_if_endpoint(point startpoint, path **pat)
{
    point pat_start, pat_end;
    int dist_start, dist_end;
    path *iter;

    iter = *pat;

    pat_start = iter->p;

    while(iter->next!=0)
        iter=iter->next;

    pat_end = iter->p;

    dist_start = abs(pat_start.x-startpoint.x) + abs(pat_start.y-startpoint.y);
    dist_end = abs(pat_end.x-startpoint.x) + abs(pat_end.y-startpoint.y);

    if(dist_start > dist_end && dist_end<5)
    {
	// Reverse the list *pat

        reverse_path(pat);
    }
}

static void reverse_path_if_startpoint(point startpoint, path **pat)
{
    point pat_start, pat_end;
    int dist_start, dist_end;
    path *iter;

    iter = *pat;

    pat_start = iter->p;

    while(iter->next!=0)
        iter=iter->next;

    pat_end = iter->p;

    dist_start = abs(pat_start.x-startpoint.x) + abs(pat_start.y-startpoint.y);
    dist_end = abs(pat_end.x-startpoint.x) + abs(pat_end.y-startpoint.y);

    if(dist_start < dist_end && dist_start<5)
    {
	// Reverse the list *pat

        reverse_path(pat);
    }
}

static void path_copy_and_cat(path **pat, path **source)
{
    path *dest, *prev=0;

    dest = *pat;

    if(dest!=0)
    {
	reverse_path_if_startpoint((*source)->p, pat);

	while(dest->next!=0)
	{
	    dest=dest->next;
	}

	reverse_path_if_endpoint(dest->p, source);

	if((abs((*source)->p.x-dest->p.x) + abs((*source)->p.y-dest->p.y)) > 5)
	{
	    puts("Assert failure");
	    printf("%d, %d\n",
		   abs((*source)->p.x-dest->p.x),
		   abs((*source)->p.y-dest->p.y));
	}

	prev = dest;
	dest=dest->next;
    }

    path *sourceiter = *source;

    while(sourceiter!=0)
    {

	dest = (path*) malloc(sizeof(path));
	memcpy(dest, sourceiter, sizeof(path));
	dest->next=0;

	if(*pat==0)
	    *pat=dest;

	if(prev!=0)
	{
	    prev->next = dest;
	}

	prev = dest;
	sourceiter=sourceiter->next;
    }
}



/*
 Trace a node, and add result to nodepath_list
 */
static void trace_node(struct gui_node *gn)
{
    GuiPin *p;
    Breadboard_Window *bbw;
    stimulus *stimulus;
    GList *pinlist=0;
    int nr_of_nodes=0;
    int i,j;
    int didnt_work=0;

    point start={-1,-1},end;

    bbw=gn->bbw;

    stimulus = gn->node->stimuli;

    // Make a glist of all gui_pins in the node
    while(stimulus!=0)
    {
	p = find_gui_pin(bbw, stimulus);

	if(p==0)
	{
	    puts("Not found");
	    g_list_free(pinlist);
            return;
	}

	pinlist = g_list_append(pinlist, p);
        nr_of_nodes++;

	stimulus=stimulus->next;
    }

    // Allocate an array of shortest_paths, indexed with 2x glist position.
//FIXME    shortest_path = (path***) malloc(nr_of_nodes*nr_of_nodes*sizeof(path*));

    printf("Tracing node %s:",gn->node->name().c_str());
    fflush(stdout);

    permutations = (int*)malloc(sizeof(int)*nr_of_nodes);
    shortest_permutation = (int*)malloc(sizeof(int)*nr_of_nodes);
    for(i=0;i<nr_of_nodes;i++)
        permutations[i]=i;

    // Trace between all stimulus, and store the distances in the array.
    for(i=0;i<nr_of_nodes;i++)
    {
	GuiPin *pi, *pj;
        GList *li, *lj;

	li = g_list_nth(pinlist,i);
        assert(li!=0);
        pi = static_cast<GuiPin*>(li->data);

	fflush(stdout);
	for(j=i+1;j<nr_of_nodes;j++)
	{
	    lj = g_list_nth(pinlist,j);
	    assert(lj!=0);
	    pj = static_cast<GuiPin*>(lj->data);

	    start.x=pi->layout_xpos/ROUTE_RES;
	    start.y=pi->layout_ypos/ROUTE_RES;

	    end.x=pj->layout_xpos/ROUTE_RES;
	    end.y=pj->layout_ypos/ROUTE_RES;

//	    printf("Tracing from %d,%d to %d,%d\n",start.x,start.y,end.x,end.y);
	    maxdepth=abs(start.x-end.x)+abs(start.y-end.y);
	    maxdepth=maxdepth*2+100; // Twice the distance, and 5 turns
//	    printf("Trying maxdepth %d\n",maxdepth);
	    trace_two_points(&shortest_path[i][j], start, end,0,R_UP);
	    if(shortest_path[i][j]==0)
	    {
		printf("\n### Couldn't trace from pin %s to pin %s!\n",
                       pi->getIOpin()->name().c_str(),
                       pj->getIOpin()->name().c_str());
		didnt_work=1;
	    }
	    pathlen[i][j]=maxdepth;

	    pathlen[j][i]=maxdepth;
            shortest_path[j][i]=shortest_path[i][j];
	}
    }

    if(didnt_work)
    {
	printf("\n###### Couldn't trace node %s!\n",gn->node->name().c_str());
	for(i=0;i<nr_of_nodes;i++)
	    for(j=i+1;j<nr_of_nodes;j++)
		clear_path(&shortest_path[i][j]);
	free(permutations);
	free(shortest_permutation);
	g_list_free(pinlist);
	return;
    }

    // Find the combination that produces the shortest node.
    int minlen = 100000;
    do
    {
	int sum=0;

//	printf("%d ",permutations[0]);
	for(i=0;i<nr_of_nodes-1;i++)
	{
//	    printf("%d ",permutations[i+1]);
	    sum+=pathlen[permutations[i]][permutations[i+1]];
	}
//	printf("length %d\n",sum);

	if(sum < minlen)
	{
	    minlen=sum;
	    for(i=0;i<nr_of_nodes;i++)
	    {
                shortest_permutation[i]=permutations[i];
	    }
	}
        // Fixme, I'd rather use next_combination().
    } while ( next_permutation( permutations, permutations+nr_of_nodes ) );

    printf(" : Length %d\n", minlen);
//    for(i=0;i<nr_of_nodes;i++)
//    {
//	printf("%d ",shortest_permutation[i]);
//    }
    //puts("");

    path *nodepath=0;
    for(i=0;i<nr_of_nodes-1;i++)
    {
	path_copy_and_cat(&nodepath,&shortest_path[shortest_permutation[i]][shortest_permutation[i+1]]);
    }

    for(i=0;i<nr_of_nodes;i++)
	for(j=i+1;j<nr_of_nodes;j++)
	    clear_path(&shortest_path[i][j]);
    free(permutations);
    free(shortest_permutation);

	if(nodepath!=0)
	{
//	    compress_path(&nodepath);

	    add_path_to_matrix(nodepath);

	    nodepath_list = g_list_append(nodepath_list, nodepath);
	}
}



GuiPin *find_gui_pin(Breadboard_Window *bbw, stimulus *pin)
{

  GList *iter = bbw->modules;

  while(iter) {
    
    GuiModule *m = static_cast<GuiModule *>(iter->data);

    int i;
    for(i=1;i<=m->module->get_pin_count();i++) {

      stimulus *p;

      p=m->module->get_pin(i);

      if(p == pin)
	{
	  GList *e;

	  e = g_list_nth(m->pins, i-1);

	  return static_cast<GuiPin*>(e->data);
	}
    }

    iter = iter->next;
  }

  return 0;
}







static gboolean expose_pin(GtkWidget *widget,
		       GdkEventExpose *event,
		       GuiPin *p)
{

    if(p->pixmap==0)
    {
	puts("bbw.c: no pixmap1!");
	return 0;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
    return 0;
}

static void treeselect_stimulus(GtkItem *item, GuiPin *pin)
{

    char text[STRING_SIZE];
    char string[STRING_SIZE];

    char *pText = "Not connected";
    char *pString = "Stimulus";
    if(!pin)
      return;

    gtk_widget_show(pin->bbw->stimulus_frame);
    gtk_widget_hide(pin->bbw->node_frame);
    gtk_widget_hide(pin->bbw->module_frame);
    gtk_widget_hide(pin->bbw->pic_frame);

    if(pin->getIOpin()) {
      snprintf(string,sizeof(string),"Stimulus %s",pin->getIOpin()->name().c_str());
      pString = string;

      if(pin->getSnode()!=0)
	snprintf(text,sizeof(text),"Connected to node %s", pin->getSnode()->name().c_str());
      else
	snprintf(text,sizeof(text),"Not connected");
      pText = text;
    }

    gtk_frame_set_label(GTK_FRAME(pin->bbw->stimulus_frame),pString);
    gtk_label_set_text(GTK_LABEL(pin->bbw->stimulus_settings_label), pText);

    pin->bbw->selected_pin = pin;
}

static void treeselect_node(GtkItem *item, struct gui_node *gui_node)
{

    char name[STRING_SIZE];
    char *text[1];
    stimulus *stimulus;
    char str[STRING_SIZE];

    text[0]=name;

//    printf("treeselect_node %p\n",gui_node);

    if(gui_node->node!=0)
    {
	snprintf(str,sizeof(str),"Node %s",gui_node->node->name().c_str());
	gtk_frame_set_label(GTK_FRAME(gui_node->bbw->node_frame),str);

	gtk_widget_show(gui_node->bbw->node_frame);
    }
    else
    {
	gtk_widget_hide(gui_node->bbw->node_frame);
    }
    gtk_widget_hide(gui_node->bbw->stimulus_frame);
    gtk_widget_hide(gui_node->bbw->module_frame);
    gtk_widget_hide(gui_node->bbw->pic_frame);

    // Clear node_clist
    gtk_clist_clear(GTK_CLIST(gui_node->bbw->node_clist));

    if(gui_node->node!=0)
    {
	// Add to node_clist
	stimulus = gui_node->node->stimuli;

	while(stimulus!=0)
	{
	    int row;

	    strncpy(name, stimulus->name().c_str(), sizeof(name));

	    row = gtk_clist_append(GTK_CLIST(gui_node->bbw->node_clist),
				   text);

	    gtk_clist_set_row_data (GTK_CLIST(gui_node->bbw->node_clist),
				    row,
				    stimulus);

	    stimulus = stimulus->next;
	}
    }

    gui_node->bbw->selected_node = gui_node;
}

static void settings_clist_cb(GtkCList       *clist,
		gint            row,
		gint            column,
		GdkEvent       *event,
		Breadboard_Window *bbw)
{

	// Save the Attribute*
	Value *attr;
	char str[256];
	char val[256];
	attr = (Value*) gtk_clist_get_row_data(GTK_CLIST(bbw->attribute_clist),
					       row);

	//attr->getAsStr(attrstr,50);
	attr->get(val, sizeof(val));

	sprintf(str,"%s = %s",attr->name().c_str(),val);
	
	gtk_entry_set_text(GTK_ENTRY(bbw->attribute_entry), str);
}

static void settings_set_cb(GtkWidget *button, 
		Breadboard_Window *bbw)
{

	const char *entry_string;
	char attribute_name[256];
	char attribute_newval[256];
	// We get here from both the button and entry->enter
	
	// Check the entry.
	entry_string=gtk_entry_get_text(GTK_ENTRY(bbw->attribute_entry));
	sscanf(entry_string,"%s = %s",attribute_name, attribute_newval);

	printf("change attribute \"%s\" to \"%s\"\n",attribute_name, attribute_newval);
	
	Value *attr;
	
	// Change the Attribute
	//attr = bbw->selected_module->module->get_attribute(attribute_name);
	attr = get_symbol_table().find(attribute_name);
	if(attr)
	{
	  try {

	    // Set attribute
	    attr->set(attribute_newval);

	    // Update clist
	    treeselect_module(0, bbw->selected_module);
	  }
	  catch (Error *err) {

	    if(err)
	      cout << __FUNCTION__ <<": " << err->toString() << endl;
	    delete err;
	  }

	}
	else
	{
	  printf("Could not find attribute \"%s\"\n",attribute_name);
	}
}

static void UpdateModuleFrame(GuiModule *p, Breadboard_Window *bbw)
{
  char buffer[STRING_SIZE];

  snprintf(buffer,sizeof(buffer),"%s settings",p->module->name().c_str());
  gtk_frame_set_label(GTK_FRAME(p->bbw->module_frame),buffer);

  if( !GTK_WIDGET_VISIBLE(GTK_CLIST(p->bbw->attribute_clist)))
    return;

  // clear clist
  gtk_clist_clear(GTK_CLIST(p->bbw->attribute_clist));

  // read attributes and add to clist
  char attribute_string[STRING_SIZE];
  char *text[1]={attribute_string};

  list <Value *>::iterator attribute_iterator;
  int row;

  for (attribute_iterator = p->module->attributes.begin();
       attribute_iterator != p->module->attributes.end();
       attribute_iterator++) {

    try {

      char attribute_value[STRING_SIZE];
      Value *locattr = *attribute_iterator;
      locattr->get(attribute_value, sizeof(attribute_value));

      sprintf(attribute_string,"%s = %s",locattr->name().c_str(),attribute_value);

      row = gtk_clist_append(GTK_CLIST(p->bbw->attribute_clist),
			   text);
      // add the Attribute* as data for the clist rows.
      gtk_clist_set_row_data(GTK_CLIST(p->bbw->attribute_clist),
			     row,
			     (gpointer)locattr);
    }

    catch (Error *err) {

      if(err)
	cout << "UpdateModuleFrame:" << err->toString() << endl;
      delete err;
    }

				    
  }

  gtk_entry_set_text(GTK_ENTRY(p->bbw->attribute_entry), "");

}

static void treeselect_module(GtkItem *item, GuiModule *p)
{
  if (p) {

    gtk_widget_hide(p->bbw->stimulus_frame);
    gtk_widget_hide(p->bbw->node_frame);
    gtk_widget_hide(p->bbw->pic_frame);

    gtk_widget_show(p->bbw->module_frame);

    UpdateModuleFrame(p, p->bbw);
  

    p->bbw->selected_module = p;
  }
}

void GuiModule::SetPosition(int nx, int ny)
{
  GList *piniter;

  nx=nx-nx%pinspacing;
  ny=ny-ny%pinspacing;

  if(nx != x || ny != y)
    {
      x=nx;
      y=ny;

      // Position module_widget
      gtk_layout_move(GTK_LAYOUT(bbw->layout), module_widget, x, y);

      // Position module_name
      gtk_layout_move(GTK_LAYOUT(bbw->layout), name_widget, x, y-10);

      // Position pins
      piniter = pins;
      while(piniter!=0)
	{
	  GuiPin *pin = static_cast<GuiPin *>(piniter->data);

	  pin->layout_xpos = x + pin->x;
	  pin->layout_ypos = y + pin->y + pin->height/2; // FIXME /2?

	  //	    if(pin->orientation==LEFT)
	  //                pin->layout_xpos-=pinspacing/2;
	  if(pin->orientation==RIGHT)
	    pin->layout_xpos+=PINLENGTH /*+ pinspacing/2*/;

	  gtk_layout_move(GTK_LAYOUT(bbw->layout),
			  pin->widget,x+pin->x,y+pin->y);

	  piniter = piniter->next;
	}
    }
}

/* FIXME: calculate distance to the edges instead of the corners. */
double GuiModule::Distance(int px, int py)
{
  double distance;
  double min_distance=100000000;

  // Upper left
  distance=sqrt((double)abs(x-px)*abs(x-px) +
		abs(y-py)*abs(y-py));
  if(distance<min_distance)
    min_distance=distance;

  // Upper right
  distance=sqrt((double)abs(x+width-px)*abs(x +width - px) +
		abs(y-py)*abs(y-py));
  if(distance<min_distance)
    min_distance=distance;

  // Lower left
  distance=sqrt((double)abs(x-px)*abs(x-px) +
		abs(y + height-py)*abs(y+height-py));
  if(distance<min_distance)
    min_distance=distance;

  // Lower right
  distance=sqrt((double)abs(x+width-px)*abs(x+width-px) +
		abs(y+height-py)*abs(y+height-py));
  if(distance<min_distance)
    min_distance=distance;

  return min_distance;
}

static GuiModule *find_closest_module(Breadboard_Window *bbw, int x, int y)
{
  GuiModule *closest=0;
  double distance, min_distance=1000000;

  GList *mi = bbw->modules;

  while(mi) {

    GuiModule *p = static_cast<GuiModule *>(mi->data);

    distance = p->Distance(x,y);
    if(distance<min_distance) {
      closest = p;
      min_distance = distance;
    }

    mi=mi->next;
  }

  return closest;
}

// FIXME
static GuiModule *dragged_module=0;
static int dragging=0;
static int grab_next_module=0;

void grab_module(GuiModule *p)
{
    dragged_module = p;
    gdk_pointer_grab(p->bbw->layout->window,
		     TRUE,
		     (GdkEventMask)(GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK),
		     p->bbw->layout->window,
		     0,
                     GDK_CURRENT_TIME);

    treeselect_module(0,dragged_module);
    dragging = 1;
    clear_nodes(p->bbw);
    draw_nodes(p->bbw);
    gtk_widget_set_app_paintable(p->bbw->layout, FALSE);
}

static void pointer_cb(GtkWidget *w,
		       GdkEventButton *event,
		       Breadboard_Window *bbw)
{
    static int x,y;

    x = (int) (event->x 
#if GTK_MAJOR_VERSION < 2
    	+ bbw->hadj->value
#endif
	);
    y = (int) (event->y 
#if GTK_MAJOR_VERSION < 2
    	+ bbw->vadj->value
#endif
	);

    switch(event->type)
    {
    case GDK_MOTION_NOTIFY:
	if(dragging && 0 != dragged_module)
	{
            dragged_module->SetPosition(x+pinspacing, y+pinspacing);
	    Value *xpos = dragged_module->module->get_attribute("xpos", false);
	    Value *ypos = dragged_module->module->get_attribute("ypos", false);
	    if(xpos)
	      xpos->set(dragged_module->x);
	    if(ypos)
	      ypos->set(dragged_module->y);
	}
	break;
    case GDK_BUTTON_PRESS:
	if(grab_next_module)
	{
	    if(dragging)
	    {
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		dragging = 0;
		gtk_widget_set_app_paintable(bbw->layout, TRUE);
		grab_next_module=0;
		update_board_matrix(bbw);
	    }
	}
	else
	{
	    dragged_module = find_closest_module(bbw, x, y);
            if (0 != dragged_module)
            {
	      gdk_pointer_grab(w->window,
			      TRUE,
			      (GdkEventMask)(GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK),
			      w->window,
			      0,
			      GDK_CURRENT_TIME);
	      treeselect_module(0,dragged_module);
	      dragging = 1;
	      clear_nodes(bbw);
              draw_nodes(bbw);
	      gtk_widget_set_app_paintable(bbw->layout, FALSE);
            }
	}
	break;
    case GDK_2BUTTON_PRESS:
	break;
    case GDK_BUTTON_RELEASE:
	if(dragging)
	{
	    gdk_pointer_ungrab(GDK_CURRENT_TIME);
	    update_board_matrix(bbw);
	    dragging = 0;
	    gtk_widget_set_app_paintable(bbw->layout, TRUE);
	    update_board_matrix(bbw);
	    UpdateModuleFrame(dragged_module, bbw);
	}
	break;
    default:
	printf("Whoops? event type %d\n",event->type);
	break;
    }
}

// When clicked on a pin
static gint button(GtkWidget *widget,
		   GdkEventButton *event,
		   GuiPin *p)
{
    if(event->type==GDK_BUTTON_PRESS &&
       event->button==1)
    {
      if(p->getSnode()) {
	struct gui_node *gn;

	gn = (struct gui_node *)
	  gtk_object_get_data(GTK_OBJECT(p->bbw->node_tree),
			      p->getSnode()->name().c_str());

	if(gn!=0) {
	  treeselect_node(0, gn);
	  return 1;
	}
      }

      treeselect_stimulus(0, p);
      //puts("Stimulus should now be selected");

      return 1;
    }

    if(event->type==GDK_2BUTTON_PRESS &&
       event->button==1)
    {
      p->toggleState();
      return 1;
    }

    if(event->type==GDK_BUTTON_PRESS &&
       event->button==2)
    {
      if(p->getSnode()) {

	struct gui_node *gn;

	gn = (struct gui_node *)
	  gtk_object_get_data(GTK_OBJECT(p->bbw->node_tree),
			      p->getSnode()->name().c_str());

	trace_node(gn);
	draw_nodes(gn->bbw);
      }
      return 1;
    }

    return 0;
}



// get_string
static void a_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=TRUE;
}
static void b_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=FALSE;
}
// used for reading a value from user when break on value is requested
const char *gui_get_string(char *prompt, char *initial_text)
{
    static GtkWidget *dialog=0;
    static GtkWidget *label;
    static GtkWidget *entry;
    static int retval;
    GtkWidget *button;
    GtkWidget *hbox;
    

    const char *string;
    
    retval=-1;

    if(dialog==0)
    {
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),"enter value");
	gtk_signal_connect_object(GTK_OBJECT(dialog),
				  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));

	label=gtk_label_new("Enter string:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,FALSE,FALSE,20);
	
	hbox = gtk_hbox_new(0,0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);

	button = gtk_button_new_with_label("OK");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(a_cb),(gpointer)&retval);
	
	button = gtk_button_new_with_label("Cancel");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(b_cb),(gpointer)&retval);

	label=gtk_label_new(prompt);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label,
			   FALSE,FALSE, 20);

	entry=gtk_entry_new();
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry,FALSE,FALSE,20);
	GTK_WIDGET_SET_FLAGS (entry, GTK_CAN_FOCUS);
	gtk_signal_connect(GTK_OBJECT(entry),
			   "activate",
			   (GtkSignalFunc)a_cb,
			   (gpointer)&retval);

    }
    else
    {
	gtk_label_set_text(GTK_LABEL(label),prompt);
    }

    gtk_entry_set_text(GTK_ENTRY(entry), initial_text);

    gtk_widget_show(dialog);

    gtk_widget_grab_focus (entry);

    gtk_grab_add(dialog);
    while(retval==-1 && GTK_WIDGET_VISIBLE(dialog))
	gtk_main_iteration();
    gtk_grab_remove(dialog);
    
    gtk_widget_hide(dialog);

    if(retval==(int)TRUE)
	string=gtk_entry_get_text(GTK_ENTRY(entry));
    else
        string=0;
    
    return string;
}

static void add_new_snode(GtkWidget *button, Breadboard_Window *bbw)
{
    const char *node_name = gui_get_string("Node name","");

    if(node_name !=0)
	new Stimulus_Node(node_name);
}



////////////////////////////////////////////////////////////////////
static gint ok_cb(GtkWidget *widget,
		   GdkEventButton *event,
		    gpointer user_data)
{
    if(event->type==GDK_2BUTTON_PRESS &&
       event->button==1)
    {
	*(int*)user_data=FALSE; // cancel=FALSE;
	return 1;
    }
    return 0;
}
static void cancel_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=TRUE; // cancel=TRUE;
}

// Select row
static void node_cb(GtkCList       *clist,
		    gint            row,
		    gint            column,
		    GdkEvent       *event,
		    gpointer user_data)

{
    Stimulus_Node *snode;

    snode = (Stimulus_Node*)gtk_clist_get_row_data (clist, row);

    *((Stimulus_Node**) user_data)=snode;
}
static void module_cb(GtkCList       *clist,
		      gint            row,
		      gint            column,
		      GdkEvent       *event,
		      gpointer user_data)
{
    char *module_type;

    module_type = (char*) gtk_clist_get_row_data (clist, row);

    strncpy((char*) user_data, module_type, STRING_SIZE);
}
static void copy_node_tree_to_clist(GtkWidget *item, gpointer clist)
{
    Stimulus_Node *node;
    char name[STRING_SIZE];
    char *text[1]={name};
    int row;

    node = (Stimulus_Node*)gtk_object_get_data(GTK_OBJECT(item), "snode");


    strcpy(name,node->name().c_str());

    row = gtk_clist_append(GTK_CLIST(clist),
			   text);

    gtk_clist_set_row_data (GTK_CLIST(clist),
			    row,
			    (gpointer)node);
}

static Stimulus_Node *select_node_dialog(Breadboard_Window *bbw)
{
    static GtkWidget *dialog;
    static GtkWidget *node_clist;
    GtkWidget *cancelbutton;
    static int cancel;

    cancel=-1;

    Stimulus_Node *snode=0;

    GtkWidget *vbox;
    GtkWidget *scrolledwindow;

    if(dialog==0)
    {

        // Build window
	dialog = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW (dialog), "Select node to connect to");

	vbox = GTK_DIALOG(dialog)->vbox;

	scrolledwindow = gtk_scrolled_window_new (0, 0);
	gtk_widget_show (scrolledwindow);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	node_clist = gtk_clist_new (1);
	gtk_widget_show (node_clist);
	gtk_container_add (GTK_CONTAINER(scrolledwindow), node_clist);

	cancelbutton = gtk_button_new_with_label ("Cancel");
	gtk_widget_show (cancelbutton);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->action_area), cancelbutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(cancelbutton),"clicked",
			   GTK_SIGNAL_FUNC(cancel_cb),(gpointer)&cancel);

	gtk_signal_connect(GTK_OBJECT(node_clist),
			   "select_row",
			   (GtkSignalFunc) node_cb,
			   (gpointer)&snode);
	gtk_signal_connect(GTK_OBJECT(node_clist),
			   "button_press_event",
			   GTK_SIGNAL_FUNC(ok_cb),
			   (gpointer)&cancel);

	gtk_window_set_default_size(GTK_WINDOW(dialog), 220, 400);
    }
    gtk_clist_clear(GTK_CLIST(node_clist));

    // Add all nodes
    gtk_container_foreach(GTK_CONTAINER(bbw->node_tree),
			  copy_node_tree_to_clist,
			  (gpointer)node_clist);

    gtk_widget_show(dialog);

    gtk_grab_add(dialog);
    while(cancel==-1 && GTK_WIDGET_VISIBLE(dialog))
	gtk_main_iteration();
    gtk_grab_remove(dialog);


    if(cancel==(int)TRUE)
    {
	gtk_widget_hide(dialog);
	return 0;
    }

    gtk_widget_hide(dialog);

    return snode;
}

static char *select_module_dialog(Breadboard_Window *bbw)
{
    static GtkWidget *dialog;
    static GtkWidget *module_clist;
    GtkWidget *cancelbutton;
    static int cancel;
    list <Module_Library *> :: iterator mi;
    static char module_type[STRING_SIZE];

    GtkWidget *vbox;
    GtkWidget *scrolledwindow;

    char *module_clist_titles[]={"Name","Library"};

    cancel=-1;
	
    if(dialog==0)
    {

        // Build window
	dialog = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW (dialog), "Select module to load");

	vbox = GTK_DIALOG(dialog)->vbox;

	scrolledwindow = gtk_scrolled_window_new (0, 0);
	gtk_widget_show (scrolledwindow);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	module_clist = gtk_clist_new_with_titles (2, module_clist_titles);
	gtk_clist_set_column_auto_resize(GTK_CLIST(module_clist),0,TRUE);
	gtk_widget_show (module_clist);
	gtk_container_add (GTK_CONTAINER(scrolledwindow), module_clist);

	cancelbutton = gtk_button_new_with_label ("Cancel");
	gtk_widget_show (cancelbutton);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->action_area), cancelbutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(cancelbutton),"clicked",
			   GTK_SIGNAL_FUNC(cancel_cb),(gpointer)&cancel);

	gtk_signal_connect(GTK_OBJECT(module_clist),
			   "select_row",
			   (GtkSignalFunc) module_cb,
			   (gpointer)&module_type);
	gtk_signal_connect(GTK_OBJECT(module_clist),
			   "button_press_event",
			   GTK_SIGNAL_FUNC(ok_cb),
			   (gpointer)&cancel);

	gtk_window_set_default_size(GTK_WINDOW(dialog), 220, 400);
    }

    gtk_clist_clear(GTK_CLIST(module_clist));

        // Add all modules
	for (mi = module_list.begin();
	     mi != module_list.end();
	     mi++)
	{

	    Module_Library *t = *mi;
	    cout << t->name() << '\n';

	    if(t->module_list) {
		// Loop through the list and display all of the modules.
		int i=0;

		while(t->module_list[i].names[0])
		{   
		    char name[STRING_SIZE];
		    char library[STRING_SIZE];
		    char *text[2]={name, library};
		    int row;

		    strncpy(name,t->module_list[i].names[0], STRING_SIZE);
		    strncpy(library,t->name(), STRING_SIZE);

		    row = gtk_clist_append(GTK_CLIST(module_clist),
					   text);

		    gtk_clist_set_row_data (GTK_CLIST(module_clist),
					    row,
					    (gpointer)t->module_list[i].names[0]);

                    i++;
		}
	    }
	}


    gtk_widget_show(dialog);

    gtk_grab_add(dialog);
    while(cancel==-1 && GTK_WIDGET_VISIBLE(dialog))
	gtk_main_iteration();
    gtk_grab_remove(dialog);


    if(cancel==(int)TRUE)
    {
	gtk_widget_hide(dialog);
	return 0;
    }

    gtk_widget_hide(dialog);

    return module_type;
}

#if 0
// Display a file in a text widget.
static void text_dialog(const char *filename)
{
    static GtkWidget *dialog;
    GtkWidget *cancelbutton;

	GtkWidget *vbox;
	GtkWidget *scrolledwindow;
	GtkWidget *text;

        char string[STRING_SIZE];

	FILE *fi=fopen(filename,"r");
	if(fi==0)
            return;

	if(dialog!=0)
	    gtk_widget_destroy(dialog);


        // Build window
	dialog = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW (dialog), filename);

	vbox = GTK_DIALOG(dialog)->vbox;

	scrolledwindow = gtk_scrolled_window_new (0, 0);
	gtk_widget_show (scrolledwindow);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	text = gtk_text_new(0,0);
	gtk_container_add (GTK_CONTAINER(scrolledwindow), text);
        gtk_widget_show(text);

	cancelbutton = gtk_button_new_with_label ("Ok");
	gtk_widget_show (cancelbutton);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->action_area), cancelbutton, FALSE, FALSE, 0);
	gtk_signal_connect_object(GTK_OBJECT(cancelbutton),"clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));

	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 400);


	while(fgets(string, sizeof(string), fi)!=0)
	{
	    gtk_text_insert(GTK_TEXT(text),
			    0,
			    0,
			    0,
			    string,
			    strlen(string));
	}

	fclose(fi);

    gtk_widget_show(dialog);

    return;
}
#endif

static void stimulus_add_node(GtkWidget *button, Breadboard_Window *bbw)
{

    Stimulus_Node *node;


    node = select_node_dialog(bbw);

    if(node!=0 && bbw->selected_pin!=0)
    {
	node->attach_stimulus(bbw->selected_pin->getIOpin());

        // Update stimulus frame
	treeselect_stimulus(0, bbw->selected_pin);
    }
}

static void add_library(GtkWidget *button, Breadboard_Window *bbw)
{

    const char *library_name;

    library_name = gui_get_string("Module library name (e.g. libgpsim_modules)","");

    if(library_name)
      module_load_library(library_name);
}

static void add_module(GtkWidget *button, Breadboard_Window *bbw)
{

    char *module_type;
    const char *module_name;

    module_type = select_module_dialog(bbw);

    if(module_type!=0)
    {
	module_name = gui_get_string("Module name", module_type);
        grab_next_module = 1;
        if(module_name != 0)
	    module_load_module(module_type, module_name);
    }
}

static void remove_module(GtkWidget *button, Breadboard_Window *bbw)
{
    GList *pin_iter;

    delete(bbw->selected_module->module);

    // FIXME the rest should be as callback from src


    // Remove pins
    pin_iter=bbw->selected_module->pins;
    while(pin_iter!=0)
    {
	GuiPin *pin = static_cast<GuiPin*>(pin_iter->data);

	gtk_widget_destroy(GTK_WIDGET(pin->widget));

	pin_iter = pin_iter->next;
    }

    // Remove widget
    gtk_container_remove(GTK_CONTAINER(bbw->layout),
			 bbw->selected_module->module_widget);
    gtk_container_remove(GTK_CONTAINER(bbw->layout),
			 bbw->selected_module->name_widget);

    // Remove from local list of modules
    bbw->modules=g_list_remove(bbw->modules, bbw->selected_module);

    // Remove module from tree
    gtk_container_remove(GTK_CONTAINER(bbw->tree),
			 bbw->selected_module->tree_item);

    gtk_widget_hide(bbw->module_frame);
    gtk_widget_hide(bbw->pic_frame);

    free(bbw->selected_module);

    bbw->selected_module=0;
}

static void node_clist_cb(GtkCList       *clist,
			  gint            row,
			  gint            column,
			  GdkEvent       *event,
			  Breadboard_Window *bbw)

{
    bbw->selected_node->selected_row = row;
}

static void remove_node(GtkWidget *button, Breadboard_Window *bbw)
{
    gtk_object_remove_data(GTK_OBJECT(bbw->node_tree),
			   bbw->selected_node->node->name().c_str());

    gtk_object_remove_data(GTK_OBJECT(bbw->selected_node->tree_item), "snode");

    gtk_container_remove(GTK_CONTAINER(bbw->node_tree),
			 bbw->selected_node->tree_item);

    delete bbw->selected_node->node;

    free(bbw->selected_node);

    bbw->selected_node=0;

    gtk_widget_hide(bbw->node_frame);
    gtk_widget_hide(bbw->stimulus_frame);
    gtk_widget_hide(bbw->module_frame);
    gtk_widget_hide(bbw->pic_frame);
}

static void remove_node_stimulus(GtkWidget *button, Breadboard_Window *bbw)
{
    stimulus *s;

    s = (stimulus*) gtk_clist_get_row_data(GTK_CLIST(bbw->node_clist), bbw->selected_node->selected_row);

    bbw->selected_node->node->detach_stimulus(s);

    gtk_clist_remove(GTK_CLIST(bbw->node_clist), bbw->selected_node->selected_row);

    bbw->selected_node->selected_row=-1;
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
static const char *file_selection_name;
static int fs_done;

static void
file_selection_ok (GtkWidget        *w,
		   GtkFileSelection *fs)
{
  file_selection_name=gtk_file_selection_get_filename (fs);

  fs_done=1;
}

static void
file_selection_cancel (GtkWidget        *w,
		       GtkFileSelection *fs)
{
    file_selection_name=0;
    fs_done=1;
}

static const char *gui_get_filename(char *filename)
{
    static GtkWidget *window = 0;

    char *prompt="Log settings";

    if (!window)
    {

	window = gtk_file_selection_new (prompt);

	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (window));

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

	gtk_signal_connect_object(GTK_OBJECT(window),
				  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(window));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC(file_selection_ok),
			    window);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
			    "clicked", GTK_SIGNAL_FUNC(file_selection_cancel),
			    window);
    }

    gtk_file_selection_set_filename(GTK_FILE_SELECTION (window),
				    filename);

    file_selection_name=0;
    gtk_widget_show_now(window);

    fs_done=0;
    file_selection_name=0;
    gtk_grab_add(window);
    while(!fs_done && GTK_WIDGET_VISIBLE(window))
	gtk_main_iteration();
    gtk_grab_remove(window);
    
    gtk_widget_hide(window);

    if(file_selection_name==0)
    {
	return 0;
    }

    return file_selection_name;
}
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
static void save_stc(GtkWidget *button, Breadboard_Window *bbw)
{
    FILE *fo;
    list <Module_Library *> :: iterator mi;
    GList *module_iterator;
    Module *m;
    const char *filename;

    filename = gui_get_filename("netlist.stc");
    if(filename == 0)
        filename="/tmp/foo.stc";
    fo = fopen(filename, "w");

    fprintf(fo, "\n# This file was written by gpsim.\n");
    fprintf(fo, "\n# You can use this file for example like this:");
    fprintf(fo, "\n#     gpsim -s mycode.cod -c netlist.stc\n");
    fprintf(fo, "\n# If you want to add commands, you can create another .stc file");
    fprintf(fo, "\n# and load this file from it. Something like this:");
    fprintf(fo, "\n# ----------- myproject.stc ---------------");
    fprintf(fo, "\n# load s mycode.cod");
    fprintf(fo, "\n# frequency 12000000");
    fprintf(fo, "\n# load c netlist.stc");
    fprintf(fo, "\n# -----------------------------------------");
    fprintf(fo, "\n# You can then just load this new file:");
    fprintf(fo, "\n#     gpsim -c myproject.stc");
    fprintf(fo, "\n# and use netlist.stc whenever you save from the breadboard.");
    fprintf(fo, "\n#");
    fprintf(fo, "\n");

    fprintf(fo, "\n\n# Processor position:\n");

    /*m=bbw->gp->cpu;
    Attribute *xpos = m->get_attribute("xpos", false);
    Attribute *ypos = m->get_attribute("ypos", false);
    if(xpos && ypos && xpos->nGet()>=0 && ypos->nGet()>=0)
      fprintf(fo, "module position %s %d %d\n",
		m->name(),
		xpos->nGet(),
		ypos->nGet());*/
    
    // Save module libraries
    fprintf(fo, "\n\n# Module libraries:\n");
    for (mi = module_list.begin();
         mi != module_list.end();
         mi++)
    {

      Module_Library *t = *mi;
      fprintf(fo, "module library %s\n",
      t->name());
    }

    // Save modules
    fprintf(fo, "\n\n# Modules:\n");
    module_iterator = bbw->modules;
    while(module_iterator!=0)
    {
      GuiModule *p;

      p = static_cast<GuiModule*>( module_iterator->data);

      list <Value *> :: iterator attribute_iterator;
      m = p->module;

      Processor *cpu;
      cpu=dynamic_cast<Processor*>(m);
      if(cpu==0)
      { // Module, not a processor, so add the load command
        fprintf(fo, "module load %s %s\n",
        m->type(),
        m->name().c_str());
      }

      for(attribute_iterator = m->attributes.begin();
          attribute_iterator != m->attributes.end();
          attribute_iterator++) {

        Value *locattr = *attribute_iterator;

        fprintf(fo, "%s=%s\n",
                locattr->name().c_str(),
                locattr->toString().c_str());
      }
      fprintf(fo, "\n");
      module_iterator=module_iterator->next;
    }

    // Save nodes and connections
    fprintf(fo, "\n\n# Connections:\n");
    list <Stimulus_Node *> :: iterator node_iterator;

    Symbol_Table &ST = get_symbol_table();
    Symbol_Table::node_symbol_iterator it;
    Symbol_Table::node_symbol_iterator itEnd = ST.endNodeSymbol();
    for(it = ST.beginNodeSymbol(); it != itEnd; it++) {
      Stimulus_Node *node = (*it)->getNode();
      assert(node != NULL);
      stimulus *stimulus;

      fprintf(fo, "node %s\n",node->name().c_str());

      if(node->stimuli!=0)
      {
        fprintf(fo, "attach %s",node->name().c_str());

        stimulus = node->stimuli;

        while(stimulus!=0)
        {
          fprintf(fo, " %s",stimulus->name().c_str());

          stimulus = stimulus->next;
        }

        fprintf(fo, "\n\n");
      }
    }

    fprintf(fo, "\n\n# End.\n");
    fclose(fo);
    //text_dialog(filename);

}

static void clear_traces(GtkWidget *button, Breadboard_Window *bbw)
{

    update_board_matrix(bbw);
}

static void trace_all_foreach_function(GtkWidget *item, gpointer bbw_gpointer)
{
    Stimulus_Node *node;
    Breadboard_Window *bbw = (Breadboard_Window*)bbw_gpointer;

    node = (Stimulus_Node*)gtk_object_get_data(GTK_OBJECT(item), "snode");

    struct gui_node * gn = (struct gui_node*) gtk_object_get_data(GTK_OBJECT(((Breadboard_Window*)bbw)->node_tree), node->name().c_str());

    trace_node(gn);
}

static void trace_all(GtkWidget *button, Breadboard_Window *bbw)
{

    update_board_matrix(bbw);

    gtk_container_foreach(GTK_CONTAINER(bbw->node_tree),
			  trace_all_foreach_function,
			  (gpointer)bbw);

    draw_nodes(bbw);

    puts("Trace all is done.");
}

//========================================================================
GuiPin::GuiPin(Breadboard_Window *_bbw,
	       int _x, int _y, 
	       eOrientation _orientation,
	       IOPIN *_iopin)
{

    iopin = _iopin;
    x=_x;
    y=_y;
    width=pinspacing;
    height=pinspacing;
    bbw=_bbw;
    gc=bbw->pinline_gc;

    y-=height/2;

    layout_xpos = 0;
    layout_ypos = 0;


    if(iopin) {
      value=iopin->getState();
      direction=iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;
      orientation=_orientation;
      type=PIN_DIGITAL;
    }
    else
    {
	value=false;
	direction=PIN_INPUT;
        orientation=_orientation;
	type=PIN_OTHER;
    }

    if(orientation==LEFT)
        x-=width;

    // Create widget
    widget = gtk_drawing_area_new();
    gtk_widget_set_events(widget,
			  gtk_widget_get_events(widget)|
			  GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(widget),
		       "button_press_event",
		       (GtkSignalFunc) button,
		       this);

    gtk_drawing_area_size(GTK_DRAWING_AREA(widget),width,height);
    gtk_signal_connect(GTK_OBJECT(widget),
		       "expose_event",
		       (GtkSignalFunc) expose_pin,
		       this);


    // Create pixmap
    pixmap = gdk_pixmap_new(bbw->window->window,
			    width,
			    height,
			    -1);

    // Draw pin
    draw();

    gtk_widget_show(widget);

}

//------------------------------------------------------------------------
// GuiPin::update() - check the state of the iopin and make the gui match
// 
void GuiPin::update()
{

  if(iopin) {
	
    bool value=iopin->getState();
    eDirection dir=iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;

    if(value!=getState() || dir!=direction) {
	  
      putState(value);
      direction=dir;

      draw();
    }
  }


}
//------------------------------------------------------------------------
void GuiPin::toggleState()
{
  if(iopin) {
    char cPinState = iopin->getForcedDrivenState();

    switch (cPinState) {
    case '0':
    case 'Z':
    case 'X':
      iopin->forceDrivenState('1');
      break;
    case '1':
      iopin->forceDrivenState('0');
      break;
    case 'W':
      iopin->forceDrivenState('w');
      break;
    case 'w':
      iopin->forceDrivenState('W');
      break;
    }
    bbw->Update();
  }
}
//------------------------------------------------------------------------
// GuiPin::draw() - draw a single pin 
//
// 
void GuiPin::draw()
{

  int pointx;
  int wingheight, wingx;
  int casex, endx;
  int y;

  switch(orientation)
    {
    case LEFT:
      casex = width;
      endx = 0;
      break;
    default:
      casex = 0;
      endx = width;
      break;
    }

  y = height/2;

  // Clear pixmap
  gdk_draw_rectangle (pixmap,
		      bbw->window->style->bg_gc[GTK_WIDGET_STATE (widget)],
		      TRUE,
		      0, 0,
		      width,
		      height);


  if(type==PIN_OTHER)
    gdk_gc_set_foreground(gc,&black_color);
  else
    gdk_gc_set_foreground(gc,getState() ? &high_output_color:&low_output_color);

  // Draw actual pin
  gdk_draw_line(pixmap,gc,
		casex,y,endx,y);

  if(type==PIN_OTHER)
    return;

  // Draw direction arrow
  wingheight=height/3;
    
  if(casex>endx)
    {
      if(direction==PIN_OUTPUT)
	{
	  pointx = endx + PINLENGTH/3;
	  wingx=endx+(PINLENGTH*2)/3;
	}
      else
	{
	  pointx = endx + (PINLENGTH*2)/3;
	  wingx=endx+PINLENGTH/3;
	}
    }
  else
    {
      if(direction==PIN_OUTPUT)
	{
	  pointx = casex + (PINLENGTH*2)/3;
	  wingx=casex+PINLENGTH/3;
	}
      else
	{
	  pointx = casex + PINLENGTH/3;
	  wingx=casex+(PINLENGTH*2)/3;
	}
    }

    
  // Draw an arrow poining at (endx,endy)
  gdk_draw_line(pixmap,gc,
		pointx,y,wingx,y+wingheight);
  gdk_draw_line(pixmap,gc,
		pointx,y,wingx,y-wingheight);

  if(widget->window!=0)
    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    pixmap,
		    0, 0,
		    0, 0,
		    width, height);
}

//------------------------------------------------------------------------
void GuiPin::addXref(CrossReferenceToGUI *newXref)
{
  xref = newXref;
}
//------------------------------------------------------------------------
void GuiPin::destroy()
{
  if(xref)
    iopin->remove_xref(xref);

  gdk_pixmap_unref(pixmap);
  gtk_widget_destroy(widget);
}

//------------------------------------------------------------------------
static gboolean name_expose(GtkWidget *widget, GdkEventExpose *event, GuiModule *p)
{
    if(p->name_pixmap==0)
    {
	puts("bbw.c: no pixmap2!");
	return 0;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->name_pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
    return 0;
}

static gboolean module_expose(GtkWidget *widget, GdkEventExpose *event, GuiModule *p)
{

    if(p->module_pixmap==0)
    {
	puts("bbw.c: no pixmap3!");
	return 0;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->module_pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
    return 0;
}

#define PACKAGESPACING 15

void GuiModule::Refresh()
{

  GList *pin_iter;

  gtk_widget_ref(module_widget);
  gtk_container_remove(GTK_CONTAINER(bbw->layout),module_widget);

  // Delete the static module pixmap if there is no widget
  // in the module.
  if(module->get_widget()==0)
    {
      gdk_pixmap_unref(module_pixmap);
      gtk_widget_destroy(module_widget);
    }

  // Delete the pins
  pin_iter=pins;
  while(pin_iter!=NULL)
    {
      GuiPin *pin = static_cast<GuiPin*>(pin_iter->data);
      pin->destroy();
      pin_iter=pin_iter->next;
    }
    
  // Destroy name widget
  gdk_pixmap_unref(name_pixmap);
  gtk_widget_destroy(name_widget);
    
  // Remove from gtk-tree 
  gtk_tree_item_remove_subtree(GTK_TREE_ITEM(tree_item));
  gtk_widget_destroy(tree_item);
    
  // Remove module from list
  bbw->modules=g_list_remove(bbw->modules, this);

  // rebuild module
  // FIXME maybe just Build()?
  Build();
  //new GuiModule(module, bbw);

  gtk_widget_unref(module_widget);
}

//========================================================================
//========================================================================
class PositionAttribute : public Float
{
protected:
  Breadboard_Window *bbw;
public:
  PositionAttribute(Breadboard_Window *_bbw,const char *n, double v);
  void set(Value *v);
};

PositionAttribute::PositionAttribute(Breadboard_Window *_bbw, const char *n, double v)
  : Float(v), bbw(_bbw)
{
  new_name((char*)n);
}

void PositionAttribute::set(Value *v)
{
  Float::set(v);
  if(bbw)
    bbw->Update();
}

/*
Value *newFloatAttribute(const char *n, double v)
{
  Float *f = new Float(v);
  f->new_name((char *)n);  //fixme

  return f;
}
*/

void GuiModule::Build()
{
  if(bIsBuilt || !bbw)
    return;

  if(!bbw->enabled)
    return;

  float package_height;

  int i;
  BreadBoardXREF *cross_reference;
  GtkWidget *da;
  /*int*/ width=50, height=18;

  Package *package = module->package;
  if(!package)
    return;     // embedded module 


  //p->module_widget = widget;
  module_widget = (GtkWidget *)module->get_widget();

  pins=0;
  pin_count=module->get_pin_count();

  Value *xpos = module->get_attribute("xpos", false);
  Value *ypos = module->get_attribute("ypos", false);
  xpos->get(x);
  ypos->get(y);

  tree_item = gtk_tree_item_new_with_label (module->name().c_str());
  gtk_signal_connect(GTK_OBJECT(tree_item),
		     "select",
		     (GtkSignalFunc) treeselect_module,
		     this);
  gtk_widget_show(tree_item);
  gtk_tree_append(GTK_TREE(bbw->tree), tree_item);

  package_height=(float)((pin_count/2+(pin_count&1)-1)*pinspacing);

  float pin_position;

  if(!module_widget) {

    // Create a static representation.
    pinnamewidth=0;
    for(i=1;i<=pin_count;i++) {

      int w;

      char *name=(char *)module->get_pin_name(i).c_str();
      if(!name)
	continue;
#if GTK_MAJOR_VERSION >= 2
      w = gdk_string_width (gdk_font_from_description(bbw->pinnamefont),name)+LABELPAD;
#else
      w = gdk_string_width (bbw->pinnamefont,name)+LABELPAD;
#endif
      if(w > pinnamewidth)
	pinnamewidth=w;
    }

    pinnamewidth+=FOORADIUS; // The 'U' at the top of the DIL module

    width =  pinnamewidth*2; // pin name widthts
    width += 2*CASELINEWIDTH+2*LABELPAD;

    height = module->get_pin_count()/2*pinspacing; // pin name height

    if(module->get_pin_count()%2)
      height += pinspacing;

    height+=2*CASELINEWIDTH+2*LABELPAD;

    da = gtk_drawing_area_new();

    gtk_drawing_area_size(GTK_DRAWING_AREA(da),width,height);

    module_pixmap = gdk_pixmap_new(bbw->window->window,
				   width,
				   height,
				   -1);

    gdk_draw_rectangle (module_pixmap,
			bbw->window->style->bg_gc[GTK_WIDGET_STATE (da)],
			TRUE,
			0, 0,
			width,
			height);

    gdk_draw_rectangle (module_pixmap,
			bbw->window->style->white_gc,
			TRUE,
			CASEOFFSET, CASEOFFSET,
			width-CASEOFFSET,
			height-CASEOFFSET);

    // Draw pin names
    for(i=1;i<=pin_count;i++)
      {
	char *name;
	int label_x, label_y;

	pin_position=package->get_pin_position(i);

	if(pin_position>=0.0 && pin_position<1.0)
	  {
	    label_x=LABELPAD+CASELINEWIDTH;
	    label_y=(int)(pin_position*package_height);
	    label_y+=LABELPAD+CASELINEWIDTH+pinspacing/2-bbw->pinnameheight/3;
	  }
	else
	  {
	    label_x=LABELPAD+width/2+FOORADIUS;
	    label_y=(int)((3.0-pin_position)*package_height);
	    label_y+=LABELPAD+CASELINEWIDTH+pinspacing/2-bbw->pinnameheight/3;
	  }

	name=(char*)module->get_pin_name(i).c_str();
	if(name==0)
	  continue;
	gdk_draw_text(module_pixmap,
#if GTK_MAJOR_VERSION >= 2
		      gdk_font_from_description(bbw->pinnamefont),
#else
		      bbw->pinnamefont,
#endif
		      bbw->pinname_gc,
		      label_x,
		      PINLENGTH/2+label_y,
		      name,strlen(name));
      }


    // Draw case outline
    gdk_gc_set_foreground(bbw->case_gc,&black_color);
    gdk_draw_line(module_pixmap,bbw->case_gc,CASEOFFSET,CASEOFFSET,width/2-FOORADIUS,CASEOFFSET);
    gdk_draw_line(module_pixmap,bbw->case_gc,width-CASEOFFSET,CASEOFFSET,width/2+FOORADIUS,CASEOFFSET);
    gdk_draw_line(module_pixmap,bbw->case_gc,width-CASEOFFSET,CASEOFFSET,width-CASEOFFSET,height-CASEOFFSET);
    gdk_draw_line(module_pixmap,bbw->case_gc,CASEOFFSET,height-CASEOFFSET,width-CASEOFFSET,height-CASEOFFSET);
    gdk_draw_line(module_pixmap,bbw->case_gc,CASEOFFSET,CASEOFFSET,CASEOFFSET,height-CASEOFFSET);
    gdk_draw_arc(module_pixmap,bbw->window->style->bg_gc[GTK_WIDGET_STATE (da)],TRUE,width/2-FOORADIUS,CASEOFFSET-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);
    gdk_draw_arc(module_pixmap,bbw->case_gc,FALSE,width/2-FOORADIUS,CASEOFFSET-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);

    //	gtk_widget_realize(da);

    gtk_signal_connect(GTK_OBJECT(da),
		       "expose_event",
		       (GtkSignalFunc) module_expose,
		       this);
    module_widget=da;

  } else {

    // Get the [from the module] provided widget's size
    GtkRequisition req;

    gtk_widget_size_request(module_widget, &req);

    width=req.width;
    height=req.height;

    if(package_height<height-pinspacing)
      package_height=(float)(height-pinspacing);
  }

  // Create xref
  cross_reference = new BreadBoardXREF();
  cross_reference->parent_window_type = WT_breadboard_window;
  cross_reference->parent_window = (gpointer) bbw;
  cross_reference->data = (gpointer) 0;
  module->xref->_add(cross_reference);

  gtk_widget_show(module_widget);


  // Create name_widget
  name_widget = gtk_drawing_area_new();
  int name_width;
  int name_height;
#if GTK_MAJOR_VERSION >= 2
  name_height = gdk_string_height(gdk_font_from_description(bbw->pinnamefont),
				  module->name().c_str());
  name_width = gdk_string_width(gdk_font_from_description(bbw->pinnamefont),
				module->name().c_str());
#else
  name_height = gdk_string_height(bbw->pinnamefont,
				  module->name().c_str());
  name_width = gdk_string_width(bbw->pinnamefont,
				module->name().c_str());
#endif
  gtk_drawing_area_size(GTK_DRAWING_AREA(name_widget),name_width,name_height);
  name_pixmap = gdk_pixmap_new(bbw->window->window,
			       name_width,
			       name_height,
			       -1);
  gdk_draw_rectangle (name_pixmap,
		      bbw->window->style->bg_gc[GTK_WIDGET_STATE (name_widget)],
		      TRUE,
		      0, 0,
		      name_width,
		      name_height);
  gdk_draw_text(name_pixmap,
#if GTK_MAJOR_VERSION >= 2
		gdk_font_from_description(bbw->pinnamefont),
#else
		bbw->pinnamefont,
#endif
		bbw->pinname_gc,
		0,name_height,
		module->name().c_str(),strlen(module->name().c_str()));

  gtk_signal_connect(GTK_OBJECT(name_widget),
		     "expose_event",
		     (GtkSignalFunc) name_expose,
		     this);

  gtk_widget_show(name_widget);


  // Create pins
  GtkWidget *subtree = gtk_tree_new();
  gtk_widget_show(subtree);
  gtk_tree_item_set_subtree(GTK_TREE_ITEM(tree_item), subtree);

  for(i=1;i<=pin_count;i++) {

    int pin_x, pin_y;

    eOrientation orientation;
    char *name;
    BreadBoardXREF *cross_reference=0;
    IOPIN *iopin;

    iopin = module->get_pin(i);


    if(iopin)
      {
	// Create xref
	cross_reference = new BreadBoardXREF();
	cross_reference->parent_window_type = WT_breadboard_window;
	cross_reference->parent_window = (gpointer) bbw;
	cross_reference->data = (gpointer) 0;
	iopin->add_xref(cross_reference);
      }

    pin_position=package->get_pin_position(i);

    // Put pin in layout
    if(pin_position>=0.0 && pin_position<1.0)
      {
	pin_x=0;
	pin_y=(int)(height/2+((pin_position-0.5)*package_height));
	orientation = LEFT;
      }
    else if(pin_position>=2.0 && pin_position<3.0)
      {
	pin_x=width;
	pin_y=(int)(height/2+((3.0-pin_position-0.5)*package_height));
	orientation = RIGHT;
      }
    else
      {
	// FIXME

	printf("################### Error:\n");
	printf("Number of pins %d\n",package->number_of_pins);
	printf("pin_position %f\n",pin_position);
	printf("pin_position2 %f\n",package->pin_position[i-1]);
	printf("i %d\n",i);
	assert(0);
      }

    GuiPin *pin = new GuiPin(bbw, pin_x, pin_y, orientation, iopin);

    pin->addXref(cross_reference);

    gtk_layout_put(GTK_LAYOUT(bbw->layout),
		   pin->widget,0,0);//PINLENGTH+pin->x,PINLENGTH+pin->y);


    pins = g_list_append(pins, pin);

    // Add pin to tree
    name=(char *)module->get_pin_name(i).c_str();
    if(name!=0)
      {
	tree_item = gtk_tree_item_new_with_label (name);
	gtk_signal_connect(GTK_OBJECT(tree_item),
			   "select",
			   (GtkSignalFunc) treeselect_stimulus,
			   pin);
	gtk_widget_show(tree_item);
	gtk_tree_append(GTK_TREE(subtree), tree_item);
      }
  }

  gtk_layout_put(GTK_LAYOUT(bbw->layout), module_widget, 0, 0);
  gtk_layout_put(GTK_LAYOUT(bbw->layout), name_widget, 0,0);

  SetPosition(x, y);
  xpos->set(x);
  ypos->set(y);

  bIsBuilt = true;

  update_board_matrix(bbw);

}

//========================================================================
//========================================================================

GuiModule::GuiModule(Module *_module, Breadboard_Window *_bbw)
  :   bIsBuilt(false), module(_module), bbw(_bbw)
{
  module_widget=0;
  name_widget=0;
  x=0;
  y=0;
  width=0;
  height=0;
  pinnamewidth=0;
  pin_count=0;

  module_pixmap=0;
  name_pixmap=0;

  tree_item=0;

  pins=0;

  if(bbw)
    bbw->modules=g_list_append(bbw->modules, this);

  Build();

}

void Breadboard_Window::Update(void)
{
  GList *iter;
  int x,y;

  // loop all modules and look for changes
  if(!enabled)
    return;

    
  if(!GTK_WIDGET_VISIBLE(window))
    return;

  iter=modules;
  while(iter!=0) {
    
    GList *pin_iter;
    GuiModule *p = static_cast<GuiModule *>(iter->data);

    if(p->bIsBuilt) {

      // Check if module has changed number of pins
      if(p->pin_count!=p->module->get_pin_count())
	// If so, refresh the gui widget
	p->Refresh();

      // Check if module has changed its position
      Value *xpos = p->module->get_attribute("xpos", false);
      Value *ypos = p->module->get_attribute("ypos", false);
      if(xpos && ypos) {
	xpos->get(x);
	ypos->get(y);

	if(p->x!=x || p->y!=y)
	  {
	    // If so, move the module
	    p->SetPosition(x, y);
	    update_board_matrix(p->bbw);
	  }
      }

      // Check if pins have changed state
      pin_iter=p->pins;
      while(pin_iter!=0) {
      

	GuiPin *pin = static_cast<GuiPin *>(pin_iter->data);
	pin->update();
	/*
	IOPIN  *iopin = pin->getIOpin();
	if(iopin) {
	
	bool value;
	eDirection dir;
	  value=iopin->getState();
	  dir=iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;

	  if(value!=pin->getState() || dir!=pin->direction) {
	  
	    pin->putState(value);
	    pin->direction=dir;

	    draw_pin(pin);
	  }
	}
	*/
	pin_iter = pin_iter->next;
      }

    }
    iter = iter->next;
  }

  //  GTKWAIT;

}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Breadboard_Window *bww)
{
  bww->ChangeView(VIEW_HIDE);
  return TRUE;
}

/* When a processor is created */
void Breadboard_Window::NewProcessor(GUI_Processor *_gp)
{
  // Create a Gui representation (note that this memory is
  // placed onto the 'modules' list.
  
  Value *xpos = gp->cpu->get_attribute("xpos", false);
  Value *ypos = gp->cpu->get_attribute("ypos", false);
  if(!xpos || !ypos) {
    xpos = new PositionAttribute(this,"xpos",(double)80);
    ypos = new PositionAttribute(this,"ypos",(double)80);
    gp->cpu->add_attribute(xpos);
    gp->cpu->add_attribute(ypos);
  }

  if(!enabled)
    return;

  new GuiModule(gp->cpu, this);

  if(!gp || !gp->cpu)
    return;

  Update();
}

/* When a module is created */
void Breadboard_Window::NewModule(Module *module)
{
  // If the xpos and ypos attributes does not exist, then create them.
  static int sx=80;
  static int sy=280;

  Value *xpos = module->get_attribute("xpos", false);
  Value *ypos = module->get_attribute("ypos", false);
  if(!xpos || !ypos) {
    xpos = new PositionAttribute(this,"xpos",(double)sx);
    ypos = new PositionAttribute(this,"ypos",(double)sy);
    module->add_attribute(xpos);
    module->add_attribute(ypos);
  }
  sy+=100;
  if(sy>LAYOUTSIZE_Y)
  {
  	sy=0;
	sx+=100;
	if(sx>LAYOUTSIZE_X)
		sx=50;
  }

  if(!enabled)
    return;

  GuiModule *p=new GuiModule(module, this);

  if(grab_next_module)
    grab_module(p);

  Update();
}


/* When a stimulus is being connected or disconnected, or a new node is created */
void Breadboard_Window::NodeConfigurationChanged(Stimulus_Node *node)
{

  if(!enabled)
    return;

  struct gui_node * gn = (struct gui_node*) gtk_object_get_data(GTK_OBJECT(node_tree), node->name().c_str());

  if(gn==0) {
    GtkWidget *node_item;

    gn = (struct gui_node *) malloc(sizeof(*gn));

    gn->bbw=this;
    gn->node=node;

    node_item = gtk_tree_item_new_with_label (node->name().c_str());
    gn->tree_item = node_item;
    gtk_signal_connect(GTK_OBJECT(node_item),
		       "select",
		       (GtkSignalFunc) treeselect_node,
		       gn);
    gtk_widget_show(node_item);
    gtk_tree_append(GTK_TREE(node_tree), node_item);
    gtk_object_set_data(GTK_OBJECT(node_tree), node->name().c_str(), gn);
    gtk_object_set_data(GTK_OBJECT(node_item), "snode", node);

  }
}

static void layout_adj_changed(GtkWidget *widget, Breadboard_Window *bbw)
{
    if(GTK_LAYOUT (bbw->layout)->bin_window==0)
	return;

    if(bbw->layout_pixmap==0)
    {
	puts("bbw.c: no pixmap4!");
	return;
    }

    int xoffset=0, yoffset=0;
    
    GtkAdjustment *xadj, *yadj;
    xadj = gtk_layout_get_hadjustment (GTK_LAYOUT(bbw->layout));
    yadj = gtk_layout_get_vadjustment (GTK_LAYOUT(bbw->layout));
    xoffset = (int) GTK_ADJUSTMENT(xadj)->value;
    yoffset = (int) GTK_ADJUSTMENT(yadj)->value;

    gdk_draw_pixmap(GTK_LAYOUT (bbw->layout)->bin_window,
		    bbw->window->style->white_gc,
		    bbw->layout_pixmap,
		    xoffset, yoffset,
#if GTK_MAJOR_VERSION >= 2
		    xoffset, yoffset,
#else
		    0, 0,
#endif
		    bbw->layout->allocation.width,
		    bbw->layout->allocation.height);

}

static gboolean layout_expose(GtkWidget *widget, GdkEventExpose *event, Breadboard_Window *bbw)
{

    if(bbw->layout_pixmap==0)
    {
	puts("bbw.c: no pixmap5!");
	return 0;
    }

    layout_adj_changed(widget, bbw);

    return 0;
}

static GtkWidget *bb_vbox(GtkWidget *window, const char *name)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox);
  gtk_object_set_data_full (GTK_OBJECT (window), name, vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox);

  return vbox;
}

static GtkWidget *bb_hbox(GtkWidget *window, const char *name)
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox);
  gtk_object_set_data_full (GTK_OBJECT (window), name, hbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox);

  return hbox;
}

GtkWidget* Breadboard_Window::add_button(const char *label, const char *name,
					 GtkSignalFunc f, GtkWidget *box)
{

  GtkWidget *button = gtk_button_new_with_label (label);
  gtk_widget_ref (button);
  gtk_object_set_data_full (GTK_OBJECT (window), name, button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button),
		     "clicked",
		     f,
		     this);

  return button;
}

void Breadboard_Window::Build(void)
{
  if(bIsBuilt)
    return;

  if(!enabled)
    return;

  GtkWidget *hpaned1;
  GtkWidget *vbox9;
  GtkWidget *vbox13;
  GtkWidget *scrolledwindow4;
  GtkWidget *viewport9;
  GtkWidget *tree1;
  GtkWidget *hbox12;
  GtkWidget *hbox15;
  GtkWidget *vbox12;
  GtkWidget *scrolledwindow3;
  GtkWidget *viewport8;
  GtkWidget *hbox11;
  GtkWidget *pic_settings_entry;
  GtkWidget *pic_settings_button;

  GtkWidget *vbox11;
  GtkWidget *scrolledwindow2;
  GtkWidget *viewport7;
  GtkWidget *hbox10;
  GtkWidget *vbox14;
  GtkWidget *hbox13;

  GtkWidget *vbox10;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport6;
  GtkWidget *hbox9;
  GtkWidget *hbox14;
  GtkWidget *scrolledwindow5;

  GdkColormap *colormap = gdk_colormap_get_system();

  gdk_color_parse("red",&high_output_color);
  gdk_color_parse("darkgreen",&low_output_color);
  g_assert(gdk_color_parse("black",&black_color)!=FALSE);
  
  gdk_colormap_alloc_color(colormap, &high_output_color,FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &low_output_color,FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &black_color,FALSE,TRUE);


  //
  // Top level window
  //

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window), "window", window);
  gtk_window_set_title (GTK_WINDOW (window), "Breadboard [Currently in development]");

  //
  // Horizontal pane 
  //

  hpaned1 = gtk_hpaned_new ();
  gtk_widget_ref (hpaned1);
  gtk_object_set_data_full (GTK_OBJECT (window), "hpaned1", hpaned1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hpaned1);
  gtk_container_add (GTK_CONTAINER (window), hpaned1);
  gtk_paned_set_position (GTK_PANED (hpaned1), 196);

  // vbox9 holds the left pane.
  vbox9 = bb_vbox(window, "vbox9");
  gtk_paned_pack1 (GTK_PANED (hpaned1), vbox9, FALSE, TRUE);

  vbox13 = bb_vbox(window, "vbox13");
  gtk_box_pack_start (GTK_BOX (vbox9), vbox13, TRUE, TRUE, 2);

  scrolledwindow4 = gtk_scrolled_window_new (0, 0);
  gtk_widget_ref (scrolledwindow4);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow4", scrolledwindow4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow4);
  gtk_box_pack_start (GTK_BOX (vbox13), scrolledwindow4, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport9 = gtk_viewport_new (0, 0);
  gtk_widget_ref (viewport9);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport9", viewport9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport9);
  gtk_container_add (GTK_CONTAINER (scrolledwindow4), viewport9);

  tree = tree1 = gtk_tree_new ();
  gtk_widget_ref (tree1);
  gtk_object_set_data_full (GTK_OBJECT (window), "tree1", tree1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_tree_set_selection_mode (GTK_TREE(tree),GTK_SELECTION_BROWSE);
  gtk_widget_show (tree1);
  gtk_container_add (GTK_CONTAINER (viewport9), tree1);

  hbox12 = bb_hbox(window, "hbox12");
  gtk_box_pack_start (GTK_BOX (vbox13), hbox12, FALSE, FALSE, 0);

  add_button("Add node","button5", (GtkSignalFunc) add_new_snode, hbox12);
  add_button("Add module","button6", (GtkSignalFunc) add_module, hbox12);
  add_button("Add library","button7", (GtkSignalFunc) add_library, hbox12);

  hbox15 = bb_hbox(window, "hbox15");
  gtk_box_pack_start (GTK_BOX (vbox13), hbox15, FALSE, FALSE, 0);

  add_button("Trace all","button25", (GtkSignalFunc) trace_all, hbox15);
  add_button("Clear traces","button26", (GtkSignalFunc) clear_traces, hbox15);




  pic_frame = gtk_frame_new ("PIC settings");
  gtk_widget_ref (pic_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_frame", pic_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
  //  gtk_widget_show (pic_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), pic_frame, TRUE, TRUE, 0);
  vbox12 = bb_vbox(window, "vbox12");
  gtk_container_add (GTK_CONTAINER (pic_frame), vbox12);

  scrolledwindow3 = gtk_scrolled_window_new (0, 0);
  gtk_widget_ref (scrolledwindow3);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow3", scrolledwindow3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow3);
  gtk_box_pack_start (GTK_BOX (vbox12), scrolledwindow3, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport8 = gtk_viewport_new (0, 0);
  gtk_widget_ref (viewport8);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport8", viewport8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport8);
  gtk_container_add (GTK_CONTAINER (scrolledwindow3), viewport8);

  pic_settings_clist = gtk_clist_new (1);
  gtk_widget_ref (pic_settings_clist);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_settings_clist", pic_settings_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pic_settings_clist);
  gtk_container_add (GTK_CONTAINER (viewport8), pic_settings_clist);

  hbox11 = bb_hbox(window, "hbox11");
  gtk_box_pack_start (GTK_BOX (vbox12), hbox11, FALSE, FALSE, 0);

  pic_settings_entry = gtk_entry_new ();
  gtk_widget_ref (pic_settings_entry);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_settings_entry", pic_settings_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pic_settings_entry);
  gtk_box_pack_start (GTK_BOX (hbox11), pic_settings_entry, FALSE, FALSE, 0);

  pic_settings_button = gtk_button_new_with_label ("Set");
  gtk_widget_ref (pic_settings_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_settings_button", pic_settings_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pic_settings_button);
  gtk_box_pack_start (GTK_BOX (hbox11), pic_settings_button, FALSE, FALSE, 0);





  node_frame = gtk_frame_new ("Node connections");
  gtk_widget_ref (node_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "node_frame", node_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
  //  gtk_widget_show (node_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), node_frame, TRUE, TRUE, 0);

  vbox11 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox11);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox11", vbox11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox11);
  gtk_container_add (GTK_CONTAINER (node_frame), vbox11);

  scrolledwindow2 = gtk_scrolled_window_new (0, 0);
  gtk_widget_ref (scrolledwindow2);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow2", scrolledwindow2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow2);
  gtk_box_pack_start (GTK_BOX (vbox11), scrolledwindow2, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport7 = gtk_viewport_new (0, 0);
  gtk_widget_ref (viewport7);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport7", viewport7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport7);
  gtk_container_add (GTK_CONTAINER (scrolledwindow2), viewport7);

  node_clist = gtk_clist_new (1);
  gtk_widget_ref (node_clist);
  gtk_object_set_data_full (GTK_OBJECT (window), "node_clist", node_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (node_clist);
  gtk_container_add (GTK_CONTAINER (viewport7), node_clist);
  gtk_signal_connect(GTK_OBJECT(node_clist),
		     "select_row",
		     (GtkSignalFunc) node_clist_cb,
		     (gpointer)this);

  hbox10 = bb_hbox(window, "hbox10");
  gtk_box_pack_start (GTK_BOX (vbox11), hbox10, FALSE, FALSE, 0);

  add_button("Remove stimulus","rsb", (GtkSignalFunc) remove_node_stimulus, hbox10);
  add_button("Remove node","rnb", (GtkSignalFunc) remove_node, hbox10);

  stimulus_frame = gtk_frame_new ("Stimulus settings");
  gtk_widget_ref (stimulus_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "stimulus_frame", stimulus_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
  //  gtk_widget_show (stimulus_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), stimulus_frame, FALSE, FALSE, 0);

  vbox14 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox14);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox14", vbox14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox14);
  gtk_container_add (GTK_CONTAINER (stimulus_frame), vbox14);


  stimulus_settings_label=gtk_label_new("");
  gtk_widget_show(stimulus_settings_label);
  gtk_box_pack_start(GTK_BOX(vbox14), stimulus_settings_label, FALSE,FALSE,0);

  hbox13 = bb_hbox(window, "hbox13");
  gtk_box_pack_start (GTK_BOX (vbox14), hbox13, FALSE, FALSE, 0);

  add_button("Connect stimulus to node","sanb", (GtkSignalFunc) stimulus_add_node, hbox13);




  module_frame = gtk_frame_new ("Module settings");
  gtk_widget_ref (module_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "module_frame", module_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
  //  gtk_widget_show (module_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), module_frame, TRUE, TRUE, 0);

  vbox10 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox10);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox10", vbox10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox10);
  gtk_container_add (GTK_CONTAINER (module_frame), vbox10);

  scrolledwindow1 = gtk_scrolled_window_new (0, 0);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox10), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport6 = gtk_viewport_new (0, 0);
  gtk_widget_ref (viewport6);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport6", viewport6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport6);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport6);

  attribute_clist = gtk_clist_new (1);
  gtk_widget_ref (attribute_clist);
  gtk_object_set_data_full (GTK_OBJECT (window), "attribute_clist", attribute_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (attribute_clist);
  gtk_container_add (GTK_CONTAINER (viewport6), attribute_clist);
  gtk_signal_connect(GTK_OBJECT(attribute_clist),
		  "select_row",
		  (GtkSignalFunc)settings_clist_cb,
		  (gpointer)this);

  hbox9 = bb_hbox(window, "hbox9");
  gtk_box_pack_start (GTK_BOX (vbox10), hbox9, FALSE, FALSE, 0);

  attribute_entry = gtk_entry_new ();
  gtk_widget_ref (attribute_entry);
  gtk_object_set_data_full (GTK_OBJECT (window), "attribute_entry", attribute_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (attribute_entry);
  gtk_box_pack_start (GTK_BOX (hbox9), attribute_entry, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(attribute_entry),
		  "activate",
		  (GtkSignalFunc) settings_set_cb,
		  this);

  add_button("Set","attribute_button", (GtkSignalFunc) settings_set_cb, hbox9);

  hbox14 = bb_hbox(window, "hbox14");
  gtk_box_pack_start (GTK_BOX (vbox10), hbox14, FALSE, FALSE, 0);

  add_button("Remove module","remove_module_button", (GtkSignalFunc) remove_module, hbox14);
  add_button("Save Configuration ...","save_stc_button", (GtkSignalFunc) save_stc, vbox9);

  scrolledwindow5 = gtk_scrolled_window_new (0, 0);
  gtk_widget_ref (scrolledwindow5);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow5", scrolledwindow5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow5);
  gtk_paned_pack2 (GTK_PANED (hpaned1), scrolledwindow5, TRUE, TRUE);

  vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolledwindow5));
  hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolledwindow5));

  layout = gtk_layout_new (hadj, vadj);
  gtk_widget_ref (layout);
  gtk_object_set_data_full (GTK_OBJECT (window), "layout", layout,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add (GTK_CONTAINER (scrolledwindow5), layout);
  gtk_layout_set_size (GTK_LAYOUT (layout), LAYOUTSIZE_X, LAYOUTSIZE_Y);
  GTK_ADJUSTMENT (GTK_LAYOUT (layout)->hadjustment)->step_increment = 10;
  GTK_ADJUSTMENT (GTK_LAYOUT (layout)->vadjustment)->step_increment = 10;
  gtk_widget_set_events(layout,
			gtk_widget_get_events(layout)|
			GDK_BUTTON_PRESS_MASK |
			GDK_BUTTON_MOTION_MASK |
			GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect(GTK_OBJECT(layout),"motion-notify-event",
		     GTK_SIGNAL_FUNC(pointer_cb),this);
  gtk_signal_connect(GTK_OBJECT(layout),"button_press_event",
		     GTK_SIGNAL_FUNC(pointer_cb),this);
  gtk_signal_connect(GTK_OBJECT(layout),"button_release_event",
		     GTK_SIGNAL_FUNC(pointer_cb),this);
  gtk_signal_connect(GTK_OBJECT(layout),"expose_event",
		     (GtkSignalFunc) layout_expose,this);

  GtkAdjustment *xadj, *yadj;
  xadj = gtk_layout_get_hadjustment (GTK_LAYOUT(layout));
  yadj = gtk_layout_get_vadjustment (GTK_LAYOUT(layout));
  gtk_signal_connect(GTK_OBJECT(xadj),"value_changed",
		     (GtkSignalFunc) layout_adj_changed,this);
  gtk_signal_connect(GTK_OBJECT(yadj),"value_changed",
		     (GtkSignalFunc) layout_adj_changed,this);

  gtk_widget_set_app_paintable(layout, TRUE);
  gtk_widget_show (layout);


  //printf("bb %s:%d, w=%d, h=%d\n",__FUNCTION__,__LINE__,width,height);
  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");
  
  //  gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
  //			     GTK_SIGNAL_FUNC (gtk_widget_destroyed), GTK_OBJECT(window));
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(delete_event), (gpointer)this);
  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
			   GTK_SIGNAL_FUNC(gui_object_configure_event),this);


  gtk_widget_realize(window);

  pinname_gc=gdk_gc_new(window->window);

  case_gc=gdk_gc_new(window->window);
  gdk_gc_set_line_attributes(case_gc,CASELINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);

#if GTK_MAJOR_VERSION >= 2
  pinstatefont = pango_font_description_from_string("Courier Bold 8");
  pinnamefont = pango_font_description_from_string("Courier Bold 8");
#else
  pinstatefont = gdk_fontset_load ("-adobe-courier-bold-r-*-*-*-80-*-*-*-*-*-*");
  pinnamefont = gdk_fontset_load ("-adobe-courier-bold-r-*-*-*-80-*-*-*-*-*-*");
#endif
  pinline_gc=gdk_gc_new(window->window);
  g_assert(pinline_gc!=0);
  gdk_gc_set_line_attributes(pinline_gc,PINLINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);

  layout_pixmap = gdk_pixmap_new(window->window,
				      LAYOUTSIZE_X,
				      LAYOUTSIZE_Y,
				      -1);

#if GTK_MAJOR_VERSION >= 2
  pinnameheight = gdk_string_height (gdk_font_from_description(pinnamefont),"9y");
#else
  pinnameheight = gdk_string_height (pinnamefont,"9y");
#endif

  if(pinspacing<pinnameheight)
    pinspacing=pinnameheight+2;

  if(pinspacing%ROUTE_RES)
    {
      pinspacing-=pinspacing%ROUTE_RES;
      pinspacing+=ROUTE_RES;
    }


  GtkWidget *tree_item;
  struct gui_node *gn;

  gn = (struct gui_node *) malloc(sizeof(*gn));
  gn->bbw=this;
  gn->node=0; // indicates that this is the root node.
  tree_item = gtk_tree_item_new_with_label ("nodes");
  //    gtk_signal_connect(GTK_OBJECT(tree_item),
  //		       "select",
  //		       (GtkSignalFunc) treeselect_node,
  //		       gn);
  gtk_widget_show(tree_item);
  gtk_tree_append(GTK_TREE(tree), tree_item);
  node_tree= gtk_tree_new();
  gtk_widget_show(node_tree);
  gtk_tree_item_set_subtree(GTK_TREE_ITEM(tree_item), node_tree);
  gtk_object_set_data(GTK_OBJECT(node_tree), "root_of_nodes", gn);

  bIsBuilt = true;

  UpdateMenuItem();

  draw_nodes(this);

  // Look module list
  list <Module *>::iterator mi;
  for(mi = instantiated_modules_list.begin();
  	mi!=instantiated_modules_list.end();
	mi++)
  {
  	NewModule(*mi);
  }

#if defined(NEW_SYMBOL_TABLE_CHANGES_REALLY_DO_WORK)
  // Loop node list
  Symbol_Table &ST = get_symbol_table();
  Symbol_Table::node_symbol_iterator it;
  Symbol_Table::node_symbol_iterator itEnd = ST.endNodeSymbol();
  for(it = ST.beginNodeSymbol(); it != itEnd; it++) {
    Stimulus_Node *node = (*it)->getNode();
    assert(node != NULL);
    if(node != NULL) {
    	NodeConfigurationChanged(node);
    }
  }
#else
  cout <<"WARNING: symbol table is broken - can't save the node list\n";
#endif
  gtk_widget_show(window);

  Update();
}


Breadboard_Window::Breadboard_Window(GUI_Processor *_gp)
{
  menu = "<main>/Windows/Breadboard";

  set_name("pinout");
  wc = WC_misc;
  wt = WT_breadboard_window;
  window = 0;

  pinstatefont = 0;
  pinnamefont = 0;
  pinname_gc = 0;
  pinline_gc = 0;
  case_gc = 0;
  node_tree = 0;

  modules=0;

  node_clist=0;

  stimulus_settings_label=0;

  stimulus_add_node_button=0;

  selected_node=0;
  selected_pin=0;
  selected_module=0;

  hadj = 0;
  vadj = 0;

  layout_pixmap=0;

  gp = _gp;

  if(!get_config())
    printf("warning: %s\n",__FUNCTION__);

  //printf("bb %s:%d, w=%d, h=%d\n",__FUNCTION__,__LINE__,width,height);

  if(enabled)
    Build();

}

#endif // HAVE_GUI
