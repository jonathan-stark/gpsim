
#ifndef __CMD_GPSIM_H__
#define __CMD_GPSIM_H__

#include <stdarg.h>
#include <stdio.h>

class ISimConsole {
public:
  virtual void Printf(const char *fmt, ...) = 0;
  virtual void VPrintf(const char *fmt, va_list argptr) = 0;
  virtual void Puts(const char*) = 0;
  virtual void Putc(const char) = 0;
  virtual char* Gets(char *, int) = 0;
};

class IUserInterface {
public:
  virtual ISimConsole &GetConsole() = 0;
  virtual void DisplayMessage(unsigned int uStringID, ...) = 0;
  virtual void DisplayMessage(FILE * pOut, unsigned int uStringID, ...) = 0;
  virtual void DisplayMessage(const char *fmt, ...) = 0;
  virtual void DisplayMessage(FILE * pOut, const char *fmt, ...) = 0;

};

extern "C" IUserInterface &GetUserInterface(void);

///
///   Gpsim string IDs
#define IDS_BREAK_READING_REG                 0
#define IDS_BREAK_READING_REG_VALUE           1
#define IDS_BREAK_READING_REG_OP_VALUE        2
#define IDS_BREAK_WRITING_REG                 3
#define IDS_BREAK_WRITING_REG_VALUE           4
#define IDS_BREAK_WRITING_REG_OP_VALUE        5
#define IDS_BREAK_ON_EXEC_ADDRESS             6
#define IDS_PROGRAM_FILE_PROCESSOR_NOT_KNOWN  7
#define IDS_FILE_NAME_TOO_LONG                8
#define IDS_FILE_NOT_FOUND                    9
#define IDS_FILE_BAD_FORMAT                   10
#define IDS_NO_PROCESSOR_SPECIFIED            11
#define IDS_PROCESSOR_INIT_FAILED             12
#define IDS_FILE_NEED_PROCESSOR_SPECIFIED     13


#endif
