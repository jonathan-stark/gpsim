#include "cmd_manager.h"
#include <strstream>
#include <algorithm>

CGpsimConsole::CGpsimConsole(FILE*) {
}

void CGpsimConsole::printf(const char *fmt, ...) {
  va_list ap;

  va_start(ap,fmt);
  vfprintf(m_pfOut, fmt, ap);
  va_end(ap);
}

void CGpsimConsole::vprintf(const char *fmt, va_list argptr) {
  vfprintf(m_pfOut, fmt, argptr);
}

void CGpsimConsole::puts(const char*s) {
  fputs(s, m_pfOut);
}

void CGpsimConsole::putc(const char c) {
  fputc(c, m_pfOut);
}

void CGpsimConsole::SetOut(FILE *pOut) {
  m_pfOut = pOut;
}

void CGpsimConsole::SetIn(FILE *pIn) {
  m_pfIn = pIn;
}

char* CGpsimConsole::gets(char *s, int size) {
  return fgets(s, size, m_pfIn);
}


CCommandManager::CCommandManager(FILE *out, FILE *in) {
  m_Console.SetOut(out);
  m_Console.SetIn(in);
}

void CCommandManager::SetFileStream(FILE *out) {
  m_Console.SetOut(out);
}

int CCommandManager::Execute(string &sName, const char *cmdline) {
  ICommandHandler *handler = find(sName.c_str());
  if (handler != NULL) {
    SetFileStream(stdout);
    return handler->Execute(cmdline, &m_Console);
  }
  return CMD_ERR_ERROR;
}

int CCommandManager::Register(ICommandHandler * ch) {
  List::iterator it = lower_bound(m_HandlerList.begin( ), m_HandlerList.end( ),
    ch, lessThan());
  if (it != m_HandlerList.end() &&
    strcmp((*it)->GetName(), ch->GetName()) == 0) {
    return CMD_ERR_PROCESSORDEFINED;
  }
  m_HandlerList.insert(it, ch);
  /*
  List::iterator handler = m_HandlerList.find(sName);
  if (handler != m_HandlerList.end( )) {
    // name already found mangle a try again
    return CMD_ERR_PROCESSORDEFINED;
  }
  m_HandlerList[sName] = ch;
  */
  // m_HandlerList.insert(List::value_type(sName, ch));
  return CMD_ERR_OK;
}

ICommandHandler * CCommandManager::find(const char *name) {
  CommandHandlerKey key(name);
  List::iterator it = lower_bound(m_HandlerList.begin( ), m_HandlerList.end( ),
    (ICommandHandler*)&key, lessThan());
  if (it != m_HandlerList.end() &&
    strcmp((*it)->GetName(), name) == 0) {
      return *it;
  }
  return NULL;
}

CCommandManager CCommandManager::m_CommandManger;

CCommandManager &CCommandManager::GetManager() {
  return m_CommandManger;
}


