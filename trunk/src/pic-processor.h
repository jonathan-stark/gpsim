/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __PIC_PROCESSORS_H__
#define __PIC_PROCESSORS_H__
#include <unistd.h>
#include <glib.h>

#include "gpsim_classes.h"
#include "modules.h"
#include "processor.h"

//#include "pic-registers.h"

//#include "pic-instructions.h"
#include "14bit-registers.h"

class EEPROM;
class instruction;
class Register;
class sfr_register;
class pic_register;

extern SIMULATION_MODES simulation_mode;

enum PROCESSOR_TYPE
{
  _PIC_PROCESSOR_,
  _14BIT_PROCESSOR_,
  _12BIT_PROCESSOR_,
  _16BIT_PROCESSOR_,
  _P12C508_,
  _P12C509_,
  _P16C84_,
  _P16CR83_,
  _P16CR84_,
  _P16F83_,
  _P16F84_,
  _P16C71_,
  _P16C712_,
  _P16C716_,
  _P16C54_,
  _P16C55_,
  _P16C61_,
  _P16C62_,
  _P16C62A_,
  _P16CR62_,
  _P16F627_,
  _P16F628_,
  _P16F648_,
  _P16C63_,
  _P16C64_,
  _P16C64A_,
  _P16CR64_,
  _P16C65_,
  _P16C65A_,
  _P16C72_,
  _P16C73_,
  _P16C74_,
  _P16F871_,
  _P16F873_,
  _P16F874_,
  _P16F877_,
  _P17C7xx_,
  _P17C75x_,
  _P17C752_,
  _P17C756_,
  _P17C756A_,
  _P17C762_,
  _P17C766_,
  _P18Cxx2_,
  _P18C2x2_,
  _P18C242_,
  _P18F242_,
  _P18F248_,
  _P18C252_,
  _P18F252_,
  _P18C442_,
  _P18C452_,
  _P18F442_,
  _P18F452_,
  _P18F1220_,
  _P18F1320_
};

// Configuration modes.
//  The configuration mode bits are the config word bits remapped.
//  The remapping removes processor dependent bit definitions.
class ConfigMode {
 public:

  enum {
    CM_FOSC0 = 1<<0,    // FOSC0 and  FOSC1 together define the PIC clock
    CM_FOSC1 = 1<<1,    // All PICs todate have these two bits, but the
                        // ones with internal oscillators use them differently
    CM_WDTE =  1<<2,    // Watch dog timer enable
    CM_CP0 =   1<<3,    // Code Protection
    CM_CP1 =   1<<4,
    CM_PWRTE = 1<<5,    // Power on/Reset timer enable
    CM_BODEN = 1<<6,    // Brown out detection enable
    CM_CPD =   1<<7,
    CM_MCLRE = 1<<8,    // MCLR enable

    CM_FOSC1x = 1<<9,   // Hack for internal oscillators
  };

  int config_mode;
  int valid_bits;
  ConfigMode(void) { 
    config_mode = 0xffff; 
    valid_bits = CM_FOSC0 | CM_FOSC1 | CM_WDTE;
  };

  virtual void set_config_mode(int new_value) { config_mode = new_value & valid_bits;};
  virtual void set_valid_bits(int new_value) { valid_bits = new_value;};
  void set_fosc0(void){config_mode |= CM_FOSC0;};
  void clear_fosc0(void){config_mode &= ~CM_FOSC0;};
  bool get_fosc0(void){return (config_mode & CM_FOSC0);};
  void set_fosc1(void){config_mode |= CM_FOSC1;};
  void clear_fosc1(void){config_mode &= ~CM_FOSC1;};
  bool get_fosc1(void){return (0 != (config_mode & CM_FOSC1));};
  bool get_fosc1x(void){return (0 != (config_mode & CM_FOSC1x));};

  void set_cp0(void)  {config_mode |= CM_CP0;  valid_bits |= CM_CP0;};
  void clear_cp0(void){config_mode &= ~CM_CP0; valid_bits |= CM_CP0;};
  bool get_cp0(void)  {return (0 != (config_mode & CM_CP0));};
  void set_cp1(void)  {config_mode |= CM_CP1;  valid_bits |= CM_CP1;};
  void clear_cp1(void){config_mode &= ~CM_CP1; valid_bits |= CM_CP1;};
  bool get_cp1(void)  {return (0 != (config_mode & CM_CP1));};

  void enable_wdt(void)  {config_mode |= CM_WDTE;};
  void disable_wdt(void) {config_mode &= ~CM_WDTE;};
  bool get_wdt(void)     {return (0 != (config_mode & CM_WDTE));};

  void enable_mclre(void)  {config_mode |= CM_MCLRE;};
  void disable_mclre(void) {config_mode &= ~CM_MCLRE;};
  bool get_mclre(void)     {return (0 != (config_mode & CM_MCLRE));};

  void enable_pwrte(void)   {config_mode |= CM_PWRTE;  valid_bits |= CM_PWRTE;};
  void disable_pwrte(void)  {config_mode &= ~CM_PWRTE; valid_bits |= CM_PWRTE;};
  bool get_pwrte(void)      {return (0 != (config_mode & CM_PWRTE));};
  bool is_valid_pwrte(void) {return (0 != (valid_bits & CM_PWRTE));};

  virtual void print(void);

};

/*==================================================================
 * FIXME - move these global references somewhere else
 */
extern int     verbose;         // If non-zero, then print diagnostics while initializing
extern guint64 gui_update_rate; // The rate (in simulation cycles) at which the gui is updated

/*==================================================================
 *
 * Here are the base class declarations for the pic processors
 */

/*
 * First, forward-declare a few class references
 */

enum IOPIN_TYPES
{
  INPUT_ONLY,          // e.g. MCLR
  BI_DIRECTIONAL,      // most iopins
  BI_DIRECTIONAL_PU,   // same as bi_directional, but with pullup resistor. e.g. portb
  OPEN_COLLECTOR       // bit4 in porta on the 18 pin midrange devices.
};


class processor_types
{
public:

  PROCESSOR_TYPE type;
  char *names[4];
  pic_processor * (*cpu_constructor) (void);
};

#ifdef HAVE_GUI
//
// Forward reference
//

class GUI_Processor;

#endif

/*
 * Define a base class processor for the pic processor family
 *
 * All pic processors are derived from this class.
 */

class pic_processor : public Processor
{
public:

  #define FILE_REGISTERS  0x100
  #define DEFAULT_PIC_CLOCK 4000000


  unsigned int config_word;      // as read from hex or cod file
  ConfigMode   *config_modes;    // processor dependent configuration bits.

  unsigned int pll_factor;       // 2^pll_factor is the speed boost the PLL adds 
                                 // to the instruction execution rate.

  double Vdd;
  double nominal_wdt_timeout;

  WDT          wdt;

  INDF         *indf;
  FSR          *fsr;
  Stack         *stack;

  Status_register *status;
  WREG          *W;
  OPTION_REG   option_reg;
  PCL          *pcl;
  PCLATH       *pclath;

  SFR_map*     sfr_map;
  int          num_of_sfrs;
  
  TMR0         tmr0;
  int          num_of_gprs;

  EEPROM      *eeprom;       // set to NULL for PIC's that don't have a data EEPROM

  void add_sfr_register(sfr_register *reg, unsigned int addr,
			unsigned int por_value=0,char *new_name=0);

  void init_program_memory(unsigned int memory_size);
  void build_program_memory(int *memory,int minaddr, int maxaddr);

  virtual instruction * disasm ( unsigned int address,unsigned int inst)=0;

  virtual void tris_instruction(unsigned int tris_register) {return;};
  virtual void create_symbols(void);
  virtual void create_stack(void) {stack = new Stack;};
  virtual bool load_hex(const char *hex_file);
  virtual void run(void);
  virtual void finish(void);

  void sleep(void);
  void step(unsigned int steps);
  void step_over(void);

  virtual void step_one(void) {
    program_memory[pc->value]->execute();
  }

  virtual void interrupt(void) { return; };
  void pm_write(void);

  virtual void set_config_word(unsigned int address, unsigned int cfg_word);
  unsigned int get_config_word(void) {return config_word;};
  virtual unsigned int config_word_address(void) const {return 0x2007;};
  virtual ConfigMode *create_ConfigMode(void) { return new ConfigMode; };
  void set_frequency(double f) { frequency = f; if(f>0) period = 1/f; };
  unsigned int time_to_cycles( double t) 
    {if(period>0) return((int) (frequency * t)); else return 0;};
  virtual void reset(RESET_TYPE r);

  virtual void por(void);
  virtual void create(void);

  void init_program_memory(int address, int value);

  virtual PROCESSOR_TYPE isa(void){return _PIC_PROCESSOR_;};
  virtual PROCESSOR_TYPE base_isa(void){return _PIC_PROCESSOR_;};

  virtual unsigned int register_memory_size () const { return FILE_REGISTERS;};

  /* The program_counter class calls these two functions to get the upper bits of the PC
   * for branching (e.g. goto) or modify PCL instructions (e.g. addwf pcl,f) */
  virtual unsigned int get_pclath_branching_jump(void)=0;
  virtual unsigned int get_pclath_branching_modpcl(void)=0;

  virtual void option_new_bits_6_7(unsigned int)=0;

  virtual void set_eeprom(EEPROM *e);
  virtual EEPROM *get_eeprom(void) { return (eeprom); }

  static Processor *construct(void);
  pic_processor(void);
};

#define cpu_pic ( (pic_processor *)cpu)


//--------------------------------------
//
// non-class helper functions.

void initialize_processor_constructor(void);
Processor *  add_processor(char * processor_type, char * processor_new_name);
void display_available_processors(void);
void dump_processor_list(void);
int find_in_available_processor_list(char * processor_type);
Processor *get_processor(unsigned int cpu_id);


#endif
