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

#include "../config.h"
#include "exports.h"

#ifndef _WIN32
#include <dlfcn.h>
#define STRICMP strcasecmp
#else
#define G_PLATFORM_WIN32
#define G_OS_WIN32
#include <glib/gmem.h>
#include <glib/gwin32.h>
#include <direct.h>
#include <windows.h>

#define STRICMP stricmp

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
unsigned long get_error();

#ifdef _WIN32
#define OS_E_FILENOTFOUND 0x0000007E
#define OS_E_MEMORYACCESS 0x000003E6
#else
// JRH - just made a guess
#define OS_E_FILENOTFOUND ENOENT
#define OS_E_MEMORYACCESS EADDRNOTAVAIL
#include <errno.h>
#endif

void translatePath(string &sPath) {
  string::size_type nPos;
  while ( (nPos = sPath.find(FOLDERDELIMITERALTERNATIVE)) != string::npos) {
    sPath[nPos] = FOLDERDELIMITER;
  }
}

int FileExtCompare(const char *pExt1, const char *pExt2) {
  if(*pExt1 == '.')
    pExt1++;
  if(*pExt2 == '.')
    pExt2++;
#ifdef _WIN32
  return stricmp(pExt1, pExt2);
#else
  return strcmp(pExt1, pExt1);
#endif
}

bool GPSIM_EXPORT IsFileExtension(const char *pszFile, const char *pFileExt) {
  string s(pszFile);
  translatePath(s);
  string::size_type pos = s.find_last_of('.');
  if(pos == string::npos) {
    if(*pFileExt == '.')
      pFileExt++;
    return *pFileExt == 0;
  }
  else {
    return FileExtCompare(pFileExt, s.substr(pos + 1).c_str()) == 0;
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

//---------------------------
//OS agnostic library loader

static void * sLoad(const char *library_name)
{
  if(!library_name)
    return 0;

  void *handle;
#ifndef _WIN32
  // According to the man page for dlopen, the RTLD_GLOBAL flag can
  // be or'd with the second pararmeter of the function call. However,
  // under Linux at least, this apparently cause *all* symbols to
  // disappear.

  handle = dlopen (library_name, RTLD_NOW); // | RTLD_GLOBAL);
#else
  handle = (void *)LoadLibrary((LPCSTR)library_name);
#endif
  return handle;
}

void * load_library(const char *library_name, char **pszError)
{
  void *handle;

  string sFile;
  string sPath(library_name);
  translatePath(sPath);
  if (STRICMP(&sPath[sPath.size() - (sizeof(MODULE_EXT)-1)], MODULE_EXT) != 0) {
    sPath.append(MODULE_EXT);
  }
  AddToSearchPath(sPath, sFile);

  // First, see if we can load the library from where ever the 
  // system thinks libraries are located.
  if( (handle = sLoad(sPath.c_str())) != 0)
    return handle;

  *pszError = get_error_message();
  unsigned long uError = get_error();
#ifdef _WIN32
  if(uError == OS_E_FILENOTFOUND) {
#else
  printf("Debug: need to define OS_E_FILENOTFOUND for Linux and test error code for failed load_library() : error = %lu\n", uError);
  if(true) {
#endif
    // Failed to find the library in the system paths, so try to load
    // from one of our paths.

    vector<string>::iterator itSearchPath;
    for (itSearchPath = asDllSearchPath.begin();
        itSearchPath != asDllSearchPath.end();
        itSearchPath++) {
      sPath = *itSearchPath + sFile;

      handle = sLoad(sPath.c_str());

      if (NULL != handle) {
        return handle;
      }
    }
    *pszError = get_error_message();
  }
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

unsigned long get_error() {
#ifdef _WIN32
  return GetLastError();
#else
  return errno;
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

void * get_library_export(const char *name, void *library_handle, char **pszError)
{
  void * pExport;
#ifndef _WIN32
  dlerror();	// Clear possible previous errors
  pExport = dlsym(library_handle, name);
#else
  pExport = (void*)GetProcAddress((HMODULE)library_handle, name);
#endif
  if (NULL == pExport && pszError != NULL) {
    *pszError = get_error_message();
  }
  return pExport;
}


