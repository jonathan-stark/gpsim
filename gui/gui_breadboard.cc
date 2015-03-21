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

#include "../config.h"
#ifdef HAVE_GUI


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <cmath>
#include <cassert>


#include <iostream>
#include <iomanip>
#include <string>
#include <typeinfo>

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

#define LABELPAD 4 // increase this so wide lines doesn't clutter labels

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
    Breadboard_Window *bbw  = static_cast<Breadboard_Window *>(parent_window);

    bbw->Update();

  }
  void Remove() { }
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
        new_point = new path;
        new_point->p = p;
        new_point->next = *pat;
        if((*pat) != 0)
        {
            dir = calculate_route_direction(p, (*pat)->p);
            if ((*pat)->dir == R_NONE)
                (*pat)->dir = dir;
        }
        new_point->dir = dir;
        *pat = new_point;
//    }
}

// Free all memory in pat
static void clear_path(path **pat)
{
  path *next;

  if (*pat==0)
    return;

  path *current_path = *pat;

  *pat = 0;

  while(current_path!=0) {
    next = current_path->next;

    delete current_path;

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
/* bug ahead, FIXME:    if(*pat!=0 && (*pat)->next!=0)
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
//      if(depth==0)
//      {
//            printf("Successful trace with %ld steps\n",calls);
//      }
        return TRUE;
    }

//    if(depth==0)
//    {
//      printf("Unsuccessful trace with %ld steps\n",calls);
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
            //                  else
            //                          assert(0);
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
        if(p->IsBuilt()) {
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
        for(i=1;i<=p->module()->get_pin_count();i++)
        {

            GList *e;

            e = g_list_nth(p->pins(), i-1);

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

static std::vector<path *> nodepath_list;

// Draw nodes in nodepath_list to layout_pixmap
static void clear_nodes(Breadboard_Window *bbw)
{
  std::vector<path *>::iterator iter = nodepath_list.begin();
  for ( ; iter != nodepath_list.end(); ++iter) {
    path *nodepath = *iter;

    clear_path(&nodepath);
  }
  nodepath_list.clear();
}

static void layout_adj_changed(GtkWidget *widget, Breadboard_Window *bbw);

// Draw node in nodepath_list to layout_pixmap
static void draw_nodes(Breadboard_Window *bbw)
{
  gdk_draw_rectangle(bbw->layout_pixmap,
    gtk_widget_get_style(bbw->window)->bg_gc[gtk_widget_get_state(bbw->window)],
    TRUE,
    0, 0, LAYOUTSIZE_X, LAYOUTSIZE_Y);

  std::vector<path *>::iterator iter = nodepath_list.begin();

  for ( ; iter != nodepath_list.end(); ++iter) {
    int last_x, last_y;
    path *current_path;

    path *nodepath = *iter;

    current_path = nodepath;

    last_x = current_path->p.x*ROUTE_RES;
    last_y = current_path->p.y*ROUTE_RES;

    current_path=current_path->next;

    gdk_gc_set_foreground(bbw->pinline_gc,&black_color);

    while(current_path!=0) {
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
  }

  layout_adj_changed(0,bbw);
}



// Here we fill board_matrix with module packages, so that trace_two_points
// know not to trace over them.
static void update_board_matrix(Breadboard_Window *bbw)
{
    int x,y, width, height;
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
    std::vector<GuiModule *>::iterator mi = bbw->modules.begin();
    for ( ; mi != bbw->modules.end(); ++mi) {

      GuiModule *p = *mi;

      if(p && p->IsBuilt()) {
        x=p->x();
        y=p->y();
        width=p->width();
        height=p->height();

        for(y = p->y() - ROUTE_RES;
            y < p->y() + height + ROUTE_RES && y/ROUTE_RES < YSIZE;
            y += ROUTE_RES)
        {
          for(x = p->x();
              x < p->x() + width && x/ROUTE_RES<XSIZE;
              x += ROUTE_RES)
                board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
        }

        // Draw barriers around pins so the tracker can only get in
        // straigt to the pin and not from the side.
        for (i = 1; i <= p->pin_count(); i++) {
          std::vector<GuiPin *> *e = p->pins();

          GuiPin *gp = (*e)[i - 1];

          switch(gp->orientation)
            {
            case LEFT:
              y = gp->y() - gp->height() / 2;
              for(x = gp->x() -  PINLENGTH;
                  x < gp->x() + gp->width();
                  x += ROUTE_RES)
                board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);

              y = gp->y() + gp->height() / 2;
              for(x = gp->x() -  PINLENGTH;
                  x < gp->x() + gp->width();
                  x += ROUTE_RES)
                board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
              break;

            case RIGHT:
              y = gp->y() - gp->height() / 2;
              for(x = gp->x() - PINLENGTH;
                  x < gp->x() + gp->width();
                  x += ROUTE_RES)
                board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
              y = gp->y() + gp->height() / 2;
              for(x = gp->x() - PINLENGTH;
                  x < gp->x() + gp->width();
                  x += ROUTE_RES)
                board_matrix[x/ROUTE_RES][y/ROUTE_RES]=(HMASK|VMASK);
              break;
            default:
              assert(0);
            }
        }
      }
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

static path *shortest_path[100][100];
static int pathlen[100][100];

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

        dest = new path;
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
  Breadboard_Window *bbw;
  int didnt_work=0;

  point start = {-1, -1}, end;

  bbw = gn->bbw;

  stimulus *stimulus = gn->node->stimuli;

  std::vector<GuiPin *> pinlist;
    // Make a glist of all gui_pins in the node
  while (stimulus) {
    GuiPin *p = find_gui_pin(bbw, stimulus);

    if (p) {
      pinlist.push_back(p);
    }

    stimulus = stimulus->next;
  }

    // Allocate an array of shortest_paths, indexed with 2x glist position.
//FIXME   shortest_path = (path***) malloc(nr_of_nodes*nr_of_nodes*sizeof(path*));

  int *permutations = new int[pinlist.size()];
  int *shortest_permutation = new int[pinlist.size()];
  for (size_t i = 0; i < pinlist.size(); i++)
    permutations[i] = i;

    // Trace between all stimulus, and store the distances in the array.
  for (size_t i = 0; i < pinlist.size(); i++) {
    GuiPin *pi = pinlist[i];

    for (size_t j = i + 1; j < pinlist.size(); j++) {
      GuiPin *pj = pinlist[j];

      start.x = pi->x() / ROUTE_RES;
      start.y = pi->y() / ROUTE_RES;

      end.x = pj->x() / ROUTE_RES;
      end.y = pj->y() / ROUTE_RES;

//          printf("Tracing from %d,%d to %d,%d\n",start.x,start.y,end.x,end.y);
      maxdepth = abs(start.x - end.x) + abs(start.y - end.y);
      maxdepth = maxdepth * 2 + 100; // Twice the distance, and 5 turns
//          printf("Trying maxdepth %d\n",maxdepth);
      trace_two_points(&shortest_path[i][j], start, end,0,R_UP);
      if (shortest_path[i][j] == 0) {
        printf("\n### Couldn't trace from pin %s to pin %s!\n",
          pi->getIOpin()->name().c_str(),
          pj->getIOpin()->name().c_str());
        didnt_work=1;
      }
      pathlen[i][j] = maxdepth;

      pathlen[j][i] = maxdepth;
      shortest_path[j][i] = shortest_path[i][j];
    }
  }

  if (didnt_work) {
    printf("\n###### Couldn't trace node %s!\n",gn->node->name().c_str());
    for (size_t i = 0; i < pinlist.size(); i++)
      for (size_t j = i + 1; j < pinlist.size(); j++)
        clear_path(&shortest_path[i][j]);
    delete[] permutations;
    delete[] shortest_permutation;
    return;
  }

    // Find the combination that produces the shortest node.
  int minlen = 100000;
  do {
    int sum=0;

//      printf("%d ",permutations[0]);
    for (size_t i = 0; i < pinlist.size() - 1; i++) {
//          printf("%d ",permutations[i+1]);
      sum += pathlen[permutations[i]][permutations[i+1]];
    }
//      printf("length %d\n",sum);

    if (sum < minlen) {
      minlen = sum;
      for (size_t i = 0; i < pinlist.size(); i++) {
        shortest_permutation[i] = permutations[i];
      }
    }
        // Fixme, I'd rather use next_combination().
  } while ( next_permutation( permutations, permutations + pinlist.size()));

//    printf(" : Length %d\n", minlen);
//    for(i=0;i<nr_of_nodes;i++)
//    {
//      printf("%d ",shortest_permutation[i]);
//    }
    //puts("");

  path *nodepath = 0;
  for (size_t i = 0; i < pinlist.size() - 1; i++) {
    path_copy_and_cat(&nodepath,&shortest_path[shortest_permutation[i]][shortest_permutation[i+1]]);
  }

  for (size_t i = 0; i < pinlist.size(); i++)
    for (size_t j = i + 1; j < pinlist.size(); j++)
      clear_path(&shortest_path[i][j]);
  delete[] permutations;
  delete[] shortest_permutation;

  if (nodepath!=0) {
//          compress_path(&nodepath);

    add_path_to_matrix(nodepath);

    nodepath_list.push_back(nodepath);
  }
}



GuiPin *find_gui_pin(Breadboard_Window *bbw, stimulus *pin)
{
  std::vector<GuiModule *>::iterator iter = bbw->modules.begin();

  for ( ; iter != bbw->modules.end(); ++iter) {
    GuiModule *m = *iter;

    for (int i = 1; i <= m->module()->get_pin_count(); ++i) {

      stimulus *p;

      p = m->module()->get_pin(i);

      if (p == pin) {
        return (*m->pins())[i - 1];
      }
    }
  }

  return 0;
}

static void treeselect_stimulus(GtkItem *item, GuiPin *pin)
{

    char text[STRING_SIZE];
    char string[STRING_SIZE];

    const char *pText = "Not connected";
    const char *pString = "Stimulus";
    if(!pin)
      return;

    gtk_widget_show(pin->bbw()->stimulus_frame);
    gtk_widget_hide(pin->bbw()->node_frame);
    gtk_widget_hide(pin->bbw()->module_frame);

    if(pin->getIOpin()) {
      g_snprintf(string, sizeof(string), "Stimulus %s", pin->getIOpin()->name().c_str());
      pString = string;

      if(pin->getSnode()!=0)
        g_snprintf(text, sizeof(text), "Connected to node %s", pin->getSnode()->name().c_str());
      else
        g_snprintf(text, sizeof(text), "Not connected");
      pText = text;
    }

    gtk_frame_set_label(GTK_FRAME(pin->bbw()->stimulus_frame),pString);
    gtk_label_set_text(GTK_LABEL(pin->bbw()->stimulus_settings_label), pText);

    pin->bbw()->selected_pin = pin;
}

static void treeselect_node(GtkItem *item, struct gui_node *gui_node)
{
    stimulus *stimulus;
    GtkListStore *list_store;

//    printf("treeselect_node %p\n",gui_node);

    if(gui_node->node!=0)
    {
      char str[STRING_SIZE];
        g_snprintf(str, sizeof(str), "Node %s", gui_node->node->name().c_str());
        gtk_frame_set_label(GTK_FRAME(gui_node->bbw->node_frame),str);

        gtk_widget_show(gui_node->bbw->node_frame);
    }
    else
    {
        gtk_widget_hide(gui_node->bbw->node_frame);
    }
    gtk_widget_hide(gui_node->bbw->stimulus_frame);
    gtk_widget_hide(gui_node->bbw->module_frame);

    // Clear node_clist
    g_object_get(gui_node->bbw->node_clist, "model", &list_store, NULL);
    gtk_list_store_clear(list_store);

    if(gui_node->node!=0)
    {
        // Add to node_clist
        stimulus = gui_node->node->stimuli;

        while(stimulus!=0)
        {
            GtkTreeIter iter;

            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter, 0, stimulus->name().c_str(), 1, stimulus, -1);

            stimulus = stimulus->next;
        }
    }

    gui_node->bbw->selected_node = gui_node;
}

static void treeselect_cb (GtkTreeSelection *selection,
                           gpointer user_data)
{
    GtkTreeModel *model;
    GtkTreeIter first_iter, iter;
    GtkTreePath *path;
    char *spath;
    
    gtk_tree_selection_get_selected (selection, &model, &iter);
    
    if (!iter.stamp)
        return;
    
    gtk_tree_model_get_iter_first (model, &first_iter);
    path = gtk_tree_model_get_path (model, &iter);
    
    spath = gtk_tree_path_to_string (path);
    
    if (spath[0] == '0')
    {
        struct gui_node *gn;
        gtk_tree_model_get (model, &iter, 1, &gn, -1);
        if (strlen (spath) > 1)
            treeselect_node (NULL, gn);
        else
            gtk_widget_hide (gn->bbw->node_frame);
    }
    else
    {
        if (strlen (spath) > 1)
        {
            GuiPin *gp;
            gtk_tree_model_get (model, &iter, 1, &gp, -1);
            treeselect_stimulus (NULL, gp);
        }
        else
        {
            GuiModule *module;
            gtk_tree_model_get (model, &iter, 1, &module, -1);
            treeselect_module (NULL, module);
        }
    }
}

static const char *mod_name;

static void settings_clist_cb(GtkTreeSelection *selection,
                       Breadboard_Window *bbw)
{

        // Save the Attribute*
        Value *attr;
        char str[256];
        char val[256];
        GtkTreeIter iter;
        GtkTreeModel *model;
        
        gtk_tree_selection_get_selected (selection, &model, &iter);
        if (!iter.stamp) // check if iter is valid
            return;

        gtk_tree_model_get(model, &iter, 1, &attr, -1);
        attr->get(val, sizeof(val));

        if (mod_name)
        {
            g_snprintf(str, sizeof(str), "%s.%s = %s", mod_name, attr->name().c_str(), val);
        }
        else
        {
            g_snprintf(str, sizeof(str), "%s = %s", attr->name().c_str(), val);
        }
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
        sscanf(entry_string, "%255s = %255s", attribute_name, attribute_newval);

        printf("change attribute \"%s\" to \"%s\"\n",attribute_name, attribute_newval);

        Value *attr;

        // Change the Attribute
        //attr = bbw->selected_module->module->get_attribute(attribute_name);
        attr = globalSymbolTable().findValue(attribute_name);
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

//static Breadboard_Window *lpBW;
static GtkWidget *attribute_clist;

static void clistOneAttribute(const SymbolEntry_t &sym)
{

  Value *pVal = dynamic_cast<Value *>(sym.second);
  if (attribute_clist && pVal) {
      // read attributes and add to clist
      char attribute_value[STRING_SIZE];
      char attribute_string[STRING_SIZE];
      GtkListStore *list_store;
      GtkTreeIter iter;

      // Filter out non-attributes
      if (  !strstr(typeid(*pVal).name(), "Attribute") )
        return;

      pVal->get(attribute_value, sizeof(attribute_value));

      g_snprintf(attribute_string, sizeof(attribute_string), "%s = %s",
                pVal->name().c_str(), attribute_value);

      g_object_get(attribute_clist, "model", &list_store, NULL);
      gtk_list_store_append(list_store, &iter);
      gtk_list_store_set(list_store, &iter,
                         0, attribute_string,
                         1, (gpointer) pVal, -1);
  }
}

static void buildCLISTAttribute(const SymbolTableEntry_t &st)
{
  if (st.first == mod_name) {
    if (verbose) cout << " gui Module Attribute Window: " << st.first << endl;
    (st.second)->ForEachSymbolTable(clistOneAttribute);
  }
}
static void UpdateModuleFrame(GuiModule *p, Breadboard_Window *bbw)
{
  char buffer[STRING_SIZE];

  g_snprintf(buffer, sizeof(buffer), "%s settings", p->module()->name().c_str());
  gtk_frame_set_label(GTK_FRAME(p->bbw()->module_frame),buffer);

  if (!gtk_widget_get_visible(p->bbw()->attribute_clist))
    return;

  // clear clist
  gtk_list_store_clear((GtkListStore*)
        gtk_tree_view_get_model((GtkTreeView*)p->bbw()->attribute_clist));

  attribute_clist = p->bbw()->attribute_clist;
  mod_name = p->module()->name().c_str();
  globalSymbolTable().ForEachModule(buildCLISTAttribute);
  attribute_clist = NULL;

  gtk_entry_set_text(GTK_ENTRY(p->bbw()->attribute_entry), "");

}

static void treeselect_module(GtkItem *item, GuiModule *p)
{
  if (p) {

    gtk_widget_hide(p->bbw()->stimulus_frame);
    gtk_widget_hide(p->bbw()->node_frame);

    gtk_widget_show(p->bbw()->module_frame);

    UpdateModuleFrame(p, p->bbw());


    p->bbw()->selected_module = p;
  }
}

void GuiModule::SetPosition(int nx, int ny)
{
  nx=nx-nx%pinspacing;
  ny=ny-ny%pinspacing;

  if(nx != m_x || ny != m_y) {
    m_x=nx;
    m_y=ny;

    Value *xpos = dynamic_cast<Value *> (m_module->findSymbol("xpos"));
    Value *ypos = dynamic_cast<Value *> (m_module->findSymbol("ypos"));
    if(xpos)
      xpos->set(m_x);
    if(ypos)
      ypos->set(m_y);

    // Position module_widget
    if (m_pinLabel_widget)
      gtk_layout_move(GTK_LAYOUT(m_bbw->layout), m_pinLabel_widget,
                      m_x,
                      m_y);
    if (m_module_widget)
      gtk_layout_move(GTK_LAYOUT(m_bbw->layout), m_module_widget,
                      m_x + m_module_x,
                      m_y + m_module_y);

    // Position module_name
    gtk_layout_move(GTK_LAYOUT(m_bbw->layout), m_name_widget, m_x, m_y-20);

    // Position pins
    std::vector<GuiPin *>::iterator piniter = m_pins.begin();
    for ( ; piniter != m_pins.end(); ++piniter) {

      GuiPin *pin = *piniter;

      if(pin->orientation==RIGHT)
        pin->SetPosition(m_x + pin->module_x()+PINLENGTH,m_y + pin->module_y() + pin->height()/2);
      else
        pin->SetPosition(m_x + pin->module_x(), m_y + pin->module_y() + pin->height()/2);

      gtk_layout_move(GTK_LAYOUT(m_bbw->layout),
                      pin->m_pinDrawingArea,m_x+pin->module_x(),m_y+pin->module_y());
    }
  }
}

void GuiModule::GetPosition(int &x, int &y)
{
  Value *xpos = dynamic_cast<Value *> (m_module->findSymbol("xpos"));
  Value *ypos = dynamic_cast<Value *> (m_module->findSymbol("ypos"));

  x = xpos ? (gint64)*xpos : m_x;
  y = ypos ? (gint64)*ypos : m_y;

}

/* FIXME: calculate distance to the edges instead of the corners. */
double GuiModule::Distance(int px, int py)
{
  double distance;
  double min_distance=100000000;

  // Upper left
  distance=sqrt((double)abs(m_x-px)*abs(m_x-px) +
                abs(m_y-py)*abs(m_y-py));
  if(distance<min_distance)
    min_distance=distance;

  // Upper right
  distance=sqrt((double)abs(m_x+m_width-px)*abs(m_x + m_width - px) +
                abs(m_y-py)*abs(m_y-py));
  if(distance<min_distance)
    min_distance=distance;

  // Lower left
  distance=sqrt((double)abs(m_x-px)*abs(m_x-px) +
                abs(m_y + m_height-py)*abs(m_y+m_height-py));
  if(distance<min_distance)
    min_distance=distance;

  // Lower right
  distance=sqrt((double)abs(m_x+m_width-px)*abs(m_x+m_width-px) +
                abs(m_y+m_height-py)*abs(m_y+m_height-py));
  if(distance<min_distance)
    min_distance=distance;

  return min_distance;
}

static GuiModule *find_closest_module(Breadboard_Window *bbw, int x, int y)
{
  GuiModule *closest = 0;
  double min_distance = 1000000;

  std::vector<GuiModule *>::iterator mi = bbw->modules.begin();

  for ( ; mi != bbw->modules.end(); ++mi) {

    GuiModule *p = *mi;
    double distance = p->Distance(x,y);
    if (distance < min_distance) {
      closest = p;
      min_distance = distance;
    }
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
    gdk_pointer_grab(gtk_widget_get_window(p->bbw()->layout),
                     TRUE,
                     (GdkEventMask)(GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK),
                     gtk_widget_get_window(p->bbw()->layout),
                     0,
                     GDK_CURRENT_TIME);

    treeselect_module(0,dragged_module);
    dragging = 1;
    clear_nodes(p->bbw());
    draw_nodes(p->bbw());
    gtk_widget_set_app_paintable(p->bbw()->layout, FALSE);
}

static void pointer_cb(GtkWidget *w,
                       GdkEventButton *event,
                       Breadboard_Window *bbw)
{
    static int x,y;

    x = (int) (event->x);
    y = (int) (event->y);

    switch(event->type)
    {
    case GDK_MOTION_NOTIFY:
        if(dragging && 0 != dragged_module)
        {
            dragged_module->SetPosition(x+pinspacing, y+pinspacing);
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
              gdk_pointer_grab(gtk_widget_get_window(w),
                              TRUE,
                              (GdkEventMask)(GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK),
                              gtk_widget_get_window(w),
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
          g_object_get_data((GObject*)p->bbw()->tree,
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
          g_object_get_data((GObject*)p->bbw()->tree,
                              p->getSnode()->name().c_str());

	if (gn)
	{
            trace_node(gn);
            draw_nodes(gn->bbw);
	}
      }
      return 1;
    }

    return 0;
}



// get_string
static void a_cb(GtkWidget *w, GtkDialog *dialog)
{
    gtk_dialog_response (dialog, GTK_RESPONSE_ACCEPT);
}
// used for reading a value from user when break on value is requested
static std::string gui_get_string(const char *prompt, const char *initial_text)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons("enter value",
    NULL,
    GTK_DIALOG_MODAL,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
    NULL);

  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
  GtkWidget *label = gtk_label_new("Enter string:");
  GtkWidget *label2 = gtk_label_new(prompt);
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), initial_text);
  gtk_widget_grab_focus(entry);
  g_signal_connect(entry, "activate", G_CALLBACK(a_cb), (gpointer)dialog);

  gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(content_area), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label2, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);

  gtk_widget_show_all(dialog);
  gint retval = gtk_dialog_run(GTK_DIALOG(dialog));

  std::string string;
  if (retval == GTK_RESPONSE_ACCEPT)
    string = gtk_entry_get_text(GTK_ENTRY(entry));

  gtk_widget_destroy(dialog);

  return string;
}

static void add_new_snode(GtkWidget *button, Breadboard_Window *bbw)
{
  std::string node_name = gui_get_string("Node name", "");

  if(!node_name.empty())
    new Stimulus_Node(node_name.c_str());
}



////////////////////////////////////////////////////////////////////
static void select_node_ok_cb(GtkTreeView *tree_view,
                  GtkTreePath *path,
                  GtkTreeViewColumn *column,
                  GtkDialog *dialog)
{
    Stimulus_Node* snode;
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    model = gtk_tree_view_get_model (tree_view);
        
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, 1, &snode, -1);
    
    g_object_set_data ((GObject*)dialog, "snode", snode);
    
    gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
}

static void select_module_ok_cb(GtkTreeView *tree_view,
                  GtkTreePath *path,
                  GtkTreeViewColumn *column,
                  GtkDialog *dialog)
{
    gtk_dialog_response (dialog, GTK_RESPONSE_ACCEPT);
}

static void copy_tree_to_clist(GtkTreeModel *model, GtkListStore *list_store)
{
    struct gui_node *gn;
    GtkTreeIter node_iter, iter, new_iter;

    gtk_tree_model_get_iter_first (model, &node_iter);
    gtk_tree_model_iter_n_children (model, &node_iter);
    gtk_tree_model_iter_children (model, &iter, &node_iter);
    
    do
    {
        gtk_tree_model_get (model, &iter, 1, &gn, -1);
   
        gtk_list_store_append(list_store, &new_iter);
        gtk_list_store_set(list_store, &new_iter,
          0, gn->node->name().c_str(), 1, (gpointer) gn->node, -1);
        
        if (!iter.stamp)
            break;
    } while (gtk_tree_model_iter_next (model, &iter));
}

static Stimulus_Node *select_node_dialog(Breadboard_Window *bbw)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons("Select node to connect to",
    GTK_WINDOW(bbw->window),
    GTK_DIALOG_MODAL,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    NULL);

  GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *scrolledwindow = gtk_scrolled_window_new (0, 0);
  gtk_box_pack_start(GTK_BOX(vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolledwindow),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkListStore *list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  GtkWidget *node_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
  g_object_unref(list_store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(node_list), FALSE);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(node_list),
    0, "Nodes", renderer, "text", 0, NULL);
  gtk_container_add(GTK_CONTAINER(scrolledwindow), node_list);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 220, 400);

  copy_tree_to_clist(
    gtk_tree_view_get_model((GtkTreeView*) bbw->tree),
    list_store);

  g_signal_connect(node_list, "row-activated",
    G_CALLBACK(select_node_ok_cb), dialog);

  gtk_widget_show_all(dialog);
  int resp = gtk_dialog_run((GtkDialog*)dialog);

  Stimulus_Node *snode = static_cast<Stimulus_Node*>(g_object_get_data((GObject*) dialog, "snode"));

  gtk_widget_destroy(dialog);

  if (resp == GTK_RESPONSE_ACCEPT)
    return snode;

  return 0;
}

static std::string select_module_dialog(Breadboard_Window *bbw)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons("Select module to load",
    GTK_WINDOW(bbw->window), GTK_DIALOG_MODAL,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    NULL);

  GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  GtkWidget *scrolledwindow = gtk_scrolled_window_new(0, 0);
  gtk_box_pack_start(GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  const gchar *module_clist_titles[] = {"Name1","Name2", "Library"};
 
#ifdef OLD_MODULE_LIBRARY
  int n_columns = 2;
  GtkListStore *list_store = gtk_list_store_new(n_columns + 1,
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  int width = 220;
#else
  int n_columns = 3;
  GtkListStore *list_store = gtk_list_store_new(n_columns + 1,
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  int width = 320;
#endif

  GtkWidget *module_clist = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
  g_object_unref(list_store);
  for (int col = 0; col < n_columns; ++col) {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
      module_clist_titles[col], renderer,
      "text", col, NULL);
    g_object_set(column,
      "resizable", TRUE,
      "sort-indicator", TRUE,
      "sort-column-id", col,
      NULL);
    gtk_tree_view_append_column((GtkTreeView*) module_clist, column);
  }

  gtk_container_add (GTK_CONTAINER(scrolledwindow), module_clist);

  g_signal_connect(module_clist, "row-activated",
    G_CALLBACK(select_module_ok_cb), dialog);

  gtk_window_set_default_size(GTK_WINDOW(dialog), width, 400);

#ifdef OLD_MODULE_LIBRARY
    ModuleLibrary::FileList::iterator  mi;
    ModuleLibrary::FileList::iterator  itFileListEnd(ModuleLibrary::GetFileList().end());
    // Add all modules
    for (mi = ModuleLibrary::GetFileList().begin();
         mi != itFileListEnd;
         ++mi)
    {

      ModuleLibrary::File *t = *mi;
      cout << t->name() << '\n';
      Module_Types * pFileTypes;
      if((pFileTypes = t->get_mod_list()))
      {
        // Loop through the list and display all of the modules.
        int i=0;

        while(pFileTypes[i].names[0])
        {
          char name[STRING_SIZE];
          char library[STRING_SIZE];
          char *text[2]={name, library};
          GtkTreeIter iter;

          g_strlcpy(name, pFileTypes[i].names[0], STRING_SIZE);
          g_strlcpy(library, t->name(), STRING_SIZE);

          gtk_list_store_append(list_store, &iter);
          gtk_list_store_set(list_store, &iter,
                             0, name,
                             1, library,
                             2, pFileTypes[i].names[1],
                             -1);
          i++;
        }
      }
    }
#else //OLD_MODULE_LIBRARY

    extern ModuleLibraries_t ModuleLibraries;



    for (
        ModuleLibraries_t::iterator mti = ModuleLibraries.begin();
        mti != ModuleLibraries.end();
        ++mti)
    {
          const char *text[3];
          
          Module_Types * (*get_mod_list)(void) = mti->second->mod_list();
          Module_Types *pLibModList = get_mod_list();

          text[2] = mti->second->user_name().c_str();
          
          if(pLibModList)
              for(Module_Types *pModTypes = pLibModList;  pModTypes->names[0]; pModTypes++) {
                GtkTreeIter iter;
                text[0] = pModTypes->names[0];
                text[1] = pModTypes->names[1];

                gtk_list_store_append(list_store, &iter);
                gtk_list_store_set(list_store, &iter,
                                   0, text[0],
                                   1, text[1],
                                   2, text[2],
                                   3, text[0],
                                   -1);
        }
    }
#endif

  gtk_widget_show_all(dialog);
  int resp = gtk_dialog_run((GtkDialog*)dialog);

  if (resp == GTK_RESPONSE_ACCEPT) {
    GtkTreeIter iter;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(module_clist));
    if (!gtk_tree_selection_get_selected(sel, NULL, &iter)) {
      gtk_widget_destroy(dialog);
      return "";
    }

    gchar *s;
    gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, 0, &s, -1);
    std::string str = s;
    g_free(s);
    gtk_widget_destroy(dialog);

    return str;
  }

  gtk_widget_destroy(dialog);

  return "";
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
  std::string library_name
    = gui_get_string("Module library name (e.g. libgpsim_modules)","");
#ifdef OLD_MODULE_LIBRARY
    if(!library_name.empty())
      ModuleLibrary::LoadFile(library_name.c_str());
#else
    if(!library_name.empty()) {
        ModuleLibrary::LoadFile(library_name);
    }
#endif
}

static void add_module(GtkWidget *button, Breadboard_Window *bbw)
{
  std::string module_type = select_module_dialog(bbw);

  if (!module_type.empty()) {
        std::string module_name = gui_get_string("Module name", module_type.c_str());
        grab_next_module = 1;
        if(!module_name.empty())
#ifdef OLD_MODULE_LIBRARY
          ModuleLibrary::NewObject(module_type.c_str(), module_name.c_str());
#else
        {
            if(!ModuleLibrary::InstantiateObject(module_type, module_name))
                fprintf(stderr, "Module load of %s %s failed\n",
                        module_type.c_str(), module_name.c_str());
        }
#endif
    }
}

static void remove_module(GtkWidget *button, Breadboard_Window *bbw)
{
    delete(bbw->selected_module->module());

    // FIXME the rest should be as callback from src


    // Remove pins
    std::vector<GuiPin *> *e = bbw->selected_module->pins();

    std::vector<GuiPin *>::iterator pin_iter = e->begin();
    for ( ; pin_iter != e->end(); ++pin_iter) {
        GuiPin *pin = *pin_iter;

        gtk_widget_destroy(GTK_WIDGET(pin->m_pinDrawingArea));
    }

    // Remove widget
    if (bbw->selected_module->module_widget())
        gtk_container_remove(GTK_CONTAINER(bbw->layout),
                         bbw->selected_module->module_widget());
    if (bbw->selected_module->pinLabel_widget())
        gtk_container_remove(GTK_CONTAINER(bbw->layout),
                         bbw->selected_module->pinLabel_widget());
    gtk_container_remove(GTK_CONTAINER(bbw->layout),
                         bbw->selected_module->name_widget());

    // Remove module from tree
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    
    selection = gtk_tree_view_get_selection ((GtkTreeView*) bbw->tree);
    gtk_tree_selection_get_selected (selection, &model, &iter);
    gtk_tree_store_set ((GtkTreeStore*) model, &iter, 1, NULL, -1);
    gtk_tree_store_remove ((GtkTreeStore*) model, &iter);
    
    // Remove from local list of modules
    std::vector<GuiModule *>::iterator mi =
      std::find(bbw->modules.begin(), bbw->modules.end(), bbw->selected_module);
    if (mi != bbw->modules.end())
      bbw->modules.erase(mi);

    gtk_widget_hide(bbw->module_frame);

    delete bbw->selected_module;

    bbw->selected_module=0;
}

static void remove_node(GtkWidget *button, Breadboard_Window *bbw)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    struct gui_node *gn;
    
    selection = gtk_tree_view_get_selection ((GtkTreeView*) bbw->tree);
    gtk_tree_selection_get_selected (selection, &model, &iter);
    gtk_tree_model_get (model, &iter, 1, &gn, -1);
    
    gtk_tree_store_remove ((GtkTreeStore*) model, &iter);
    
    g_object_set_data (G_OBJECT (bbw->tree), gn->node->name().c_str(), NULL);
    delete gn;
    
    gtk_widget_hide(bbw->node_frame);
    gtk_widget_hide(bbw->stimulus_frame);
    gtk_widget_hide(bbw->module_frame);
}

static void remove_node_stimulus(GtkWidget *button, Breadboard_Window *bbw)
{
    stimulus *s;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    selection = gtk_tree_view_get_selection ((GtkTreeView*) bbw->node_clist);
    gtk_tree_selection_get_selected (selection, &model, &iter);
    gtk_tree_model_get (model, &iter, 1, &s, -1);

    bbw->selected_node->node->detach_stimulus(s);

    gtk_list_store_remove ((GtkListStore*) model, &iter);
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

// Returns a file name which must be freed with g_free() or NULL
static char *gui_get_filename(const char *filename)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Log settings",
    NULL,
    GTK_FILE_CHOOSER_ACTION_SAVE,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
    NULL);

  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), filename);

  char *file = NULL;
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  }

  gtk_widget_destroy(dialog);

  return file;
}

static FILE *fo;

static void OneAttribute(const SymbolEntry_t &sym)
{
    Value *pVal = dynamic_cast<Value *>(sym.second);
    if (pVal && fo)
    {
          // read attributes and add to clist

        if (  strstr(typeid(*pVal).name(), "Attribute") )
        {
             char attribute_value[STRING_SIZE];

            pVal->get(attribute_value, sizeof(attribute_value));
            fprintf(fo, "%s.%s = %s\n", mod_name, pVal->name().c_str(),
                attribute_value);
        }
      }
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
static void save_stc(GtkWidget *button, Breadboard_Window *bbw)
{
    Module *m;

    char *filename = gui_get_filename("netlist.stc");
    if(!filename)
        return;
    if ((fo = fopen(filename, "w")) == NULL)
    {
        perror(filename);
        g_free(filename);
        return;
    }
    g_free(filename);

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

#ifdef OLD_MODULE_LIBRARY
    // Save module libraries
    fprintf(fo, "\n\n# Module libraries:\n");
    ModuleLibrary::FileList::iterator  mi;
          // Add all modules
    for (mi = ModuleLibrary::GetFileList().begin();
              mi != ModuleLibrary::GetFileList().end();
              ++mi)
    {

      ModuleLibrary::File *t = *mi;
      fprintf(fo, "module library %s\n",
      t->name());
    }
#else
    extern ModuleLibraries_t ModuleLibraries;


    for (
        ModuleLibraries_t::iterator mti = ModuleLibraries.begin();
        mti != ModuleLibraries.end();
        ++mti)
    {
          fprintf(fo, "module library %s\n", mti->second->user_name().c_str());
    }
#endif

    // Save modules
    fprintf(fo, "\n\n# Modules:\n");
    std::vector<GuiModule *>::iterator module_iterator = bbw->modules.begin();
    for ( ; module_iterator != bbw->modules.end(); ++module_iterator)
    {
      GuiModule *p = *module_iterator;

      m = p->module();

      SymbolTable_t *st = &m->getSymbolTable();

      Processor *cpu;
      cpu=dynamic_cast<Processor*>(m);
      if(cpu==0)
      { // Module, not a processor, so add the load command
        fprintf(fo, "module load %s %s\n",
                m->type(),
                m->name().c_str());
      }
/*
      else
      {
        fprintf(fo, "processor %s %s\n",
        m->type(),
        m->name().c_str());
      }
*/

      mod_name = m->name().c_str();
      st->ForEachSymbolTable(OneAttribute);
      fprintf(fo, "\n");
    }

    // Save nodes and connections
    fprintf(fo, "\n\n# Connections:\n");

    std::vector<Stimulus_Node *>::iterator list = bbw->nodes.begin();
    for ( ; list != bbw->nodes.end(); ++list)
    {
        Stimulus_Node *node = *list;
        stimulus *stimulus;

        fprintf(fo, "node %s\n",node->name().c_str());

        if(node->stimuli!=0)
        {
            fprintf(fo, "attach %s",node->name().c_str());

            for (stimulus = node->stimuli; stimulus; stimulus = stimulus->next)
                fprintf(fo, " %s",stimulus->name().c_str());
        }
        fprintf(fo, "\n\n");
    }


    fprintf(fo, "\n\n# End.\n");
    fclose(fo);
    fo = NULL;
    //text_dialog(filename);

}

static void clear_traces(GtkWidget *button, Breadboard_Window *bbw)
{

    update_board_matrix(bbw);
}

static void trace_all(GtkWidget *button, Breadboard_Window *bbw)
{
    struct gui_node *gn;
    GtkTreeModel *model;
    GtkTreeIter p_iter, c_iter;
    
    update_board_matrix(bbw);

    if ((model = gtk_tree_view_get_model ((GtkTreeView*) bbw->tree)) == NULL)
	return;
    if(!gtk_tree_model_get_iter_first (model, &p_iter))
	return;
    if (!gtk_tree_model_iter_children (model, &c_iter, &p_iter))
	return;
    
    do
    {
        gtk_tree_model_get (model, &c_iter, 1, &gn, -1);
        trace_node(gn);
    } while (gtk_tree_model_iter_next (model, &c_iter));    

    draw_nodes(bbw);

    if (verbose)
        puts("Trace all is done.");
}

//========================================================================
GuiBreadBoardObject::GuiBreadBoardObject(Breadboard_Window *bbw,int x, int y)
  : m_bbw(bbw), m_x(x), m_y(y), m_width(0), m_height(0), m_bIsBuilt(false)
{
}
GuiBreadBoardObject::~GuiBreadBoardObject()
{
}

void GuiBreadBoardObject::SetPosition(int x, int y)
{
  m_x = x;
  m_y = y;
}

void GuiPin::SetModulePosition(int x, int y)
{
  m_module_x = x;
  m_module_y = y;
}

//========================================================================
GuiPin::GuiPin(Breadboard_Window *_bbw,
               GuiModule *pModule,
		Package *_package,
               unsigned int pin_number)
  : GuiBreadBoardObject(_bbw, 0, 0),
    package(_package),
    m_pModule(pModule), m_module_x(0), m_module_y(0),
    m_label_x(0), m_label_y(0),
    m_pkgPinNumber(pin_number)
{

  IOPIN *iopin = getIOpin();
  m_width=pinspacing;
  m_height=pinspacing;

  orientation = LEFT;

  if(iopin) {
    value=iopin->getState();
    direction=iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;
    //orientation=_orientation;
    type=PIN_DIGITAL;
  }
  else
    {
      value=false;
      direction=PIN_INPUT;
      //orientation=_orientation;
      type=PIN_OTHER;
    }

  // Create widget
  m_pinDrawingArea = gtk_drawing_area_new();
  gtk_widget_set_events(m_pinDrawingArea,
                        gtk_widget_get_events(m_pinDrawingArea)|
                        GDK_BUTTON_PRESS_MASK);
  g_signal_connect(m_pinDrawingArea,
                     "button_press_event",
                     G_CALLBACK(button),
                     this);

  gtk_widget_set_size_request(m_pinDrawingArea, m_width, m_height);
  g_signal_connect(m_pinDrawingArea,
                     "expose_event",
                     G_CALLBACK(expose_pin),
                     this);

  gtk_widget_show(m_pinDrawingArea);
}

const char *GuiPin::pinName()
{

  IOPIN *iopin = getIOpin();

  return iopin ? iopin->name().c_str() : 0;
}
//------------------------------------------------------------------------
// GuiPin::update() - check the state of the iopin and make the gui match
//
void GuiPin::Update()
{
  IOPIN *iopin = getIOpin();

  if(iopin) {

    bool value=iopin->getState();
    eDirection dir=iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;

    if(value!=getState() || dir!=direction) {

      putState(value);
      direction=dir;

      Draw();
    }
  }
}

//------------------------------------------------------------------------
void GuiPin::toggleState()
{
  IOPIN *iopin = getIOpin();
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
    m_bbw->Update();
  }
}
//------------------------------------------------------------------------
// GuiPin::draw() - draw a single pin
//
//
void GuiPin::Draw()
{
  GdkWindow *gdk_win = gtk_widget_get_window(m_pinDrawingArea);
  if (!gdk_win)
    return;

  gdk_window_invalidate_rect(gdk_win, NULL, FALSE);
}

gboolean GuiPin::expose_pin(GtkWidget *widget, GdkEventExpose *event, GuiPin *p)
{
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));

  int pointx;
  int wingheight, wingx;
  int casex, endx;

  switch (p->orientation) {
  case LEFT:
    casex = p->m_width;
    endx = 0;
    break;
  default:
    casex = 0;
    endx = p->m_width;
    break;
  }

  int y = p->m_height / 2;

  if (p->type != PIN_OTHER)
    gdk_cairo_set_source_color(cr, p->getState() ? &high_output_color : &low_output_color);

  // Draw actual pin
  cairo_set_line_width(cr, PINLINEWIDTH);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

  cairo_move_to(cr, casex, y);
  cairo_line_to(cr, endx, y);
  cairo_stroke(cr);

  if (p->type == PIN_OTHER) {
    cairo_destroy(cr);
    return FALSE;
  }

  // Draw direction arrow
  wingheight = p->m_height / 3;

  if (casex > endx) {
    if (p->direction == PIN_OUTPUT) {
      pointx = endx + PINLENGTH /3;
      wingx = endx + (PINLENGTH * 2) / 3;
    } else {
      pointx = endx + (PINLENGTH * 2) / 3;
      wingx = endx + PINLENGTH / 3;
    }
  } else {
    if (p->direction == PIN_OUTPUT) {
      pointx = casex + (PINLENGTH * 2) / 3;
      wingx = casex + PINLENGTH / 3;
    } else {
      pointx = casex + PINLENGTH / 3;
      wingx = casex + (PINLENGTH * 2) / 3;
    }
  }

  // Draw an arrow poining at (endx,endy)
  cairo_move_to(cr, wingx, wingheight + y);
  cairo_line_to(cr, pointx, y);
  cairo_line_to(cr, wingx, wingheight - y);

  cairo_stroke(cr);
  cairo_destroy(cr);

  return FALSE;
}

void GuiPin::SetLabelPosition(int x, int y)
{
  m_label_x = x;
  m_label_y = y;
}

//------------------------------------------------------------------------
// GuiPin::DrawLabel() - draw the label for a single pin
//
//
void GuiPin::DrawLabel(GdkPixmap *module_pixmap)
{
  const char *name;
  IOPIN *iopin = getIOpin();


  name = iopin ? iopin->name().c_str() : "";
  if(*name && m_bbw ) {

    gdk_draw_text(module_pixmap,
                  gdk_font_from_description(m_bbw->pinnamefont),
                  m_bbw->pinname_gc,
                  m_label_x,
                  m_label_y,
                  name,strlen(name));


  }
}
//------------------------------------------------------------------------
// GuiPin::DrawGUIlabel() - Erase label and change to that set by newGUIname
//
//
int GuiPin::DrawGUIlabel(GdkPixmap *module_pixmap,  int pinnameWidths[])
{
  IOPIN *iopin = getIOpin();

  const char *name = iopin ? iopin->GUIname().c_str() : "";
  if(*name && m_bbw && iopin->is_newGUIname()) {
    iopin->clr_is_newGUIname();

    int orient = (m_label_x <= LABELPAD+CASELINEWIDTH)?0:2; // Left or Right label?

    // Clear label area
    gdk_draw_rectangle(module_pixmap,
      gtk_widget_get_style(m_bbw->window)->white_gc,
      TRUE,
      m_label_x, m_label_y-m_height+CASEOFFSET,
      pinnameWidths[orient], m_height);

    gdk_draw_text(module_pixmap,
                  gdk_font_from_description(m_bbw->pinnamefont),
                  m_bbw->pinname_gc,
                  m_label_x,
                  m_label_y,
                  name,strlen(name));


    return 1;
  }
  return 0;
}
//------------------------------------------------------------------------
void GuiPin::addXref(CrossReferenceToGUI *newXref)
{
  xref = newXref;
}
//------------------------------------------------------------------------
void GuiPin::Destroy()
{
  if(xref)
    getIOpin()->remove_xref(xref);

  gtk_widget_destroy(m_pinDrawingArea);
}

//------------------------------------------------------------------------
static gboolean name_expose(GtkWidget *widget, GdkEventExpose *event, GuiModule *p)
{
  if (!p->name_pixmap()) {
    puts("bbw.c: no pixmap2!");
    return FALSE;
  }

  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));

  gdk_cairo_set_source_pixmap(cr, p->name_pixmap(), 0.0, 0.0);
  cairo_paint(cr);
  cairo_destroy(cr);

  return FALSE;
}

static gboolean module_expose(GtkWidget *widget, GdkEventExpose *event, GuiModule *p)
{
  if (!p->module_pixmap()) {
    puts("bbw.c: no pixmap3!");
    return 0;
  }

  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));

  gdk_cairo_set_source_pixmap(cr, p->module_pixmap(), 0.0, 0.0);
  cairo_paint(cr);
  cairo_destroy(cr);

  return FALSE;
}

void GuiModule::Draw()
{
}

void GuiModule::Destroy()
{
}

#define PACKAGESPACING 15

void GuiModule::Update()
{
  g_object_ref(m_pinLabel_widget);
  gtk_container_remove(GTK_CONTAINER(m_bbw->layout),m_pinLabel_widget);

  // Delete the static module pixmap if there is no widget
  // in the module.
  if(m_module->get_widget()==0)
    {
      g_object_unref(m_module_pixmap);
      gtk_widget_destroy(m_pinLabel_widget);
    }

  // Delete the pins
  std::vector<GuiPin *>::iterator pin_iter = m_pins.begin();

  for ( ; pin_iter != m_pins.end(); ++pin_iter) {
    GuiPin *pin = *pin_iter;
    pin->Destroy();
  }

  // Destroy name widget
  g_object_unref(m_name_pixmap);
  gtk_widget_destroy(m_name_widget);

  // Remove module from list
  std::vector<GuiModule *>::iterator iter =
    std::find(m_bbw->modules.begin(), m_bbw->modules.end(), this);
  if (iter != m_bbw->modules.end())
    m_bbw->modules.erase(iter);

  // rebuild module
  Build();

  g_object_unref(m_pinLabel_widget);
}

void GuiModule::UpdatePins()
{
  int change = 0;
  std::vector<GuiPin *>::iterator pin_iter = m_pins.begin();

  for ( ; pin_iter != m_pins.end(); ++pin_iter) {
    GuiPin *pin = *pin_iter;
    change = pin->DrawGUIlabel(m_module_pixmap, pinnameWidths) ? 1 : change;
    pin->Update();
  }

  GdkWindow *gdk_win = gtk_widget_get_window(m_pinLabel_widget);
  if (change && gdk_win)  // Pin label changed, Draw Labels
  {
    gdk_draw_pixmap(gdk_win,
      gtk_widget_get_style(m_pinLabel_widget)->fg_gc[gtk_widget_get_state(m_pinLabel_widget)],
      m_module_pixmap,
      0, 0, 0, 0, m_width, m_height);
  }

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

//------------------------------------------------------------------------
void GuiModule::DrawCaseOutline(GtkWidget *da)
{
  gdk_draw_rectangle(m_module_pixmap,
    gtk_widget_get_style(m_bbw->window)->bg_gc[gtk_widget_get_state(m_bbw->window)],
    TRUE,
    0, 0, m_width, m_height);

  cairo_t *cr = gdk_cairo_create (m_module_pixmap);

  cairo_rectangle(cr, CASEOFFSET,CASEOFFSET, m_width-2*CASEOFFSET, m_height-2*CASEOFFSET);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_fill_preserve (cr);
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);

  cairo_destroy(cr);
}
//------------------------------------------------------------------------

//
static float hackPackageHeight=0.0;

void GuiModule::AddPin(unsigned int pin_number)
{
  IOPIN *iopin = m_module->get_pin(pin_number);
  BreadBoardXREF *cross_reference = 0;
  if(iopin) {
    // Create xref
    cross_reference = new BreadBoardXREF();
    cross_reference->parent_window = (gpointer) m_bbw;
    cross_reference->data = NULL;
    iopin->add_xref(cross_reference);
  }

  GuiPin *pin = new GuiPin(m_bbw, this, m_module->package, pin_number);
  pin->addXref(cross_reference);
  m_pins.push_back(pin);
}

void GuiModule::AddPinGeometry(GuiPin *pin)
{
  eOrientation orientation;
  unsigned int pin_number = pin->number();

  // Get the X and Y coordinates for this pin
  // (the coordinates are referenced to the module's origin)
  int pin_x, pin_y;
  int label_x, label_y;

  const PinGeometry *pPinGeometry = m_module->package->getPinGeometry(pin_number);

  if (pPinGeometry->bNew) {
    switch (pPinGeometry->m_orientation) {

    case UP:
      pin_x   = (int)pPinGeometry->m_x;
      pin_y   = (int)pPinGeometry->m_y;

      label_x = pin_x + LABELPAD + CASELINEWIDTH;
      label_y = pin_y + LABELPAD + CASELINEWIDTH;

      orientation = UP;
      break;
    case LEFT:
      pin_x   = (int)pPinGeometry->m_x - pinspacing;
      pin_y   = (int)pPinGeometry->m_y;

      label_x = LABELPAD + CASELINEWIDTH;
      label_y = pin_y + LABELPAD + CASELINEWIDTH;

      orientation = LEFT;
      break;
    case RIGHT:
      pin_x = m_width + (int)pPinGeometry->m_x;
      pin_y = (int)pPinGeometry->m_y;

      label_x = pin_x + LABELPAD + CASELINEWIDTH + m_width/2;
      label_y = pin_y + LABELPAD + CASELINEWIDTH;

      orientation = RIGHT;
      break;
    case DOWN:
      pin_x = (int)pPinGeometry->m_x;
      pin_y = m_height + (int)pPinGeometry->m_y;

      label_x = pin_x + LABELPAD + CASELINEWIDTH;
      label_y = pin_y + LABELPAD + CASELINEWIDTH;

      orientation = DOWN;
      break;
    default:
      printf("################### Error:\n");
      printf("Undefined orientation.\n");
      assert(0);
    }
  } else {

    // old style -- to be deprecated.
    float pin_position=m_module->package->get_pin_position(pin_number);

    // Put pin in layout
    if(pin_position>=0.0 && pin_position<1.0) {
      pin_x=-pinspacing;
      pin_y=(int)(m_height/2+((pin_position-0.5)*hackPackageHeight))-pinspacing/2;
      orientation = LEFT;

      label_x=LABELPAD+CASELINEWIDTH;
      label_y=(int)(pin_position*hackPackageHeight);
      label_y+=LABELPAD+CASELINEWIDTH+pinspacing/2-m_bbw->pinnameheight/3;
      label_y+=PINLENGTH/2;

    }
    else if(pin_position>=2.0 && pin_position<3.0) {
      pin_x=m_width;
      pin_y=(int)(m_height/2+((3.0-pin_position-0.5)*hackPackageHeight))-pinspacing/2;
      orientation = RIGHT;

      label_x= m_width/2  + CASELINEWIDTH;
      label_y=(int)((3.0-pin_position)*hackPackageHeight);
      label_y+=LABELPAD+CASELINEWIDTH+pinspacing/2-m_bbw->pinnameheight/3;
      label_y+=PINLENGTH/2;

    }
    else {

      // FIXME

      printf("################### Error:\n");
      printf("Number of pins %u\n", m_module->package->number_of_pins);
      printf("pin_position %f\n",pin_position);
      printf("pin_position2 %f\n",m_module->package->get_pin_position(pin_number));
      printf("pin_number %u\n", pin_number);
      assert(0);
    }
  }

  pin->SetModulePosition(pin_x,pin_y);
  pin->SetLabelPosition(label_x,label_y);
  pin->SetOrientation(orientation);
  pin->Draw();

}

static void createLabel(GtkWidget **da,
                        GdkPixmap **pixmap,
                        GtkWidget *parent,
                        GdkGC *gc,
                        const char *text,
                        PangoFontDescription *font)
{
  *da = gtk_drawing_area_new();

  int height = gdk_string_height(gdk_font_from_description(font), text);
  int width  = gdk_string_width(gdk_font_from_description(font), text);

  gtk_widget_set_size_request(*da, width, height);
  *pixmap = gdk_pixmap_new(gtk_widget_get_window(parent),
                           width,
                           height,
                           -1);
  gdk_draw_rectangle(*pixmap,
    gtk_widget_get_style(parent)->bg_gc[gtk_widget_get_state(*da)],
    TRUE,
    0, 0, width, height);

  gdk_draw_text(*pixmap,
                gdk_font_from_description(font),
                gc,
                0,height,
                text,strlen(text));

}
void GuiModule::BuildReferenceDesignator()
{
  createLabel(&m_name_widget, &m_name_pixmap, m_bbw->window, m_bbw->pinname_gc,
              m_module->name().c_str(),m_bbw->pinnamefont);

  g_signal_connect(m_name_widget, "expose_event",
    G_CALLBACK(name_expose), this);
}

//------------------------------------------------------------------------
void GuiModule::Build()
{
  if(m_bIsBuilt || !m_bbw)
    return;

  if(!m_bbw->enabled)
    return;


  int i;
  int nx, ny;

  BreadBoardXREF *cross_reference;

  m_width=50;
  m_height=18;

  Package *package = m_module->package;
  if(!package)
    return;     // embedded module


  m_module_widget = (GtkWidget *)m_module->get_widget();

  m_pin_count = m_module->get_pin_count();

  /*
  Value *xpos = m_module->get_attribute("xpos", false);
  Value *ypos = m_module->get_attribute("ypos", false);
  xpos->get(nx);
  ypos->get(ny);
  */
  GetPosition(nx,ny);

  GtkTreeIter module_titer, pin_titer;
  GtkTreeStore *tree_store;
  
  g_object_get (m_bbw->tree, "model", &tree_store, NULL);
  gtk_tree_store_append (tree_store, &module_titer, NULL);
  gtk_tree_store_set (tree_store, &module_titer,
                      0, m_module->name().c_str(),
                      1, this,
                      -1);

  hackPackageHeight=(float)((m_pin_count/2+(m_pin_count&1)-1)*pinspacing);

  // Find the length of the longest pin name in each direction
  // The directions are in the order of left, up , right, down.
  int pinmax_x = 0;
  int pinmax_y = 0;
  for(i=1;i<=m_pin_count;i++) {

    PinGeometry *pPinGeometry = m_module->package->getPinGeometry(i);

    pPinGeometry->convertToNew();
    if (pPinGeometry->bNew) {
      pinmax_x = pinmax_x < pPinGeometry->m_x ? pinmax_x : ((int) pPinGeometry->m_x);
      pinmax_y = pinmax_y < pPinGeometry->m_y ? pinmax_y : ((int) pPinGeometry->m_y);
    }

    int w=0;
    const char *name = m_module->get_pin_name(i).c_str();

    if(name && pPinGeometry->m_bShowPinname)
      w = gdk_string_width(gdk_font_from_description(m_bbw->pinnamefont), name);

    if(w > pinnameWidths[pPinGeometry->m_orientation])
      pinnameWidths[pPinGeometry->m_orientation]= w;

    AddPin(i);

  }
  if(!m_module_widget) {

    // Create a static representation.

    m_width =  pinnameWidths[0] + pinnameWidths[2] + 2 * FOORADIUS;
    m_width += 2*CASELINEWIDTH+2*LABELPAD;

    m_height = m_module->get_pin_count()/2*pinspacing; // pin name height
    if(m_module->get_pin_count()%2)
      m_height += pinspacing;

    m_height+=2*CASELINEWIDTH+2*LABELPAD;

    m_module_pixmap = gdk_pixmap_new(gtk_widget_get_window(m_bbw->window),
                                     m_width,
                                     m_height,
                                     -1);
    m_pinLabel_widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(m_pinLabel_widget, m_width, m_height);
    gtk_widget_show_all (m_pinLabel_widget);
    DrawCaseOutline(m_pinLabel_widget);
    g_signal_connect(m_pinLabel_widget,
                         "expose_event",
                         G_CALLBACK(module_expose),
                         this);
    gtk_widget_show(m_pinLabel_widget);


  } else {
    // Get the [from the module] provided widget's size
    GtkRequisition req;

    gtk_widget_size_request(m_module_widget, &req);

    m_width=req.width;
    m_height=req.height;
    gtk_widget_show(m_module_widget);
 }


  // Create xref
  cross_reference = new BreadBoardXREF();
  cross_reference->parent_window = (gpointer) m_bbw;
  cross_reference->data = (gpointer) 0;
  m_module->xref->_add(cross_reference);



  // Create name_widget
  BuildReferenceDesignator();
  gtk_widget_show(m_name_widget);


  std::vector<GuiPin *>::iterator pin_iter = m_pins.begin();

  for ( ; pin_iter != m_pins.end(); ++pin_iter) {
    GuiPin *pin = *pin_iter;
    AddPinGeometry(pin);
    if (m_module_pixmap)
      pin->DrawLabel(m_module_pixmap);

    gtk_layout_put(GTK_LAYOUT(m_bbw->layout),pin->m_pinDrawingArea,0,0);

    // Add pin to tree
    const char *name = pin->pinName();
    if(name) {
      gtk_tree_store_append (tree_store, &pin_titer, &module_titer);
      gtk_tree_store_set (tree_store, &pin_titer,
                          0, name,
                          1, pin,
                          -1);
    }
  }


  if (m_pinLabel_widget)
    gtk_layout_put(GTK_LAYOUT(m_bbw->layout), m_pinLabel_widget, 0, 0);
  if (m_module_widget)
    gtk_layout_put(GTK_LAYOUT(m_bbw->layout), m_module_widget, 0, 0);
  gtk_layout_put(GTK_LAYOUT(m_bbw->layout), m_name_widget, 0,0);

  SetPosition(nx, ny);

  m_bIsBuilt = true;

  update_board_matrix(m_bbw);

}


//========================================================================
//========================================================================

GuiModule::GuiModule(Module *_module, Breadboard_Window *_bbw)
  : GuiBreadBoardObject(_bbw,0,0), m_module(_module), m_module_widget(0),
    m_pinLabel_widget(0), m_module_x(0), m_module_y(0), m_name_widget(0),
    m_pin_count(0), m_module_pixmap(0), m_name_pixmap(0)
{
  m_width=0;
  m_height=0;

  pinnameWidths[0] = 0;
  pinnameWidths[1] = 0;
  pinnameWidths[2] = 0;
  pinnameWidths[3] = 0;

  if(m_bbw) {
    m_bbw->modules.push_back(this);

    if (m_module) {
      Value *xpos = dynamic_cast<Value*> (m_module->findSymbol("xpos"));
      Value *ypos = dynamic_cast<Value*> (m_module->findSymbol("xpos"));
      if(!xpos || !ypos) {
        xpos = new PositionAttribute(m_bbw,"xpos",(double)80);
        ypos = new PositionAttribute(m_bbw,"ypos",(double)80);
        m_module->addSymbol(xpos);
        m_module->addSymbol(ypos);
      }
    }
  }
}
//========================================================================
GuiDipModule::GuiDipModule(Module *_module, Breadboard_Window *_bbw)
  : GuiModule(_module, _bbw)
{
}

void GuiDipModule::DrawCaseOutline(GtkWidget *da)
{
  gdk_draw_rectangle(m_module_pixmap,
    gtk_widget_get_style(m_bbw->window)->bg_gc[gtk_widget_get_state(m_bbw->window)],
    TRUE,
    0, 0,
    m_width, m_height);

  cairo_t *cr = gdk_cairo_create (m_module_pixmap);

  cairo_line_to (cr, CASEOFFSET, CASEOFFSET);
  cairo_line_to (cr, CASEOFFSET, m_height-2*CASEOFFSET);
  cairo_line_to (cr, m_width-CASEOFFSET, m_height-2*CASEOFFSET);
  cairo_line_to (cr, m_width-CASEOFFSET, CASEOFFSET);
  cairo_arc (cr, m_width/2-CASEOFFSET, CASEOFFSET, FOORADIUS, 0, M_PI);

  cairo_close_path (cr);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_fill_preserve (cr);
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);

  cairo_destroy(cr);
}

//========================================================================
void Breadboard_Window::Update(void)
{
  int x,y;

  // loop all modules and look for changes
  if(!enabled)
    return;


  if(!gtk_widget_get_visible(window))
    return;

  std::vector<GuiModule *>::iterator iter = modules.begin();
  for ( ; iter != modules.end(); ++iter) {

    GuiModule *p = *iter;

    if(p->IsBuilt()) {

      // Check if module has changed number of pins
      if(p->pin_count()!=p->module()->get_pin_count())
        // If so, refresh the gui widget
        p->Update();

      // Check if module has changed its position
      p->GetPosition(x,y);

      if(p->x()!=x || p->y()!=y) {
        // If so, move the module
        p->SetPosition(x, y);
        update_board_matrix(p->bbw());
      }

      // Check if pins have changed state
      p->UpdatePins();
    }
    else
    {
        p->Build();
        Update();

    }
  }
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
  /*
  Value *xpos = gp->cpu->get_attribute("xpos", false);
  Value *ypos = gp->cpu->get_attribute("ypos", false);
  if(!xpos || !ypos) {
    xpos = new PositionAttribute(this,"xpos",(double)80);
    ypos = new PositionAttribute(this,"ypos",(double)80);
    gp->cpu->add_attribute(xpos);
    gp->cpu->add_attribute(ypos);
  }
  */

  m_MainCpuModule = new GuiDipModule(gp->cpu, this);

  if(!enabled)
    return;

  m_MainCpuModule->Build();

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
  /*
  Value *xpos = module->get_attribute("xpos", false);
  Value *ypos = module->get_attribute("ypos", false);
  if(!xpos || !ypos) {
    xpos = new PositionAttribute(this,"xpos",(double)sx);
    ypos = new PositionAttribute(this,"ypos",(double)sy);
    module->add_attribute(xpos);
    module->add_attribute(ypos);
  }
  */
  sy+=100;
  if(sy>LAYOUTSIZE_Y)
  {
        sy=0;
        sx+=100;
        if(sx>LAYOUTSIZE_X)
                sx=50;
  }

  GuiModule *p=new GuiModule(module, this);

  if(!enabled)
    return;

  p->Build();

  if(grab_next_module)
    grab_module(p);

  Update();
}


/* When a stimulus is being connected or disconnected, or a new node is created */
void Breadboard_Window::NodeConfigurationChanged(Stimulus_Node *node)
{
  if (std::find(nodes.begin(), nodes.end(), node) == nodes.end())
    nodes.push_back(node);

  if (!node_iter)
    return;

  struct gui_node *gn = (struct gui_node*) g_object_get_data (G_OBJECT (tree), node->name().c_str());
  GtkTreeStore *tree_store;
  
  g_object_get (tree, "model", &tree_store, NULL);

  if (!gn) {
    GtkTreeIter parent_iter, iter;

    gn = new gui_node;

    gn->bbw = this;
    gn->node = node;

    g_object_set_data(G_OBJECT(tree), node->name().c_str(), gn);
    gtk_tree_model_get_iter_first ((GtkTreeModel*) tree_store, &parent_iter);
    gtk_tree_store_append (tree_store, &iter, &parent_iter);
    gtk_tree_store_set (tree_store, &iter,
                        0, node->name().c_str(),
                        1, gn,
                        -1);

  }
}

static void layout_adj_changed(GtkWidget *widget, Breadboard_Window *bbw)
{
  GdkWindow *bin_win = gtk_layout_get_bin_window(GTK_LAYOUT(bbw->layout));

  if (!bin_win)
    return;

  if (!bbw->layout_pixmap) {
    puts("bbw.c: no pixmap4!");
    return;
  }

  GtkAdjustment *xadj = gtk_layout_get_hadjustment(GTK_LAYOUT(bbw->layout));
  GtkAdjustment *yadj = gtk_layout_get_vadjustment(GTK_LAYOUT(bbw->layout));
   
  int xoffset = (int) gtk_adjustment_get_value(GTK_ADJUSTMENT(xadj));
  int yoffset = (int) gtk_adjustment_get_value(GTK_ADJUSTMENT(yadj));

  GtkAllocation allocation;
  gtk_widget_get_allocation(bbw->layout, &allocation);

  gdk_draw_pixmap(bin_win,
    gtk_widget_get_style(bbw->window)->white_gc,
    bbw->layout_pixmap,
    xoffset, yoffset,
    xoffset, yoffset,
    allocation.width,
    allocation.height);
}

static gboolean layout_expose(GtkWidget *widget, GdkEventExpose *event, Breadboard_Window *bbw)
{
  if (!bbw->layout_pixmap) {
    puts("bbw.c: no pixmap5!");
    return 0;
  }

  layout_adj_changed(widget, bbw);

  return 0;
}

static GtkWidget *bb_vbox(GtkWidget *window, const char *name)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  g_object_ref(vbox);
  g_object_set_data_full (G_OBJECT (window), name, vbox,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (vbox);

  return vbox;
}

static GtkWidget *bb_hbox(GtkWidget *window, const char *name)
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  g_object_ref(hbox);
  g_object_set_data_full (G_OBJECT (window), name, hbox,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hbox);

  return hbox;
}

GtkWidget* Breadboard_Window::add_button(const char *label, const char *name,
                                         GCallback f, GtkWidget *box)
{

  GtkWidget *button = gtk_button_new_with_label (label);
  g_object_ref(button);
  g_object_set_data_full (G_OBJECT (window), name, button,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(f),
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
  GtkWidget *tree1;
  GtkWidget *hbox12;
  GtkWidget *hbox15;

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
  
  GtkCellRenderer *renderer;
  GtkListStore *list_store;
  GtkTreeStore *tree_store;
  GtkTreeSelection *selection;

  GdkColormap *colormap = gdk_colormap_get_system();

  gdk_color_parse("red",&high_output_color);
  gdk_color_parse("green",&low_output_color);
  gdk_color_parse("black",&black_color);

  gdk_colormap_alloc_color(colormap, &high_output_color,FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &low_output_color,FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &black_color,FALSE,TRUE);


  //
  // Top level window
  //

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_object_set_data (G_OBJECT (window), "window", window);
  gtk_window_set_title (GTK_WINDOW (window), "Breadboard");

  //
  // Horizontal pane
  //

  hpaned1 = gtk_hpaned_new ();
  g_object_ref(hpaned1);
  g_object_set_data_full (G_OBJECT (window), "hpaned1", hpaned1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hpaned1);
  gtk_container_add (GTK_CONTAINER (window), hpaned1);
  gtk_paned_set_position (GTK_PANED (hpaned1), 196);

  // vbox9 holds the left pane.
  vbox9 = bb_vbox(window, "vbox9");
  gtk_paned_pack1 (GTK_PANED (hpaned1), vbox9, FALSE, TRUE);

  vbox13 = bb_vbox(window, "vbox13");
  gtk_box_pack_start (GTK_BOX (vbox9), vbox13, TRUE, TRUE, 2);

  scrolledwindow4 = gtk_scrolled_window_new (0, 0);
  g_object_ref(scrolledwindow4);
  g_object_set_data_full (G_OBJECT (window), "scrolledwindow4", scrolledwindow4,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (scrolledwindow4);
  gtk_box_pack_start (GTK_BOX (vbox13), scrolledwindow4, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  renderer = gtk_cell_renderer_text_new ();
  tree_store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  tree = tree1 = gtk_tree_view_new_with_model ((GtkTreeModel*) tree_store);
  gtk_tree_view_insert_column_with_attributes ((GtkTreeView*) tree1,
                                               0, "",
                                               renderer,
                                               "text", 0,
                                               NULL);
  g_object_set (tree1,
                "headers-visible", FALSE,
                "enable-tree-lines", TRUE,
                NULL);
  g_signal_connect (gtk_tree_view_get_selection ((GtkTreeView*) tree1),
                    "changed", (GCallback) treeselect_cb,
                    NULL);
  g_object_ref(tree1);
  g_object_set_data_full (G_OBJECT (window), "tree1", tree1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (tree1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow4), tree1);

  hbox12 = bb_hbox(window, "hbox12");
  gtk_box_pack_start (GTK_BOX (vbox13), hbox12, FALSE, FALSE, 0);

  add_button("Add node","button5", G_CALLBACK(add_new_snode), hbox12);
  add_button("Add module","button6", G_CALLBACK(add_module), hbox12);
  add_button("Add library","button7", G_CALLBACK(add_library), hbox12);

  hbox15 = bb_hbox(window, "hbox15");
  gtk_box_pack_start (GTK_BOX (vbox13), hbox15, FALSE, FALSE, 0);

  add_button("Trace all","button25", G_CALLBACK(trace_all), hbox15);
  add_button("Clear traces","button26", G_CALLBACK(clear_traces), hbox15);



  node_frame = gtk_frame_new ("Node connections");
  g_object_ref(node_frame);
  g_object_set_data_full (G_OBJECT (window), "node_frame", node_frame,
                            (GDestroyNotify) g_object_unref);
  //  gtk_widget_show (node_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), node_frame, TRUE, TRUE, 0);

  vbox11 = gtk_vbox_new (FALSE, 0);
  g_object_ref(vbox11);
  g_object_set_data_full (G_OBJECT (window), "vbox11", vbox11,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (vbox11);
  gtk_container_add (GTK_CONTAINER (node_frame), vbox11);

  scrolledwindow2 = gtk_scrolled_window_new (0, 0);
  g_object_ref(scrolledwindow2);
  g_object_set_data_full (G_OBJECT (window), "scrolledwindow2", scrolledwindow2,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (scrolledwindow2);
  gtk_box_pack_start (GTK_BOX (vbox11), scrolledwindow2, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport7 = gtk_viewport_new (0, 0);
  g_object_ref(viewport7);
  g_object_set_data_full (G_OBJECT (window), "viewport7", viewport7,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (viewport7);
  gtk_container_add (GTK_CONTAINER (scrolledwindow2), viewport7);

  renderer = gtk_cell_renderer_text_new();
  list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  node_clist = gtk_tree_view_new_with_model((GtkTreeModel*) list_store);
  gtk_tree_view_insert_column_with_attributes((GtkTreeView*) node_clist, 0,
                                              "Nodes", renderer,
                                              "text", 0, NULL);
  g_object_set(node_clist, "headers-visible", FALSE, NULL);

  g_object_ref(node_clist);
  g_object_set_data_full (G_OBJECT (window), "node_clist", node_clist,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (node_clist);
  gtk_container_add (GTK_CONTAINER (viewport7), node_clist);

  hbox10 = bb_hbox(window, "hbox10");
  gtk_box_pack_start (GTK_BOX (vbox11), hbox10, FALSE, FALSE, 0);

  add_button("Remove stimulus","rsb", G_CALLBACK(remove_node_stimulus), hbox10);
  add_button("Remove node","rnb", G_CALLBACK(remove_node), hbox10);

  stimulus_frame = gtk_frame_new ("Stimulus settings");
  g_object_ref(stimulus_frame);
  g_object_set_data_full (G_OBJECT (window), "stimulus_frame", stimulus_frame,
                            (GDestroyNotify) g_object_unref);
  //  gtk_widget_show (stimulus_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), stimulus_frame, FALSE, FALSE, 0);

  vbox14 = gtk_vbox_new (FALSE, 0);
  g_object_ref(vbox14);
  g_object_set_data_full (G_OBJECT (window), "vbox14", vbox14,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (vbox14);
  gtk_container_add (GTK_CONTAINER (stimulus_frame), vbox14);


  stimulus_settings_label=gtk_label_new("");
  gtk_widget_show(stimulus_settings_label);
  gtk_box_pack_start(GTK_BOX(vbox14), stimulus_settings_label, FALSE,FALSE,0);

  hbox13 = bb_hbox(window, "hbox13");
  gtk_box_pack_start (GTK_BOX (vbox14), hbox13, FALSE, FALSE, 0);

  add_button("Connect stimulus to node","sanb", G_CALLBACK(stimulus_add_node), hbox13);




  module_frame = gtk_frame_new ("Module settings");
  g_object_ref (module_frame);
  g_object_set_data_full (G_OBJECT (window), "module_frame", module_frame,
                            (GDestroyNotify) g_object_unref);
  //  gtk_widget_show (module_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), module_frame, TRUE, TRUE, 0);

  vbox10 = gtk_vbox_new (FALSE, 0);
  g_object_ref(vbox10);
  g_object_set_data_full (G_OBJECT (window), "vbox10", vbox10,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (vbox10);
  gtk_container_add (GTK_CONTAINER (module_frame), vbox10);

  scrolledwindow1 = gtk_scrolled_window_new (0, 0);
  g_object_ref(scrolledwindow1);
  g_object_set_data_full (G_OBJECT (window), "scrolledwindow1", scrolledwindow1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox10), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport6 = gtk_viewport_new (0, 0);
  g_object_ref(viewport6);
  g_object_set_data_full (G_OBJECT (window), "viewport6", viewport6,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (viewport6);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport6);

  renderer = gtk_cell_renderer_text_new();
  list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  attribute_clist = gtk_tree_view_new_with_model((GtkTreeModel*) list_store);
  gtk_tree_view_insert_column_with_attributes((GtkTreeView*) attribute_clist, 0,
                                              "Attributes", renderer,
                                              "text", 0, NULL);
  g_object_set(attribute_clist, "headers-visible", FALSE, NULL);


  g_object_ref (attribute_clist);
  g_object_set_data_full (G_OBJECT (window), "attribute_clist", attribute_clist,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (attribute_clist);
  gtk_container_add (GTK_CONTAINER (viewport6), attribute_clist);
  selection = gtk_tree_view_get_selection ((GtkTreeView*) attribute_clist);
  g_signal_connect (selection,
                    "changed", (GCallback) settings_clist_cb,
                    this);
  hbox9 = bb_hbox(window, "hbox9");
  gtk_box_pack_start (GTK_BOX (vbox10), hbox9, FALSE, FALSE, 0);

  attribute_entry = gtk_entry_new ();
  g_object_ref(attribute_entry);
  g_object_set_data_full (G_OBJECT (window), "attribute_entry", attribute_entry,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (attribute_entry);
  gtk_box_pack_start (GTK_BOX (hbox9), attribute_entry, FALSE, FALSE, 0);
  g_signal_connect(attribute_entry,
                  "activate",
                  G_CALLBACK(settings_set_cb),
                  this);

  add_button("Set","attribute_button", G_CALLBACK(settings_set_cb), hbox9);

  hbox14 = bb_hbox(window, "hbox14");
  gtk_box_pack_start (GTK_BOX (vbox10), hbox14, FALSE, FALSE, 0);

  add_button("Remove module","remove_module_button", G_CALLBACK(remove_module), hbox14);
  add_button("Save Configuration ...","save_stc_button", G_CALLBACK(save_stc), vbox9);

  scrolledwindow5 = gtk_scrolled_window_new (0, 0);
  g_object_ref(scrolledwindow5);
  g_object_set_data_full (G_OBJECT (window), "scrolledwindow5", scrolledwindow5,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (scrolledwindow5);
  gtk_paned_pack2 (GTK_PANED (hpaned1), scrolledwindow5, TRUE, TRUE);

  vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolledwindow5));
  hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolledwindow5));

  layout = gtk_layout_new (hadj, vadj);
  g_object_ref(layout);
  g_object_set_data_full (G_OBJECT (window), "layout", layout,
                            (GDestroyNotify) g_object_unref);
  gtk_container_add (GTK_CONTAINER (scrolledwindow5), layout);
  gtk_layout_set_size (GTK_LAYOUT (layout), LAYOUTSIZE_X, LAYOUTSIZE_Y);

  gtk_widget_set_events(layout,
                        gtk_widget_get_events(layout)|
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_MOTION_MASK |
                        GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(layout, "motion-notify-event",
                     G_CALLBACK(pointer_cb), this);
  g_signal_connect(layout, "button_press_event",
                     G_CALLBACK(pointer_cb), this);
  g_signal_connect(layout, "button_release_event",
                     G_CALLBACK(pointer_cb), this);
  g_signal_connect(layout, "expose_event",
                     G_CALLBACK(layout_expose), this);

  GtkAdjustment *xadj = gtk_layout_get_hadjustment(GTK_LAYOUT(layout));
  gtk_adjustment_set_step_increment(xadj, 10.0);
  GtkAdjustment *yadj = gtk_layout_get_vadjustment(GTK_LAYOUT(layout));
  gtk_adjustment_set_step_increment(yadj, 10.0);

  g_signal_connect(xadj, "value_changed",
                     G_CALLBACK(layout_adj_changed), this);
  g_signal_connect(yadj, "value_changed",
                     G_CALLBACK(layout_adj_changed), this);

  gtk_widget_set_app_paintable(layout, TRUE);
  gtk_widget_show (layout);


  //printf("bb %s:%d, w=%d, h=%d\n",__FUNCTION__,__LINE__,width,height);
  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_window_move(GTK_WINDOW(window), x, y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

  //  gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
  //                         GTK_SIGNAL_FUNC (gtk_widget_destroyed), GTK_OBJECT(window));
  g_signal_connect (window, "delete_event",
                      G_CALLBACK(delete_event), (gpointer)this);
  g_signal_connect_after(window, "configure_event",
                           G_CALLBACK(gui_object_configure_event), this);


  gtk_widget_realize(window);

  GdkWindow *win_gdk_window = gtk_widget_get_window(window);

  pinname_gc=gdk_gc_new(win_gdk_window);

  case_gc=gdk_gc_new(win_gdk_window);
  gdk_gc_set_line_attributes(case_gc,CASELINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);

  pinstatefont = pango_font_description_from_string("Courier Bold 8");
  pinnamefont = pango_font_description_from_string("Courier Bold 8");

  pinline_gc=gdk_gc_new(win_gdk_window);

  gdk_gc_set_line_attributes(pinline_gc,PINLINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);

  layout_pixmap = gdk_pixmap_new(win_gdk_window,
                                      LAYOUTSIZE_X,
                                      LAYOUTSIZE_Y,
                                      -1);

  pinnameheight = gdk_string_height (gdk_font_from_description(pinnamefont),"9y");

  if(pinspacing<pinnameheight)
    pinspacing=pinnameheight+2;

  if(pinspacing%ROUTE_RES)
    {
      pinspacing-=pinspacing%ROUTE_RES;
      pinspacing+=ROUTE_RES;
    }


  struct gui_node *gn;

  gn = new gui_node;
  gn->bbw=this;
  gn->node=0; // indicates that this is the root node.
  GtkTreeIter iter;
  gtk_tree_store_append (tree_store, &iter, NULL);
  gtk_tree_store_set (tree_store, &iter,
                      0, "nodes",
                      1, gn,
                      -1);
  node_iter = &iter;

  // Handle nodes added before breadboard GUI enabled
  std::vector<Stimulus_Node *>::iterator list = nodes.begin();
  for ( ; list != nodes.end(); ++list)
        NodeConfigurationChanged(*list);

  bIsBuilt = true;

  UpdateMenuItem();

  draw_nodes(this);
  // cout << "FIXME gui_breadboard.cc " << __FUNCTION__ << endl;
  /*
  // Loop module list
  Symbol_Table::module_symbol_iterator mi = get_symbol_table().beginModuleSymbol();
  Symbol_Table::module_symbol_iterator itModEnd = get_symbol_table().endModuleSymbol();
  for(mi = get_symbol_table().beginModuleSymbol();
      mi!=itModEnd;
      mi++)
  {
        NewModule((*mi)->get_module());
  }

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
  */
  //gtk_widget_show_all(window);
  gtk_widget_show(window);

  Update();
}

const char *Breadboard_Window::name()
{
  return "pinout";
}

Breadboard_Window::Breadboard_Window(GUI_Processor *_gp)
  : pinstatefont(0), pinnamefont(0), pinname_gc(0), pinline_gc(0),
    case_gc(0), node_clist(0),
    stimulus_settings_label(0), stimulus_add_node_button(0),
    layout_pixmap(0), hadj(0), vadj(0), node_iter(0),
    selected_pin(0), selected_node(0), selected_module(0)
{
  menu = "/menu/Windows/Breadboard";

  wc = WC_misc;
  wt = WT_breadboard_window;
  gp = _gp;

  if(!get_config())
    printf("warning: %s\n",__FUNCTION__);

  //printf("bb %s:%d, w=%d, h=%d\n",__FUNCTION__,__LINE__,width,height);

  if(enabled)
    Build();

}

#endif // HAVE_GUI
