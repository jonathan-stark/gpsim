/*
   Copyright (C) 1998-2003 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

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

#include "../config.h"
#include "gpsim_def.h"
#include <unistd.h>
#include <glib.h>

#include "gpsim_classes.h"
#include "modules.h"
#include "processor.h"
#include "xref.h"
#include "attributes.h"

#include "fopen-path.h"
#include "cmd_gpsim.h"

//------------------------------------------------------------------------
// active_cpu  is a pointer to the pic processor that is currently 'active'. 
// 'active' means that it's the one currently being simulated or the one
// currently being manipulated by the user (e.g. register dumps, break settings)

Processor *active_cpu = 0;

// create instances of inline get_active_cpu() and set_active_cpu() methods
// by taking theirs address
static Processor *(*dummy_get_active_cpu)(void) = get_active_cpu;
static void (*dummy_set_active_cpu)(Processor *act_cpu) = set_active_cpu;

static char pkg_version[] = PACKAGE_VERSION;

//------------------------------------------------------------------------
//
// Processor - Constructor
//

Processor::Processor(void)
{
  m_Capabilities = 0;
  if(verbose)
    cout << "pic_processor constructor\n";

  pc = 0;

  mFrequency = new Float("frequency",20e6, " oscillator frequency.");
  set_ClockCycles_per_Instruction(1);
  set_Vdd(5.0);
  setWarnMode(true);
  setSafeMode(true);
  setUnknownMode(true);
  setBreakOnReset(true);

  readTT = 0;
  writeTT = 0;

  interface = new ProcessorInterface(this);

  // let the processor version number simply be gpsim's version number.
  version = &pkg_version[0];

  get_trace().cycle_counter(get_cycles().value);

}


//-------------------------------------------------------------------
Processor::~Processor()
{
  // register_bank points to current active bank
  // pc is allocated by the derived class
  delete []program_memory;
  delete registers;
  destroyProgramMemoryAccess(pma);
}

unsigned long Processor::GetCapabilities() {
  return m_Capabilities;
}


void Processor::initializeAttributes()
{
  add_attribute(new WarnModeAttribute(this));
  add_attribute(new SafeModeAttribute(this));
  add_attribute(new UnknownModeAttribute(this));
  add_attribute(new BreakOnResetAttribute(this));

  add_attribute(mFrequency);
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
}
double Processor::get_frequency()
{
  double d=0.0;

  if(mFrequency)
    mFrequency->get(d);

  return d;
}

double  Processor::get_OSCperiod()
{
  double f = get_frequency();

  if(f>0.0)
    return 1/f;
  else
    return 0.0;
}

void Processor::set(const char *cP,int len)
{

}

void Processor::get(char *cP, int len)
{
  cP[0] = 0;
}

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

  if (registers  == 0)
    {
      cout << "*** ERROR *** Out of memory - PIC register space\n";
      exit (1);
    }


  // For processors with banked memory, the register_bank corresponds to the
  // active bank. Let this point to the beginning of the register array for
  // now.

  register_bank = registers;

  rma.set_cpu(this);
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

void Processor::create_invalid_registers (void)
{
  unsigned int i;

  if(verbose)
    cout << "Creating invalid registers " << register_memory_size()<<"\n";

  // Now, initialize any undefined register as an 'invalid register'
  // Note, each invalid register is given its own object. This enables
  // the simulation code to efficiently capture any invalid register
  // access. Furthermore, it's possible to set break points on
  // individual invalid file registers. By default, gpsim halts whenever
  // there is an invalid file register access.

  for (i = 0; i < register_memory_size(); i++)
    {
      if (0 == registers[i])
      {
	  registers[i] = new InvalidRegister(i);
	  //registers[i]->address = 0;    // BAD_REGISTER;
	  registers[i]->alias_mask = 0;
	  registers[i]->value.put(0);	// unimplemented registers are read as 0

	  registers[i]->set_cpu(this);

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

  char str[100];
  for  (j = start_address; j <= end_address; j++) {

    registers[j] = new Register;
    if (alias_offset) {
      registers[j + alias_offset] = registers[j];
      registers[j]->alias_mask = alias_offset;
    } else
      registers[j]->alias_mask = 0;

    registers[j]->address = j;

    registers[j]->set_write_trace(getWriteTT(j));
    registers[j]->set_read_trace(getReadTT(j));

    //The default register name is simply its address
    sprintf (str, "0x%02x", j);
    registers[j]->new_name(str);

    registers[j]->set_cpu(this);

  }

}

//-------------------------------------------------------------------
//    delete_file_registers
//
//  The purpose of this member function is to delete file registers
//

void Processor::delete_file_registers(unsigned int start_address, unsigned int end_address)
{


  //  FIXME - this function is bogus.
  // The aliased registers do not need to be searched for - the alias mask
  // can tell at what addresses a register is aliased.

#define SMALLEST_ALIAS_DISTANCE  32
  unsigned int i,j;


  for (j = start_address; j <= end_address; j++) {
    if(registers[j]) {

      if(registers[j]->alias_mask) {
	// This registers appears in more than one place. Let's find all
	// of its aliases.
	for(i=SMALLEST_ALIAS_DISTANCE; i<register_memory_size(); i+=SMALLEST_ALIAS_DISTANCE)
	  if(registers[j] == registers[i])
	    registers[i] = 0;
      }

      delete registers[j];
      registers[j] = 0;
    }
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

  for (j = start_address; j <= end_address; j++)
    {
      if (alias_offset)
	{
	  registers[j + alias_offset] = registers[j];
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
// is to be allocated AND it should be an integer of the form of 2^n. 
// If the memory size is not of the form of 2^n, then this routine will
// round up to the next integer that is of the form 2^n.
//   Once the memory has been allocated, this routine will initialize
// it with the 'bad_instruction'. The bad_instruction is an instantiation
// of the instruction class that chokes gpsim if it is executed. Note that
// there is only one instance of 'bad_instruction'.

void Processor::init_program_memory (unsigned int memory_size)
{
  if(verbose)
    cout << "Initializing program memory: 0x"<<memory_size<<" words\n";
  if ((memory_size-1) & memory_size)
    {
      cout << "*** WARNING *** memory_size should be of the form 2^N\n";
      memory_size = (memory_size + ~memory_size) & MAX_PROGRAM_MEMORY;
      cout << "gpsim is rounding up to memory_size = " << memory_size << '\n';
    }
  // Initialize 'program_memory'. 'program_memory' is a pointer to an array of
  // pointers of type 'instruction'. This is where the simulated instructions
  // are stored.
  program_memory = new instruction *[memory_size];
  if (program_memory == 0)
    {
      cout << "*** ERROR *** Out of memory for program space\n";
      exit (1);
    }

  for (unsigned int i = 0; i < memory_size; i++)
    {
      program_memory[i] = &bad_instruction;
      program_memory[i]->set_cpu(this);
    }

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
  init_program_memory_at_index(map_pm_address2index(address), value);
}

void Processor::init_program_memory_at_index(unsigned int uIndex, unsigned int value)
{
  unsigned int address = map_pm_index2address(uIndex);
  if(uIndex < program_memory_size())
    {
      if(program_memory[uIndex] != 0 && program_memory[uIndex] != &bad_instruction) {
        // this should not happen
        delete program_memory[uIndex];
      }
      program_memory[uIndex] = disasm(address,value);
      if(program_memory[uIndex] == 0)
        program_memory[uIndex] = &bad_instruction;
      program_memory[uIndex]->add_line_number_symbol(address);
    }
  else
    set_out_of_range_pm(address,value);  // could be e2prom

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
void Processor::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  cout << "Warning::Out of range address " << address << " value " << value << endl;
  cout << "Max allowed address is " << (program_memory_size()-1) << '\n';

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
  if(uIndex < program_memory_size()) {

    program_memory[uIndex]->update_line_number(file_id,sline,lst_line,0,0);

    //printf("%s address=%x, File ID= %d, sline=%d, lst_line=%d\n", __FUNCTION__,address,file_id,sline,lst_line);

    FileContext *fc = files[file_id];

    if(fc && sline > fc->max_line())
      fc->max_line(sline);

    // If there's a listing file then handle it as well
    if(files.list_id() >=0 && lst_line != 0) {
      fc = files[files.list_id()];
    
      // FIX ME - what is this '+5' junk?
      if(fc && lst_line+5 > fc->max_line())
        fc->max_line(lst_line+5);
    }

  }

}

//-------------------------------------------------------------------
// read_src_files - this routine will open all of the source files 
//   associated with the project and associate their line numbers
//   with the addresses of the opcodes they generated.
//

void Processor::read_src_files(void)
{
#if USE_OLD_FILE_CONTEXT == 1

  // Are there any src files ?
  for(int i=0; i<number_of_source_files; i++) {


    // did this src file generate any code?
    if(files[i].max_line > 0) {
      
      // Create an array whose index corresponds to the
      // line number of a source file line and whose data
      // is the offset (in bytes) from the beginning of the 
      // file. (e.g. files[3].line_seek[20] references the
      // 20th line of the third source file.)
      files[i].line_seek = new int[files[i].max_line+1];

      // Create an array whose index is a source file line 
      // number and whose data is the program memory address
      // associated with this lines. Note, this array is initalized
      // to all -1's here, but later will get filled with the
      // proper addresses.

      files[i].pm_address = new int[files[i].max_line+1];

      if( 0 == (files[i].file_ptr = fopen_path(files[i].name,"r")))
        continue;

      rewind(files[i].file_ptr);

      char buf[256],*s;
      files[i].line_seek[0] = 0;
      for(int j=1; j<=files[i].max_line; j++) {
        files[i].pm_address[j] = -1;
        files[i].line_seek[j] = ftell(files[i].file_ptr);
        s = fgets(buf,256,files[i].file_ptr);
        if(s != buf)
	        break;
      }
    }
  }
#else

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

  unsigned int addr;
  for(addr = 0; addr<program_memory_size(); addr++) {

    if( (program_memory[addr]->isa() != instruction::INVALID_INSTRUCTION)) {

      FileContext *fc = files[program_memory[addr]->get_file_id()];
      
      if(fc)
        fc->put_address(program_memory[addr]->get_src_line(),
                        map_pm_index2address(addr));

    }
  }

#endif
}


//-------------------------------------------------------------------
//
// processor -- list
//
// Display the contents of either a source or list file
//
void Processor::list(unsigned int file_id, 
		     unsigned int pc_val, 
		     unsigned int start_line, 
		     unsigned int end_line)
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
      file_id = program_memory[pc_val]->file_id;
      line = program_memory[pc_val]->get_src_line();
      pc_line = program_memory[pc->value]->get_src_line();
    }

  start_line += line;
  end_line += line;

  FileContext *fc = files[file_id];
  if(fc == NULL)
    return;

  if(start_line < 0) start_line = 0;

  if(end_line > fc->max_line())
    end_line = fc->max_line();

  cout << " listing " << fc->name() << " Starting line " << start_line
       << " Ending line " << end_line << '\n';


  for(unsigned int i=start_line; i<=end_line; i++)
  {

    char buf[256];

    fc->ReadLine(i,
		    buf,
		    sizeof(buf));

    if (pc_line == i)
      cout << "==>";
    else
      cout << "   ";

    cout << buf;
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

  if(s >= e)
    return;

  unsigned int start_address = map_pm_address2index(s);
  unsigned int end_address = map_pm_address2index(e);

  if(start_address >= program_memory_size()) {
    if(s <0)
      start_address = 0;
    else 
      return;
  }
  if(end_address  >= program_memory_size()) {
    if(e<0)
      return;
    else
      end_address = program_memory_size()-1;
  }

  char str[50];

  for(unsigned int i = start_address; i<=end_address; i++)
    {
      str[0] =0;
      if (map_pm_address2index(pc->get_value()) == i)
        cout << "==>";
      else
        cout << "   ";
      inst = program_memory[i];

      // Breakpoints replace the program memory with an instruction that has
      // an opcode larger than 16 bits.

      if(program_memory[i]->opcode < 0x10000)
      {
        cout << ' ';
      }
      else
      {
        cout << 'B';
        Breakpoint_Instruction *bpi =  (Breakpoint_Instruction *)program_memory[i];
        inst = bpi->replaced;
      }

      if(files.nsrc_files() && use_src_to_disasm)
      {
        char buf[256];

        files.ReadLine(program_memory[i]->file_id,
            program_memory[i]->src_line - 1,
            buf,
            sizeof(buf));
        cout << buf;
      }
      else
        cout << hex << setw(4) << setfill('0') << map_pm_index2address(i) << "  "
             << hex << setw(4) << setfill('0') << inst->opcode << "    "
             << inst->name(str,sizeof(str)) << '\n';
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
//-------------------------------------------------------------------
int ProgramMemoryAccess::find_closest_address_to_line(int file_id, int src_line)
{
  int closest_address = -1;

  if(!cpu)
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

#if 0
  int distance = cpu->program_memory_size();

  for(int i = cpu->program_memory_size()-1; i>=0; i--) {

    // Find the closet instruction to the src file line number
    if( (cpu->program_memory[i]->isa() != instruction::INVALID_INSTRUCTION) &&
	(cpu->program_memory[i]->get_file_id()==file_id)                    &&
	(abs(cpu->program_memory[i]->get_src_line() - src_line) < distance))

      {
	distance = abs(cpu->program_memory[i]->get_src_line() - src_line);
	closest_address = i;

	if(distance == 0)
	  return cpu->map_pm_index2address(closest_address);
      }
  }

  return cpu->map_pm_index2address(closest_address);
#endif

}
//-------------------------------------------------------------------
int ProgramMemoryAccess::find_address_from_line(FileContext *fc, int src_line)
{
  if(!cpu)
    return -1;

  if(fc)
    return fc->get_address(src_line);
  return -1;
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
unsigned int ProgramMemoryAccess::get_src_line(unsigned int address)
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
unsigned int ProgramMemoryAccess::get_file_id(unsigned int address)
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
void ProgramMemoryAccess::set_break_at_address(unsigned int address)
{
  if(hasValid_opcode_at_address(address))
    bp.set_execution_break(cpu, address);
}

//-------------------------------------------------------------------
void ProgramMemoryAccess::set_notify_at_address(unsigned int address, TriggerObject *cb)
{
  if(hasValid_opcode_at_address(address))
    bp.set_notify_break(cpu, address, cb);

}

//-------------------------------------------------------------------
void ProgramMemoryAccess::set_profile_start_at_address(unsigned int address,
						       TriggerObject *cb)
{
  unsigned int pm_index = cpu->map_pm_address2index(address);

  if(pm_index<cpu->program_memory_size()) 
    if (cpu->program_memory[pm_index]->isa() != instruction::INVALID_INSTRUCTION)
      bp.set_profile_start_break(cpu, address, cb);
}

//-------------------------------------------------------------------
void ProgramMemoryAccess::set_profile_stop_at_address(unsigned int address,
						      TriggerObject *cb)
{
  if(hasValid_opcode_at_address(address))
    bp.set_profile_stop_break(cpu, address, cb);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_break_at_address(unsigned int address, 
						enum instruction::INSTRUCTION_TYPES type = 
						instruction::BREAKPOINT_INSTRUCTION)
{
  unsigned int uIndex = cpu->map_pm_address2index(address);
  if( uIndex >= 0  && uIndex<cpu->program_memory_size()) {

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
    *ppAddressLocation = br->replaced;
  }
  else {
    Breakpoint_Instruction *pLast = br;
    // Find myself in the chain
    while(br != NULL) {
      if (br == pInstruction) {
        // found
        pLast->replaced = br->replaced;
        // for good measure
        br->replaced = NULL;
        return 1;
      }
      else {
        pLast = br;
        br = dynamic_cast<Breakpoint_Instruction *>(br->replaced);
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

unsigned int Processor::getWriteTT(unsigned int j)
{
  if(!writeTT) {
    writeTT = new RegisterWriteTraceType(this,0,1);
    trace.allocateTraceType(writeTT);
  }

  // The upper 8-bits define the dynamically allocated trace type
  // The lower 8-bits will record the register value that is written.
  // The middle 16-bits are the register address
  
  return writeTT->type = (writeTT->type & 0xff000000) | ((j & 0xffff) << 8);

}

unsigned int Processor::getReadTT(unsigned int j)
{
  if(!readTT) {
    readTT = new RegisterReadTraceType(this,0,1);
    trace.allocateTraceType(readTT);
  }

  // The upper 8-bits define the dynamically allocated trace type
  // The lower 8-bits will record the register value that is written.
  // The middle 16-bits are the register address

  return (readTT->type & 0xff000000) | ((j & 0xffff) << 8);
}

//-------------------------------------------------------------------
//
// run  -- Begin simulating and don't stop until there is a break.
// FIXME - shouldn't this be a pure virtual function?
void Processor::run (bool refresh)
{

  cout << __FUNCTION__ << endl;
}

//-------------------------------------------------------------------
//
// step - Simulate one (or more) instructions. If a breakpoint is set
// at the current PC-> 'step' will go right through it. (That's supposed
// to be a feature.)
//
void Processor::step (unsigned int steps, bool refresh)
{
  if(!steps)
    return;

  if(simulation_mode != eSM_STOPPED) {
    if(verbose)
      cout << "Ignoring step request because simulation is not stopped\n";
    return;
  }

  simulation_mode = eSM_SINGLE_STEPPING;
  do
    {

      if(bp.have_sleep() || bp.have_pm_write())
	{
	  // If we are sleeping or writing to the program memory (18cxxx only)
	  // then step one cycle - but don't execute any code  

	  get_cycles().increment();
	  if(refresh)
	    trace_dump(0,1);

	}
      else if(bp.have_interrupt())
	{
	  interrupt();
	}
      else
	{

	  step_one(refresh);
	  get_trace().cycle_counter(get_cycles().value);
	  if(refresh)
	    trace_dump(0,1);

	} 

    }
  while(!bp.have_halt() && --steps>0);

  bp.clear_halt();
  simulation_mode = eSM_STOPPED;

  if(refresh)
    get_interface().simulation_has_stopped();
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
//
// finish
//
// this method really only applies to processors with stacks.

void Processor::finish(void)
{

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
  cout << " a generic processor cannot be created " << __FILE__ << __LINE__ <<endl;
  exit(1);
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
	  p=((Breakpoint_Instruction *)p)->replaced;
	  break;
	}

    }

  return 0;
}


//-------------------------------------------------------------------
guint64 Processor::cycles_used(unsigned int address)
{
    return program_memory[address]->cycle_count;
}

//-------------------------------------------------------------------
MemoryAccess::MemoryAccess(Processor *new_cpu)
{
  cpu = new_cpu;
}

Processor *MemoryAccess::get_cpu(void)
{
  return cpu;
}

void MemoryAccess::set_cpu(Processor *p)
{ 
  cpu = p;
}



//-------------------------------------------------------------------
//-------------------------------------------------------------------
//


ProgramMemoryAccess::ProgramMemoryAccess(Processor *new_cpu) :
  MemoryAccess(new_cpu)
{
  init(new_cpu);
}

void ProgramMemoryAccess::init(Processor *new_cpu)
{

  set_cpu(new_cpu);

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

 
  if(hasValid_opcode_at_index(uIndex)) {

    cpu->program_memory[uIndex] = new_instruction;

    new_instruction->update();
  }

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
      cpu->program_memory[cpu->map_pm_address2index(address)] = last->replaced;
      return;
    }
    do {
      if(typeid(Breakpoint_Instruction) != typeid(*last->replaced) &&
        typeid(RegisterAssertion) != typeid(*last->replaced))
        // not found
        return;
      Breakpoint_Instruction *replaced = (Breakpoint_Instruction*)last->replaced;
      if(toRemove == replaced) {
        // remove from the chain
        last->replaced = replaced->replaced;
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
      return &bad_instruction;
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
	          p=((Breakpoint_Instruction *)p)->replaced;
                  break;
        }

    }
    return 0;
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

  if(addr < cpu->program_memory_size())
    return cpu->program_memory[addr]->name(buffer,size);

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

  if( (addr < cpu->program_memory_size()) && (_state == 0))
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

  //new_inst->xref = old_inst->xref;

  if(b) 
    b->replaced = new_inst;
  else
    cpu->program_memory[uIndex] = new_inst;

  cpu->program_memory[uIndex]->is_modified=1;
  
  //if(cpu->program_memory[addr]->xref)
  cpu->program_memory[uIndex]->update();
  
  delete(old_inst);
}

//--------------------------------------------------------------------------

void  ProgramMemoryAccess::assign_xref(unsigned int address, gpointer xref)
{

  instruction &q = *getFromAddress(address);
  if(q.isa()==instruction::INVALID_INSTRUCTION)
    return;

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
    {
      unsigned int initial_line = cpu->pma->get_src_line(cpu->pc->get_value());
      unsigned int initial_pc = cpu->pc->get_value();

      while(1) {
	cpu->step(1,false);

	if(( cpu->pc->get_value() == initial_pc) ||
	   (get_src_line(cpu->pc->get_value()) != initial_line)) {
	  if(refresh)
	    get_interface().simulation_has_stopped();
	  break;
	}
      }

      break;
    }
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
    {

#if 0
      //
      // High level language step-overs are only supported for 
      // pic processors
      //

      pic_processor *pic = dynamic_cast<pic_processor *>cpu;

      if(!pic) {
	cout << "step-over is not supported for non-PIC processors\n";
	return;
      }
      unsigned int initial_line = cpu->pma.get_src_line(cpu->pc->get_value());
      unsigned int initial_pc = cpu->pc->get_value();
      unsigned int initial_stack_depth = pic->stack->pointer&pic->stack->stack_mask;

      while(1)
	{
	  step(1);

	  if(initial_stack_depth < pic->stack->pointer&pic->stack->stack_mask)
	    gpsim_finish(processor_id);

	  if(pic->pc->get_value()==initial_pc)
	    break;

	  if(get_src_line(initial_pc) != initial_line)
	    {
	      if(gpsim_get_file_id(processor_id, pic->pc->get_value()))
		break;
	    }
	}
      break;
#endif
      cout << "HLL Step is not supported\n";
    }
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

bool  ProgramMemoryAccess::isModified(unsigned int address)
{

  if((address < cpu->program_memory_size()) && 
     cpu->program_memory[address]->is_modified)
    return true;

  return false;
}

//========================================================================
// Register Memory Access 

RegisterMemoryAccess::RegisterMemoryAccess(Processor *new_cpu) :
  MemoryAccess(new_cpu)
{
  cpu = 0;
  registers = 0;
  nRegisters = 0;
}

RegisterMemoryAccess::~RegisterMemoryAccess()
{
  // registers memory is owned by the Processor class
}

//--------------------------------------------------------------------------

Register *RegisterMemoryAccess::get_register(unsigned int address)
{

  if(!cpu || !registers || nRegisters<=address)
    return 0;

  Register *reg = registers[address];

  // If there are breakpoints set on the register, then drill down
  // through them until we get to the real register.

  while(reg->isa() == Register::BP_REGISTER)
    reg = ((BreakpointRegister *)reg)->replaced;


  return reg;

}

//--------------------------------------------------------------------------
void RegisterMemoryAccess::set_Registers(Register **_registers, int _nRegisters) 
{ 
  nRegisters = _nRegisters; 
  registers = _registers;
}
//-------------------------------------------------------------------
bool RegisterMemoryAccess::hasBreak(unsigned int address)
{

  if(!cpu || !registers || nRegisters<=address)
    return false;

  return registers[address]->isa() == Register::BP_REGISTER;

}

static InvalidRegister AnInvalidRegister;

//-------------------------------------------------------------------
Register &RegisterMemoryAccess::operator [] (unsigned int address)
{

  if(!registers || get_size()<=address)
    return AnInvalidRegister;

  return *registers[address];
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

Processor * ProcessorConstructor::ConstructProcessor() {
  return cpu_constructor();
}

ProcessorConstructorList * ProcessorConstructor::processor_list = new ProcessorConstructorList();

ProcessorConstructorList * ProcessorConstructor::GetList() {
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
  for (processor_iterator = processor_list->begin();
       processor_iterator != processor_list->end(); 
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

void ProcessorConstructorList::dump(void)
{

  list <ProcessorConstructor *> :: iterator processor_iterator;

  const int nPerRow = 4;   // Number of names to print per row.

  int i,j,k,longest;

  ProcessorConstructor *p;


  // loop through all of the processors and find the 
  // one with the longest name

  longest = 0;

  for (processor_iterator = processor_list->begin();  
       processor_iterator != processor_list->end(); 
       processor_iterator++) {

    p = *processor_iterator;

    k = strlen(p->names[1]);
    if(k>longest)
      longest = k;

  }


  // Print the name of each processor.

  for (processor_iterator = processor_list->begin();  
       processor_iterator != processor_list->end(); ) {

    for(i=0; i<nPerRow && processor_iterator != processor_list->end(); i++) {

      p = *processor_iterator++;
      cout << p->names[1];

      if(i<nPerRow-1) {

        // if this is not the last processor in the column, then
        // pad a few spaces to align the columns.

        k = longest + 2 - strlen(p->names[1]);
        for(j=0; j<k; j++)
          cout << ' ';
      }
    }
    cout << '\n';
  } 

}


//------------------------------------------------------------------------

FileContext::FileContext(string &new_name)
{
  name_str = new_name;
  fptr = NULL;
  line_seek = 0;
  _max_line =0;
  pm_address = 0;
}

FileContext::FileContext(char *new_name)
{
  name_str = string(new_name);
  fptr = NULL;
  line_seek = 0;
  _max_line =0;
  pm_address = 0;
}

FileContext::~FileContext(void)
{
  delete line_seek;
  delete pm_address;
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

  if( (max_line() < 0) || name_str.length() == 0)
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
  if(line_seek)
    delete line_seek;

  line_seek = new vector<int>(max_line()+1);
  pm_address = new vector<int>(max_line()+1);

  std::rewind(fptr);

  char buf[256],*s;
  (*line_seek)[0] = 0;
  for(unsigned int j=1; j<=max_line(); j++) {

    (*pm_address)[j] = -1;
    (*line_seek)[j] = ftell(fptr);
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

  if(!fptr)
    return 0;

  fseek(fptr, 
	(*line_seek)[line_number],
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
void FileContext::rewind(void)
{
  if(fptr)
    fseek(fptr,0,SEEK_SET);
}

//----------------------------------------
void FileContext::open(const char *mode)
{
  if(!fptr)
    fptr = fopen_path(name_str.c_str(), mode);

}
//----------------------------------------
void FileContext::close() {
  if(fptr != NULL) {
    fclose(fptr);
    fptr = NULL;
  }
}

//----------------------------------------
int FileContext::get_address(unsigned int line_number)
{
  if(line_number < max_line()  && pm_address)
    return (*pm_address)[line_number];
  return -1;
}
//----------------------------------------
void FileContext::put_address(unsigned int line_number, unsigned int address)
{
  if(line_number < max_line()  && pm_address)
    (*pm_address)[line_number] = address;
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
  for (it = begin(); it != itEnd; it++) {
    it->close();
  }
}

int FileContextList::Find(string &fname)
{
  if(lastFile) {
    for (int i = 0; i < lastFile; i++) {
      if ((*this)[i]->name() == fname)
        return i;
    }
  }
  return -1;
}

int FileContextList::Add(string &new_name)
{
  push_back(FileContext(new_name));
  lastFile++;
  back().open("r");
  if(verbose)
    cout << "Added new file named: " << new_name 
	 << "  id = " << lastFile << endl;

  return lastFile-1;
}

int FileContextList::Add(char *new_name)
{
  push_back(FileContext(new_name));
  lastFile++;
  back().open("r");
  if(verbose)
    cout << "Added new file named: " << new_name 
         << "  id = " << lastFile << endl;

  return lastFile-1;
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

  return 0;
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
