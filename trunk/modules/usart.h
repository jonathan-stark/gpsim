/*
   Copyright (C) 1998,1999,2000,2001 T. Scott Dattalo

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


#ifndef __USART_MODULE_H__
#define __USART_MODULE_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../config.h"

#ifdef HAVE_GUI
#define GTK_ENABLE_BROKEN
#include <gtk/gtk.h>
#endif


#include <glib.h>

#include "../src/modules.h"


class TXREG;
class RCREG;
class RxBaudRateAttribute;
class TxBaudRateAttribute;
class TxBuffer;
class RxBuffer;
class Boolean;

class USARTModule : public Module
{
 public:

#ifdef HAVE_GUI
  GtkWidget *window, *text;
#endif // HAVE_GUI
 
  void CreateGraphics(void);
  virtual void show_tx(unsigned int data);
  virtual void SendByte(unsigned tx_byte);


  // Inheritances from the Package class
  virtual void create_iopin_map();

  USARTModule(const char *new_name);
  ~USARTModule();

  static Module *USART_construct(const char *new_name=NULL);

  virtual void new_rx_edge(unsigned int);
  virtual bool mGetTxByte(unsigned int &);
  virtual void newRxByte(unsigned int);
  virtual void get(char *, int len);
private:
  RxBaudRateAttribute *m_RxBaud;
  TxBaudRateAttribute *m_TxBaud;
  Boolean  *m_CRLF;
  TxBuffer *m_TxBuffer;
  RxBuffer *m_RxBuffer;

  RCREG *m_rcreg;
  TXREG *m_txreg;

  unsigned char * m_TxFIFO;
  int m_FifoLen;
  int m_FifoHead;
  int m_FifoTail;

};
#endif //  __USART_MODULE_H__
