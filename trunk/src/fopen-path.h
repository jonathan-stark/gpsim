/* -*- Mode: C++; c-file-style: "bsd"; comment-column: 40 -*- */
/*
   Copyright (C) 2000 Daniel Christian, T. Scott Dattalo

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

#include "exports.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

LIBGPSIM_EXPORT void InitSourceSearchAsSymbol();
LIBGPSIM_EXPORT void set_search_path (const char *path);
LIBGPSIM_EXPORT FILE *fopen_path (const char *filename, const char *perms);

#ifdef __cplusplus
}
#endif /* __cplusplus */
