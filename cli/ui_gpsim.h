
#ifndef __UI_GPSIM_H__
#define __UI_GPSIM_H__

#include "../src/cmd_gpsim.h"

class CGpsimConsole : public ISimConsole {
public:
  CGpsimConsole();
  void Printf(const char *fmt, ...);
  void VPrintf(const char *fmt, va_list argptr);
  void Puts(const char*);
  void Putc(const char);
  const char* Gets(char *, int);

  void SetOut(FILE *pOut);
  void SetIn(FILE *pIn);

protected:
  FILE * m_pfOut;
  FILE * m_pfIn;
};

#endif
