
#ifndef __CMD_GPSIM_H__
#define __CMD_GPSIM_H__

#include <stdarg.h>

#define CMD_ERR_OK                    0
#define CMD_ERR_ABORTED               1
#define CMD_ERR_ERROR                 2
#define CMD_ERR_PROCESSORDEFINED      3
#define CMD_ERR_PROCESSORNOTDEFINED   4
#define CMD_ERR_COMMANDNOTDEFINED     5

class ISimConsole {
public:
  virtual void printf(const char *fmt, ...) = 0;
  virtual void vprintf(const char *fmt, va_list argptr) = 0;
  virtual void puts(const char*) = 0;
  virtual void putc(const char) = 0;
  virtual char* gets(char *, int) = 0;
};

class ICommandHandler {
public:
  virtual char *GetName(void) = 0;
  virtual int Execute(const char * commandline, ISimConsole *out) = 0;
};

#define GPSIM_GETCOMMANDHANDLER "GetCommandHandler"
typedef ICommandHandler * (*PFNGETCOMMANDHANDLER)(void);
#endif
