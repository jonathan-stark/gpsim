/*
   Copyright (C) 1998-2003 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

/*
  stuff that needs to be fixed:

  Register aliasing
  The "invalid instruction" in program memory.

*/

#include <stdio.h>
#ifdef _WIN32
#include "uxtime.h"
#else
#include <sys/time.h>
#endif
#include <time.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdio>

#include "../config.h"
#include "gpsim_def.h"
#include <glib.h>

#include "gpsim_classes.h"
#include "modules.h"
#include "processor.h"
#include "pic-processor.h"
#include "xref.h"
#include "attributes.h"

#include "fopen-path.h"
#include "cmd_gpsim.h"
#include "sim_context.h"

#include "clock_phase.h"

#include <typeinfo>

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//------------------------------------------------------------------------
// active_cpu  is a pointer to the pic processor that is currently 'active'.
// 'active' means that it's the one currently being simulated or the one
// currently being manipulated by the user (e.g. register dumps, break settings)

Processor *active_cpu = 0;

// create instances of inline get_active_cpu() and set_active_cpu() methods
// by taking theirs address
Processor *(*dummy_get_active_cpu)(void) = get_active_cpu;
void (*dummy_set_active_cpu)(Processor *act_cpu) = set_active_cpu;

static char pkg_version[] = PACKAGE_VERSION;

class CPU_Freq : public Float
{
public:
  CPU_Freq(Processor * _cpu, double freq); //const char *_name, double newValue, const char *desc);

  virtual void set(double d);

private:
  Processor * cpu;
};

CPU_Freq::CPU_Freq(Processor * _cpu, double freq)
  : Float("frequency",freq, " oscillator frequency."),
    cpu(_cpu)
{
}

void CPU_Freq::set(double d)
{
  pic_processor *pCpu = dynamic_cast<pic_processor *>(cpu);
  Float::set ( d );
  if ( cpu )
    cpu->update_cps();
  if ( pCpu )
    pCpu->wdt.update();
}


CPU_Vdd::CPU_Vdd(Processor * _cpu, double vdd)
  : Float("Vdd", vdd , "Processor supply voltage"),
    cpu(_cpu)
{
}

void CPU_Vdd::set(double d)
{
  Float::set ( d );
  if ( cpu )
    cpu->update_vdd();
}

//------------------------------------------------------------------------
//
// Processor - Constructor
//

Processor::Processor(const char *_name, const char *_desc)
  : Module(_name, _desc),
    pma(0),
    rma(this),
    ema(this),
    pc(0),
    bad_instruction(0, 0x3fff, 0)
{
  registers = 0;

  m_pConstructorObject = 0;
  m_Capabilities = 0;
  m_ProgramMemoryAllocationSize = 0;
  if(verbose)
    cout << "processor constructor\n";

  addSymbol(mFrequency = new CPU_Freq(this,20e6));

  set_ClockCycles_per_Instruction(4);
  update_cps();
  setWarnMode(true);
  setSafeMode(true);
  setUnknownMode(true);
  setBreakOnReset(true);

  // derived classes need to override these values
  m_uPageMask    = 0x00;
  m_uAddrMask    = 0xff;

  readTT = 0;
  writeTT = 0;

  interface = new ProcessorInterface(this);

  // let the processor version number simply be gpsim's version number.
  version = &pkg_version[0];

  get_trace().cycle_counter(get_cycles().get());

  addSymbol(m_pWarnMode = new WarnModeAttribute(this));
  addSymbol(m_pSafeMode = new SafeModeAttribute(this));
  addSymbol(m_pUnknownMode = new UnknownModeAttribute(this));
  addSymbol(m_pBreakOnReset = new BreakOnResetAttribute(this));

  m_vdd = new CPU_Vdd(this, 5.0);
  addSymbol(m_vdd);
  m_pbBreakOnInvalidRegisterRead = new Boolean("BreakOnInvalidRegisterRead",
    true, "Halt simulation when an invalid register is read from.");
  addSymbol(m_pbBreakOnInvalidRegisterRead);

  m_pbBreakOnInvalidRegisterWrite = new Boolean("BreakOnInvalidRegisterWrite",
    true, "Halt simulation when an invalid register is written to.");
  addSymbol(m_pbBreakOnInvalidRegisterWrite);
  set_Vdd(5.0);
}



//-------------------------------------------------------------------
Processor::~Processor()
{


  deleteSymbol(m_pbBreakOnInvalidRegisterRead);
  deleteSymbol(m_pbBreakOnInvalidRegisterWrite);
  deleteSymbol(m_pWarnMode);
  deleteSymbol(m_pSafeMode);
  deleteSymbol(m_pUnknownMode);
  deleteSymbol(m_pBreakOnReset);
  deleteSymbol(mFrequency);
  deleteSymbol(m_vdd);

  delete interface;

  delete_invalid_registers();

  delete m_UiAccessOfRegisters;
  delete []registers;
  delete readTT;
  delete writeTT;

  destroyProgramMemoryAccess(pma);


  for (unsigned int i = 0; i < m_ProgramMemoryAllocationSize; i++)
  {
    if (program_memory[i] != &bad_instruction)
    {
      delete program_memory[i];
    }
  }

  delete []program_memory;

}

unsigned long Processor::GetCapabilities() {
  return m_Capabilities;
}

//-------------------------------------------------------------------
// Simulation modes:
void Processor::setWarnMode(bool newWarnMode)
{
  bWarnMode = newWarnMode;
}
void Processor::setSafeMode(bool newSafeMode)
{
  bSafeMode = newSafeMode;
}
void Processor::setUnknownMode(bool newUnknownMode)
{
  bUnknownMode = newUnknownMode;
}
void Processor::setBreakOnReset(bool newBreakOnReset)
{
  bBreakOnReset = newBreakOnReset;
}

//------------------------------------------------------------------------
// Attributes

void Processor::set_frequency(double f)
{
  if(mFrequency)
    mFrequency->set(f);
  update_cps();
}
double Processor::get_frequency()
{
  double d=0.0;

  if(mFrequency)
    mFrequency->get(d);

  return d;
}

void Processor::update_cps(void)
{
  get_cycles().set_instruction_cps((guint64)(get_frequency()/clocks_per_inst));
}

double  Processor::get_OSCperiod()
{
  double f = get_frequency();

  if(f>0.0)
    return 1/f;
  else
    return 0.0;
}
/*
void Processor::set(const char *cP,int len)
{

}

void Processor::get(char *cP, int len)
{
  cP[0] = 0;
}
*/
//-------------------------------------------------------------------
//
// init_register_memory (unsigned int memory_size)
//
// Allocate an array for holding register objects.
//

void Processor::init_register_memory (unsigned int memory_size)
{

  if(verbose)
    cout << __FUNCTION__ << " memory size: " << memory_size << '\n';

  registers = new Register *[memory_size];
  m_UiAccessOfRegisters = new RegisterCollection(this,
                                                 "ramData",
                                                 registers,
                                                 memory_size);

  if (registers  == 0)
    {
      throw new FatalError("Out of memory - PIC register space");
    }


  // For processors with banked memory, the register_bank corresponds to the
  // active bank. Let this point to the beginning of the register array for
  // now.

  register_bank = registers;

  rma.set_Registers(registers, memory_size);

  // Make all of the file registers 'undefined' (each processor derived from this base
  // class defines its own register mapping).

  for (unsigned int i = 0; i < memory_size; i++)
    registers[i] = 0;


}

//-------------------------------------------------------------------
//
//
// create_invalid_registers
//
//   The purpose of this function is to complete the initialization
// of the file register memory by placing an instance of an 'invalid
// file register' at each 'invalid' memory location. Most of PIC's
// do not use the entire address space available, so this routine
// fills the voids.
//

void Processor::create_invalid_registers ()
{
  unsigned int addr;

  if(verbose)
    cout << "Creating invalid registers " << register_memory_size()<<"\n";

  // Now, initialize any undefined register as an 'invalid register'
  // Note, each invalid register is given its own object. This enables
  // the simulation code to efficiently capture any invalid register
  // access. Furthermore, it's possible to set break points on
  // individual invalid file registers. By default, gpsim halts whenever
  // there is an invalid file register access.

  for (addr = 0; addr < register_memory_size(); addr+=map_rm_index2address(1)) {

    unsigned int index = map_rm_address2index(addr);

    if (!registers[index]) {
      char nameBuff[100];
      snprintf(nameBuff,sizeof(nameBuff), "INVREG_%X",addr);


      registers[index] = new InvalidRegister(this, nameBuff);
      registers[index]->setAddress(addr);
    }
  }

}

//-------------------------------------------------------------------
//
// Delete invalid registers
//
void Processor::delete_invalid_registers ()
{
  unsigned int i=0;

  for (i = 0; i < rma.get_size(); i++) {
   // cout << __FUNCTION__ << "  reg: 0x"<<hex << i << " ptr:" << registers[i] << endl;
	
    InvalidRegister *pReg = dynamic_cast<InvalidRegister *> (registers[i]);
    if (pReg) {
      delete registers[i];
      registers[i]= 0;
      
    }
    else if (registers[i])
   {
      char reg_name[11];
      cout << __FUNCTION__ << "  reg: 0x"<<hex << i << " ptr:" << 
	registers[i] ;
      cout.flush();
      strncpy(reg_name, registers[i]->name().c_str(), 10);
      reg_name[10] = 0;
      cout << " " << reg_name <<endl;
   }

  }
}

//-------------------------------------------------------------------
//    add_file_registers
//
//  The purpose of this member function is to allocate memory for the
// general purpose registers.
//

void Processor::add_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
{

  unsigned int j;

  // Initialize the General Purpose Registers:
  Dprintf((" from 0x%x to 0x%x alias 0x%x\n", start_address, end_address, alias_offset));

  char str[100];
  for  (j = start_address; j <= end_address; j++) {

#ifdef DEBUG
    if (j == 0x11)
    {
	printf("Processor::add_file_registers j 0x%x\n", j);
    }
#endif
    if (registers[j] && (registers[j]->isa() == Register::INVALID_REGISTER))
	delete registers[j];
    else if (registers[j])
	cout << __FUNCTION__ << " Already register " << registers[j]->name() 
		<< " at 0x" << hex << j <<endl; 

    //The default register name is simply its address
    snprintf (str, sizeof(str), "REG%03X", j);
    registers[j] = new Register(this, str);

    if (alias_offset) {
      registers[j + alias_offset] = registers[j];
      registers[j]->alias_mask = alias_offset;
    } else
      registers[j]->alias_mask = 0;

    registers[j]->setAddress(j);
    RegisterValue rv = getWriteTT(j);
    registers[j]->set_write_trace(rv);
    rv = getReadTT(j);
    registers[j]->set_read_trace(rv);
  }

}

//-------------------------------------------------------------------
//    delete_file_registers
//
//  The purpose of this member function is to delete file registers
//

void Processor::delete_file_registers(unsigned int start_address,
                                      unsigned int end_address,
                                      bool bRemoveWithoutDelete)
{
#define DFR_DEBUG 0
  if (DFR_DEBUG)
    cout << __FUNCTION__
         << "  start:" << hex << start_address
         << "  end:" << hex << end_address
         << endl;

  //  FIXME - this function is bogus.
  // The aliased registers do not need to be searched for - the alias mask
  // can tell at what addresses a register is aliased.

#define SMALLEST_ALIAS_DISTANCE  32
#define ALIAS_MASK (SMALLEST_ALIAS_DISTANCE-1)
  unsigned int i,j;

  if (start_address != end_address)
      Dprintf(("from 0x%x to 0x%x\n", start_address, end_address));

  for (j = start_address; j <= end_address; j++) {
    if(registers[j]) {

      Register *thisReg = registers[j];
      Register *replaced = thisReg->getReplaced();

      if(thisReg->alias_mask) {
        // This register appears in more than one place. Let's find all
        // of its aliases.
        for(i=j&ALIAS_MASK; i<rma.get_size(); i+=SMALLEST_ALIAS_DISTANCE)
          if(thisReg == registers[i]) {
            if(DFR_DEBUG)
              cout << "   removing at address:" << hex << i << endl;
            registers[i] = 0;
          }
      }
      if(DFR_DEBUG)
        cout << " deleting: " << hex << j << endl;
      registers[j] = 0;
      if (!bRemoveWithoutDelete)
      {
	if (replaced) delete replaced;
        delete thisReg;
      }
    }
    else
	printf("%s register 0x%x already deleted\n", __FUNCTION__, j);
  }
}

//-------------------------------------------------------------------
//
//
//    alias_file_registers
//
//  The purpose of this member function is to alias the
// general purpose registers.
//

void Processor::alias_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
{

  unsigned int j;

  // FIXME -- it'd probably make better sense to keep a list of register addresses at
  // which a particular register appears.
#ifdef DEBUG
  if (start_address == 0x20)
	printf("DEBUG trace %x\n", start_address);
  if (start_address != end_address)
	Dprintf((" from 0x%x to 0x%x alias_offset 0x%x\n", start_address, end_address, alias_offset));
#endif

  for (j = start_address; j <= end_address; j++)
    {
      if (alias_offset && (j+alias_offset < rma.get_size()))
        {
	  if (registers[j + alias_offset])
	  {
		if (registers[j + alias_offset] == registers[j])
		{
		    printf("alias_file_register Duplicate alias %s from 0x%x to 0x%x \n",registers[j + alias_offset]->name().c_str(), j, j+alias_offset);
		}
		else
		{
		    delete registers[j + alias_offset];
		}
	   }
          registers[j + alias_offset] = registers[j];
          if (registers[j])
            registers[j]->alias_mask = alias_offset;
        }
    }

}

//-------------------------------------------------------------------
//
// init_program_memory(unsigned int memory_size)
//
// The purpose of this member function is to allocate memory for the
// pic's code space. The 'memory_size' parameter tells how much memory
// is to be allocated 
//
//  The following is not correct for 18f2455 and 18f4455 processors
//  so test has been disabled (RRR)
//
//  AND it should be an integer of the form of 2^n.
// If the memory size is not of the form of 2^n, then this routine will
// round up to the next integer that is of the form 2^n.
//
//   Once the memory has been allocated, this routine will initialize
// it with the 'bad_instruction'. The bad_instruction is an instantiation
// of the instruction class that chokes gpsim if it is executed. Note that
// each processor owns its own 'bad_instruction' object.

void Processor::init_program_memory (unsigned int memory_size)
{
  if(verbose)
    cout << "Initializing program memory: 0x"<<memory_size<<" words\n";
#ifdef RRR
  if ((memory_size-1) & memory_size)
    {
      cout << "*** WARNING *** memory_size should be of the form 2^N\n";
      memory_size = (memory_size + ~memory_size) & MAX_PROGRAM_MEMORY;
      cout << "gpsim is rounding up to memory_size = " << memory_size << '\n';
    }
#endif
  // Initialize 'program_memory'. 'program_memory' is a pointer to an array of
  // pointers of type 'instruction'. This is where the simulated instructions
  // are stored.
  program_memory = new instruction *[memory_size];
  if (program_memory == 0) {
    throw new FatalError("Out of memory for program space");
  }

  m_ProgramMemoryAllocationSize = memory_size;

  bad_instruction.set_cpu(this);
  for (unsigned int i = 0; i < memory_size; i++)
    program_memory[i] = &bad_instruction;

  pma = createProgramMemoryAccess(this);
  pma->name();
}

ProgramMemoryAccess * Processor::createProgramMemoryAccess(Processor *processor) {
  return new ProgramMemoryAccess(processor);
}

void Processor::destroyProgramMemoryAccess(ProgramMemoryAccess *pma) {
  delete pma;
}


//-------------------------------------------------------------------
// init_program_memory(int address, int value)
//
// The purpose of this member fucntion is to instantiate an Instruction
// object in the program memory. If the opcode is invalid, then a 'bad_instruction'
// is inserted into the program memory instead. If the address is beyond
// the program memory address space, then it may be that the 'opcode' is
// is in fact a configuration word.
//

void Processor::init_program_memory(unsigned int address, unsigned int value)
{
  unsigned int uIndex = map_pm_address2index(address);

  if (!program_memory) {
    std::stringstream buf;
    buf << "ERROR: internal bug " << __FILE__ << ":" << __LINE__;
    throw new FatalError(buf.str());
  }

  if(uIndex < program_memory_size()) {

    if(program_memory[uIndex] != 0 && program_memory[uIndex]->isa() != instruction::INVALID_INSTRUCTION) {
      // this should not happen
      delete program_memory[uIndex];
    }
    program_memory[uIndex] = disasm(address,value);
    if(program_memory[uIndex] == 0)
      program_memory[uIndex] = &bad_instruction;
    //program_memory[uIndex]->add_line_number_symbol();
  }
  else if (set_config_word(address, value))
    ;
  else
    set_out_of_range_pm(address,value);  // could be e2prom


}
//-------------------------------------------------------------------
//erase_program_memory(unsigned int address)
//
//	Checks if a program memory location contains an instruction
//	and deletes it if it does. 
//
void Processor::erase_program_memory(unsigned int address)
{
  unsigned int uIndex = map_pm_address2index(address);

  if (!program_memory) {
    std::stringstream buf;
    buf << "ERROR: internal bug " << __FILE__ << ":" << __LINE__;
    throw new FatalError(buf.str());
  }

  if(uIndex < program_memory_size()) {

    if(program_memory[uIndex] != 0 && program_memory[uIndex]->isa() != instruction::INVALID_INSTRUCTION) {
      delete program_memory[uIndex];
      program_memory[uIndex] = &bad_instruction;
    }
  }
  else
  {
    cout << "Erase Program memory\n";
    cout << "Warning::Out of range address " << hex << address << endl;
    cout << "Max allowed address is 0x" << hex << (program_address_limit()-1) << '\n';
  
  }


}

void Processor::init_program_memory_at_index(unsigned int uIndex, unsigned int value)
{
  init_program_memory(map_pm_index2address(uIndex), value);
}

void Processor::init_program_memory_at_index(unsigned int uIndex,
                                             const unsigned char *bytes, int nBytes)
{

  for (int i =0; i<nBytes/2; i++)
    init_program_memory_at_index(uIndex+i, (((unsigned int)bytes[2*i+1])<<8)  | bytes[2*i]);

}

//------------------------------------------------------------------
// Fetch the rom contents at a particular address.
unsigned int Processor::get_program_memory_at_address(unsigned int address)
{
  unsigned int uIndex = map_pm_address2index(address);


  return (uIndex < program_memory_size() && program_memory[uIndex])
    ? program_memory[uIndex]->get_opcode()
    : 0xffffffff;

}

//-------------------------------------------------------------------
// build_program_memory - given an array of opcodes this function
// will convert them into instructions and insert them into the
// simulated program memory.
//

void Processor::build_program_memory(unsigned int *memory,
                                     unsigned int minaddr,
                                     unsigned int maxaddr)
{

  for (unsigned int i = minaddr; i <= maxaddr; i++)
    if(memory[i] != 0xffffffff)
      init_program_memory(i, memory[i]);

}

//-------------------------------------------------------------------
/** @brief Write a word of data into memory outside flash
 *
 *  This method is called when loading data from the COD or HEX file
 *  and the address is not in the program ROM or normal config space.
 *  In this base class, there is no such memory. Real processors,
 *  particularly those with EEPROM, will need to override this method.
 *
 *  @param  address Memory address to set. Byte address on 18F
 *  @param  value   Word data to write in.
 */
void Processor::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  cout << "Warning::Out of range address " << address << " value " << value << endl;
  cout << "Max allowed address is 0x" << hex << (program_address_limit()-1) << '\n';

}


//-------------------------------------------------------------------
//
// attach_src_line - This member function establishes the one-to-one link
// between instructions and the source code that create them.

void Processor::attach_src_line(unsigned int address,
                                unsigned int file_id,
                                unsigned int sline,
                                unsigned int lst_line)
{
  unsigned int uIndex = map_pm_address2index(address);
  if(uIndex < program_memory_size())
    program_memory[uIndex]->update_line_number(file_id,sline,lst_line,-1,-1);
  else
      printf ( "%s:Address %03X out of range\n", __FUNCTION__, uIndex );
}

//-------------------------------------------------------------------
// read_src_files - this routine will open all of the source files
//   associated with the project and associate their line numbers
//   with the addresses of the opcodes they generated.
//

void Processor::read_src_files(void)
{
  int i;
  // Are there any src files ?
  for(i=0; i<files.nsrc_files(); i++) {


    FileContext *fc = files[i];

    // did this src file generate any code?
    if(fc && fc->max_line() > 0) {

      // Create an array whose index corresponds to the
      // line number of a source file line and whose data
      // is the offset (in bytes) from the beginning of the
      // file. (e.g. files[3].line_seek[20] references the
      // 20th line of the third source file.)

      fc->ReadSource();
    }
  }

  // Associate source files with the instructions they generated.
  unsigned int addr;
  for(addr = 0; addr<program_memory_size(); addr++) {

    if( (program_memory[addr]->isa() != instruction::INVALID_INSTRUCTION) &&
	(program_memory[addr]->get_file_id() >= 0)) {

      FileContext *fc = files[program_memory[addr]->get_file_id()];

      if(fc)
        fc->put_address(program_memory[addr]->get_src_line(),
                        map_pm_index2address(addr));

    }
  }

  // Associate the list file with
  if (files.list_id() >= 0) {

    // Parse the list file.
    //printf("read_src_files List file:%d %d\n",files.list_id(),files.nsrc_files());

    FileContext *fc = files[files.list_id()];
    if (!fc)
      return;

    fc->ReadSource();
    fc->rewind();

    char buf[256];
    int line = 1;

    while(fc->gets(buf,sizeof(buf))) {

      int address;
      int opcode;

      if (sscanf(buf,"%x   %x",&address, &opcode) == 2) {
        unsigned int uIndex = map_pm_address2index(address);
        if (uIndex < program_memory_size()) {
          program_memory[uIndex]->update_line_number(-1,-1,line,-1,-1);
          fc->put_address(line,address);
        }
      }

      line++;
    }

  }
}


//-------------------------------------------------------------------
//
// processor -- list
//
// Display the contents of either a source or list file
//
void Processor::list(unsigned int file_id,
                     unsigned int pc_val,
                     int start_line,
                     int end_line)
{


  if(files.nsrc_files() == 0)
    return;

  if(pc_val > program_memory_size())
    return;

  if(program_memory[pc_val]->isa() == instruction::INVALID_INSTRUCTION)
    {
      cout << "There's no code at address 0x" << hex << pc_val << '\n';
      return;
    }

  unsigned int line,pc_line;
  if(file_id)
    {
      file_id = files.list_id();
      line = program_memory[pc_val]->get_lst_line();
      pc_line = program_memory[pc->value]->get_lst_line();
    }
  else
    {
      file_id = program_memory[pc_val]->get_file_id();
      line = program_memory[pc_val]->get_src_line();
      pc_line = program_memory[pc->value]->get_src_line();
    }

  start_line += line;
  end_line += line;

  FileContext *fc = files[file_id];
  if(fc == NULL)
    return;

  start_line = (start_line < 0) ? 0 : start_line;
  end_line   = (end_line <= start_line) ? (start_line + 5) : end_line;
  end_line   = (end_line > (int)fc->max_line()) ? fc->max_line() : end_line;
  cout << " listing " << fc->name() << " Starting line " << start_line
       << " Ending line " << end_line << '\n';

  if (end_line == start_line)
    return;


  for(unsigned int i=start_line; i<=(unsigned int)end_line; i++)
  {

    char buf[256];

    fc->ReadLine(i, buf, sizeof(buf));

    if (pc_line == i)
      cout << "==>";
    else
      cout << "   ";

    cout << buf;
  }
}


static void trim(char * pBuffer) {
  size_t iPos = 0;
  char * pChar = pBuffer;
  while(*pChar != 0 && ::isspace(*pChar)) {
    pChar++;
  }
  if(pBuffer != pChar) {
    memmove(pBuffer, pChar, strlen(pBuffer) - iPos);
  }
  iPos = strlen(pBuffer);
  pChar = pBuffer + iPos - 1;
  while( pChar > pBuffer && ::isspace(*pChar)) {
    *pChar = 0;
    pChar--;
  }
}

//-------------------------------------------------------------------
//
// disassemble - Disassemble the contents of program memory from
// 'start_address' to 'end_address'. The instruction at the current
// PC is marked with an arrow '==>'. If an instruction has a break
// point set on it then it will be marked with a 'B'. The instruction
// mnemonics come from the class declarations for each instruction.
// However, it is possible to modify this on a per instruction basis.
// In other words, each instruction in the program memory has it's
// own instantiation. So a MOVWF at address 0x20 is different than
// one at address 0x21. It is possible to change the mnemonic of
// one without affecting the other. As of version 0.0.7 though, this
// is not implemented.
//

void Processor::disassemble (signed int s, signed int e)
{
  instruction *inst;
  int use_src_to_disasm = 0;

  if(s > e)
    return;

  unsigned int start_PMindex = map_pm_address2index(s);
  unsigned int end_PMindex   = map_pm_address2index(e);

  if(start_PMindex >= program_memory_size()) {
    if(s <0)
      start_PMindex = 0;
    else
      return;
  }
  if(end_PMindex  >= program_memory_size()) {
    if(e<0)
      return;
    else
      end_PMindex = program_memory_size()-1;
  }

  const int iConsoleWidth = 80;
  char str[iConsoleWidth];
  char str2[iConsoleWidth];
  if (!pc) {
    std::stringstream buf;
    buf << "ERROR: internal bug " << __FILE__ << ":" << __LINE__;
    throw new FatalError(buf.str());
  }
  unsigned uPCAddress = pc->get_value();
  const char *pszPC;
  char cBreak;
  ISimConsole &Console = GetUserInterface().GetConsole();

  int iLastFileId = -1;
  FileContext *fc = NULL;

  for(unsigned int PMindex = start_PMindex; PMindex<=end_PMindex; PMindex++) {

    unsigned int uAddress = map_pm_index2address(PMindex);
    str[0] = 0;
    pszPC = (uPCAddress == uAddress) ? "==>" : "   ";

    inst = program_memory[PMindex];

    // If this is not a "base" instruction then it has been replaced
    // with something like a break point.
    cBreak = ' ';
    if(!inst->isBase()) {
      cBreak = 'B';
      inst = pma->get_base_instruction(PMindex);
    }

    if(inst->get_file_id() != -1) {
      fc = files[inst->get_file_id()];
      if(iLastFileId != inst->get_file_id())
        Console.Printf("%s\n", fc->name().c_str());
      iLastFileId = inst->get_file_id();
    }
    else
      fc = 0;

    //const char *pLabel = get_symbol_table().findProgramAddressLabel(uAddress);
    //if(*pLabel != 0)
    //  cout << pLabel << ":" << endl;
    AddressSymbol *pAddr = dynamic_cast<AddressSymbol *>(inst->getLineSymbol());
    if (pAddr)
      cout << pAddr->name() << ':' << endl;

    if(fc && files.nsrc_files() && use_src_to_disasm) {
      char buf[256];

      files.ReadLine(inst->get_file_id(),
                     inst->get_src_line() - 1,
                     buf,
                     sizeof(buf));
      cout << buf;
    }  else {
      if(fc != NULL && inst->get_src_line() != -1) {
        if(fc->ReadLine(inst->get_src_line(), str2, iConsoleWidth - 33)
           != NULL) {
          trim(str2);
        }
        else {
          str2[0] = 0;
        }
      }
      else {
        str2[0] = 0;
      }
      inst->name(str, sizeof(str));
      char *pAfterNumonic = strchr(str, '\t');
      int iNumonicWidth = pAfterNumonic ? pAfterNumonic - str : 5;
      int iOperandsWidth = 14;
      int iSrc = iOperandsWidth - (strlen(str) - iNumonicWidth - 1);
      //        Console.Printf("0.........1.........2.........3.........4.........5.........6.........7.........");
      //        Console.Printf("%d, strlen(str)=%d\n", iNumonicWidth, strlen(str));
      const char *pFormat = (opcode_size() <=2) ?  "% 3s%c%04x  %04x  %s %*s%s\n" :  "% 3s%c%04x  %06x  %s %*s%s\n";
      Console.Printf(pFormat,
                     pszPC, cBreak, uAddress, inst->get_opcode(),
                     str, iSrc, "", str2);
    }
  }
}


//-------------------------------------------------------------------
void Processor::save_state(FILE *fp)
{

  if(!fp)
    return;

  unsigned int i;

  fprintf(fp,"PROCESSOR:%s\n",name().c_str());

  for(i=1; i<register_memory_size(); i++) {

    Register *reg = rma.get_register(i);

    if(reg && reg->isa() != Register::INVALID_REGISTER) {

      fprintf(fp,"R:%X:%s:(%X,%X)\n",
              reg->address,
              reg->name().c_str(),
              reg->value.get(),
              reg->value.geti());

    }
  }
  if(pc)
    fprintf(fp,"P:0:PC:%X\n",pc->value);
}

//-------------------------------------------------------------------
void Processor::save_state(void)
{

  unsigned int i;

  for(i=0; i<register_memory_size(); i++) {

    Register *reg = rma.get_register(i);

    if(reg && reg->isa() != Register::INVALID_REGISTER) {
      reg->put_trace_state(reg->getRV_notrace());
    }
  }

  if(pc)
    pc->put_trace_state(pc->value);

}

//-------------------------------------------------------------------
void Processor::load_state(FILE *fp)
{
  if(!fp)
    return;

  cout << "Not implemented\n";
}

/* If Vdd is changed, fix up the digital high low thresholds */
void Processor::update_vdd()
{
    IOPIN *pin;
    for(int i=1; i <= get_pin_count(); i++)
    {
	pin = get_pin(i);
	if (pin)
	{
	    pin->set_digital_threshold(get_Vdd());
	}
    }
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::find_closest_address_to_line(int file_id, int src_line)
{
  int closest_address = -1;

  if ((!cpu) || (file_id == -1))
    return closest_address;

  FileContext *fc = cpu->files[file_id];

  if(fc)
  {
    int offset=0;
    while((unsigned int)(src_line+offset)<fc->max_line())
    {
      closest_address = fc->get_address(src_line+offset);
      if(closest_address>=0)
        return closest_address;
      offset++;
    }
    offset=-1;
    while(src_line+offset>=0)
    {
      closest_address = fc->get_address(src_line+offset);
      if(closest_address>=0)
        return closest_address;
      offset--;
    }
  }

  return closest_address;


}
//-------------------------------------------------------------------
int ProgramMemoryAccess::find_address_from_line(FileContext *fc, int src_line)
{
  return (cpu && fc) ? fc->get_address(src_line) : -1;
}
//--------------------------------------------------------------------------
//
// temporary - this could is going to get deleted as soon as file related
// stuff gets put into its own object/class.

void ProgramMemoryAccess::set_hll_mode(unsigned int new_hll_mode)
{
  switch(new_hll_mode) {
  case ASM_MODE:
    hll_mode = ASM_MODE;
    break;
  case HLL_MODE:
    hll_mode = HLL_MODE;
  }
}
//--------------------------------------------------------------------------
int ProgramMemoryAccess::get_src_line(unsigned int address)
{
  unsigned int line=0;

  if(!cpu || !cpu->IsAddressInRange(address))
    return INVALID_VALUE;

  switch(get_hll_mode()) {

  case ASM_MODE:
    line = getFromAddress(address)->get_src_line();
    break;

  case HLL_MODE:
    line = getFromAddress(address)->get_hll_src_line();
    break;
  }

  return line;
}

//--------------------------------------------------------------------------
int ProgramMemoryAccess::get_file_id(unsigned int address)
{

  if(!cpu)
    return INVALID_VALUE;

  switch(get_hll_mode()) {

  case ASM_MODE:
    return getFromAddress(address)->get_file_id();
    break;

  case HLL_MODE:
    return getFromAddress(address)->get_hll_file_id();
    break;
  }

  return INVALID_VALUE;

}

//-------------------------------------------------------------------
unsigned int ProgramMemoryAccess::set_break_at_address(unsigned int address)
{
  if(hasValid_opcode_at_address(address))
    return bp.set_execution_break(cpu, address);

  return INVALID_VALUE;
}

//-------------------------------------------------------------------
unsigned int ProgramMemoryAccess::set_notify_at_address(unsigned int address, TriggerObject *cb)
{
  if(hasValid_opcode_at_address(address))
    return bp.set_notify_break(cpu, address, cb);

  return INVALID_VALUE;
}

//-------------------------------------------------------------------
unsigned int ProgramMemoryAccess::set_profile_start_at_address(unsigned int address,
                                                       TriggerObject *cb)
{
  unsigned int pm_index = cpu->map_pm_address2index(address);

  if(pm_index<cpu->program_memory_size())
    if (cpu->program_memory[pm_index]->isa() != instruction::INVALID_INSTRUCTION)
      return bp.set_profile_start_break(cpu, address, cb);

  return INVALID_VALUE;
}

//-------------------------------------------------------------------
unsigned int ProgramMemoryAccess::set_profile_stop_at_address(unsigned int address,
                                                      TriggerObject *cb)
{
  if(hasValid_opcode_at_address(address))
    return bp.set_profile_stop_break(cpu, address, cb);
  return INVALID_VALUE;

}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_break_at_address(unsigned int address,
                                                enum instruction::INSTRUCTION_TYPES type =
                                                instruction::BREAKPOINT_INSTRUCTION)
{
  unsigned int uIndex = cpu->map_pm_address2index(address);
  if ( uIndex<cpu->program_memory_size() ) {

    instruction *instr = find_instruction(address,type);
    if(instr!=0) {
      int b = ((Breakpoint_Instruction *)instr)->bpn & BREAKPOINT_MASK;
      // this actually removes the object
      bp.clear( b );
      return 1;
    }
  }

  return 0;
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_break_at_address(unsigned int address,
                                                instruction * pInstruction) {
  if(!cpu || !cpu->IsAddressInRange(address))
    return -1;

  instruction **ppAddressLocation = &cpu->program_memory[cpu->map_pm_address2index(address)];
  Breakpoint_Instruction *br = dynamic_cast<Breakpoint_Instruction *>(*ppAddressLocation);
  if (br == pInstruction) {
    // at the head of the chain
    *ppAddressLocation = br->getReplaced();
    br->setReplaced(0);
  }
  else {
    Breakpoint_Instruction *pLast = br;
    // Find myself in the chain
    while(br != NULL) {
      if (br == pInstruction) {
        // found -- remove from the chain
        pLast->setReplaced(br->getReplaced());
        br->setReplaced(0);
        return 1;
      }
      else {
        pLast = br;
        br = dynamic_cast<Breakpoint_Instruction *>(br->getReplaced());
      }
    }
  }
  return 0;
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_notify_at_address(unsigned int address)
{
  return clear_break_at_address(address,instruction::NOTIFY_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_profile_start_at_address(unsigned int address)
{
  return clear_break_at_address(address,instruction::PROFILE_START_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_profile_stop_at_address(unsigned int address)
{
  return clear_break_at_address(address,instruction::PROFILE_STOP_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::address_has_break(unsigned int address,
                                           enum instruction::INSTRUCTION_TYPES type)
{
  if(find_instruction(address,type)!=0)
    return 1;

  return 0;
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::address_has_notify(unsigned int address)
{
  return address_has_break(address,instruction::NOTIFY_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::address_has_profile_start(unsigned int address)
{
  return address_has_break(address,instruction::PROFILE_START_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::address_has_profile_stop(unsigned int address)
{
  return address_has_break(address,instruction::PROFILE_STOP_INSTRUCTION);
}


//-------------------------------------------------------------------
void ProgramMemoryAccess::toggle_break_at_address(unsigned int address)
{
  if(address_has_break(address))
    clear_break_at_address(address);
  else
    set_break_at_address(address);
}
//-------------------------------------------------------------------

void ProgramMemoryAccess::set_break_at_line(unsigned int file_id, unsigned int src_line)
{
  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
      set_break_at_address(address);
}

void ProgramMemoryAccess::clear_break_at_line(unsigned int file_id, unsigned int src_line)
{

  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
      clear_break_at_address(address);
}

void ProgramMemoryAccess::toggle_break_at_line(unsigned int file_id, unsigned int src_line)
{
  toggle_break_at_address(find_closest_address_to_line(file_id, src_line));
}

//-------------------------------------------------------------------
//
Processor * Processor::construct(void)
{

  cout << " Can't create a generic processor\n";

  return 0;

}




//-------------------------------------------------------------------
void Processor::trace_dump(int type, int amount)
{
  trace.dump(amount, stdout);
}

//-------------------------------------------------------------------
// Decode a single trace item
int Processor::trace_dump1(int type, char *buffer, int bufsize)
{
  snprintf(buffer, bufsize,"*** INVALID TRACE *** 0x%x",type);

  return 1;
}

//-------------------------------------------------------------------
// getWriteTT
//
// For devices with more than 64k of registers (which are not supported
// at the moment), we can enhance this function to return different Trace
// type objects base on the upper address bits.

RegisterValue Processor::getWriteTT(unsigned int j)
{
  if(!writeTT) {
    writeTT = new RegisterWriteTraceType(this,2);
    trace.allocateTraceType(writeTT);
  }

  // The upper 8-bits define the dynamically allocated trace type
  // The lower 8-bits will record the register value that is written.
  // The middle 16-bits are the register address
  unsigned int tt = (writeTT->type() & 0xff000000) | ((j & 0xffff) << 8);

  return RegisterValue(tt, tt + (1<<24) );
}

RegisterValue Processor::getReadTT(unsigned int j)
{
  if(!readTT) {
    readTT = new RegisterReadTraceType(this,2);
    trace.allocateTraceType(readTT);
  }

  // The upper 8-bits define the dynamically allocated trace type
  // The lower 8-bits will record the register value that is written.
  // The middle 16-bits are the register address
  unsigned int tt = (readTT->type() & 0xff000000) | ((j & 0xffff) << 8);

  return RegisterValue(tt, tt + (1<<24) );
}


//-------------------------------------------------------------------
//
// step_over - In most cases, step_over will simulate just one instruction.
// However, if the next instruction is a branching one (e.g. goto, call,
// return, etc.) then a break point will be set after it and gpsim will
// begin 'running'. This is useful for stepping over time-consuming calls.
//

void Processor::step_over (bool refresh)
{
  step(1,refresh); // Try one step
}

//-------------------------------------------------------------------

void Processor::run_to_address (unsigned int destination)
{


  if(simulation_mode != eSM_STOPPED) {
    if(verbose)
      cout << "Ignoring run-to-address request because simulation is not stopped\n";
    return;
  }
  // Set a temporary break point

  unsigned int bp_num = bp.set_execution_break(this, destination);
  run();
  bp.clear(bp_num);

}

//-------------------------------------------------------------------
//
//
//    create
//
//  The purpose of this member function is to 'create' a pic processor.
// Since this is a base class member function, only those things that
// are common to all pics are created.

void Processor::create (void)
{
    std::stringstream buf;
    buf << " a generic processor cannot be created " << __FILE__ << ":" << __LINE__;
    throw new FatalError(buf.str());
}

//-------------------------------------------------------------------
void Processor::dump_registers (void)
{
  //  parse_string("dump");

}

//-------------------------------------------------------------------
void Processor::Debug()
{
  cout << " === Debug === \n";
  if(pc)
    cout << "PC=0x"<<hex << pc->value << endl;
}


//-------------------------------------------------------------------
instruction *ProgramMemoryAccess::find_instruction(unsigned int address,
                                                   enum instruction::INSTRUCTION_TYPES type)
{
  unsigned int uIndex = cpu->map_pm_address2index(address);
  if(cpu->program_memory_size()<=uIndex)
    return 0;

  instruction *p = getFromIndex(uIndex);
  if(p->isa()==instruction::INVALID_INSTRUCTION)
    return 0;



  for(;;)
    {
      if(p->isa()==type)
        return p;

      switch(p->isa())
        {
        case instruction::MULTIWORD_INSTRUCTION:
        case instruction::INVALID_INSTRUCTION:
        case instruction::NORMAL_INSTRUCTION:
          return 0;
        case instruction::BREAKPOINT_INSTRUCTION:
        case instruction::NOTIFY_INSTRUCTION:
        case instruction::PROFILE_START_INSTRUCTION:
        case instruction::PROFILE_STOP_INSTRUCTION:
        case instruction::ASSERTION_INSTRUCTION:
          p=((Breakpoint_Instruction *)p)->getReplaced();
          break;
        }

    }

  return 0;
}


//-------------------------------------------------------------------
guint64 Processor::cycles_used(unsigned int address)
{
    return program_memory[address]->getCyclesUsed();
}

//-------------------------------------------------------------------
MemoryAccess::MemoryAccess(Processor *new_cpu)
{
  cpu = new_cpu;
}
MemoryAccess::~MemoryAccess()
{
}

Processor *MemoryAccess::get_cpu(void)
{
  return cpu;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
// Program memory interface used by the command line
class ProgramMemoryCollection : public IIndexedCollection
{
public:
  ProgramMemoryCollection(Processor *pProcessor,
                          const char *collection_name,
                          ProgramMemoryAccess *pPma);
  ~ProgramMemoryCollection();

  virtual unsigned int GetSize();
  virtual Value &GetAt(unsigned int uAddress, Value *pValue=0);
  virtual void SetAt(unsigned int uAddress, Value *pValue);
  virtual void ConsolidateValues(int &iColumnWidth,
                                 vector<string> &aList,
                                 vector<string> &aValue);
  virtual unsigned int GetLowerBound();
  virtual unsigned int GetUpperBound();
  virtual bool bIsIndexInRange(unsigned int uIndex);
private:
  Processor *   m_pProcessor;
  ProgramMemoryAccess   *m_pPma;
  Integer       m_ReturnValue;
};


ProgramMemoryCollection::ProgramMemoryCollection (Processor   *pProcessor,
                                                  const char  *pC_collection_name,
                                                  ProgramMemoryAccess *pPma) :
  IIndexedCollection(16), m_ReturnValue(0)
{
  m_pProcessor = pProcessor;
  Value::new_name(pC_collection_name);
  m_pPma = pPma;
  pProcessor->addSymbol(this);
}

ProgramMemoryCollection::~ProgramMemoryCollection()
{
  if (m_pProcessor)
    m_pProcessor->removeSymbol(this);
}

unsigned int ProgramMemoryCollection::GetSize()
{
  return m_pProcessor->program_memory_size();
}

Value &ProgramMemoryCollection::GetAt(unsigned int uAddress, Value *)
{
  //m_pProcessor->map_pm_address2index
  m_ReturnValue.set((int)m_pPma->get_rom(uAddress));
  m_ReturnValue.setBitmask( (1<<(m_pProcessor->opcode_size()*8)) - 1);
  ostringstream sIndex;
  sIndex << Value::name() << "[" << hex << m_szPrefix << uAddress << "]" << '\000';
  m_ReturnValue.new_name(sIndex.str().c_str());
  return m_ReturnValue;
}

void ProgramMemoryCollection::SetAt(unsigned int uAddress, Value *pValue)
{
  Integer *pInt = dynamic_cast<Integer*>(pValue);
  if(pInt == NULL) {
    throw new Error("rValue is not an Integer");
  }
  else {
    m_pPma->put_rom(uAddress, (unsigned int)(int)*pInt);
  }
}

void ProgramMemoryCollection::ConsolidateValues(int &iColumnWidth,
                                           vector<string> &aList,
                                           vector<string> &aValue)
{
  unsigned int  uFirstAddress = 0;
  unsigned int  uAddress;
  //Register *    pReg = m_ppRegisters[0];
  //Integer       uLastValue(pReg->getRV_notrace().data);
  Integer uLastValue(m_pPma->get_opcode(0));
  uLastValue.setBitmask((1<<(m_pProcessor->opcode_size()*8)) - 1);

  unsigned int uSize = GetSize();

  for(uAddress = 0; uAddress < uSize; uAddress++) {
    ostringstream sIndex;

    unsigned int ui_opcode = m_pPma->get_opcode(uAddress);

    if((unsigned int)uLastValue != ui_opcode) {
      PushValue(uFirstAddress, uAddress,
                &uLastValue, aList, aValue);
      iColumnWidth = max(iColumnWidth, (int)aList.back().size());
      uFirstAddress = uAddress;
      uLastValue = ui_opcode;
    }
  }
  uAddress--;
  // Record the last set of elements
  if(uFirstAddress <= uAddress) {
    PushValue(uFirstAddress, uAddress,
              &uLastValue, aList, aValue);
    iColumnWidth = max(iColumnWidth, (int)aList.back().size());
  }
}

//void RegisterCollection::SetAt(ExprList_t* pIndexers, Expression *pExpr) {
//  throw Error("RegisterCollection::SetAt() not implemented");
//}

unsigned int ProgramMemoryCollection::GetLowerBound()
{
  return 0;
}

unsigned int ProgramMemoryCollection::GetUpperBound()
{
  return GetSize() - 1;
}
bool ProgramMemoryCollection::bIsIndexInRange(unsigned int uAddress)
{
  return m_pPma->get_rom(uAddress) != 0xffffffff;
  // (uIndex >= GetLowerBound() &&  uIndex <= GetUpperBound()) ||

}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// ProgramMemoryAccess
//
// The ProgramMemoryAccess class provides an interface to the processor's
// program memory. On Pic processors, this is the memory where instructions
// are stored.
//

ProgramMemoryAccess::ProgramMemoryAccess(Processor *new_cpu) :
  MemoryAccess(new_cpu)
{
  init(new_cpu);
  m_pRomCollection = new ProgramMemoryCollection(new_cpu,
                                                 "romData",
                                                 this);
}
ProgramMemoryAccess::~ProgramMemoryAccess()
{
  delete m_pRomCollection;
}

void ProgramMemoryAccess::init(Processor *new_cpu)
{

  _address = _opcode = _state = 0;
  hll_mode = ASM_MODE;

  // add the 'main' pma to the list pma context's. Processors may
  // choose to add multiple pma's to the context list. The gui
  // will build a source browser for each one of these. The purpose
  // is to provide more than one way of debugging the code. (e.g.
  // this is useful for debugging interrupt versus non-interrupt code).

  if(cpu)
    cpu->pma_context.push_back(this);


}
/*
void ProgramMemoryAccess::name(string & new_name)
{
  name_str = new_name;
}
*/
void ProgramMemoryAccess::putToAddress(unsigned int address, instruction *new_instruction)
{
    putToIndex(cpu->map_pm_address2index(address), new_instruction);
}

void ProgramMemoryAccess::putToIndex(unsigned int uIndex, instruction *new_instruction)
{

  if(!new_instruction)
    return;

  cpu->program_memory[uIndex] = new_instruction;

  new_instruction->update();

}

void ProgramMemoryAccess::remove(unsigned int address, instruction *bp_instruction)
{
  if(!bp_instruction)
    return;

  instruction *instr = cpu->program_memory[cpu->map_pm_address2index(address)];
  if (typeid(Breakpoint_Instruction) == typeid(*instr) ||
    typeid(RegisterAssertion) == typeid(*instr)) {
    Breakpoint_Instruction* toRemove = (Breakpoint_Instruction*)bp_instruction;
    Breakpoint_Instruction *last = (Breakpoint_Instruction*)instr;
    if (toRemove == last) {
      cpu->program_memory[cpu->map_pm_address2index(address)] = last->getReplaced();
      return;
    }
    do {
      if(typeid(Breakpoint_Instruction) != typeid(*last->getReplaced()) &&
        typeid(RegisterAssertion) != typeid(*last->getReplaced()))
        // not found
        return;
      Breakpoint_Instruction *replaced = (Breakpoint_Instruction*)last->getReplaced();
      if(toRemove == replaced) {
        // remove from the chain
        last->setReplaced(replaced->getReplaced());
        return;
      }
      last = replaced;
    }
    while(typeid(Breakpoint_Instruction) != typeid(*last));
  }
  // if we get here the object was not in the list or was
  // not a Breakpoint_Instruction
//  assert(typeid(*instr) != typeid(Breakpoint_Instruction));
}

instruction *ProgramMemoryAccess::getFromAddress(unsigned int address)
{
  if(!cpu || !cpu->IsAddressInRange(address))
      return &cpu->bad_instruction;
  unsigned int uIndex = cpu->map_pm_address2index(address);
  return getFromIndex(uIndex);
}

instruction *ProgramMemoryAccess::getFromIndex(unsigned int uIndex)
{
  if(uIndex < cpu->program_memory_size())
    return cpu->program_memory[uIndex];
  else
    return 0;
}

// like get, but will ignore instruction break points
instruction *ProgramMemoryAccess::get_base_instruction(unsigned int uIndex)
{
    instruction *p;

    p=getFromIndex(uIndex);

    if(p==0)
        return 0;

    for(;;)
    {
        switch(p->isa())
        {
        case instruction::MULTIWORD_INSTRUCTION:
        case instruction::INVALID_INSTRUCTION:
        case instruction::NORMAL_INSTRUCTION:
                  return p;
        case instruction::BREAKPOINT_INSTRUCTION:
        case instruction::NOTIFY_INSTRUCTION:
        case instruction::PROFILE_START_INSTRUCTION:
        case instruction::PROFILE_STOP_INSTRUCTION:
        case instruction::ASSERTION_INSTRUCTION:
                  p=((Breakpoint_Instruction *)p)->getReplaced();
                  break;
        }

    }
    return 0;
}

//----------------------------------------
// get_rom - return the rom contents from program memory
//           If the address is normal program memory, then the opcode
//           of the instruction at that address is returned.
//           If the address is some other special memory (like configuration
//           memory in a PIC) then that data is returned instead.

unsigned int ProgramMemoryAccess::get_rom(unsigned int addr)
{
  return cpu->get_program_memory_at_address(addr);
}

//----------------------------------------
// put_rom - write new data to the program memory.
//           If the address is in normal program memory, then a new instruction
//           will be generated (if possible). If the address is some other
//           special memory (like configuration memory), then that area will
//           be updated.
//
void ProgramMemoryAccess::put_rom(unsigned int addr,unsigned int value)
{
  return cpu->init_program_memory(addr,value);
}

//----------------------------------------
// get_opcode - return an opcode from program memory.
//              If the address is out of range return 0.

unsigned int ProgramMemoryAccess::get_opcode(unsigned int addr)
{
  instruction * pInstr = getFromAddress(addr);
  if(pInstr != 0)
    return pInstr->get_opcode();
  else
    return 0;
}


//----------------------------------------
// get_opcode_name - return an opcode name from program memory.
//                   If the address is out of range return 0;

char *ProgramMemoryAccess::get_opcode_name(unsigned int addr, char *buffer, unsigned int size)
{

  unsigned int uIndex = cpu->map_pm_address2index(addr);
  if(uIndex < cpu->program_memory_size())
    return cpu->program_memory[uIndex]->name(buffer,size);

  *buffer = 0;
  return 0;
}

//----------------------------------------
// Get the current value of the program counter.
unsigned int ProgramMemoryAccess::get_PC(void)
{
  if(cpu && cpu->pc)
    return cpu->pc->get_value();

  return 0;
}

//----------------------------------------
// Get the current value of the program counter.
void ProgramMemoryAccess::set_PC(unsigned int new_pc)
{
  if(cpu && cpu->pc)
    return cpu->pc->put_value(new_pc);
}

Program_Counter *ProgramMemoryAccess::GetProgramCounter(void)
{
  if(cpu)
    return cpu->pc;

  return 0;
}

void ProgramMemoryAccess::put_opcode_start(unsigned int addr, unsigned int new_opcode)
{

  unsigned int uIndex = cpu->map_pm_address2index(addr);
  if( (uIndex < cpu->program_memory_size()) && (_state == 0))
    {
      _state = 1;
      _address = addr;
      _opcode = new_opcode;
      get_cycles().set_break_delta(40000, this);
      bp.set_pm_write();
    }

}

void ProgramMemoryAccess::put_opcode(unsigned int addr, unsigned int new_opcode)
{
  unsigned int uIndex = cpu->map_pm_address2index(addr);
  if(uIndex >= cpu->program_memory_size())
    return;


  instruction *old_inst = get_base_instruction(uIndex);
  instruction *new_inst = cpu->disasm(addr,new_opcode);

  if(new_inst==0)
  {
      puts("FIXME, in ProgramMemoryAccess::put_opcode");
      return;
  }

  if(!old_inst) {
    putToIndex(uIndex,new_inst);
    return;
  }

  if(old_inst->isa() == instruction::INVALID_INSTRUCTION) {
    putToIndex(uIndex,new_inst);
    return;
  }

  // Now we need to make sure that the instruction we are replacing is
  // not a multi-word instruction. The 12 and 14 bit cores don't have
  // multi-word instructions, but the 16 bit cores do. If we are replacing
  // the second word of a multiword instruction, then we only need to
  // 'uninitialize' it.

  // if there was a breakpoint set at addr, save a pointer to the breakpoint.
  Breakpoint_Instruction *b=bpi;
  instruction *prev = get_base_instruction(cpu->map_pm_address2index(addr-1));

  if(prev)
    prev->initialize(false);

  new_inst->update_line_number(old_inst->get_file_id(),
                               old_inst->get_src_line(),
                               old_inst->get_lst_line(),
                               old_inst->get_hll_src_line(),
                               old_inst->get_hll_file_id());


  if(b)
    b->setReplaced(new_inst);
  else
    cpu->program_memory[uIndex] = new_inst;

  cpu->program_memory[uIndex]->setModified(true);

  cpu->program_memory[uIndex]->update();

  delete(old_inst);
}

//--------------------------------------------------------------------------

void  ProgramMemoryAccess::assign_xref(unsigned int address, XrefObject * xref)
{

  instruction &q = *getFromAddress(address);
  if(q.isa()==instruction::INVALID_INSTRUCTION)
  {
    delete (int *)xref->data;
    delete xref;
    return;
   }


  q.add_xref(xref);
}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::step(unsigned int steps,bool refresh)
{

  if(!cpu)
    return;

  switch(get_hll_mode()) {

  case ASM_MODE:
    cpu->step(steps,refresh);
    break;

  case HLL_MODE:
    unsigned int initial_pc = cpu->pc->get_value();
    int initial_line = cpu->pma->get_src_line(initial_pc);
    int initial_file = cpu->pma->get_file_id(initial_pc);

    while(1) {
      cpu->step(1,false);

      unsigned int current_pc = cpu->pc->get_value();
      int current_line = cpu->pma->get_src_line(current_pc);
      int current_file = cpu->pma->get_file_id(current_pc);

      if(current_line<0 || current_file<0)
        continue;

      if(current_pc == initial_pc ||
         current_line != initial_line ||
         current_file != initial_file) {
        if(refresh)
          get_interface().simulation_has_stopped();
        break;
      }
    }
    break;
  }
}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::step_over(bool refresh)
{

  if(!cpu)
    return;

  switch(get_hll_mode()) {

  case ASM_MODE:
    cpu->step_over(refresh);
    break;

  case HLL_MODE:
    pic_processor *pic = dynamic_cast<pic_processor *>(cpu);
    if(!pic) {
      cout << "step-over is not supported for non-PIC processors\n";
      return;
    }

    unsigned int initial_pc = cpu->pc->get_value();
    int initial_line = cpu->pma->get_src_line(initial_pc);
    int initial_file = cpu->pma->get_file_id(initial_pc);
    unsigned int initial_stack_depth = pic->stack->pointer&pic->stack->stack_mask;

    while(1) {
      cpu->step(1,false);

      if(initial_stack_depth < (pic->stack->pointer&pic->stack->stack_mask))
        cpu->finish();

      unsigned int current_pc = cpu->pc->get_value();
      int current_line = cpu->pma->get_src_line(current_pc);
      int current_file = cpu->pma->get_file_id(current_pc);

      if(current_line<0 || current_file<0)
        continue;

      if(current_pc == initial_pc ||
         current_line != initial_line ||
         current_file != initial_file) {
        if(refresh)
          get_interface().simulation_has_stopped();
        break;
      }
    }
    break;
  }

}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::run(bool refresh)
{
  cpu->run(refresh);

}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::stop(void)
{
  bp.halt();
}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::finish(void)
{
  cpu->finish();
}


//--------------------------------------------------------------------------

bool  ProgramMemoryAccess::hasValid_opcode_at_address(unsigned int address)
{

  if(getFromAddress(address)->isa() != instruction::INVALID_INSTRUCTION)
    return true;

  return false;
}

bool  ProgramMemoryAccess::hasValid_opcode_at_index(unsigned int uIndex)
{

  if((getFromIndex(uIndex))->isa() != instruction::INVALID_INSTRUCTION)
    return true;

  return false;
}
//--------------------------------------------------------------------------

bool  ProgramMemoryAccess::isModified(unsigned int address)     // ***FIXME*** - address or index?
{
  unsigned int uIndex = cpu->map_pm_address2index(address);

  if((uIndex < cpu->program_memory_size()) &&
     cpu->program_memory[uIndex]->bIsModified())
    return true;

  return false;
}

//========================================================================
// Register Memory Access

RegisterMemoryAccess::RegisterMemoryAccess(Processor *new_cpu) :
  MemoryAccess(new_cpu)
{
  registers = 0;
  nRegisters = 0;
}

RegisterMemoryAccess::~RegisterMemoryAccess()
{

}

//--------------------------------------------------------------------------

Register *RegisterMemoryAccess::get_register(unsigned int address)
{

  if(!cpu || !registers || nRegisters<=address)
    return 0;

  Register *reg = registers[address];

  // If there are breakpoints set on the register, then drill down
  // through them until we get to the real register.

  return reg ? reg->getReg() : 0;
}

//--------------------------------------------------------------------------
void RegisterMemoryAccess::set_Registers(Register **_registers, int _nRegisters)
{
  nRegisters = _nRegisters;
  registers = _registers;
}
//------------------------------------------------------------------------
// insertRegister - Each register address may contain a linked list of registers.
// The top most register is the one that is referenced whenever a processor
// accesses the register memory. The primary purpose of the linked list is to
// support register breakpoints. For example, a write breakpoint is implemented
// with a breakpoint class derived from the register class. Setting a write
// breakpoint involves creating the write breakpoint object and placing it
// at the top of the register linked list. Then, when a processor attempts
// to write to this register, the breakpoint object will capture this and
// halt the simulation.

bool RegisterMemoryAccess::insertRegister(unsigned int address, Register *pReg)
{

  if(!cpu || !registers || nRegisters <= address ||!pReg)
    return false;

  Register *ptop = registers[address];
  pReg->setReplaced(ptop);
  registers[address] = pReg;

  return true;
}

//------------------------------------------------------------------------
// removeRegister - see comment on insertRegister. This method removes
// a register object from the breakpoint linked list.

bool RegisterMemoryAccess::removeRegister(unsigned int address, Register *pReg)
{
  if(!cpu || !registers || nRegisters <= address ||!pReg)
    return false;

  Register *ptop = registers[address];

  if (ptop == pReg  &&  pReg->getReplaced())
    registers[address] = pReg->getReplaced();
  else
    while (ptop) {
      Register *pNext = ptop->getReplaced();
      if (pNext == pReg) {
        ptop->setReplaced(pNext->getReplaced());
        return true;
      }
      ptop = pNext;
    }

  return false;
}

//-------------------------------------------------------------------
bool RegisterMemoryAccess::hasBreak(unsigned int address)
{

  if(!cpu || !registers || nRegisters <= address)
    return false;

  return registers[address]->isa() == Register::BP_REGISTER;

}

static InvalidRegister AnInvalidRegister(0,"AnInvalidRegister");

//-------------------------------------------------------------------
Register &RegisterMemoryAccess::operator [] (unsigned int address)
{

  if(!registers || get_size() <= address)
    return AnInvalidRegister;

  return *registers[address];
}

void RegisterMemoryAccess::reset (RESET_TYPE r)
{
  for(unsigned int i=0; i<nRegisters; i++)
  {
    // Do not reset aliased registers
    if ( !(operator[](i).alias_mask && (operator[](i).alias_mask & i)))
        operator[](i).reset(r);
  }
}

//========================================================================
// Processor Constructor

ProcessorConstructorList *ProcessorConstructorList::processor_list;

ProcessorConstructorList * ProcessorConstructorList::GetList() {
  return processor_list = ProcessorConstructor::GetList();
}

ProcessorConstructor::ProcessorConstructor(tCpuContructor _cpu_constructor,
                                           const char *name1,
                                           const char *name2,
                                           const char *name3,
                                           const char *name4)
{
  cpu_constructor = _cpu_constructor;  // Pointer to the processor constructor
  names[0] = name1;                    // First name
  names[1] = name2;                    //  and three aliases...
  names[2] = name3;
  names[3] = name4;
  // Add the processor to the list of supported processors:
  GetList()->push_back(this);
}

//------------------------------------------------------------
Processor * ProcessorConstructor::ConstructProcessor(const char *opt_name)
{
  // Instantiate a specific processor. If a name is provided, then that
  // will be used. Otherwise, the third name in the list of aliases for
  // this processor will be used instead. (Why 3rd?... Before optional
  // processor names were allowed, the default name matched what is now
  // the third alias; this maintains a backward compatibility).

  if (opt_name && strlen(opt_name))
    return cpu_constructor(opt_name);
  return cpu_constructor(names[2]);
}

ProcessorConstructorList * ProcessorConstructor::processor_list;

ProcessorConstructorList * ProcessorConstructor::GetList()
{
  if(processor_list == NULL) {
    processor_list = new ProcessorConstructorList();
  }
  return processor_list;
}



//------------------------------------------------------------
// findByType -- search through the list of supported processors for
//               the one matching 'name'.


ProcessorConstructor *ProcessorConstructorList::findByType(const char *name)
{
  ProcessorConstructorList::iterator processor_iterator;
  ProcessorConstructorList *pl = ProcessorConstructor::GetList();

  for (processor_iterator = pl->begin();
       processor_iterator != pl->end();
       ++processor_iterator) {

    ProcessorConstructor *p = *processor_iterator;
    for(int j=0; j<nProcessorNames; j++)
      if(p->names[j] && strcmp(name,p->names[j]) == 0)
        return p;
  }
  return 0;
}

//------------------------------------------------------------
// dump() --  Print out a list of all of the processors
//

string ProcessorConstructorList::DisplayString(void)
{
  ostringstream stream;
  list <ProcessorConstructor *> :: iterator processor_iterator;

  const int nPerRow = 4;   // Number of names to print per row.

  int i,j,k,longest;

  ProcessorConstructorList *pl = ProcessorConstructor::GetList();
  ProcessorConstructor *p;

  // loop through all of the processors and find the
  // one with the longest name

  longest = 0;

  for (processor_iterator = pl->begin();
       processor_iterator != pl->end();
       ++processor_iterator) {

    p = *processor_iterator;

    k = strlen(p->names[1]);
    if(k>longest)
      longest = k;

  }


  // Print the name of each processor.

  for (processor_iterator = pl->begin();
       processor_iterator != pl->end(); ) {

    for(i=0; i<nPerRow && processor_iterator != pl->end(); i++) {

      p = *processor_iterator++;
      stream << p->names[1];

      if(i<nPerRow-1) {

        // if this is not the last processor in the column, then
        // pad a few spaces to align the columns.

        k = longest + 2 - strlen(p->names[1]);
        for(j=0; j<k; j++)
          stream << ' ';
      }
    }
    stream << endl;
  }
  stream << ends;
  return string(stream.str());
}


//------------------------------------------------------------------------

FileContext::FileContext(string &new_name)
  : name_str(new_name)
{
  fptr = NULL;
  m_uiMaxLine = 0;
  m_bIsList = false;
  m_bIsHLL = false;
}

FileContext::FileContext(const char *new_name)
  : name_str(new_name)
{
  fptr = NULL;
  m_uiMaxLine = 0;
  m_bIsList = false;
  m_bIsHLL = false;
}

FileContext::~FileContext(void)
{
}

//----------------------------------------
// ReadSource
//
// This will open the file for this FileContext
// and fill the line_seek vector with the file
// positions corresponding to the start of every
// source line in the file.
//
// e.g. lineseek[20] describes where the 20'th source
// line is in the file.

void FileContext::ReadSource(void)
{

  if( (max_line() <= 0) || name_str.length() == 0)
    return;

  const char *str = name_str.c_str();

  // If the file is not open yet, then try to open it.
  if(!fptr)
    fptr = fopen_path(str,"r");

  // If the file still isn't open, then we have a problem
  // FIXME - should some corrective action be taken?
  if(!fptr) {
    cout << "Unable to open " << str << endl;
    return;
  }

  line_seek.resize(max_line()+1);
  pm_address.resize(max_line()+1);

  std::rewind(fptr);

  char buf[256],*s;
  line_seek[0] = 0;
  for(unsigned int j=1; j<=max_line(); j++) {
    pm_address[j] = -1;
    line_seek[j] = ftell(fptr);
    s = fgets(buf,256,fptr);

    if(s != buf)
      break;
  }
}

//----------------------------------------
// ReadLine
//
// Read one line from a source file.

char *FileContext::ReadLine(unsigned int line_number, char *buf, unsigned int nBytes)
{
  buf[0] = '\0';
  if(!fptr)
    return buf;

  fseek(fptr,
        line_seek[line_number],
        SEEK_SET);
  return fgets(buf, nBytes, fptr);

}

//----------------------------------------
//
char *FileContext::gets(char *buf, unsigned int nBytes)
{
  if(!fptr)
    return 0;

  return fgets(buf, nBytes, fptr);

}

//----------------------------------------
unsigned int FileContext::max_line()
{
  if(fptr && !m_uiMaxLine) {
    char buff[256];
    rewind();
    m_uiMaxLine=0;
    while(fgets(buff,sizeof(buff),fptr))
      m_uiMaxLine++;
  }
  return m_uiMaxLine;
}

//----------------------------------------
void FileContext::rewind()
{
  if(fptr)
    fseek(fptr,0,SEEK_SET);
}

//----------------------------------------
void FileContext::open(const char *mode)
{
  if(!fptr) {
    fptr = fopen_path(name_str.c_str(), mode);
    max_line();
  }
}
//----------------------------------------
void FileContext::close()
{
  if(fptr != NULL) {
    fclose(fptr);
    fptr = NULL;
  }
}

//----------------------------------------
int FileContext::get_address(unsigned int line_number)
{
  if(line_number <= max_line() && pm_address.size() > line_number)
    return pm_address[line_number];
  return -1;
}
//----------------------------------------
void FileContext::put_address(unsigned int line_number, unsigned int address)
{
  if(line_number <= max_line()  && pm_address.size() > line_number && pm_address[line_number]<0)
    pm_address[line_number] = address;
}

//------------------------------------------------------------------------


FileContextList::FileContextList()
{
  lastFile=0;
  list_file_id = -1;  // assume that no list file is present.
}

FileContextList::~FileContextList(void)
{
  FileContextList::iterator it;
  FileContextList::iterator itEnd = end();
  for (it = begin(); it != itEnd; ++it)
    it->close();
}

static bool EndsWith(string &sSubject, string &sSubstring)
{
  if(sSubject.size() < sSubstring.size()) {
    return false;
  }
  else {
    string sSubjectEnding = sSubject.substr(sSubject.size() -
                                            sSubstring.size());
    return sSubjectEnding == sSubstring;
  }
}

int FileContextList::Find(string &fname)
{
  if(lastFile) {
    for (int i = 0; i < lastFile; i++) {
      if(EndsWith((*this)[i]->name(), fname)) {
        return i;
      }
    }
  }
  return -1;
}

extern bool bHasAbsolutePath(string &fname);

int FileContextList::Add(string &new_name, bool hll)
{

  string sFull = bHasAbsolutePath(new_name) ? new_name : (sSourcePath + new_name);

  push_back(FileContext(sFull));
  back().setHLLId(hll);
  lastFile++;
  if(CSimulationContext::GetContext()->IsSourceEnabled()) {
    back().open("r");
    if(verbose)
      cout << "Added new file named: " << new_name
           << "  id = " << lastFile << endl;
  }

  return lastFile-1;
}

int FileContextList::Add(const char *new_name, bool hll)
{
  string sNewName(new_name);
  return Add (sNewName, hll);
}

FileContext *FileContextList::operator [] (int file_id)
{
  if(file_id<0 || file_id >= lastFile)
    return 0;
  return &this->_Myt::at(file_id);
}

char *FileContextList::ReadLine(int file_id, int line_number, char *buf, int nBytes)
{
  FileContext *fc = operator[](file_id);
  if(fc)
    return fc->ReadLine(line_number, buf, nBytes);

  buf[0] = '\0';
  return buf;
}

//----------------------------------------
//
char *FileContextList::gets(int file_id, char *buf, int nBytes)
{
  FileContext *fc = operator[](file_id);
  if(fc)
  return fc->gets(buf, nBytes);

  return 0;
}

//----------------------------------------
void FileContextList::rewind(int file_id)
{
  FileContext *fc = operator[](file_id);
  if(fc)
    return fc->rewind();

}

extern void EnsureTrailingFolderDelimiter(string &sPath);
extern void SplitPathAndFile(string &sSource, string &sFolder, string &sFile);

//----------------------------------------
void FileContextList::SetSourcePath(const char *pPath) {
  string sPath(pPath);
  string sFolder, sFile;
  SplitPathAndFile(sPath, sSourcePath, sFile);
  EnsureTrailingFolderDelimiter(sSourcePath);
}

//----------------------------------------
//
void FileContextList::list_id(int new_list_id)
{
  FileContext *fc = operator[](list_file_id);
  if (fc)
    fc->setListId(false);

  list_file_id = new_list_id;
  fc = operator[](list_file_id);
  if (fc)
    fc->setListId(true);

}
