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


#include "../src/modules.h"
#include "../src/stimuli.h"
#include "../src/pic-processor.h"
#include "../src/symbol.h"
#include "../src/stimuli.h"

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

#define ROUTE_RES (2*PINLINEWIDTH)

static void treeselect_module(GtkItem *item, struct gui_module *p);

///////////////////////////////////////////////////////////////////////
// Start of autorouting pain 
///////////////////////////////////////////////////////////////////////
#define XSIZE LAYOUTSIZE_X/ROUTE_RES 
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

static inline int allow_horiz(point &p)
{
  if(board_matrix[p.x][p.y] & HMASK)
    return FALSE;
  return TRUE;
}

static inline int allow_vert(point &p)
{
  if(board_matrix[p.x][p.y] & VMASK)
    return FALSE;

  return TRUE;
}

// Find the birds direction of choice to get to from s to e.
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

// Free all of pat
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
    if(retval==TRUE)
    {
	// We found a path to end. Add point p to path.
/*	if(*pat!=0 && (*pat)->next!=0)
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
    /*
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
} */

#if 0
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
	gui_module *p;

	p = (struct gui_module*) mi->data;

        x=p->x;
	y=p->y/*-PINLENGTH*/;
	width=p->width/*+PINLENGTH*/;
	height=p->height/*+PINLENGTH*/;

        // Debug. This shows the boxes that limits traceing.
	gdk_draw_rectangle(bbw->layout_pixmap,
			   bbw->case_gc,0,
			   x,y,width,height);

	// Draw barriers around pins so the tracker can only get in
        // straigt to the pin and not from the side.
	for(i=1;i<=p->module->get_pin_count();i++)
	{

	    GList *e;
            struct gui_pin *gp;

	    e = g_list_nth(p->pins, i-1);

	    gp = (struct gui_pin*)e->data;

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
        mi=mi->next;
    }
}
#endif

static GList *nodepath_list;

// Draw node in nodepath_list to layout_pixmap
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
//			((GUI_Object*)bbw)->window->style->white_gc,
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
/*    gdk_draw_pixmap(GTK_LAYOUT (bbw->layout)->bin_window,
		    ((GUI_Object*)bbw)->window->style->white_gc,
		    bbw->layout_pixmap,
		    0, 0,
		    0, 0,
		    LAYOUTSIZE_X,
		    LAYOUTSIZE_Y);

    gtk_widget_queue_draw(bbw->layout);*/
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
    while(mi!=0)
    {
	gui_module *p;

	p = (struct gui_module*) mi->data;

        x=p->x;
	y=p->y/*-PINLENGTH*/;
	width=p->width/*+PINLENGTH*/;
	height=p->height/*+PINLENGTH*/;

	for(;y<p->y+height;y+=ROUTE_RES)
	{
	    x=p->x;
	    for(;x<p->x+width;x+=ROUTE_RES)
	    {
		board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
	    }
	}

	// Draw barriers around pins so the tracker can only get in
        // straigt to the pin and not from the side.
	for(i=1;i<=p->module->get_pin_count();i++)
	{
//	    struct stimulus *s;
	    GList *e;
            struct gui_pin *gp;

//	    s=p->module->get_pin(i);


	    e = g_list_nth(p->pins, i-1);

	    gp = (struct gui_pin*)e->data;

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

static struct gui_pin *find_gui_pin(Breadboard_Window *bbw, stimulus *pin);

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
    struct gui_pin *p;
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
	struct gui_pin *pi, *pj;
        GList *li, *lj;

	li = g_list_nth(pinlist,i);
        assert(li!=0);
        pi = (gui_pin*) li->data;
	printf(" %s",pi->iopin->name().c_str());
	fflush(stdout);
	for(j=i+1;j<nr_of_nodes;j++)
	{
	    lj = g_list_nth(pinlist,j);
	    assert(lj!=0);
	    pj = (gui_pin*) lj->data;

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
                       pi->iopin->name().c_str(),
                       pj->iopin->name().c_str());
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


///////////////////////////////////////////////////////////////////////
// Pain is over!
///////////////////////////////////////////////////////////////////////

struct gui_pin *find_gui_pin(Breadboard_Window *bbw, stimulus *pin)
{
    GList *iter;
    int i;
    gui_module *m;



    iter = bbw->modules;
    while(iter!=0)
    {
	m = (gui_module *)iter->data;

//	Package *pa;
//	pa=dynamic_cast<Package*>(m);
//	assert(pa!=0);

	for(i=1;i<=m->module->get_pin_count();i++)
	{
	    struct stimulus *p;

	    p=m->module->get_pin(i);

	    if(p == pin)
	    {
                GList *e;

		e = g_list_nth(m->pins, i-1);

                return (struct gui_pin*)e->data;
	    }
	}

	iter = iter->next;
    }

    return 0;
}






static void draw_pin(struct gui_pin *pin)
{
    int pointx;
    int wingheight, wingx;
    int casex, endx;
    int y;

    switch(pin->orientation)
    {
    case LEFT:
	casex = pin->width;
        endx = 0;
        break;
    default:
	casex = 0;
        endx = pin->width;
        break;
    }

    y = pin->height/2;

    // Clear pixmap
    gdk_draw_rectangle (pin->pixmap,
			pin->bbw->window->style->bg_gc[GTK_WIDGET_STATE (pin->widget)],
			TRUE,
			0, 0,
			pin->width,
			pin->height);


    if(pin->type==PIN_OTHER)
	gdk_gc_set_foreground(pin->gc,&black_color);
    else
	gdk_gc_set_foreground(pin->gc,pin->value>0?&high_output_color:&low_output_color);

    // Draw actual pin
    gdk_draw_line(pin->pixmap,pin->gc,
		  casex,y,endx,y);

    if(pin->type==PIN_OTHER)
	return;

    // Draw direction arrow
    wingheight=pin->height/3;
    
    if(casex>endx)
    {
	if(pin->direction==PIN_OUTPUT)
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
	if(pin->direction==PIN_OUTPUT)
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
    gdk_draw_line(pin->pixmap,pin->gc,
		  pointx,y,wingx,y+wingheight);
    gdk_draw_line(pin->pixmap,pin->gc,
		  pointx,y,wingx,y-wingheight);

    if(pin->widget->window!=0)
	gdk_draw_pixmap(pin->widget->window,
			pin->widget->style->fg_gc[GTK_WIDGET_STATE (pin->widget)],
			pin->pixmap,
			0, 0,
			0, 0,
			pin->width, pin->height);
}

static void expose_pin(GtkWidget *widget,
		       GdkEventExpose *event,
		       struct gui_pin *p)
{
    if(p->pixmap==0)
    {
	puts("bbw.c: no pixmap1!");
	return;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
}

static void treeselect_stimulus(GtkItem *item, struct gui_pin *pin)
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

    if(pin->iopin) {
      snprintf(string,sizeof(string),"Stimulus %s",pin->iopin->name().c_str());
      pString = string;

      if(pin->iopin->snode!=0)
	snprintf(text,sizeof(text),"Connected to node %s", pin->iopin->snode->name().c_str());
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
	Attribute *attr;
	char attrstr[50];
	char str[256];
	attr = (Attribute*) gtk_clist_get_row_data(GTK_CLIST(bbw->attribute_clist),
			row);

	attr->sGet(attrstr,50);
	
	sprintf(str,"%s = %s",attr->get_name(),attrstr);
	
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
	
	Attribute *attr;
	
	// Change the Attribute
	attr = bbw->selected_module->module->get_attribute(attribute_name);
	if(attr)
	{
		// Set attribute
		attr->set(atoi(attribute_newval));

		// Update clist
		treeselect_module(0, bbw->selected_module);
	}
	else
	{
		printf("Could not find attribute \"%s\"\n",attribute_name);
	}
}

static void treeselect_module(GtkItem *item, struct gui_module *p)
{
    char buffer[STRING_SIZE];
    snprintf(buffer,sizeof(buffer),"%s settings",p->module->name().c_str());
    switch(p->type)
    {
    case PIC_MODULE:
	gtk_widget_hide(p->bbw->stimulus_frame);
	gtk_widget_hide(p->bbw->node_frame);
	gtk_widget_hide(p->bbw->module_frame);
	gtk_widget_show(p->bbw->pic_frame);
        gtk_frame_set_label(GTK_FRAME(p->bbw->pic_frame),buffer);
        break;
    case EXTERNAL_MODULE:
	gtk_widget_hide(p->bbw->stimulus_frame);
	gtk_widget_hide(p->bbw->node_frame);
	gtk_widget_hide(p->bbw->pic_frame);
	gtk_widget_show(p->bbw->module_frame);
	gtk_frame_set_label(GTK_FRAME(p->bbw->module_frame),buffer);

	// freeze
        gtk_clist_freeze(GTK_CLIST(p->bbw->attribute_clist));
	// clear clist
        gtk_clist_clear(GTK_CLIST(p->bbw->attribute_clist));
	// read attributes and add to clist
	{
	    char attribute_string[STRING_SIZE];
	    char *text[1]={attribute_string};
            list <Attribute *> :: iterator attribute_iterator;
	    int row;

	    for (attribute_iterator = p->module->attributes.begin();
		 attribute_iterator != p->module->attributes.end();
		 attribute_iterator++) {

		Attribute *locattr = *attribute_iterator;

		//cout << locattr->get_name();

                strcpy(attribute_string,locattr->get_name());

		    char buf[50];
		    locattr->sGet(buf,50);

                    strcat(attribute_string," = ");
		    strcat(attribute_string,buf);
		    row = gtk_clist_append(GTK_CLIST(p->bbw->attribute_clist),
					   text);
		    // add the Attribute* as data for the clist rows.
		    gtk_clist_set_row_data(GTK_CLIST(p->bbw->attribute_clist),
				    row,
				    (gpointer)locattr);
				    
	    }
	}
	// thaw
        gtk_clist_thaw(GTK_CLIST(p->bbw->attribute_clist));
	
	gtk_entry_set_text(GTK_ENTRY(p->bbw->attribute_entry), "");

	// Set button callback with Attribute* when selecting clist row.
        // "resistance = 100"

        break;
    }

    p->bbw->selected_module = p;
}

static void position_module(struct gui_module *p, int x, int y)
{
    GList *piniter;
    struct gui_pin *pin;

    x=x-x%pinspacing;
    y=y-y%pinspacing;

    if(x != p->x || y != p->y)
    {
	p->x=x;
	p->y=y;

	// Position module_widget
        gtk_layout_move(GTK_LAYOUT(p->bbw->layout), p->module_widget, p->x, p->y);

	// Position module_name
        gtk_layout_move(GTK_LAYOUT(p->bbw->layout), p->name_widget, p->x, p->y-10);

        // Position pins
	piniter = p->pins;
	while(piniter!=0)
	{
	    pin = (struct gui_pin*) piniter->data;

            pin->layout_xpos = p->x + pin->x;
            pin->layout_ypos = p->y + pin->y + pin->height/2; // FIXME /2?

//	    if(pin->orientation==LEFT)
//                pin->layout_xpos-=pinspacing/2;
	    if(pin->orientation==RIGHT)
		pin->layout_xpos+=PINLENGTH /*+ pinspacing/2*/;

	    gtk_layout_move(GTK_LAYOUT(p->bbw->layout),
			    pin->widget,p->x+pin->x,p->y+pin->y);

            piniter = piniter->next;
	}
    }
}

static double module_distance(struct gui_module *p, int x, int y)
{
    double distance;
    double min_distance=100000000;

    // Upper left
    distance=sqrt((double)abs(p->x-x)*abs(p->x-x) +
		  abs(p->y-y)*abs(p->y-y));
    if(distance<min_distance)
        min_distance=distance;

    // Upper right
    distance=sqrt((double)abs(p->x+p->width-x)*abs(p->x+p->width-x) +
		  abs(p->y-y)*abs(p->y-y));
    if(distance<min_distance)
        min_distance=distance;

    // Lower left
    distance=sqrt((double)abs(p->x-x)*abs(p->x-x) +
		  abs(p->y+p->height-y)*abs(p->y+p->height-y));
    if(distance<min_distance)
        min_distance=distance;

    // Lower right
    distance=sqrt((double)abs(p->x+p->width-x)*abs(p->x+p->width-x) +
		  abs(p->y+p->height-y)*abs(p->y+p->height-y));
    if(distance<min_distance)
	min_distance=distance;

/*    printf("Module %s x=%d, y=%d, width=%d, height %d\n",
	   p->module->name(),
	   p->x,
	   p->y,
	   p->width,
	   p->height);
    printf("distance %f\n\n",min_distance);
*/
    return min_distance;
}

static struct gui_module *find_closest_module(Breadboard_Window *bbw, int x, int y)
{
    GList *mi;
    gui_module *closest=0;
    double distance, min_distance=1000000;

    mi = bbw->modules;

    while(mi!=0)
    {
	gui_module *p;

	p = (struct gui_module*) mi->data;

	distance = module_distance(p,x,y);
	if(distance<min_distance) {
	    closest = p;
	    min_distance = distance;
	}

        mi=mi->next;
    }

    return closest;
}

// FIXME
static struct gui_module *dragged_module;
static int dragging;
static int grab_next_module;

void grab_module(gui_module *p)
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

    x = (int) (event->x + bbw->hadj->value);
    y = (int) (event->y + bbw->vadj->value);

    switch(event->type)
    {
    case GDK_MOTION_NOTIFY:
	if(dragging)
	{
            position_module(dragged_module, x+pinspacing, y+pinspacing);
	    //dragged_module->module->x = dragged_module->x;
	    //dragged_module->module->y = dragged_module->y;
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
		   struct gui_pin *p)
{
    if(event->type==GDK_BUTTON_PRESS &&
       event->button==1)
    {
	if(p->iopin!=0)
	{
	    if(p->iopin->snode!=0)
	    {
		struct gui_node *gn;

		gn = (struct gui_node *)
		    gtk_object_get_data(GTK_OBJECT(p->bbw->node_tree),
					p->iopin->snode->name().c_str());

		if(gn!=0)
		{
		    treeselect_node(0, gn);
		    return 1;
		}
	    }

	    treeselect_stimulus(0, p);
	}
	return 1;
    }

    if(event->type==GDK_2BUTTON_PRESS &&
       event->button==1)
    {
	if(p->direction==PIN_OUTPUT)
	{
	    p->iopin->put_state_value(p->value<0?1:0);
	}
	else
	{
	    p->iopin->toggle();
	}
	return 1;
    }

    if(event->type==GDK_BUTTON_PRESS &&
       event->button==2)
    {
	if(p->iopin->snode)
	{
	    struct gui_node *gn;

	    gn = (struct gui_node *)
		gtk_object_get_data(GTK_OBJECT(p->bbw->node_tree),
				    p->iopin->snode->name().c_str());

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

    if(retval==TRUE)
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


    if(cancel==TRUE)
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


    if(cancel==TRUE)
    {
	gtk_widget_hide(dialog);
	return 0;
    }

    gtk_widget_hide(dialog);

    return module_type;
}

#if 0
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

    struct Stimulus_Node *node;


    node = select_node_dialog(bbw);

    if(node!=0 && bbw->selected_pin!=0)
    {
	node->attach_stimulus(bbw->selected_pin->iopin);

        // Update stimulus frame
	treeselect_stimulus(0, bbw->selected_pin);
    }
}

static void add_library(GtkWidget *button, Breadboard_Window *bbw)
{
    const char *library_name;

    library_name = gui_get_string("Module library name (e.g. libgpsim_modules.so)","");

    if(library_name!=0)
    {
        module_load_library(library_name);
    }
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
	struct gui_pin *pin;

	pin = (struct gui_pin *) pin_iter->data;

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
    list <Module *> :: iterator module_iterator;
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

    // Save processor command. How?
    m=bbw->gp->cpu;
    /*
    if(m && m->x>=0 && m->y>=0)
	fprintf(fo, "module position %s %d %d\n",
		m->name(),
		m->x,
		m->y);
    */
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
    for (module_iterator = instantiated_modules_list.begin();
	 module_iterator != instantiated_modules_list.end();
	 module_iterator++)
    {
	list <Attribute*> :: iterator attribute_iterator;
	m = *module_iterator;

	fprintf(fo, "module load %s %s\n",
		m->type(),
		m->name().c_str());
	/*
        if(m->x>=0 && m->y>=0)
	    fprintf(fo, "module position %s %d %d\n",
		    m->name(),
		    m->x,
		    m->y);
	*/

	for(attribute_iterator = m->attributes.begin();
		attribute_iterator != m->attributes.end();
		attribute_iterator++) {
		Attribute *locattr = *attribute_iterator;
		char attrval[50];
		locattr->sGet(attrval,50);
		
		fprintf(fo, "module set %s %s %s\n",
				m->name().c_str(),
				locattr->get_name(),
				attrval);
	}
    }


    // Save nodes and connections
    fprintf(fo, "\n\n# Connections:\n");
    list <Stimulus_Node *> :: iterator node_iterator;

    for (node_iterator = node_list.begin();
	 node_iterator != node_list.end();
	 node_iterator++)
    {
	Stimulus_Node *node = *node_iterator;
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

static struct gui_pin *create_gui_pin(Breadboard_Window *bbw, int x, int y, orientation orientation, IOPIN *iopin)
{
    struct gui_pin *pin;

    pin=(struct gui_pin *)malloc(sizeof(*pin));

    pin->iopin = iopin;
    pin->x=x;
    pin->y=y;
    pin->width=pinspacing;
    pin->height=pinspacing;
    pin->bbw=bbw;
    pin->gc=bbw->pinline_gc;

    pin->y-=pin->height/2;

    pin->layout_xpos = 0;
    pin->layout_ypos = 0;


    if(iopin!=0)
    {
	pin->value=iopin->get_state();
	pin->direction=iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;
	pin->orientation=orientation;
        pin->type=PIN_DIGITAL;
    //debug
    //pin->direction=PIN_INPUT;
    }
    else
    {
	pin->value=0;
	pin->direction=PIN_INPUT;
        pin->orientation=orientation;
	pin->type=PIN_OTHER;
    }

    if(pin->orientation==LEFT)
        pin->x-=pin->width;

    // Create widget
    pin->widget = gtk_drawing_area_new();
    gtk_widget_set_events(pin->widget,
			  gtk_widget_get_events(pin->widget)|
			  GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(pin->widget),
		       "button_press_event",
		       (GtkSignalFunc) button,
		       pin);

    gtk_drawing_area_size(GTK_DRAWING_AREA(pin->widget),pin->width,pin->height);
    gtk_signal_connect(GTK_OBJECT(pin->widget),
		       "expose_event",
		       (GtkSignalFunc) expose_pin,
		       pin);


    // Create pixmap
    pin->pixmap = gdk_pixmap_new(bbw->window->window,
				pin->width,
				pin->height,
				-1);

    // Draw pin
    draw_pin(pin);

    gtk_widget_show(pin->widget);

    return pin;
}

static void name_expose(GtkWidget *widget, GdkEventExpose *event, struct gui_module *p)
{
    if(p->name_pixmap==0)
    {
	puts("bbw.c: no pixmap2!");
	return;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->name_pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
}

static void module_expose(GtkWidget *widget, GdkEventExpose *event, struct gui_module *p)
{
    if(p->module_pixmap==0)
    {
	puts("bbw.c: no pixmap3!");
	return;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->module_pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
}

#define PACKAGESPACING 15

struct gui_module *create_gui_module(Breadboard_Window *bbw,
                                 enum module_type type,
				 class Module *module,
				 GtkWidget *widget)
{
    static int x=80;
    static int y=80;
    static int max_x;
    float package_height;

    struct gui_module *p;
    int i;
    BreadBoardXREF *cross_reference;
    GtkWidget *da;
    int width=50, height=18;

    p = (struct gui_module*) malloc(sizeof(*p));

    p->bbw=bbw;
    p->module=module;
    p->module_widget = widget;
    p->type=type;
    p->x=-1;
    p->y=-1;

    p->pins=0;




    // FIXME. Perhaps the bbw should use Package instead of Module?
    Package *package = p->module->package;
    float pin_position;
    if(!package)
      package=dynamic_cast<Package*>(p->module);

    assert(package!=0);

    GtkWidget *tree_item;
    tree_item = gtk_tree_item_new_with_label (p->module->name().c_str());
    gtk_signal_connect(GTK_OBJECT(tree_item),
		       "select",
		       (GtkSignalFunc) treeselect_module,
		       p);
    gtk_widget_show(tree_item);
    gtk_tree_append(GTK_TREE(bbw->tree), tree_item);
    p->tree_item = tree_item;

    package_height=(p->module->get_pin_count()/2+(p->module->get_pin_count()&1)-1)*pinspacing;

    if(p->module_widget==0)
    {
	// Create a static representation.
	p->pinnamewidth=0;
	for(i=1;i<=p->module->get_pin_count();i++)
	{
	    char *name;
	    int width;

	    name=(char *)p->module->get_pin_name(i).c_str();
	    if(name==0)
		continue;
#if GTK_MAJOR_VERSION >= 2
	    width = gdk_string_width (gdk_font_from_description(bbw->pinnamefont),name)+LABELPAD;
#else
	    width = gdk_string_width (bbw->pinnamefont,name)+LABELPAD;
#endif
	    if(width>p->pinnamewidth)
		p->pinnamewidth=width;
	}

        p->pinnamewidth+=FOORADIUS; // The 'U' at the top of the DIL module

	p->width=p->pinnamewidth*2; // pin name widthts
	p->width+=2*CASELINEWIDTH+2*LABELPAD;

	p->height=module->get_pin_count()/2*pinspacing; // pin name height
	if(module->get_pin_count()%2)
            p->height+=pinspacing;
        p->height+=2*CASELINEWIDTH+2*LABELPAD;

	da = gtk_drawing_area_new();

	gtk_drawing_area_size(GTK_DRAWING_AREA(da),p->width,p->height);



	p->module_pixmap = gdk_pixmap_new(bbw->window->window,
				p->width,
				p->height,
				-1);



	gdk_draw_rectangle (p->module_pixmap,
			    bbw->window->style->bg_gc[GTK_WIDGET_STATE (da)],
			    TRUE,
			    0, 0,
			    p->width,
			    p->height);

	gdk_draw_rectangle (p->module_pixmap,
			    bbw->window->style->white_gc,
			    TRUE,
			    CASEOFFSET, CASEOFFSET,
			    p->width-CASEOFFSET,
			    p->height-CASEOFFSET);

	// Draw pin names
	for(i=1;i<=p->module->get_pin_count();i++)
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
		label_x=LABELPAD+p->width/2+FOORADIUS;
		label_y=(int)((3.0-pin_position)*package_height);
		label_y+=LABELPAD+CASELINEWIDTH+pinspacing/2-bbw->pinnameheight/3;
	    }

	    name=(char*)p->module->get_pin_name(i).c_str();
	    if(name==0)
		continue;
	    gdk_draw_text(p->module_pixmap,
#if GTK_MAJOR_VERSION >= 2
			  gdk_font_from_description(p->bbw->pinnamefont),
#else
			  p->bbw->pinnamefont,
#endif
			  p->bbw->pinname_gc,
			  label_x,
			  PINLENGTH/2+label_y,
			  name,strlen(name));
	}


        // Draw case outline
	gdk_gc_set_foreground(p->bbw->case_gc,&black_color);
	gdk_draw_line(p->module_pixmap,p->bbw->case_gc,CASEOFFSET,CASEOFFSET,p->width/2-FOORADIUS,CASEOFFSET);
	gdk_draw_line(p->module_pixmap,p->bbw->case_gc,p->width-CASEOFFSET,CASEOFFSET,p->width/2+FOORADIUS,CASEOFFSET);
	gdk_draw_line(p->module_pixmap,p->bbw->case_gc,p->width-CASEOFFSET,CASEOFFSET,p->width-CASEOFFSET,p->height-CASEOFFSET);
	gdk_draw_line(p->module_pixmap,p->bbw->case_gc,CASEOFFSET,p->height-CASEOFFSET,p->width-CASEOFFSET,p->height-CASEOFFSET);
	gdk_draw_line(p->module_pixmap,p->bbw->case_gc,CASEOFFSET,CASEOFFSET,CASEOFFSET,p->height-CASEOFFSET);
	gdk_draw_arc(p->module_pixmap,bbw->window->style->bg_gc[GTK_WIDGET_STATE (da)],TRUE,p->width/2-FOORADIUS,CASEOFFSET-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);
	gdk_draw_arc(p->module_pixmap,p->bbw->case_gc,FALSE,p->width/2-FOORADIUS,CASEOFFSET-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);

//	gtk_widget_realize(da);

	gtk_signal_connect(GTK_OBJECT(da),
			   "expose_event",
			   (GtkSignalFunc) module_expose,
			   p);

        p->module_widget=da;
    }
    else
    {
        // Get the [from the module] provided widget's size
        GtkRequisition req;

	gtk_widget_size_request(p->module_widget, &req);

	p->width=req.width;
	p->height=req.height;

	if(package_height<p->height-pinspacing)
            package_height=p->height-pinspacing;
    }

    if(y+p->height>LAYOUTSIZE_Y-30)
    {
	// When we reach the bottom of the layout, we move up again
	// and to the right of current row of modules.
	y=30;
	x=max_x+4*PINLENGTH;
    }

    // Create xref
    cross_reference = new BreadBoardXREF();
    cross_reference->parent_window_type = WT_breadboard_window;
    cross_reference->parent_window = (gpointer) bbw;
    cross_reference->data = (gpointer) 0;
    p->module->xref->_add(cross_reference);

    gtk_widget_show(p->module_widget);


    // Create name_widget
    p->name_widget = gtk_drawing_area_new();
#if GTK_MAJOR_VERSION >= 2
    height = gdk_string_height(gdk_font_from_description(bbw->pinnamefont),
			       p->module->name().c_str());
    width = gdk_string_width(gdk_font_from_description(bbw->pinnamefont),
			    p->module->name().c_str());
#else
    height = gdk_string_height(bbw->pinnamefont,
			       p->module->name().c_str());
    width = gdk_string_width(bbw->pinnamefont,
			    p->module->name().c_str());
#endif
    gtk_drawing_area_size(GTK_DRAWING_AREA(p->name_widget),width,height);
    p->name_pixmap = gdk_pixmap_new(bbw->window->window,
			       width,
			       height,
			       -1);
    gdk_draw_rectangle (p->name_pixmap,
			bbw->window->style->bg_gc[GTK_WIDGET_STATE (p->name_widget)],
			TRUE,
			0, 0,
			width,
			height);
    gdk_draw_text(p->name_pixmap,
#if GTK_MAJOR_VERSION >= 2
		  gdk_font_from_description(p->bbw->pinnamefont),
#else
                  p->bbw->pinnamefont,
#endif
                  p->bbw->pinname_gc,
		  0,height,
		  p->module->name().c_str(),strlen(p->module->name().c_str()));
    gtk_signal_connect(GTK_OBJECT(p->name_widget),
		       "expose_event",
		       (GtkSignalFunc) name_expose,
		       p);
    gtk_widget_show(p->name_widget);


    // Create pins
    GtkWidget *subtree = gtk_tree_new();
    gtk_widget_show(subtree);
    gtk_tree_item_set_subtree(GTK_TREE_ITEM(tree_item), subtree);
    for(i=1;i<=p->module->get_pin_count();i++)
    {
	int pin_x, pin_y;
	struct gui_pin *pin;
	enum orientation orientation;
        char *name;
	BreadBoardXREF *cross_reference;
	IOPIN *iopin;

        iopin = p->module->get_pin(i);


	if(iopin!=0)
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
	    pin_y=(int)(p->height/2+((pin_position-0.5)*package_height));
	    orientation = LEFT;
	}
	else if(pin_position>=2.0 && pin_position<3.0)
	{
	    pin_x=p->width;
	    pin_y=(int)(p->height/2+((3.0-pin_position-0.5)*package_height));
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

	pin = create_gui_pin(bbw,
			     pin_x,
			     pin_y,
			     orientation,
			     iopin);

	gtk_layout_put(GTK_LAYOUT(bbw->layout),
		      pin->widget,0,0);//PINLENGTH+pin->x,PINLENGTH+pin->y);


	p->pins = g_list_append(p->pins, pin);

        // Add pin to tree
	name=(char *)p->module->get_pin_name(i).c_str();
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

    gtk_layout_put(GTK_LAYOUT(bbw->layout), p->module_widget, 0, 0);
    gtk_layout_put(GTK_LAYOUT(bbw->layout), p->name_widget, 0,0);

    position_module(p, x, y);
    //p->module->x = p->x;
    //p->module->y = p->y;
    update_board_matrix(p->bbw);

    bbw->modules=g_list_append(bbw->modules, p);

    y+=p->height+PACKAGESPACING;

    if(x+p->width>max_x)
	max_x=x+p->width;

    return p;
}

void Breadboard_Window::Update(void)
{
  GList *iter;

  // loop all modules and update their pins

  iter=modules;
  while(iter!=0) {
    
    GList *pin_iter;
    struct gui_module *p;

    p = (struct gui_module*)iter->data;

    /*
    // Check if module has changed its position
    if(p->module->x!=p->x || p->module->y!=p->y)
      {
	if(p->module->x>=0 && p->module->y>=0)
	  {
	    // printf(" to position %d %d",p->module->x, p->module->y);

	    position_module(p, p->module->x, p->module->y);
	    p->module->x = p->x;
	    p->module->y = p->y;
	    update_board_matrix(p->bbw);
	  }
      }
    */

    // Check if pins have changed state
    pin_iter=p->pins;
    while(pin_iter!=0) {
      
      struct gui_pin *pin;

      int value;
      direction dir;

      pin = (struct gui_pin *) pin_iter->data;

      if(pin->iopin!=0) {
	
	value=pin->iopin->get_state();
	dir=pin->iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;

	if(value!=pin->value || dir!=pin->direction) {
	  
	  pin->value=value;
	  pin->direction=dir;

	  draw_pin(pin);
	}
      }
      pin_iter = pin_iter->next;
    }

    iter = iter->next;
  }
}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Breadboard_Window *bww)
{
  bww->ChangeView(VIEW_HIDE);
  return TRUE;
}

#if 0
static void check_for_modules(Breadboard_Window *bbw)
{
    list <Module *> :: iterator module_iterator;

    for (module_iterator = instantiated_modules_list.begin();
	 module_iterator != instantiated_modules_list.end();
	 module_iterator++)
    {
	Module *m = *module_iterator;

        bbw->NewModule(m);
    }
}

static void check_for_nodes(Breadboard_Window *bbw)
{
    list <Stimulus_Node *> :: iterator node_iterator;

    for (node_iterator = node_list.begin();
	 node_iterator != node_list.end();
	 node_iterator++)
    {
	Stimulus_Node *node = *node_iterator;

	bbw->NodeConfigurationChanged(node);
    }
}
#endif
/* When a processor is created */
void Breadboard_Window::NewProcessor(GUI_Processor *_gp)
{

  if(!is_built)
    return;

  if(!gp || !gp->cpu)
    return;

  create_gui_module(this, PIC_MODULE, gp->cpu,0);

  Update();
}

/* When a module is created */
void Breadboard_Window::NewModule(Module *module)
{
  if(!is_built)
    Build();


  GtkWidget *widget=0;
  
  //if(module->widget!=0)
  //  widget=GTK_WIDGET(module->widget);

  struct gui_module *p=create_gui_module(this, EXTERNAL_MODULE, module, widget);

  if(grab_next_module)
    grab_module(p);


  Update();
}


/* When a stimulus is being connected or disconnected, or a new node is created */
void Breadboard_Window::NodeConfigurationChanged(Stimulus_Node *node)
{
  if(!is_built)
    Build();

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

    int xoffset, yoffset;
    GtkAdjustment *xadj, *yadj;

    xadj = gtk_layout_get_hadjustment (GTK_LAYOUT(bbw->layout));
    yadj = gtk_layout_get_vadjustment (GTK_LAYOUT(bbw->layout));

    xoffset = (int) GTK_ADJUSTMENT(xadj)->value;
    yoffset = (int) GTK_ADJUSTMENT(yadj)->value;

    gdk_draw_pixmap(GTK_LAYOUT (bbw->layout)->bin_window,
		    bbw->window->style->white_gc,
		    bbw->layout_pixmap,
		    xoffset, yoffset,
		    0, 0,
		    bbw->layout->allocation.width,
		    bbw->layout->allocation.height);

    gtk_widget_queue_draw(bbw->layout);
}

static void layout_expose(GtkWidget *widget, GdkEventExpose *event, Breadboard_Window *bbw)
{
    if(bbw->layout_pixmap==0)
    {
	puts("bbw.c: no pixmap5!");
	return;
    }

    layout_adj_changed(widget, bbw);

    int xoffset, yoffset;
    GtkAdjustment *xadj, *yadj;

    xadj = gtk_layout_get_hadjustment (GTK_LAYOUT(widget));
    yadj = gtk_layout_get_vadjustment (GTK_LAYOUT(widget));

    xoffset = (int) GTK_ADJUSTMENT(xadj)->value;
    yoffset = (int) GTK_ADJUSTMENT(yadj)->value;

    gdk_draw_pixmap(GTK_LAYOUT (widget)->bin_window,
			    bbw->window->style->white_gc,
		    bbw->layout_pixmap,
		    event->area.x+xoffset, event->area.y+yoffset,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);

    gtk_widget_queue_draw(widget);
}

void Breadboard_Window::Build(void)
{

  GtkWidget *hpaned1;
  GtkWidget *vbox9;
  GtkWidget *vbox13;
  GtkWidget *scrolledwindow4;
  GtkWidget *viewport9;
  GtkWidget *tree1;
  GtkWidget *hbox12;
  GtkWidget *hbox15;
  GtkWidget *button5;
  GtkWidget *button6;
  GtkWidget *button7;
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
  GtkWidget *remove_node_button;
  GtkWidget *remove_stimulus_button;

  GtkWidget *vbox14;
  //  GtkWidget *scrolledwindow6;
  //  GtkWidget *viewport10;
  GtkWidget *hbox13;

  GtkWidget *vbox10;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport6;
  GtkWidget *hbox9;
  GtkWidget *hbox14;
  GtkWidget *remove_module_button;
  GtkWidget *save_stc_button;
  GtkWidget *scrolledwindow5;

  GtkWidget *button25, *button26;

  GdkColormap *colormap = gdk_colormap_get_system();

  gdk_color_parse("red",&high_output_color);
  gdk_color_parse("darkgreen",&low_output_color);
  g_assert(gdk_color_parse("black",&black_color)!=FALSE);
  
  gdk_colormap_alloc_color(colormap, &high_output_color,FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &low_output_color,FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &black_color,FALSE,TRUE);



  //printf("bb %s:%d, w=%d, h=%d\n",__FUNCTION__,__LINE__,width,height);


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window), "window", window);
  gtk_window_set_title (GTK_WINDOW (window), "Breadboard [Currently in development]");

  hpaned1 = gtk_hpaned_new ();
  gtk_widget_ref (hpaned1);
  gtk_object_set_data_full (GTK_OBJECT (window), "hpaned1", hpaned1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hpaned1);
  gtk_container_add (GTK_CONTAINER (window), hpaned1);
  gtk_paned_set_position (GTK_PANED (hpaned1), 196);

  vbox9 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox9);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox9", vbox9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox9);
  gtk_paned_pack1 (GTK_PANED (hpaned1), vbox9, FALSE, TRUE);

  vbox13 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox13);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox13", vbox13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox13);
  gtk_box_pack_start (GTK_BOX (vbox9), vbox13, TRUE, TRUE, 2);

  scrolledwindow4 = gtk_scrolled_window_new (0, 0);
  gtk_widget_ref (scrolledwindow4);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow4", scrolledwindow4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow4);
  gtk_box_pack_start (GTK_BOX (vbox13), scrolledwindow4, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

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

  hbox12 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox12);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox12", hbox12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox12);
  gtk_box_pack_start (GTK_BOX (vbox13), hbox12, FALSE, FALSE, 0);

  button5 = gtk_button_new_with_label ("Add node");
  gtk_widget_ref (button5);
  gtk_object_set_data_full (GTK_OBJECT (window), "button5", button5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button5);
  gtk_box_pack_start (GTK_BOX (hbox12), button5, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button5),
		     "clicked",
		     (GtkSignalFunc) add_new_snode,
		     this);

  button6 = gtk_button_new_with_label ("Add module");
  gtk_widget_ref (button6);
  gtk_object_set_data_full (GTK_OBJECT (window), "button6", button6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button6);
  gtk_box_pack_start (GTK_BOX (hbox12), button6, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button6),
		     "clicked",
		     (GtkSignalFunc) add_module,
		     this);

  button7 = gtk_button_new_with_label ("Add library");
  gtk_widget_ref (button7);
  gtk_object_set_data_full (GTK_OBJECT (window), "button7", button7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button7);
  gtk_box_pack_start (GTK_BOX (hbox12), button7, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button7),
		     "clicked",
		     (GtkSignalFunc) add_library,
		     this);

  hbox15 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox15);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox15", hbox15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox15);
  gtk_box_pack_start (GTK_BOX (vbox13), hbox15, FALSE, FALSE, 0);

  button25 = gtk_button_new_with_label ("Trace all");
  gtk_widget_ref (button25);
  gtk_object_set_data_full (GTK_OBJECT (window), "button25", button25,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button25);
  gtk_box_pack_start (GTK_BOX (hbox15), button25, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button25),
		     "clicked",
		     (GtkSignalFunc) trace_all,
		     this);

  button26 = gtk_button_new_with_label ("Clear traces ");
  gtk_widget_ref (button26);
  gtk_object_set_data_full (GTK_OBJECT (window), "button26", button26,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button26);
  gtk_box_pack_start (GTK_BOX (hbox15), button26, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button26),
		     "clicked",
		     (GtkSignalFunc) clear_traces,
		     this);





  pic_frame = gtk_frame_new ("PIC settings");
  gtk_widget_ref (pic_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_frame", pic_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
  //  gtk_widget_show (pic_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), pic_frame, TRUE, TRUE, 0);

  vbox12 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox12);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox12", vbox12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox12);
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

  hbox11 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox11);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox11", hbox11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox11);
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

  hbox10 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox10);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox10", hbox10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox10);
  gtk_box_pack_start (GTK_BOX (vbox11), hbox10, FALSE, FALSE, 0);

  remove_stimulus_button = gtk_button_new_with_label ("Remove stimulus");
  gtk_widget_ref (remove_stimulus_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "remove_stimulus_button", remove_stimulus_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (remove_stimulus_button);
  gtk_box_pack_start (GTK_BOX (hbox10), remove_stimulus_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(remove_stimulus_button),
		     "clicked",
		     (GtkSignalFunc) remove_node_stimulus,
		     this);

  remove_node_button = gtk_button_new_with_label ("Remove node");
  gtk_widget_ref (remove_node_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "remove_node_button", remove_node_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (remove_node_button);
  gtk_box_pack_start (GTK_BOX (hbox10), remove_node_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(remove_node_button),
		     "clicked",
		     (GtkSignalFunc) remove_node,
		     this);

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
  /*
    scrolledwindow6 = gtk_scrolled_window_new (0, 0);
    gtk_widget_ref (scrolledwindow6);
    gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow6", scrolledwindow6,
    (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (scrolledwindow6);
    gtk_box_pack_start (GTK_BOX (vbox14), scrolledwindow6, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow6), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    viewport10 = gtk_viewport_new (0, 0);
    gtk_widget_ref (viewport10);
    gtk_object_set_data_full (GTK_OBJECT (window), "viewport10", viewport10,
    (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (viewport10);
    gtk_container_add (GTK_CONTAINER (scrolledwindow6), viewport10);

    bbw->stimulus_settings_clist = gtk_clist_new (1);
    gtk_widget_ref (bbw->stimulus_settings_clist);
    gtk_object_set_data_full (GTK_OBJECT (window), "bbw->stimulus_settings_clist", bbw->stimulus_settings_clist,
    (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (bbw->stimulus_settings_clist);
    gtk_container_add (GTK_CONTAINER (viewport10), bbw->stimulus_settings_clist);
  */
  hbox13 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox13);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox13", hbox13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox13);
  gtk_box_pack_start (GTK_BOX (vbox14), hbox13, FALSE, FALSE, 0);

  stimulus_add_node_button = gtk_button_new_with_label ("Connect stimulus to node");
  gtk_widget_ref (stimulus_add_node_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "stimulus_add_node_button", stimulus_add_node_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (stimulus_add_node_button);
  gtk_box_pack_start (GTK_BOX (hbox13), stimulus_add_node_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(stimulus_add_node_button),
		     "clicked",
		     (GtkSignalFunc) stimulus_add_node,
		     this);





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

  hbox9 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox9);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox9", hbox9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox9);
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

  attribute_button = gtk_button_new_with_label ("Set");
  gtk_widget_ref (attribute_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "attribute_button", attribute_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (attribute_button);
  gtk_box_pack_start (GTK_BOX (hbox9), attribute_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(attribute_button),
		  "clicked",
		  (GtkSignalFunc) settings_set_cb,
		  this);


  hbox14 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox14);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox14", hbox14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox14);
  gtk_box_pack_start (GTK_BOX (vbox10), hbox14, FALSE, FALSE, 0);

  remove_module_button = gtk_button_new_with_label ("Remove module");
  gtk_widget_ref (remove_module_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "remove_module_button", remove_module_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (remove_module_button);
  gtk_box_pack_start (GTK_BOX (hbox14), remove_module_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(remove_module_button),
		     "clicked",
		     (GtkSignalFunc) remove_module,
		     this);

  save_stc_button = gtk_button_new_with_label ("Save configuration  ...");
  gtk_widget_ref (save_stc_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "save_stc_button", save_stc_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (save_stc_button);
  gtk_box_pack_start (GTK_BOX (vbox9), save_stc_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(save_stc_button),
		     "clicked",
		     (GtkSignalFunc) save_stc,
		     this);

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

  is_built=1;
  enabled = 1;

  NewProcessor(gp);

  UpdateMenuItem();

  draw_nodes(this);

  if(enabled)
    gtk_widget_show(window);

  //printf("bb %s:%d, w=%d, h=%d\n",__FUNCTION__,__LINE__,width,height);

}


Breadboard_Window::Breadboard_Window(GUI_Processor *_gp)
{
  menu = "<main>/Windows/Breadboard";

  set_name("pinout");
  wc = WC_misc;
  wt = WT_breadboard_window;
  window = 0;
  is_built = 0;
  enabled = 0;

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
