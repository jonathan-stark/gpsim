/*
   Copyright (C) 2004 Borut Razem

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
Boston, MA 02111-1307, USA. */

#include <windows.h>
#include <io.h>
#include <fcntl.h>

#include "fd2raw.h"


/*
 * a hack to call _setmode() from MSVCRT.DLL:
 * glib runtime uses MSVCRT.DLL, while gpsim executable uses MSVCR71.DLL.
 * To set the mode of file handle, which is used in glib, the _setmode()
 * from MSVCRT.DLL ahould be called.
 */

static int win32_setmode(int fd, int mode)
{
  int ret = -1;
  HINSTANCE hinst;

  /* Get a handle to the DLL module. */

  hinst = LoadLibrary("MSVCRT.DLL");

  /* If the handle is valid, try to get the function address. */

  if (NULL == hinst)
    g_win32_error_message(GetLastError());
  else
    {
      int (*proc_add)(int, int);
      proc_add = (int (*)(int, int)) GetProcAddress(hinst, "_setmode");

      /* If the function address is valid, call the function. */

      if (NULL == proc_add)
        g_win32_error_message(GetLastError());
      else
        ret = (proc_add) (fd, mode);

      /* Free the DLL module. */

      if (0 == FreeLibrary(hinst))
        g_win32_error_message(GetLastError());
    }

  return ret;
}


/*
 * set file descriptor to raw mode
 */

bool win32_fd_to_raw(int fd)
{
  HANDLE handle = (HANDLE)_get_osfhandle(fd);

  /* if a console */
  if (0 != (GetFileType(handle) & FILE_TYPE_CHAR))
    {
      DWORD orig_mode;

      /* set it to raw mode */
      GetConsoleMode(handle, &orig_mode);
      SetConsoleMode(handle, orig_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
      win32_setmode(0, _O_BINARY);

      return true;
    }

  return false;
}


/*
 * set is_readable flag for character device channel
 * (this should be done in giowin32.c, g_io_win32_fd_get_flags_internal())
 */

void win32_set_is_readable(GIOChannel *channel)
{
  HANDLE handle = (HANDLE)_get_osfhandle(g_io_channel_unix_get_fd(channel));

  if (0 != (GetFileType(handle) & FILE_TYPE_CHAR))
    {
#if !defined _MSC_VER || _MSC_VER < 1300
      OSVERSIONINFOA *posvi = (OSVERSIONINFOA *)alloca(sizeof(OSVERSIONINFOA));
      posvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
      GetVersionExA(posvi);

      if (_winmajor > 4 || VER_PLATFORM_WIN32_NT == posvi->dwPlatformId)
#else
      if (_winmajor > 4 || VER_PLATFORM_WIN32_NT == _osplatform)
#endif
        {
          DWORD orig_mode;
          char c;
          DWORD count;

          /* remember the original console mode */
          GetConsoleMode(handle, &orig_mode);
          /* set console to raw mode */
          SetConsoleMode(handle, orig_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
          /* set is_readable flag (this should be done in giowin32.c, g_io_win32_fd_get_flags_internal()) */
          channel->is_readable = (ReadConsole(handle, &c, 0, &count, NULL) != 0);
          /* restore the original mode */
          SetConsoleMode(handle, orig_mode);
        }
      else
        {
          channel->is_readable = TRUE;
        }
    }
}
