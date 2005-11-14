#include "cmd_manager.h"
#include <sstream>
#include <algorithm>

//
//  CCommandManager
//////////////////////////////////////////////////

CCommandManager::CCommandManager() {
}

int CCommandManager::Execute(string &sName, const char *cmdline) {
  ICommandHandler *handler = find(sName.c_str());
  if (handler != NULL) {
    return handler->Execute(cmdline, &GetConsole());
  }
  return CMD_ERR_PROCESSORNOTDEFINED;
}

int CCommandManager::Register(ICommandHandler * ch) {
  List::iterator it = lower_bound(m_HandlerList.begin( ), m_HandlerList.end( ),
    ch, lessThan());
  if (it != m_HandlerList.end() &&
    strcmp((*it)->GetName(), ch->GetName()) == 0) {
    return CMD_ERR_PROCESSORDEFINED;
  }
  m_HandlerList.insert(it, ch);
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

void CCommandManager::ListToConsole() {
  ISimConsole &console = GetConsole();
  List::iterator it;
  List::iterator itEnd = m_HandlerList.end();
  for(it = m_HandlerList.begin( ); it != itEnd; it++) {
    console.Printf("%s\n", (*it)->GetName());
  }
}
