#include <string>
#include <sstream>


#include "ui_gpsim.h"
#include "../src/symbol.h"
#include "../src/cmd_manager.h"

const char * s_psEnglishMessages[] = {
  "",                                                     // Place holder so we don't have a zero
  "break reading register %s\n",                          // IDS_BREAK_READING_REG
  "break reading register %s with value %u\n",            // IDS_BREAK_READING_REG_VALUE
  "break reading register %s %s %u\n",                    // IDS_BREAK_READING_REG_OP_VALUE
  "break writing register %s\n",                          // IDS_BREAK_WRITING_REG
  "break writing register %s with value %u\n",            // IDS_BREAK_WRITING_REG_VALUE
  "break writing register %s %s %u\n",                    // IDS_BREAK_WRITING_REG_OP_VALUE
  "execution break at address %s\n",                      // IDS_BREAK_ON_EXEC_ADDRESS
  "unrecognized processor in the program file\n",         // IDS_PROGRAM_FILE_PROCESSOR_NOT_KNOWN
  "file name '%s' is too long\n",                         // IDS_FILE_NAME_TOO_LONG
  "file %s not found\n",                                  // IDS_FILE_NOT_FOUND
  "file %s is not formatted properly\n",                  // IDS_FILE_BAD_FORMAT
  "no processor has been specified\n",                    // IDS_NO_PROCESSOR_SPECIFIED
  "processor %s initialization failed\n",                 // IDS_PROCESSOR_INIT_FAILED
  "the program file type does not contain processor\n"    // first part of IDS_FILE_NEED_PROCESSOR_SPECIFIED
  "you need to specify processor with the processor command\n", // IDS_FILE_NEED_PROCESSOR_SPECIFIED
  "an appropriate list file for %s was not found\n",      // IDS_LIST_FILE_NOT_FOUND
  NULL,     // IDS_
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

  virtual const char * FormatProgramAddress(unsigned int uAddress);
  virtual const char * FormatRegisterAddress(unsigned int uAddress,
    unsigned int uMask);
  virtual const char * FormatLabeledValue(const char * pLabel,
    unsigned int uValue);
  virtual const char * FormatLabeledValue(const char * pLabel,
    unsigned int uValue, int iRadix, const char *pHexPrefix);
  virtual const char * FormatValue(unsigned int uValue);
  virtual const char * FormatValue(gint64 uValue);
  virtual const char * FormatValue(gint64 uValue, int iRadix);

  virtual void SetProgramAddressRadix(int iRadix);
  virtual void SetRegisterAddressRadix(int iRadix);
  virtual void SetValueRadix(int iRadix);

  static Integer  s_iValueRadix;
  static String   s_sValueHexPrefix;
  static Integer  s_iProgAddrRadix;
  static String   s_sProgAddrHexPrefix;
  static Integer  s_iRAMAddrRadix;
  static String   s_sRAMAddrHexPrefix;

protected:
  string        m_sLabeledAddr;
  string        m_sFormatValueGint64;

  const char ** m_paStrings;
  CGpsimConsole m_Console;

};

Integer CGpsimUserInterface::s_iValueRadix(       "UIValueRadix",             IUserInterface::eHex);
String  CGpsimUserInterface::s_sValueHexPrefix(   "UIValueHexPrefix",         "$");
Integer CGpsimUserInterface::s_iProgAddrRadix(    "UIProgamAddressRadix",     IUserInterface::eHex);
String  CGpsimUserInterface::s_sProgAddrHexPrefix("UIProgamAddressHexPrefix", "$");
Integer CGpsimUserInterface::s_iRAMAddrRadix(     "UIRAMAddressRadix",        IUserInterface::eHex);
String  CGpsimUserInterface::s_sRAMAddrHexPrefix( "UIRAMAddressHexPrefix",    "$");

CGpsimUserInterface s_GpsimUI(s_psEnglishMessages);

void initialize_ConsoleUI()
{
  CGpsimUserInterface::s_iValueRadix;
  CGpsimUserInterface::s_sValueHexPrefix;
  s_GpsimUI.SetStreams(stdin, stdout);
}

extern "C" IUserInterface & GetUserInterface(void) {
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

void CGpsimUserInterface::SetProgramAddressRadix(int iRadix) {
  s_iProgAddrRadix = iRadix;
}

void CGpsimUserInterface::SetRegisterAddressRadix(int iRadix) {
  s_iRAMAddrRadix = iRadix;
}

void CGpsimUserInterface::SetValueRadix(int iRadix) {
  s_iValueRadix = iRadix;
}


const char * CGpsimUserInterface::FormatProgramAddress(unsigned int uAddress) {
  const char * pLabel = get_symbol_table().findProgramAddressLabel(uAddress);
  return FormatLabeledValue(pLabel, uAddress, s_iProgAddrRadix, s_sProgAddrHexPrefix);
}

const char * CGpsimUserInterface::FormatRegisterAddress(unsigned int uAddress,
                                                        unsigned int uMask) {
  register_symbol * pRegSym = get_symbol_table().findRegisterSymbol(uAddress, uMask);
  const char * pLabel = pRegSym == NULL ? "" : pRegSym->name().c_str();
  return FormatLabeledValue(pLabel, uAddress, s_iRAMAddrRadix, s_sRAMAddrHexPrefix);
}

const char * CGpsimUserInterface::FormatLabeledValue(const char * pLabel,
                                                     unsigned int uValue) {
  return FormatLabeledValue(pLabel, uValue, s_iValueRadix, s_sValueHexPrefix);
}

const char * CGpsimUserInterface::FormatLabeledValue(const char * pLabel,
                                                     unsigned int uValue,
                                                     int iRadix,
                                                     const char *pHexPrefix) {

  ostringstream m_osLabeledAddr;
  string sPrefix;
  switch(iRadix) {
  case eHex:
    m_osLabeledAddr << hex;
    sPrefix = pHexPrefix;
    break;
  case eDec:
    m_osLabeledAddr << dec;
    break;
  case eOct:
    m_osLabeledAddr << oct;
    sPrefix = "0";
    break;
  }
  if(*pLabel != 0) {
    m_osLabeledAddr << pLabel << "(" << sPrefix << uValue << ")";
  }
  else {
    m_osLabeledAddr << sPrefix << uValue;
  }
  m_sLabeledAddr = m_osLabeledAddr.str();
  return m_sLabeledAddr.c_str();
}

const char * CGpsimUserInterface::FormatValue(unsigned int uValue) {
  return FormatLabeledValue(NULL, uValue, s_iValueRadix, s_sValueHexPrefix);
}

const char * CGpsimUserInterface::FormatValue(gint64 uValue) {
  return FormatValue(uValue, s_iValueRadix);
}

const char * CGpsimUserInterface::FormatValue(gint64 uValue, int iRadix) {
  ostringstream osValue;
  string sPrefix;
  switch(iRadix) {
  case eHex:
    osValue << hex;
    sPrefix = s_sValueHexPrefix;
    break;
  case eDec:
    osValue << dec;
    break;
  case eOct:
    osValue << oct;
    sPrefix = "0";
    break;
  }
  osValue << sPrefix << uValue;
  m_sFormatValueGint64 = osValue.str();
  return m_sFormatValueGint64.c_str();
}


