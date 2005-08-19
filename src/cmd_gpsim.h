
#ifndef __CMD_GPSIM_H__
#define __CMD_GPSIM_H__

#include <stdarg.h>
#include <stdio.h>
#include "exports.h"
#include "glib.h"

#ifndef __REGISTERS_H__
#include "registers.h"
#endif

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
  enum {
    eHex,
    eDec,
    eOct,
  };

  virtual ISimConsole &GetConsole() = 0;
  virtual void DisplayMessage(unsigned int uStringID, ...) = 0;
  virtual void DisplayMessage(FILE * pOut, unsigned int uStringID, ...) = 0;
  virtual void DisplayMessage(const char *fmt, ...) = 0;
  virtual void DisplayMessage(FILE * pOut, const char *fmt, ...) = 0;

  virtual const char * FormatProgramAddress(unsigned int uAddress) = 0;
  virtual const char * FormatProgramAddress(unsigned int uAddress,
    unsigned int uMask, int iRadix) = 0;
  virtual const char * FormatRegisterAddress(unsigned int uAddress,
    unsigned int uMask) = 0;
  virtual const char * FormatLabeledValue(const char * pLabel,
    unsigned int uValue) = 0;
  virtual const char * FormatValue(unsigned int uValue) = 0;
  virtual const char * FormatValue(gint64 uValue) = 0;
  virtual const char * FormatValue(gint64 uValue, unsigned int uMask,
    int iRadix) = 0;

  virtual char *       FormatValue(char *str, int len,
    int iRegisterSize, RegisterValue value) = 0;

  virtual void SetProgramAddressRadix(int iRadix) = 0;
  virtual void SetRegisterAddressRadix(int iRadix) = 0;
  virtual void SetValueRadix(int iRadix) = 0;

  virtual void SetProgramAddressMask(unsigned int uMask) = 0;
  virtual void SetRegisterAddressMask(unsigned int uMask) = 0;
  virtual void SetValueMask(unsigned int uMask) = 0;

  virtual void NotifyExitOnBreak(int iExitCode) = 0;
};

extern "C" IUserInterface & GetUserInterface(void);

///
///   Gpsim string IDs
#define IDS_BREAK_READING_REG                 1
#define IDS_BREAK_READING_REG_VALUE           2
#define IDS_BREAK_READING_REG_OP_VALUE        3
#define IDS_BREAK_WRITING_REG                 4
#define IDS_BREAK_WRITING_REG_VALUE           5
#define IDS_BREAK_WRITING_REG_OP_VALUE        6
#define IDS_BREAK_ON_EXEC_ADDRESS             7
#define IDS_PROGRAM_FILE_PROCESSOR_NOT_KNOWN  8
#define IDS_FILE_NAME_TOO_LONG                9
#define IDS_FILE_NOT_FOUND                    10
#define IDS_FILE_BAD_FORMAT                   11
#define IDS_NO_PROCESSOR_SPECIFIED            12
#define IDS_PROCESSOR_INIT_FAILED             13
#define IDS_FILE_NEED_PROCESSOR_SPECIFIED     14
#define IDS_LIST_FILE_NOT_FOUND               15


#endif
