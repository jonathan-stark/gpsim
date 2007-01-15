#include <string>
#include <ostream>
using namespace std;

#include "command.h"
#include "cmd_shell.h"
#include "../src/cmd_manager.h"

cmd_shell c_shell;

static cmd_options cmd_shell_options[] =
{
  {0,0,0}
};


cmd_shell::cmd_shell()
  : command("!",0)
{ 
  

  brief_doc = string("Shell out to another program or module's command line interface");

  long_doc = string ("!cmd.exe copy a.c b.c\n"
    "!picxx args\n"
    "\n");

  op = cmd_shell_options; 
}

string sTarget;
void cmd_shell::shell(String *cmd)
{
  sTarget = cmd->getVal();
  char *pArguments = (char *)sTarget.c_str();

  if(*pArguments == '\0') {
     CCommandManager::GetManager().ListToConsole();
  }
  else {

    while(pArguments != NULL && *pArguments != '\0' && !isspace(*pArguments))
      pArguments++;
    *pArguments = 0;
    pArguments++;
    int iResult;
    iResult = CCommandManager::GetManager().Execute(sTarget, pArguments);
    if (iResult == CMD_ERR_PROCESSORNOTDEFINED)
      printf("%s module command processor not found\n", sTarget.c_str());
  }
}
