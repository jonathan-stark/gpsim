
#ifndef __CMD_GPSIM_H__
#define __CMD_GPSIM_H__

#include <stdarg.h>

#if defined(putc)
#undef putc
#endif

class ISimConsole {
public:
  virtual void Printf(const char *fmt, ...) = 0;
  virtual void VPrintf(const char *fmt, va_list argptr) = 0;
  virtual void Puts(const char*) = 0;
  virtual void Putc(const char) = 0;
  virtual char* Gets(char *, int) = 0;
};

#endif
