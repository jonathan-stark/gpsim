#include "ui_gpsim.h"

const char * s_psEnglishMessages[] = {
  "break reading register 0x%04x\n",                      // IDS_BREAK_READING_REG
  "break reading register 0x%04x with value %u\n",        // IDS_BREAK_READING_REG_VALUE
  "break reading register 0x%04x %s %u\n",                // IDS_BREAK_READING_REG_OP_VALUE
  "break writing register 0x%04x\n",                      // IDS_BREAK_WRITING_REG
  "break writing register 0x%04x with value %u\n",        // IDS_BREAK_WRITING_REG_VALUE
  "break writing register 0x%04x %s %u\n",                // IDS_BREAK_WRITING_REG_OP_VALUE
  "execution break at address 0x%03x\n",                    // IDS_BREAK_ON_EXEC_ADDRESS
  NULL,     // IDS_
};

CGpsimUserInterface s_GpsimUI(s_psEnglishMessages);

extern "C" IUserInterface &GetUserInterface(void) {
  return s_GpsimUI;
}

///
///  CGpsimConsole
///  Connector between the gpsim console and the
///  console handler for the loaded modules.
//////////////////////////////////////////////////

CGpsimConsole::CGpsimConsole() {
}

void CGpsimConsole::Printf(const char *fmt, ...) {
  va_list ap;

  va_start(ap,fmt);
  vfprintf(m_pfOut, fmt, ap);
  va_end(ap);
}

void CGpsimConsole::VPrintf(const char *fmt, va_list argptr) {
  vfprintf(m_pfOut, fmt, argptr);
}

void CGpsimConsole::Puts(const char*s) {
  fputs(s, m_pfOut);
}

void CGpsimConsole::Putc(const char c) {
  fputc(c, m_pfOut);
}

char* CGpsimConsole::Gets(char *s, int size) {
  return fgets(s, size, m_pfIn);
}

void CGpsimConsole::SetOut(FILE *pOut) {
  m_pfOut = pOut;
}

void CGpsimConsole::SetIn(FILE *pIn) {
  m_pfIn = pIn;
}


///
///   CGpsimUserInterface
///
CGpsimUserInterface::CGpsimUserInterface(const char *paStrings[]) {
  m_paStrings = paStrings;
}

void CGpsimUserInterface::SetStreams(FILE *in, FILE *out) {
  m_Console.SetOut(out);
  m_Console.SetIn(in);
}

ISimConsole &CGpsimUserInterface::GetConsole(void) {
  return m_Console;
}

void CGpsimUserInterface::DisplayMessage(unsigned int uStringID, ...) {
  va_list ap;
  va_start(ap,uStringID);
  m_Console.VPrintf(m_paStrings[uStringID], ap);
  va_end(ap);
}

void CGpsimUserInterface::DisplayMessage(FILE * pOut, unsigned int uStringID, ...) {
  va_list ap;
  va_start(ap,uStringID);
  if (pOut == NULL || pOut == stdout) {
    m_Console.VPrintf(m_paStrings[uStringID], ap);
  }
  else {
    vfprintf(pOut, m_paStrings[uStringID], ap);
  }   
  va_end(ap);
}

void CGpsimUserInterface::DisplayMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap,fmt);
  m_Console.VPrintf(fmt, ap);
  va_end(ap);
}

void CGpsimUserInterface::DisplayMessage(FILE * pOut, const char *fmt, ...) {
  va_list ap;
  va_start(ap,fmt);
  if (pOut == NULL || pOut == stdout) {
    m_Console.VPrintf(fmt, ap);
  }
  else {
    vfprintf(pOut, fmt, ap);
  }   
  va_end(ap);
}

