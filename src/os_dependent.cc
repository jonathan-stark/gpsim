/*
   Copyright (C) 1998 T. Scott Dattalo

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



/* os_dependent.cc  -                   */
/* version 0.1                          */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#ifndef _WIN32
#include <dlfcn.h>
#else
#define G_PLATFORM_WIN32
#define G_OS_WIN32
#include <glib/gmem.h>
#include <glib/gwin32.h>
#include <direct.h>
#include <windows.h>
#endif

using namespace std;

#ifdef _WIN32
#define MODULE_EXT ".dll"
#define FOLDERDELIMITER '\\'
#define FOLDERDELIMITERALTERNATIVE '/'
#define PATHDELIMITER ";"
#else
#define MODULE_EXT ".so"
#define FOLDERDELIMITER '/'
#define FOLDERDELIMITERALTERNATIVE '\\'
#define PATHDELIMITER ":"
#endif

char * get_error_message();

void translatePath(string &sPath) {
  string::size_type nPos;
  while ( (nPos = sPath.find(FOLDERDELIMITERALTERNATIVE)) != string::npos) {
    sPath[nPos] = FOLDERDELIMITER;
  }
}

static vector<string> asDllSearchPath;

void AddToSearchPath(string &sFolder, string &sFile) {
  string::size_type LastDelimiter = sFolder.find_last_of(FOLDERDELIMITER);
  if (LastDelimiter == string::npos) {
    sFile = sFolder;
  }
  else {
    string sNewFolder;
    sNewFolder = sFolder.substr(0, LastDelimiter + 1);
    sFile = sFolder.substr(LastDelimiter + 1);
    vector<string>::iterator it = find(asDllSearchPath.begin(),
      asDllSearchPath.end(), sNewFolder);
    if (it == asDllSearchPath.end()) {
      asDllSearchPath.insert(asDllSearchPath.begin(), sNewFolder);
    }
  }
}

void * load_library(const char *library_name, char **pszError)
{
  void *handle;
  string sFile;
  string sPath(library_name);
  translatePath(sPath);
  if (stricmp(&sPath[sPath.size() - 4], MODULE_EXT) != 0) {
    sPath.append(MODULE_EXT);
  }
  AddToSearchPath(sPath, sFile);
  vector<string>::iterator itSearchPath;
  for (itSearchPath = asDllSearchPath.begin();
      itSearchPath != asDllSearchPath.end();
      itSearchPath++) {
     sPath = *itSearchPath + sFile;
#ifndef _WIN32
    // According to the man page for dlopen, the RTLD_GLOBAL flag can
    // be or'd with the second pararmeter of the function call. However,
    // under Linux at least, this apparently cause *all* symbols to
    // disappear.

    handle = dlopen (sPath.c_str(), RTLD_NOW); // | RTLD_GLOBAL);
#else
    handle = (void *)LoadLibrary((LPCSTR)sPath.c_str());
#endif
    if (NULL != handle) {
      return handle;
    }
  }
  *pszError = get_error_message();
  return NULL;
}

void free_library(void *handle)
{
#ifdef _WIN32
  FreeLibrary((HMODULE)handle);
#else
  dlclose (handle);
#endif
}

char * get_error_message() {
#ifndef _WIN32
  return dlerror();
#else
  return g_win32_error_message(GetLastError());
#endif
}

void free_error_message(char * pszError)
{
#ifdef _WIN32
  g_free(pszError);
#endif
}

void * get_library_export(const char *name, void *library_handle)
{
  void * pExport;
  char *pszError;
#ifndef _WIN32
  dlerror();	// Clear possible previous errors
  pExport = dlsym(library_handle, name);
#else
  pExport = GetProcAddress((HMODULE)library_handle, name);
#endif
  if (NULL == pExport) {
    pszError = get_error_message();
    // cout << "WARNING: non-conforming module library\n"
	  //      << "  gpsim libraries should have the get_mod_list() function defined\n";
    fprintf(stderr, "%s\n", pszError);
    free_error_message(pszError);
  }
  return pExport;
}


