#ifndef __ICD_H__
#define __ICD_H__

#include "gpsim_classes.h"
#include "exports.h"

#define ID_LOC_ADDR      0x2000
#define DEVICE_ID_ADDR   0x2006
#define CONF_WORD_ADDR   0x2007
#define ID_LOC_LEN       8

#define MAX_PROG_MEM_SIZE 0x2000            /* 16F877 has 8K word flash */
#define MAX_MEM_SIZE (MAX_PROG_MEM_SIZE + 0x200)    /* + ID location + EEPROM */
#define UNINITIALIZED -1               /* Used to flag that a memory location isn't used */




LIBGPSIM_EXPORT bool get_use_icd();
int icd_connect(const char *dev);
int icd_reconnect(void);
int icd_disconnect(void);
int icd_detected(void);
char *icd_version(void);
char *icd_target(void);
float icd_vdd(void);
float icd_vpp(void);
int icd_reset(void);

int icd_erase(void);
int icd_prog(int *mem);

int icd_has_debug_module(void);
int icd_step(void);
int icd_run(void);
int icd_halt(void);
int icd_stopped(void);

int icd_get_state();
int icd_get_file();

int icd_set_break(int address);
int icd_clear_break(void);

int icd_read_file(int address);
int icd_write_file(int address, int data);
void icd_set_bulk(int flag);

int icd_read_eeprom(int address);
int icd_write_eeprom(int address, int data);

/*
class icd_Register : public file_register
{
        public:
          file_register *replaced;   // A pointer to the register that this break replaces
          int is_stale;

      icd_Register();

  virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

      virtual void put_value(unsigned int new_value);
       virtual void put(unsigned int new_value);
          virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_statusReg : public Status_register
{
        public:
          Status_register *replaced;   // A pointer to the register that this break replaces
          int is_stale;

      icd_statusReg();

  virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

      virtual void put_value(unsigned int new_value);
       virtual void put(unsigned int new_value);
          virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_WREG : public WREG
{
        public:
          WREG *replaced;   // A pointer to the register that this break replaces
          int is_stale;

      icd_WREG();

  virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

      virtual void put_value(unsigned int new_value);
       virtual void put(unsigned int new_value);
          virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_PC : public Program_Counter
{
        Program_Counter *replaced;
        int is_stale;

        icd_PC();

        virtual void put_value(unsigned int new_value);
        virtual unsigned int get_value(void);
};
*/

#endif


