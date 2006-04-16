/*
   Copyright (C) 2006 T. Scott Dattalo

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

#ifndef __ttl_H__
#define __ttl_H__

#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/modules.h"

namespace TTL {
  //------------------------------------------------------------------------
  //
  //  TTL devices
  //


  class TTLbase : public Module
  {
  public:

    TTLbase(const char *_name, const char *desc);
    ~TTLbase();

    virtual void setClock(bool) {}
    virtual void setEnable(bool) {}
    //virtual void create_iopin_map();
    virtual void update_state()=0;
    //GtkWidget *create_pixmap(char **pixmap_data);

  protected:
    double  m_dVdd;
    bool    m_bClock;
    bool    m_bEnable;

  };

  class Clock;
  class Enable;

  class TTL377 : public TTLbase
  {
  public:

    TTL377(const char *_name);
    static Module *construct(const char *new_name=NULL);

    virtual void create_iopin_map();
    virtual void setClock(bool);
    virtual void setEnable(bool);
    virtual void update_state();
  protected:
    Clock  *m_clock;
    Enable *m_enable;
    IOPIN **m_D;
    IO_bi_directional **m_Q;
  };

} // end of namespace TTL

#endif // __ttl_H__
