#ifndef __CMD_MANAGER_H__
#define __CMD_MANAGER_H__
#include <stdio.h>
#include "cmd_gpsim.h"
#include <vector>
#include <string>
#include <functional>
using namespace std;

class CGpsimConsole : public ISimConsole {
public:
  CGpsimConsole(FILE* pOut = NULL);
  void printf(const char *fmt, ...);
  void vprintf(const char *fmt, va_list argptr);
  void puts(const char*);
  void putc(const char);
  char* gets(char *, int);

  void SetOut(FILE *pOut);
  void SetIn(FILE *pIn);

protected:
  FILE * m_pfOut;
  FILE * m_pfIn;
};

/*
struct lessCommandManager : binary_function<ICommandHandler, ICommandHandler, bool> {
bool operator()(const string& _X, const string& _Y) const
	{return (_X < _Y); }
};
*/

class CommandHandlerKey : public ICommandHandler {
public:
  CommandHandlerKey(const char *name) {
    m_name = name;
  }
  virtual char *GetName(void) {return (char*)m_name; }
  virtual int Execute(const char * commandline, ISimConsole *out) {
    return CMD_ERR_COMMANDNOTDEFINED;}
  const char * m_name;
};



class CCommandManager {
public:
  CCommandManager(FILE *out = NULL, FILE *in = NULL);
  void  SetFileStream(FILE *out);
  int   Register(ICommandHandler * ch);
  int   Execute(string &sName, const char *cmdline);

  static CCommandManager m_CommandManger;
  static CCommandManager &GetManager();
  ICommandHandler * find(const char *name);

private:
  struct lessThan : binary_function<ICommandHandler*, ICommandHandler*, bool> {
    bool operator()(const ICommandHandler* left, const ICommandHandler* right) const {
      return strcmp(((ICommandHandler*)left)->GetName(),
        ((ICommandHandler*)right)->GetName()) < 0;
    }
  };

  // typedef map<string, ICommandHandler*, lessCommandManager> List;
  typedef vector<ICommandHandler*> List;

  List                  m_HandlerList;
  CGpsimConsole         m_Console;
};

#endif