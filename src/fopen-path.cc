/* -*- Mode: C++; c-file-style: "bsd"; comment-column: 40 -*- */
/*
   Copyright (C) 2000 Daniel Christian, T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

//
// fopen-path.cc
//
// Modified version of fopen, that searches a path.  
// Technically this is a c++ file, but it should work in C too.
// The functions use a C calling convention for compatibility.

#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <assert.h>

#include "../config.h"
#include "gpsim_def.h"
#include "pic-processor.h"

#include "fopen-path.h"
					// should be command line
static char **searchPath = 0;
static int searchPathCount = 0;

class CSourceSearchPath : public String 
{
public:
        CSourceSearchPath() : String("SourcePath", NULL, "Search path for source files") 
        {
        }
  virtual void set(const char *cP,int len=0) {
    set_search_path(cP);
  }
  virtual void set(Value *pValue) {
    String *pString = dynamic_cast<String*>(pValue);
    if(pString != NULL) {
      set_search_path(pString->getVal());
    }
  }

  string toString() {
    string sPath;
    for (int iIndex = 0;
        iIndex < searchPathCount;
        ++iIndex) {
      sPath.append(searchPath[iIndex]);
      if(iIndex < searchPathCount - 1) {
        sPath.append(":");
      }
    }
    return sPath;
  }

  char *toString(char *return_str, int len)
  {
    string sPath;
    for (int iIndex = 0;
        iIndex < searchPathCount && len < 0;
        ++iIndex) {
      char *pFolder = searchPath[iIndex];
      strncpy(return_str, pFolder, len);
      len -= strlen(pFolder);
      if(iIndex < searchPathCount) {
        len--;
      }
    }
    return return_str;
  }
};

///
/// InitSourceSearchSymbol 
/// Used to initialize the CSourceSearchPath during startup.
/// The symbol table will delete the CSourceSearchPath
/// object so CSourceSearchPath cannot be static.
void InitSourceSearchAsSymbol() {
  // The symbol table will delete the CSourceSearchPath object
  globalSymbolTable().addSymbol(new CSourceSearchPath());
}


// Given a colon separated path, setup searchPath and searchPathCount
// Fix:: Any old values are lost (and the memory leaked).
void set_search_path (const char *path)
{
    const char *cp, *tp;
    int	pathLen;
    int ii;
    char **pathStr;

    if (!path || !*path) {		// clear the path
        searchPathCount = 0;
        if(searchPath != 0) {
          free(searchPath);
          searchPath = 0;
        }
        if (verbose) cout << "Clearing Search directory.\n";
            return;
    }
					// count colons to figure length
    for (cp = path, pathLen = 0;
         *cp;
         ++cp) {
      if (':' == *cp)
        ++pathLen;
    }
    ++pathLen;				// always one more segments than colons
    // searchPath = (char *[])calloc (pathLen, sizeof (char *));

    if(searchPath != 0) {
      free(searchPath);
    }
    // allocate an array of string pointers with one extra set to NULL
    // to mark the end of the array.
    searchPath = static_cast<char **>(calloc (pathLen, sizeof (char *)));
    assert (0 != searchPath);

    // Parse the colon delimited string path and put each folder into
    // the string array.
    for (cp = path, pathStr = searchPath, ii = 0, tp = strchr (path, ':');
	       (0 != tp) && (ii < pathLen);
	       ++ii) {
	
      assert ((0 != cp) && (0 != tp));
      if (tp == cp) {               // treat empty path as "."
	        *pathStr = strdup (".");  // allocate, in case we free later
      } else {                      // copy out the string section
	        const char *sp;
	        char *dp;
	        *pathStr = (char *)malloc (tp - cp + 1);
	        assert (0 != *pathStr);
	        for (dp = *pathStr, sp = cp; sp < tp; *dp++ = *sp++);
	        *dp = 0;                  // NULL terminate
      }
      if (verbose) cout << "Search directory: " << *pathStr << '\n';

      ++pathStr;
      cp = tp+1;
      tp = strchr (cp, ':');
    }
    if (*cp) {
        *pathStr = strdup (cp);   // get last one
    } else {
        *pathStr = strdup (".");  // allocate, in case we free later
    }
    if (verbose) cout << "Search directory: " << *pathStr << '\n';
    searchPathCount = pathLen;
}

//-----------------------------------------------------------
// Try to open a file on a series of paths.  First try as an absolute path.
// This tries to keep as much of the original file path as possible.
// for example: fopen_path ("src/pic/foo.inc", "r"), will try
//	src/pic/foo.inc,
//	PATH1/src/pic/foo.inc, PATH1/pic/foo.inc, PATH1/foo.inc
//	PATH2/src/pic/foo.inc, PATH2/pic/foo.inc, PATH2/foo.inc
//	...
// It also converts back slashes to forward slashes (for MPLAB compatibility)
FILE *fopen_path(const char *filename, const char *perms)
{
  FILE *fp;
  const char *fileStr;			// filename walker
  char **pathStr;			// path pointer
  int	ii;				// loop counter
  char	*cp;				// for string walking
  char	nameBuff[256];			// where to build new filename

  assert (strlen (filename) <= (sizeof (nameBuff) - 1));
  strcpy (nameBuff, filename);
  for (cp = nameBuff; *cp; ++cp) {	// convert DOS slash to Unix slash
      if ('\\' == *cp) *cp = '/';
  }

  fp = fopen (nameBuff, perms);		// first try absolute path
  if (0 != fp) {
      if (verbose) printf ("Found %s as %s\n", filename, nameBuff);
      return fp;
  }
					// check along each directory
  for (pathStr = searchPath, ii=0;
       ii < searchPathCount;
       ++ii, ++pathStr) {
					// check each subdir in path
      for (fileStr = filename;
           fileStr && *fileStr;
           fileStr = strpbrk (fileStr+1, "/\\")) {
        strcpy (nameBuff, *pathStr);
        strcat (nameBuff, fileStr);
        assert (strlen (nameBuff) <= (sizeof (nameBuff) - 1));
        for (cp = nameBuff; *cp; ++cp) { // convert DOS slash to Unix slash
          if ('\\' == *cp)
            *cp = '/';
        }
	if (verbose) {
	  printf ("Trying to open %s\n", nameBuff);
	}
        fp = fopen (nameBuff, perms);	// try it
        if (0 != fp) {
          if (verbose)
            printf ("Found %s as %s\n", filename, nameBuff);
          return fp;
        }
      }
  }
  if (verbose) {
      printf ("Failed to open %s in path: ", filename);
      for (pathStr = searchPath, ii=0;
           ii < searchPathCount;
           ++ii, ++pathStr) {
        printf ("%s ", *pathStr);
      }
      printf ("\n");
  }

  return 0;
}
