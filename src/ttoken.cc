/*
   Copyright (C) 1998-2003 T. Scott Dattalo

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

#include "ttoken.h"
#include "pthread-wrap.h"

namespace gpsim {

  Token::Token()
  {
    thread = new ThreadWrapper();
  }

  void Token::Initialize(void *(*child) (void *), void *data)
  {

    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init (&thread->cvWaitOnParent, NULL);
    pthread_cond_init (&thread->cvWaitOnChild, NULL);

    grab();

    pthread_attr_init(&thread->thAttribute);
    pthread_attr_setdetachstate(&thread->thAttribute, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread->thHostInterface, &thread->thAttribute, child, data);
  }

  void Token::waitForChild()
  {
    pthread_cond_wait(&thread->cvWaitOnChild, &thread->mutex);
  }

  void Token::grab()
  {
    pthread_mutex_lock(&thread->mutex);
  }

  void Token::passToChild()
  {
    pthread_cond_signal(&thread->cvWaitOnParent);
    pthread_cond_wait(&thread->cvWaitOnChild, &thread->mutex);
  }

  void Token::passToParent()
  {
    pthread_cond_signal(&thread->cvWaitOnChild);
    pthread_cond_wait(&thread->cvWaitOnParent, &thread->mutex);
  }

} // end of namespace gpsim
