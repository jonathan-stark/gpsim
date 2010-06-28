/*
   Copyright (C) 1998 T. Scott Dattalo

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

#include <typeinfo>
#include <stdio.h>
#ifdef _WIN32
#include "uxtime.h"
#include "unistd.h"
#else
#include <unistd.h>
#endif
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <time.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>

#include "../config.h"

#include "exports.h"
#include "gpsim_def.h"
#include "pic-processor.h"
#include "pic-registers.h"
#include "picdis.h"
#include "symbol.h"
#include "stimuli.h"
#include "p16x5x.h"
#include "p16f62x.h"
#include "p16f8x.h"
#include "p16f88x.h"
#include "p16x8x.h"
#include "p16f87x.h"
#include "p16x6x.h"
#include "p16x7x.h"
#include "p12x.h"
#include "p12f6xx.h"
#ifdef P17C7XX  // code no longer works
#include "p17c75x.h"
#endif
#include "p18x.h"
#include "icd.h"

#include "fopen-path.h"

guint64 simulation_start_cycle;

#include "cod.h"


#include "clock_phase.h"

class phaseCaptureInterrupt : public ProcessorPhase
{
public:
  phaseCaptureInterrupt(Processor *pcpu);
  ~phaseCaptureInterrupt();
  virtual ClockPhase *advance();
  void firstHalf();
protected:
  ClockPhase *m_pCurrentPhase;
  ClockPhase *m_pNextNextPhase;
};


ClockPhase *mCurrentPhase=0;
phaseExecute1Cycle *mExecute1Cycle=0;
phaseExecute2ndHalf *mExecute2ndHalf=0;     // misnomer - should be 2-cycle
phaseExecuteInterrupt *mExecuteInterrupt=0;
phaseCaptureInterrupt *mCaptureInterrupt=0;
phaseIdle *mIdle=0;

void setCurrentPhase(ClockPhase *pPhase)
{
  mCurrentPhase = pPhase;
}

const char* phaseDesc(ClockPhase *pPhase)
{
  if (pPhase == mExecute1Cycle)
    return ("mExecute1Cycle");
  if (pPhase == mExecute2ndHalf)
    return ("mExecute2ndHalf");
  if (pPhase == mExecuteInterrupt)
    return ("mExecuteInterrupt");
  if (pPhase == mCaptureInterrupt)
    return ("mCaptureInterrupt");
  return "unknown phase";
}

phaseCaptureInterrupt::phaseCaptureInterrupt(Processor *pcpu)
  :  ProcessorPhase(pcpu)
{
}
phaseCaptureInterrupt::~phaseCaptureInterrupt()
{}
#define Rprintf(arg) {printf("0x%06X %s() ",cycles.get(),__FUNCTION__); printf arg; }
ClockPhase *phaseCaptureInterrupt::advance()
{

  //Rprintf (("phaseCaptureInterrupt\n"));
  if (m_pNextPhase == mExecute2ndHalf)
    m_pNextPhase->advance();

  m_pcpu->interrupt();

  return m_pNextPhase;
}
void phaseCaptureInterrupt::firstHalf()
{
  m_pCurrentPhase = mCurrentPhase;

  m_pNextPhase = this;
  m_pNextNextPhase = mCurrentPhase->getNextPhase();
  mCurrentPhase->setNextPhase(this);
  mCurrentPhase = this;
}


//================================================================================
// Global Declarations
//  FIXME -  move these global references somewhere else


//================================================================================
//
// pic_processor
//
// This file contains all (most?) of the code that simulates those features
// common to all pic microcontrollers.
//
//

ProcessorConstructor pP10F200(P10F200::construct ,
                              "__10F200",   "pic10f200",    "p10f200",  "10f200");
ProcessorConstructor pP10F202(P10F202::construct ,
                              "__10F202",   "pic10f202",    "p10f202",  "10f202");
ProcessorConstructor pP10F204(P10F204::construct ,
                              "__10F204",   "pic10f204",    "p10f204",  "10f204");
ProcessorConstructor pP10F220(P10F220::construct ,
                              "__10F220",   "pic10f220",    "p10f220",  "10f220");
ProcessorConstructor pP10F222(P10F222::construct ,
                              "__10F222",   "pic10f222",    "p10f222",  "10f222");
ProcessorConstructor pP12C508(P12C508::construct ,
                              "__12C508",   "pic12c508",    "p12c508",  "12c508");
ProcessorConstructor pP12C509(P12C509::construct ,
                              "__12C509",   "pic12c509",    "p12c509",  "12c509");
ProcessorConstructor pP12CE518(P12CE518::construct ,
                              "__12ce518",  "pic12ce518",   "p12ce518", "12ce518");
ProcessorConstructor pP12CE519(P12CE519::construct ,
                              "__12ce519",  "pic12ce519",   "p12ce519", "12ce519");
ProcessorConstructor pP12F508(P12F508::construct ,
                              "__12F508",   "pic12f508",    "p12f508",  "12f508");
ProcessorConstructor pP12F509(P12F509::construct ,
                              "__12F509",   "pic12f509",    "p12f509",  "12f509");
ProcessorConstructor pP12F510(P12F510::construct ,
                              "__12F510",   "pic12f510",    "p12f510",  "12f510");
ProcessorConstructor pP12F629(P12F629::construct ,
                              "__12F629",   "pic12f629",    "p12f629",  "12f629");
ProcessorConstructor pP12F675(P12F675::construct ,
                              "__12F675",   "pic12f675",    "p12f675",  "12f675");
ProcessorConstructor pP12F683(P12F683::construct ,
                              "__12F683",   "pic12f683",    "p12f683",  "12f683");
ProcessorConstructor pP16C54(P16C54::construct ,
                             "__16C54",     "pic16c54",     "p16c54",   "16c54");
ProcessorConstructor pP16C55(P16C55::construct ,
                             "__16C55",     "pic16c55",     "p16c55",   "16c55");
ProcessorConstructor pP16C56(P16C56::construct ,
                             "__16C56",     "pic16c56",     "p16c56",   "16c56");
ProcessorConstructor pP16C61(P16C61::construct ,
                             "__16C61",     "pic16c61",     "p16c61",   "16c61");
ProcessorConstructor pP16C62(P16C62::construct ,
                             "__16C62",     "pic16c62",     "p16c62",   "16c62");
ProcessorConstructor pP16C62A(P16C62::construct ,
                              "__16C62A",   "pic16c62a",    "p16c62a",  "16c62a");
ProcessorConstructor pP16CR62(P16C62::construct ,
                              "__16CR62",   "pic16cr62",    "p16cr62",  "16cr62");
ProcessorConstructor pP16C63(P16C63::construct ,
                             "__16C63",     "pic16c63",     "p16c63",   "16c63");
ProcessorConstructor pP16C64(P16C64::construct ,
                             "__16C64",     "pic16c64",     "p16c64",   "16c64");
ProcessorConstructor pP16C65(P16C65::construct ,
                             "__16C65",     "pic16c65",     "p16c65",   "16c65");
ProcessorConstructor pP16C65A(P16C65::construct ,
                             "__16C65A",    "pic16c65a",    "p16c65a",  "16c65a");
ProcessorConstructor pP16C71(P16C71::construct ,
                             "__16C71",     "pic16c71",     "p16c71",   "16c71");
ProcessorConstructor pP16C712(P16C712::construct ,
                              "__16C712",   "pic16c712",    "p16c712",  "16c712");
ProcessorConstructor pP16C716(P16C716::construct ,
                              "__16C716",   "pic16c716",    "p16c716",  "16c716");
ProcessorConstructor pP16C72(P16C72::construct ,
                             "__16C72",     "pic16c72",     "p16c72",   "16c72");
ProcessorConstructor pP16C73(P16C73::construct ,
                             "__16C73",     "pic16c73",     "p16c73",   "16c73");
ProcessorConstructor pP16C74(P16C74::construct ,
                             "__16C74",     "pic16c74",     "p16c74",   "16c74");
ProcessorConstructor pP16C84(P16C84::construct ,
                             "__16C84",     "pic16c84",     "p16c84",   "16c84");
ProcessorConstructor pP16CR83(P16CR83::construct ,
                              "__16CR83",   "pic16cr83",    "p16cr83",  "16cr83");
ProcessorConstructor pP16CR84(P16CR84::construct ,
                              "__16CR84",   "pic16cr84",    "p16cr84",  "16cr84");
ProcessorConstructor pP16F73(P16F73::construct ,
                             "__16F73",     "pic16f73",     "p16f73",   "16f73");
ProcessorConstructor pP16F74(P16F74::construct ,
                             "__16F74",     "pic16f74",     "p16f74",   "16f74");
ProcessorConstructor pP16F83(P16F83::construct ,
                             "__16F83",     "pic16f83",     "p16f83",   "16f83");
ProcessorConstructor pP16F84(P16F84::construct ,
                             "__16F84",     "pic16f84",     "p16f84",   "16f84");
ProcessorConstructor pP16F87(P16F87::construct ,
                              "__16F87",    "pic16f87",     "p16f87",   "16f87");
ProcessorConstructor pP16F88(P16F88::construct ,
                              "__16F88",    "pic16f88",     "p16f88",   "16f88");
ProcessorConstructor pP16F882(P16F882::construct ,
                     "__16F882",    "pic16f882",     "p16f882",   "16f882");
ProcessorConstructor pP16F883(P16F883::construct ,
                     "__16F883",    "pic16f883",     "p16f883",   "16f883");
ProcessorConstructor pP16F884(P16F884::construct ,
                     "__16F884",    "pic16f884",     "p16f884",   "16f884");
ProcessorConstructor pP16F886(P16F886::construct ,
                     "__16F886",    "pic16f886",     "p16f886",   "16f886");
ProcessorConstructor pP16F887(P16F887::construct ,
                     "__16F887",    "pic16f887",     "p16f887",   "16f887");
ProcessorConstructor pP16F627(P16F627::construct ,
                              "__16F627",   "pic16f627",    "p16f627",  "16f627");
ProcessorConstructor pP16F627A(P16F627::construct ,
                              "__16F627A",  "pic16f627a",   "p16f627a", "16f627a");
ProcessorConstructor pP16F628(P16F628::construct ,
                              "__16F628",   "pic16f628",    "p16f628",  "16f628");
ProcessorConstructor pP16F628A(P16F628::construct ,
                              "__16F628A",  "pic16f628a",   "p16f628a", "16f628a");
ProcessorConstructor pP16F630(P16F630::construct ,
                              "__16F630",   "pic16f630",    "p16f630",  "16f630");
ProcessorConstructor pP16F648(P16F648::construct ,
                              "__16F648",   "pic16f648",    "p16f648",  "16f648");
ProcessorConstructor pP16F648A(P16F648::construct ,
                              "__16F648A",  "pic16f648a",   "p16f648a", "16f648a");
ProcessorConstructor pP16F676(P16F676::construct ,
                              "__16F676",   "pic16f676",    "p16f676",  "16f676");
ProcessorConstructor pP16F818(P16F818::construct ,
                              "__16F818",   "pic16f818",    "p16f818",  "16f818");
ProcessorConstructor pP16F819(P16F819::construct ,
                              "__16F819",   "pic16f819",    "p16f819",  "16f819");
ProcessorConstructor pP16F871(P16F871::construct ,
                              "__16F871",   "pic16f871",    "p16f871",  "16f871");
ProcessorConstructor pP16F873(P16F873::construct ,
                              "__16F873",   "pic16f873",    "p16f873",  "16f873");
ProcessorConstructor pP16F874(P16F874::construct ,
                              "__16F874",   "pic16f874",    "p16f874",  "16f874");
ProcessorConstructor pP16F876(P16F876::construct ,
                              "__16F876",   "pic16f876",    "p16f876",  "16f876");
ProcessorConstructor pP16F877(P16F877::construct ,
                              "__16F877",   "pic16f877",    "p16f877",  "16f877");
ProcessorConstructor pP16F873A(P16F873A::construct ,
                              "__16F873a",  "pic16f873a",   "p16f873a", "16f873a");
ProcessorConstructor pP16F874A(P16F874A::construct ,
                              "__16F874a",  "pic16f874a",   "p16f874a", "16f874a");
ProcessorConstructor pP16F876A(P16F876A::construct ,
                              "__16F876a",  "pic16f876a",   "p16f876a", "16f876a");
ProcessorConstructor pP16F877A(P16F877A::construct ,
                              "__16F877a",  "pic16f877a",   "p16f877a", "16f877a");
#ifdef P17C7XX  // code no longer works
ProcessorConstructor pP17C7xx(P17C7xx::construct ,
                              "__17C7xx", "pic17c7xx",  "p17c7xx", "17c7xx");
ProcessorConstructor pP17C75x(P17C75x::construct ,
                              "__17C75x", "pic17c75x",  "p17c75x", "17c75x");
ProcessorConstructor pP17C752(P17C752::construct ,
                              "__17C752", "pic17c752",  "p17c752", "17c752");
ProcessorConstructor pP17C756(P17C756::construct ,
                              "__17C756", "pic17c756",  "p17c756", "17c756");
ProcessorConstructor pP17C756A(P17C756A::construct ,
                               "__17C756A", "pic17c756a",   "p17c756a", "17c756a");
ProcessorConstructor pP17C762(P17C762::construct ,
                              "__17C762", "pic17c762",  "p17c762", "17c762");
ProcessorConstructor pP17C766(P17C766::construct ,
                              "__17C766", "pic17c766",  "p17c766", "17c766");
#endif // P17C7XX
ProcessorConstructor pP18C242(P18C242::construct ,
                              "__18C242",   "pic18c242",    "p18c242",  "18c242");
ProcessorConstructor pP18C252(P18C252::construct ,
                              "__18C252",   "pic18c252",    "p18c252",  "18c252");
ProcessorConstructor pP18C442(P18C442::construct ,
                              "__18C442",   "pic18c442",    "p18c442",  "18c442");
ProcessorConstructor pP18C452(P18C452::construct ,
                              "__18C452",   "pic18c452",    "p18c452",  "18c452");
ProcessorConstructor pP18F242(P18F242::construct ,
                              "__18F242",   "pic18f242",    "p18f242",  "18f242");
ProcessorConstructor pP18F248(P18F248::construct ,
                              "__18F248",   "pic18f248",    "p18f248",  "18f248");
ProcessorConstructor pP18F252(P18F252::construct ,
                              "__18F252",   "pic18f252",    "p18f252",  "18f252");
ProcessorConstructor pP18F442(P18F442::construct ,
                              "__18F442",   "pic18f442",    "p18f442",  "18f442");
ProcessorConstructor pP18F448(P18F448::construct ,
                              "__18F448",   "pic18f448",    "p18f448",  "18f448");
ProcessorConstructor pP18F452(P18F452::construct,
                              "__18F452",   "pic18f452",    "p18f452",  "18f452");
ProcessorConstructor pP18F1220(P18F1220::construct,
                              "__18F1220",  "pic18f1220",   "p18f1220", "18f1220");
ProcessorConstructor pP18F1320(P18F1320::construct,
                              "__18F1320",  "pic18f1320",   "p18f1320", "18f1320");
ProcessorConstructor pP18F2321(P18F2321::construct,
                              "__18F2321",  "pic18f2321",   "p18f2321", "18f2321");
ProcessorConstructor pP18F2455(P18F2455::construct,
                              "__18F2455",  "pic18f2455",   "p18f2455", "18f2455");
ProcessorConstructor pP18F4321(P18F4321::construct,
                              "__18F4321",  "pic18f4321",   "p18f4321", "18f4321");
ProcessorConstructor pP18F4455(P18F4455::construct,
                              "__18F4455",  "pic18f4455",   "p18f4455", "18f4455");


//========================================================================
// Trace Type for Resets

class InterruptTraceObject : public ProcessorTraceObject
{
public:
  InterruptTraceObject(Processor *_cpu);
  virtual void print(FILE *fp);
};

class InterruptTraceType : public ProcessorTraceType
{
public:
  InterruptTraceType(Processor *_cpu);
  TraceObject *decode(unsigned int tbi);
  void record();
  int dump_raw(Trace *pTrace,unsigned int tbi, char *buf, int bufsize);

  unsigned int m_uiTT;
};

//------------------------------------------------------------
InterruptTraceObject::InterruptTraceObject(Processor *_cpu)
  : ProcessorTraceObject(_cpu)
{
}
void InterruptTraceObject::print(FILE *fp)
{
  fprintf(fp, "  %s *** Interrupt ***\n",
                   (cpu ? cpu->name().c_str() : ""));
}

//------------------------------------------------------------
InterruptTraceType::InterruptTraceType(Processor *_cpu)
  : ProcessorTraceType(_cpu,1,"Interrupt")
{
  m_uiTT = trace.allocateTraceType(this);
}

TraceObject *InterruptTraceType::decode(unsigned int tbi)
{
  //unsigned int tv = trace.get(tbi);
  return new InterruptTraceObject(cpu);
}

void InterruptTraceType::record()
{
  trace.raw(m_uiTT);
}

int InterruptTraceType::dump_raw(Trace *pTrace,unsigned int tbi, char *buf, int bufsize)
{
  if (!pTrace)
    return 0;

  int n = TraceType::dump_raw(pTrace, tbi,buf,bufsize);

  buf += n;
  bufsize -= n;

  int m = snprintf(buf, bufsize,
                   " %s *** Interrupt ***",
                   (cpu ? cpu->name().c_str() : ""));
  return m > 0 ? (m+n) : n;
}
//-------------------------------------------------------------------
void pic_processor::set_eeprom(EEPROM *e)
{
  eeprom = e;
  ema.set_Registers(e->rom, e->rom_size);
}

//-------------------------------------------------------------------
void pic_processor::BP_set_interrupt()
{

  m_pInterruptTT->record();
  mCaptureInterrupt->firstHalf();
}

//-------------------------------------------------------------------
//
// sleep - Begin sleeping and stay asleep until something causes a wake
//

void pic_processor::sleep ()
{
}
//-------------------------------------------------------------------
//
// enter_sleep - The processor is about to go to sleep, so update
//  the status register.

void pic_processor::enter_sleep()
{
  status->put_TO(1);
  status->put_PD(0);

  wdt.update();
  pc->increment();
  mCurrentPhase->setNextPhase(mIdle);
  mCurrentPhase = mIdle;
  mCurrentPhase->setNextPhase(mIdle);
  m_ActivityState = ePASleeping;

}

//-------------------------------------------------------------------
//
// exit_sleep

void pic_processor::exit_sleep()
{

  m_ActivityState = ePAActive;
  mCurrentPhase->setNextPhase(mExecute1Cycle);
}

//-------------------------------------------------------------------
//
// is_sleeping

bool pic_processor::is_sleeping()
{

  return m_ActivityState == ePASleeping;
}

//-------------------------------------------------------------------
//
// pm_write - program memory write
//

void pic_processor::pm_write ()
{
  m_ActivityState = ePAPMWrite;

  do
    get_cycles().increment();     // burn cycles until we're through writing
  while(bp.have_pm_write());

  simulation_mode = eSM_RUNNING;

}

static bool realtime_mode = false;
static bool realtime_mode_with_gui = false;

void EnableRealTimeMode(bool bEnable) {
  realtime_mode = bEnable;
}

void EnableRealTimeModeWithGui(bool bEnable) {
  realtime_mode_with_gui = bEnable;
}

extern void update_gui();

class RealTimeBreakPoint : public TriggerObject
{
public:
  Processor *cpu;
  struct timeval tv_start;
  guint64 cycle_start;
  guint64 future_cycle;
  int warntimer;
  guint64 period;            // callback period in us

//#define REALTIME_DEBUG
  guint64 diffmax;
  guint64 diffsum;
  int diffsumct;
  struct timeval stat_start;

  RealTimeBreakPoint()
  {
    cpu = 0;
    warntimer = 1;
    period = 1;
    future_cycle = 0;
    diffsum=0;
    diffsumct=0;
    diffmax=0;
  }

  void start(Processor *active_cpu)
  {
    if(!active_cpu)
      return;

    diffsum=0;
    diffsumct=0;
    diffmax=0;

    // Grab the system time and record the simulated pic's time.
    // We'll then set a break point a short time in the future
    // and compare how the two track.

    cpu = active_cpu;

    gettimeofday(&tv_start,0);
    stat_start=tv_start;

    cycle_start=get_cycles().get();

    guint64 fc = cycle_start+100;

    //cout << "real time start : " << cycle_start << '\n';

    if(future_cycle)
      get_cycles().reassign_break(future_cycle, fc, this);
    else
      get_cycles().set_break(fc, this);

    future_cycle = fc;

  }

  void stop()
  {

    //cout << "real time stop : " << future_cycle << '\n';

#ifdef REALTIME_DEBUG
    dump_stats();
#endif

    // Clear any pending break point.
    if(future_cycle) {
      cout << " real time clearing\n";
      get_cycles().clear_break(this);
      future_cycle = 0;
      if(realtime_mode_with_gui)
      {
        update_gui();
      }
    }

  }

#ifdef REALTIME_DEBUG
  void dump_stats(void)
  {
    struct timeval tv;
    gettimeofday(&tv,0);
    double simulation_time = (tv.tv_sec-stat_start.tv_sec)+(tv.tv_usec-stat_start.tv_usec)/1000000.0; // in seconds
    if(diffsumct>0 && simulation_time>0)
    {
      cout << dec << "Average realtime error: " << diffsum/diffsumct << " microseconds. Max: "<<diffmax<<endl;
      if(realtime_mode_with_gui)
        cout << "Number of realtime callbacks (gui refreshes) per second:";
      else
        cout << "Number of realtime callbacks per second:";
      cout << diffsumct/(double)simulation_time << endl;
    }
    stat_start=tv;
    diffsum=0;
    diffsumct=0;
    diffmax=0;
  }
#endif

  void callback()
  {
    guint64 system_time;	// wall clock time since datum in micro seconds
    guint64 simulation_time;	// simulation time since datum in micro seconds
    guint64 diff_us;
    struct timeval tv;

    // We just hit the break point. A few moments ago we
    // grabbed a snap shot of the system time and the simulated
    // pic's time. Now we're going to compare the two deltas and
    // see how well they've tracked. 
    //
    // If the host is running faster than the PIC, we'll put the 
    // host to sleep briefly.
    //
    // If the host is running slower than the PIC, lengthen the
    // time between GUI updates.


    gettimeofday(&tv,0);

    system_time = (tv.tv_sec-tv_start.tv_sec)*1000000ULL+(tv.tv_usec-tv_start.tv_usec); // in micro-seconds

    simulation_time = ((get_cycles().get()-cycle_start)*4.0e6*cpu->get_OSCperiod());


    if( simulation_time > system_time)
    {
        // we are simulating too fast

	diff_us = simulation_time - system_time;

	if(period > diff_us)
          period -= diff_us;
        else
          period=1;
        usleep((unsigned int)diff_us);
    }
    else
    {
	diff_us = system_time - simulation_time;

	period+=diff_us;

        if(period>1000000)
            period=1000000; // limit to a one second callback period

        if(diff_us>1000000)
        {
            // we are simulating too slow
            if(warntimer<10)
                warntimer++;
            else
            {
                warntimer=0;
                puts("Processor is too slow for realtime mode!");
            }
        }
        else
            warntimer=0;
    }

    guint64 delta_cycles= (guint64)(period*cpu->get_frequency()/4000000);
    if(delta_cycles<1)
      delta_cycles=1;

    // Look at realtime_mode_with_gui and update the gui if true
    if(realtime_mode_with_gui)
    {
        update_gui();
    }

#ifdef REALTIME_DEBUG
    if(tv.tv_sec<stat_start.tv_sec+10) {
      diffsum+=diff_us;
      diffsumct++;
    }
    else
    {
      dump_stats();
    }
    if(diff_us>diffmax)
      diffmax=diff_us;

    static guint64 oldtime = 0;
    //cout<<dec<<"dt="<<(system_time-oldtime)/1000 << "\tdiff_us="<<diff_us<<"\tdelta_cycles="<<delta_cycles<<"\tperiod="<<period<<endl;
    oldtime = system_time;
#endif

    guint64 fc = get_cycles().get() + delta_cycles;

    if(future_cycle)
      get_cycles().reassign_break(future_cycle, fc, this);
    else
      get_cycles().set_break(fc, this);

    future_cycle = fc;

  }

};

RealTimeBreakPoint realtime_cbp;

//-------------------------------------------------------------------
void pic_processor::save_state()
{
  Processor::save_state();

  if(W)
    W->put_trace_state(W->value);

  if(eeprom)
    eeprom->save_state();
}

//-------------------------------------------------------------------
//
// run  -- Begin simulating and don't stop until there is a break.
//

void pic_processor::run (bool refresh)
{
  if(simulation_mode != eSM_STOPPED) {
    if(verbose)
      cout << "Ignoring run request because simulation is not stopped\n";
    return;
  }

  simulation_mode = eSM_RUNNING;

  // If the first instruction we're simulating is a break point,
  // then ignore it.

  if(realtime_mode)
    realtime_cbp.start(active_cpu);

  simulation_start_cycle = get_cycles().get();
  bp.clear_global();

  mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;


    do
      mCurrentPhase = mCurrentPhase->advance();
    while(!bp.global_break);

  if(realtime_mode)
    realtime_cbp.stop();

    /* FIXME
    if(bp.have_pm_write())
      pm_write();
    */

    /*
    if(bp.have_socket_break()) {
      cout << " socket break point \n";
      Interface *i = gi.get_socket_interface();
      if (i)
        i->Update(0);
      bp.clear_socket_break();
    }
    */

    //} while(!bp.global_break);


  bp.clear_global();
  trace.cycle_counter(get_cycles().get());

  simulation_mode = eSM_STOPPED;


}


//-------------------------------------------------------------------
//
// step - Simulate one (or more) instructions. If a breakpoint is set
// at the current PC-> 'step' will go right through it. (That's supposed
// to be a feature.)
//

void pic_processor::step (unsigned int steps, bool refresh)
{

  if(!steps)
    return;

  if(get_use_icd())
  {
      if(steps!=1)
      {
          cout << "Can only step one step in ICD mode"<<endl;
      }
      icd_step();
      pc->get_value();
      disassemble((signed int)pc->value, (signed int)pc->value); // FIXME, don't want this in HLL ICD mode.
      if(refresh)
        gi.simulation_has_stopped();
      return;
  }


  if(simulation_mode != eSM_STOPPED) {
    if(verbose)
      cout << "Ignoring step request because simulation is not stopped\n";
    return;
  }

  simulation_mode = eSM_SINGLE_STEPPING;


  mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;

  do
    mCurrentPhase = mCurrentPhase->advance();
  while(!bp.have_halt() && --steps>0);

  // complete the step if this is a multi-cycle instruction.

  if (mCurrentPhase == mExecute2ndHalf)
    while (mCurrentPhase != mExecute1Cycle)
      mCurrentPhase = mCurrentPhase->advance();

  get_trace().cycle_counter(get_cycles().get());
  if(refresh)
    trace_dump(0,1);

  bp.clear_halt();
  simulation_mode = eSM_STOPPED;

  if(refresh)
    get_interface().simulation_has_stopped();

}

//-------------------------------------------------------------------
void pic_processor::step_cycle()
{
  mCurrentPhase = mCurrentPhase->advance();
}

//
//-------------------------------------------------------------------
//
// step_over - In most cases, step_over will simulate just one instruction.
// However, if the next instruction is a branching one (e.g. goto, call,
// return, etc.) then a break point will be set after it and gpsim will
// begin 'running'. This is useful for stepping over time-consuming calls.
//

void pic_processor::step_over (bool refresh)
{

  if(simulation_mode != eSM_STOPPED) {
    if(verbose)
      cout << "Ignoring step-over request because simulation is not stopped\n";
    return;
  }

  unsigned int saved_pc = pma->get_PC();
  instruction *nextInstruction = pma->getFromAddress(saved_pc);
  if (!nextInstruction) {
    // this is really fatal...
    return;
  }
  unsigned int nextExpected_pc =
    saved_pc + map_pm_index2address(nextInstruction->instruction_size());

  step(1,false); // Try one step -- without refresh

  // if the pc did not advance just one instruction, then some kind of branch occurred.

  unsigned int current_pc = pma->get_PC();
  if( ! (current_pc >= saved_pc && current_pc <= nextExpected_pc)) {

    // If the branch is not a skip instruction then we'll set a break point and run.
    // (note, the test that's performed will treat a goto $+2 as a skip.

    instruction *nextNextInstruction = pma->getFromAddress(nextExpected_pc);
    unsigned int nextNextExpected_pc = nextExpected_pc +
      (nextNextInstruction ? map_pm_index2address(nextNextInstruction->instruction_size()) : 0);

    if (! (current_pc >= saved_pc && current_pc <= nextNextExpected_pc)) {

      unsigned int bp_num = pma->set_break_at_address(nextExpected_pc);
      if (bp_num != INVALID_VALUE) {
        run();
        bp.clear(bp_num);
      }
    }

  }

  // note that we don't need to tell the gui to update its windows since
  // that is already done by step() or run().

  if(refresh)
    get_interface().simulation_has_stopped();

}

//-------------------------------------------------------------------
//
// finish
//
// this method really only applies to processors with stacks.

void pic_processor::finish()
{
  if(!stack)
    return;

  run_to_address( stack->contents[(stack->pointer-1) & stack->stack_mask]);

  get_interface().simulation_has_stopped();
}


//-------------------------------------------------------------------
//
// reset - reset the pic based on the desired reset type.
//

void pic_processor::reset (RESET_TYPE r)
{
  bool bHaltSimulation = getBreakOnReset();

  if(get_use_icd())
  {
      puts("RESET");
      icd_reset();
      disassemble((signed int)pc->get_value(), (signed int)pc->get_value());
      gi.simulation_has_stopped();
      return;
  }

  m_pResetTT->record(r);
  if(r == SOFT_RESET) {

    pc->reset();
    gi.simulation_has_stopped();
    cout << " --- Soft Reset (not fully implemented)\n";
    return;
  }

  rma.reset(r);
  pc->reset();
  stack->reset();
  wdt.reset(r);

  bp.clear_global();

  switch (r) {
  case POR_RESET:
    if(verbose) {
      cout << "POR\n";
      if(config_modes) config_modes->print();
    }
    bHaltSimulation = false;
    mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;
    m_ActivityState = ePAActive;
    break;

  case MCLR_RESET:
    mCurrentPhase = mCurrentPhase ? mCurrentPhase : mIdle;
    mCurrentPhase->setNextPhase(mIdle);
    m_ActivityState = ePAIdle;
    break;

  case IO_RESET:
    mCurrentPhase = mExecute1Cycle;
    mCurrentPhase->setNextPhase(mExecute1Cycle);
    m_ActivityState = ePAActive;
    break;

  case WDT_RESET:
  case EXIT_RESET:
    mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;
    mCurrentPhase->setNextPhase(mExecute1Cycle);
    m_ActivityState = ePAActive;
    break;

  default:
    m_ActivityState = ePAActive;
    break;
  }

  if(bHaltSimulation || getBreakOnReset()) {
    bp.halt();
    gi.simulation_has_stopped();
  }
}

//-------------------------------------------------------------------
//
// pic_processor -- constructor
//

pic_processor::pic_processor(const char *_name, const char *_desc)
  : Processor(_name,_desc),
    wdt(this, 18.0e-3),indf(0),fsr(0), stack(0), status(0),
    W(0), pcl(0), pclath(0),m_PCHelper(0),
    tmr0(this,"tmr0","Timer 0"),
    m_configMemory(0),
    m_MCLR(0), m_MCLR_Save(0), m_MCLRMonitor(0)
{

   mExecute1Cycle    = new phaseExecute1Cycle(this);
   mExecute2ndHalf   = new phaseExecute2ndHalf(this);
   mExecuteInterrupt = new phaseExecuteInterrupt(this);
   mCaptureInterrupt = new phaseCaptureInterrupt(this);
   mIdle             = new phaseIdle(this);
   mCurrentPhase   = mExecute1Cycle;

  m_Capabilities = eSTACK | eWATCHDOGTIMER;

  if(verbose)
    cout << "pic_processor constructor\n";

  eeprom = 0;
  config_modes = create_ConfigMode();

  pll_factor = 0;

  Integer::setDefaultBitmask(0xff);

  // Test code for logging to disk:
  GetTraceLog().switch_cpus(this);
  m_pResetTT = new ResetTraceType(this);
  m_pInterruptTT = new InterruptTraceType(this);

}
//-------------------------------------------------------------------
pic_processor::~pic_processor()
{
  delete m_pResetTT;
  delete m_pInterruptTT;

  delete_sfr_register(W);
  delete_sfr_register(pcl);

  delete_sfr_register(pclath);
  delete_sfr_register(status);
  delete_sfr_register(indf);
  delete m_PCHelper;
  delete stack;

  delete mExecute1Cycle;
  delete mExecute2ndHalf;
  delete mExecuteInterrupt;
  delete mCaptureInterrupt;
  delete mIdle;

  delete config_modes;
  delete m_configMemory;

  if (m_MCLR)
    m_MCLR->setMonitor(0);
  if (m_MCLR_Save)
    m_MCLR_Save->setMonitor(0);
  if (m_MCLRMonitor)
    delete m_MCLRMonitor;

}
//-------------------------------------------------------------------
//
//
//    create
//
//  The purpose of this member function is to 'create' a pic processor.
// Since this is a base class member function, only those things that
// are common to all pics are created.

void pic_processor::create ()
{

  init_program_memory (program_memory_size());

  init_register_memory (register_memory_size());

  // Now, initialize the core stuff:
  pc->set_cpu(this);

  W = new WREG(this,"W","Working Register");

  pcl = new PCL(this,"pcl", "Program Counter Low");
  pclath = new PCLATH(this,"pclath", "Program Counter Latch High");
  status = new Status_register(this,"status", "Processor status");
  indf = new INDF(this,"indf","Indirect register");

  register_bank = &registers[0];  // Define the active register bank

  Vdd = 5.0;                      // Assume 5.0 volt power supply

  if(pma) {
    m_PCHelper = new PCHelper(this,pma);
    rma.SpecialRegisters.push_back(m_PCHelper);
    rma.SpecialRegisters.push_back(status);
    rma.SpecialRegisters.push_back(W);

    pma->SpecialRegisters.push_back(m_PCHelper);
    pma->SpecialRegisters.push_back(status);
    pma->SpecialRegisters.push_back(W);

  }

  create_config_memory();

}

//-------------------------------------------------------------------
//
// add_sfr_register
//
// The purpose of this routine is to add one special function register
// to the file registers. If the sfr has a physical address (like the
// status or tmr0 registers) then a pointer to that register will be
// placed in the file register map.

// FIXME It doesn't make any sense to initialize the por_value here!
// FIXME The preferred way is to initialize all member data in their
// FIXME parent's constructor.

void pic_processor::add_sfr_register(Register *reg, unsigned int addr,
                                     RegisterValue por_value, const char *new_name)
{

  reg->set_cpu(this);
  if(addr < register_memory_size())
    {
      registers[addr] = reg;
      registers[addr]->address = addr;
      registers[addr]->alias_mask = 0;
      if(new_name)
        registers[addr]->new_name(new_name);

      RegisterValue rv = getWriteTT(addr);
      registers[addr]->set_write_trace(rv);
      rv = getReadTT(addr);
      registers[addr]->set_read_trace(rv);
    }

  reg->value       = por_value;
  reg->por_value   = por_value;  /// FIXME why are we doing this?
  reg->initialize();
}

//-------------------------------------------------------------------
//
// delete_sfr_register
//
void pic_processor::delete_sfr_register(Register *pReg)
{

  if (pReg) {

    unsigned int a = pReg->getAddress();
    if (0)
      cout << __FUNCTION__ << " addr = 0x"<<hex<<a
           <<" reg " << pReg->name()<<endl;
    if (a<rma.get_size() && registers[a] == pReg)
      delete_file_registers(a,a);
    else
      delete pReg;

    pReg = 0;
  }

}

//-------------------------------------------------------------------
//
// delete_sfr_register
//
void pic_processor::remove_sfr_register(Register *ppReg)
{

  if (ppReg) {

    unsigned int a = ppReg->getAddress();
    if (registers[a] == ppReg)
      delete_file_registers(a,a,true);
  }

}

//-------------------------------------------------------------------
//
// init_program_memory
//
// The purpose of this member function is to allocate memory for the
// pic's code space. The 'memory_size' parameter tells how much memory
// is to be allocated AND it should be an integer of the form of 2^n.
// If the memory size is not of the form of 2^n, then this routine will
// round up to the next integer that is of the form 2^n.
//   Once the memory has been allocated, this routine will initialize
// it with the 'bad_instruction'. The bad_instruction is an instantiation
// of the instruction class that chokes gpsim if it is executed. Note that
// each processor owns its own 'bad_instruction' object.

void pic_processor::init_program_memory (unsigned int memory_size)
{

  if(verbose)
    cout << "Initializing program memory: 0x"<<memory_size<<" words\n";

  // The memory_size_mask is used by the branching instructions

  pc->memory_size = memory_size;

  Processor::init_program_memory(memory_size);
}

void pic_processor::create_symbols ()
{


  if(verbose)
    cout << __FUNCTION__ << " register memory size = " << register_memory_size() << '\n';

  for(unsigned int i = 0; i<register_memory_size(); i++) {

    switch (registers[i]->isa()) {
    case Register::SFR_REGISTER:
      //if(!symbol_table.find((char *)registers[i]->name().c_str()))
      //  symbol_table.add_register(registers[i]);
      //
      addSymbol(registers[i]);
      break;
    default:
      break;
    }
  }

  pc->set_description("Program Counter");  // Fixme put this in the pc constructor.
  addSymbol(pc);
  addSymbol(&wdt);
}


//-------------------------------------------------------------------

bool pic_processor::set_config_word(unsigned int address,unsigned int cfg_word)
{

  if (m_configMemory)
  {
      for(int i = 0; m_configMemory->getConfigWord(i); i++)
      {
        if (m_configMemory->getConfigWord(i)->ConfigWordAdd() == address)
        {
            m_configMemory->getConfigWord(i)->set((int)cfg_word);
            if (i == 0 && config_modes)
            {
                config_word = cfg_word;
                config_modes->config_mode = (config_modes->config_mode & ~7) |
                                        (cfg_word & 7);
            }

            return true;
        }
     }
  }

  return false;

}


unsigned int pic_processor::get_config_word(unsigned int address)
{
  return address == config_word_address() ? config_word : 0xffffffff;
}



//-------------------------------------------------------------------
//
// load_hex
//

bool pic_processor::LoadProgramFile(const char *pFilename, FILE *pFile,
                                    const char *pProcessorName)
{
  Processor * pProcessor = this;
  // Tries the file type based on the file extension first.
  // If it fails tries the other type. This code will need
  // to change if pic_processor is moved to its own module
  // because then we cannot garrentee that these file types
  // will be the first two in the list.
  ProgramFileType * aFileTypes[] = {
    ProgramFileTypeList::GetList()[0],  // IntelHexProgramFileType
    ProgramFileTypeList::GetList()[1]   // PicCodProgramFileType
  };
  if(IsFileExtension(pFilename,"cod")) {
    // If 'cod' file extension, try PicCodProgramFileType first
    swap(aFileTypes[0], aFileTypes[1]);
  }
  int iReturn  = aFileTypes[0]->LoadProgramFile(&pProcessor, pFilename, pFile, pProcessorName);
  if (iReturn != ProgramFileType::SUCCESS) {
    fseek(pFile, 0, SEEK_SET);
    iReturn = aFileTypes[1]->LoadProgramFile(&pProcessor, pFilename, pFile, pProcessorName);
  }
  return iReturn == ProgramFileType::SUCCESS;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
//  ConfigMode
//
void ConfigMode::print()
{


  if(config_mode & CM_FOSC1x) {
    // Internal Oscillator type processor

    switch(config_mode& (CM_FOSC0 | CM_FOSC1)) {  // Lower two bits are the clock type
    case 0: cout << "LP"; break;
    case CM_FOSC0: cout << "XT"; break;
    case CM_FOSC1: cout << "Internal RC"; break;
    case (CM_FOSC0|CM_FOSC1): cout << "External RC"; break;

    }
  }else {
    switch(config_mode& (CM_FOSC0 | CM_FOSC1)) {  // Lower two bits are the clock type
    case 0: cout << "LP"; break;
    case CM_FOSC0: cout << "XT"; break;
    case CM_FOSC1: cout << "HS"; break;
    case (CM_FOSC0|CM_FOSC1): cout << "RC"; break;
    }
  }

  cout << " oscillator\n";

  if(valid_bits & CM_WDTE)
    cout << " WDT is " << (get_wdt() ? "enabled\n" : "disabled\n");

  if(valid_bits & CM_MCLRE)
    cout << "MCLR is " << (get_mclre() ? "enabled\n" : "disabled\n");

  if(valid_bits & CM_CP0) {

    if(valid_bits & CM_CP1) {
      cout << "CP0 is " << (get_cp0() ? "high\n" : "low\n");
      cout << "CP1 is " << (get_cp1() ? "high\n" : "low\n");
    } else {

      cout << "code protection is " << (get_cp0() ? "enabled\n" : "disabled\n");

    }
  }


}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void ProgramMemoryAccess::callback()
{

  if(_state)
    {
      _state = 0;
      //cout << __FUNCTION__ << " address= " << address << ", opcode= " << opcode << '\n';
      //cpu->program_memory[address]->opcode = opcode;
      put_opcode(_address,_opcode);
      // FIXME trace.opcode_write(_address,_opcode);
      bp.clear_pm_write();
    }

}

//--------------------------------------------------
WDT::WDT(pic_processor *p_cpu, double _timeout)
  : gpsimObject("WDT","Watch Dog Timer"),
    cpu(p_cpu), breakpoint(0),prescale(1), postscale(128), future_cycle(0),
    timeout(_timeout), wdte(false), cfgw_enable(false)
{
}

//--------------------------------------------------
void WDT::update()
{
  if(wdte) {
    // FIXME - the WDT should not be tied to the instruction counter...
    guint64 delta_cycles;


    delta_cycles = (guint64)(postscale*prescale*timeout/get_cycles().seconds_per_cycle());

   if (verbose)
   {
        cout << "WDT::update timeout in " << (postscale*prescale*timeout);
        cout << " seconds (" << dec << delta_cycles << " cycles), ";
        cout << "CPU frequency " << (cpu->get_frequency()) << endl;
   }

    guint64 fc = get_cycles().get() + delta_cycles ;

    if(future_cycle) {


      if(verbose)
        cout << "WDT::update:  moving break from " << future_cycle << " to " << fc << '\n';

      get_cycles().reassign_break(future_cycle, fc, this);

    } else {
      get_cycles().set_break(fc, this);
    }
    future_cycle = fc;
  }

}

//--------------------------------------------------
// WDT::put - shouldn't be called?
//

void WDT::put(unsigned int new_value)
{
  cout << "WDT::put should not be called\n";
}
void WDT::set_timeout( double _timeout)
{
  timeout = _timeout;
  update();
}
//  TMR0 prescale is WDT postscale
void WDT::set_postscale(unsigned int newPostscale)
{
  unsigned int value = 1<< newPostscale;
  if (verbose)
      cout << "WDT::set_postscale postscale = " << dec << value << endl;
  if (value != postscale) {
    postscale = value;
    update();
  }
}
void WDT::swdten(bool enable)
{
    if (cfgw_enable)
        return;

    if (wdte != enable)
    {
        wdte = enable;
        warned = 0;
        if(verbose)
            cout << " WDT swdten "
                << ( (enable) ?  "enabling\n" : ", but disabling WDT\n");
        if (wdte)
        {
            update();
        }
        else
        {
            if (future_cycle) {
                cout << "Disabling WDT\n";
                get_cycles().clear_break(this);
                future_cycle = 0;
            }
        }
    }
}
// For WDT period select 0-11
void WDT::set_prescale(unsigned int newPrescale)
{
  unsigned int value = 1<< (5 + newPrescale);
  if (verbose)
      cout << "WDT::set_prescale prescale = " << dec << value << endl;
  if (value != prescale) {
    prescale = value;
    update();
  }
}
void WDT::initialize(bool enable)
{
  wdte = enable;
  cfgw_enable = enable;
  warned = 0;

  if(verbose)
    cout << " WDT init called "<< ( (enable) ? "enabling\n" :", but disabling WDT\n");

  if(wdte) {
        update();
  } else {

    if (future_cycle) {
      cout << "Disabling WDT\n";
      get_cycles().clear_break(this);
      future_cycle = 0;
    }
  }

}

void WDT::reset(RESET_TYPE r)
{
  switch (r) {
  case POR_RESET:
  case EXIT_RESET:
    update();
    break;
  case MCLR_RESET:
    if (future_cycle)
      get_cycles().clear_break(this);
    future_cycle = 0;
    break;
  default:
    ;
  }

}
void WDT::set_breakpoint(unsigned int bpn)
{
  breakpoint = bpn;
}

void WDT::callback()
{


  if(wdte) {
    if(verbose)
      cout<<"WDT timeout: " << hex << get_cycles().get() << '\n';



    if(breakpoint)
      bp.halt();
    else if (cpu->is_sleeping())
    {
        cout << "WDT expired during sleep\n";
        update();
        cpu->exit_sleep();
        cpu->status->put_TO(0);
    }
    else
    {
      // The TO bit gets cleared when the WDT times out.
        cout << "WDT expired reset\n";
        update();
        cpu->status->put_TO(0);
        cpu->reset(WDT_RESET);
    }
  }

}

void WDT::clear()
{
  if(wdte)
    update();
  else if(!warned) {
    warned = 1;
    cout << "The WDT is not enabled - clrwdt has no effect!\n";
  }
}

void WDT::callback_print()
{

  cout << "WDT\n";
}


//------------------------------------------------------------------------
// ConfigMemory - Base class
ConfigWord::ConfigWord(const char *_name, unsigned int default_val, const char *desc,
                       pic_processor *pCpu, unsigned int addr)
  : Integer(_name, default_val, desc), m_pCpu(pCpu), m_addr(addr)
{
  /*
  if (m_pCpu)
    m_pCpu->addSymbol(this);
  */
}

//------------------------------------------------------------------------
ConfigMemory::ConfigMemory(pic_processor *pCpu, unsigned int nWords)
  : m_pCpu(pCpu), m_nConfigWords(nWords)
{
  if (nWords > 0 && nWords < 100) {

    m_ConfigWords = new ConfigWord *[nWords];
    for (unsigned int i = 0; i < nWords; i++)
      m_ConfigWords[i] = 0;
  }
}

ConfigMemory::~ConfigMemory()
{

  for (unsigned int i = 0; i < m_nConfigWords; i++)
    if (m_ConfigWords[i])
      m_pCpu->deleteSymbol(m_ConfigWords[i]);

  delete [] m_ConfigWords;
}

int ConfigMemory::addConfigWord(unsigned int addr, ConfigWord *pConfigWord)
{
  if (addr < m_nConfigWords) {
    if (m_ConfigWords[addr])
	m_pCpu->deleteSymbol(m_ConfigWords[addr]);
    m_ConfigWords[addr] = pConfigWord;
    m_pCpu->addSymbol(pConfigWord);
    return 1;
  }
  delete pConfigWord;
  return 0;
}

ConfigWord *ConfigMemory::getConfigWord(unsigned int addr)
{
  return addr < m_nConfigWords ? m_ConfigWords[addr] : 0;
}
//-------------------------------------------------------------------
class MCLRPinMonitor : public PinMonitor
{
public:
  MCLRPinMonitor(pic_processor *pCpu);
  ~MCLRPinMonitor() {}

  virtual void setDrivenState(char);
  virtual void setDrivingState(char) {}
  virtual void set_nodeVoltage(double) {}
  virtual void putState(char) {}
  virtual void setDirection() {}
private:
  pic_processor *m_pCpu;
  char m_cLastResetState;
};

MCLRPinMonitor::MCLRPinMonitor(pic_processor *pCpu)
  : m_pCpu(pCpu),
    m_cLastResetState('I')  // I is not a valid state. It's used here for 'I'nitialization
{
}


void MCLRPinMonitor::setDrivenState(char newState)
{
  if (newState =='0' || newState =='w') {
    m_cLastResetState = '0';
    m_pCpu->reset(MCLR_RESET);
  }

  if (newState =='1' || newState =='W') {
    if (m_cLastResetState == '0')
      m_pCpu->reset(EXIT_RESET);

    m_cLastResetState = '1';
  }

}
//-------------------------------------------------------------------
void pic_processor::createMCLRPin(int pkgPinNumber)
{
  if (m_MCLR) {
    cout << "BUG?: assigning multiple MCLR pins: " << __FILE__ << dec << " " << __LINE__ << endl;
  }
  if(package) {
    m_MCLR = new IO_open_collector("MCLR");
    package->assign_pin(pkgPinNumber,m_MCLR);

    m_MCLRMonitor = new MCLRPinMonitor(this);
    m_MCLR->setMonitor(m_MCLRMonitor);
  }
}
//-------------------------------------------------------------------
// This function is called instead of createMCLRPin where the pin
// is already defined, but the configuration word has set the function
// to MCLR


void pic_processor::assignMCLRPin(int pkgPinNumber)
{
  if (package)
  {

    if (m_MCLR == NULL)
    {
	m_MCLR_pin = pkgPinNumber;
    	m_MCLR = new IO_open_collector("MCLR");
	m_MCLR_Save = package->get_pin(pkgPinNumber);
    	package->assign_pin(pkgPinNumber,m_MCLR, false);

    	m_MCLRMonitor = new MCLRPinMonitor(this);
    	m_MCLR->setMonitor(m_MCLRMonitor);
    	m_MCLR->newGUIname("MCLR");
    }
    else if (m_MCLR != package->get_pin(pkgPinNumber))
    {

        cout << "BUG?: assigning multiple MCLR pins: " 
	<< dec << pkgPinNumber << " " << __FILE__ <<  " " 
	<< __LINE__ << endl;
    }
  }
}
//-------------------------------------------------------------------
// This function sets the pin currently set as MCLR back to its original function
void pic_processor::unassignMCLRPin()
{
  
    if (package && m_MCLR_Save)
    {

	package->assign_pin(m_MCLR_pin,m_MCLR_Save, false);
        m_MCLR_Save->newGUIname(m_MCLR_Save->name().c_str());
	if (m_MCLR)
	{
	    m_MCLR->setMonitor(0);
	    delete m_MCLR;
	    m_MCLR = NULL;
	    if (m_MCLRMonitor)
	    {
		delete m_MCLRMonitor;
		m_MCLRMonitor = NULL;
	    }
	}
    }
}
//--------------------------------------------------
//
class IO_SignalControl : public SignalControl
{
public:
  IO_SignalControl(char _dir){ direction = _dir; }
  ~IO_SignalControl(){}
  char getState() { return direction; }
  void release() { }
private:
  char direction;
};

// This function sets a label on a pin and if PinMod is defined
// removes its control from it's port register
//
void pic_processor::set_clk_pin(unsigned int pkg_Pin_Number, 
		PinModule *PinMod, 
		const char * name,
		bool in,
                PicPortRegister *m_port,
                PicTrisRegister *m_tris,
                PicLatchRegister *m_lat)
{
  IOPIN *m_pin = package->get_pin(pkg_Pin_Number);
  if (name)
	m_pin->newGUIname(name);
  else
        m_pin->newGUIname(package->get_pin_name(pkg_Pin_Number).c_str());
  if (PinMod)
  {
	if (m_port)
	{
	    unsigned int mask = m_port->getEnableMask();
	    mask &= ~(1<< PinMod->getPinNumber());
	    m_port->setEnableMask(mask);
	    if (m_tris)
		m_tris->setEnableMask(mask);
	    if (m_lat)
		m_lat->setEnableMask(mask);
	}
    	PinMod->setSource(new IO_SignalControl('0'));
    	PinMod->setControl(new IO_SignalControl(in ? '1' : '0'));
 	PinMod->updatePinModule();
  }
}
// This function reverses the effects of the previous function
void pic_processor::clr_clk_pin(unsigned int pkg_Pin_Number, 
		PinModule *PinMod,
		PicPortRegister *m_port,
		PicTrisRegister *m_tris,
		PicLatchRegister *m_lat)
{
  IOPIN *m_pin = package->get_pin(pkg_Pin_Number);
  m_pin->newGUIname(package->get_pin_name(pkg_Pin_Number).c_str());
  if (PinMod)
  {
	if (m_port)
	{
	    unsigned int mask = m_port->getEnableMask();
	    mask |= (1<< PinMod->getPinNumber());
	    m_port->setEnableMask(mask);
	    if (m_tris)
		m_tris->setEnableMask(mask);
	    if (m_lat)
		m_lat->setEnableMask(mask);
	}
    	PinMod->setSource(0);
    	PinMod->setControl(0);
 	PinMod->updatePinModule();
  }
}

void pic_processor::osc_mode(unsigned int value)
{
  IOPIN *m_pin;
  unsigned int pin_Number =  get_osc_pin_Number(0);
  
  if (pin_Number < 253)
  {
	m_pin = package->get_pin(pin_Number);
  }
  if ( (pin_Number =  get_osc_pin_Number(1)) < 253 &&
	(m_pin = package->get_pin(pin_Number)))
  {
	pll_factor = 0;
        if (value < 5)
	{
	    set_clk_pin(pin_Number, m_osc_Monitor[1], "OSC2", true);
	}
	else if(value == 6 )
	{
	    pll_factor = 2;
	    set_clk_pin(pin_Number, m_osc_Monitor[1], "CLKO", false);
	}
	else
	{
	    clr_clk_pin(pin_Number, m_osc_Monitor[1]);
	}
  }
  
}
