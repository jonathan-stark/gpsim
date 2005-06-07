
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
  char* Gets(char *, int);

  void SetOut(FILE *pOut);
  void SetIn(FILE *pIn);

protected:
  FILE * m_pfOut;
  FILE * m_pfIn;
};

class CGpsimUserInterface : public IUserInterface {
public:
  CGpsimUserInterface(const char *paStrings[]);

  void CGpsimUserInterface::SetStreams(FILE *in, FILE *out);
  virtual ISimConsole &GetConsole();
  virtual void DisplayMessage(unsigned int uStringID, ...);
  virtual void DisplayMessage(FILE * pOut, unsigned int uStringID, ...);
  virtual void DisplayMessage(const char *fmt, ...);
  virtual void DisplayMessage(FILE * pOut, const char *fmt, ...);

protected:
  const char ** m_paStrings;
  CGpsimConsole m_Console;

};

extern "C" CGpsimUserInterface s_GpsimUI;

#endif
