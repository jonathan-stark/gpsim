#!/bin/sh
#
# A meta-script that invokes each of the individual
# regression tests.

if [ $# -gt 1 ]
then
  echo "Usage: `basename $0` [ <gpsim_path> ] "
  exit 0
fi

case "$1" in
    *clean)
        DIRS='breakpoints instructions_12bit instructions_14bit instructions_16bit 
node_test p16f84 p18f452_ports p16f628 digital_stim analog_stim p12ce518 eeprom_wide 
interrupts_14bit macro_test logic_test resistor usart_test txisr_test tmr0_16bit 
switch_test p18f comparator a2d psp ttl ccp wavegen spi i2c port_stim 
p12c509'

        echo ${DIRS}
        for i in ${DIRS} ; do
            echo $i
            cd $i
            make clean
            cd ..
        done

        exit 0
        ;;

    *)
        if [ $# -gt 0 ]
            then
            GPSIM_PATH=$1
            export GPSIM_PATH
        fi
        ;;
esac

RT=./rt.sh

# Basic breakpoint test
${RT} breakpoints sim

# Instruction set simulation of the mid-range devices
${RT} instructions_14bit sim_instructions_14bit
${RT} instructions_14bit sim_branching

#instruction set simulation for the 16bit cores:
${RT} instructions_16bit sim

#instruction set simulation for the 12bit cores:
${RT} instructions_12bit sim

${RT} node_test sim

#${RT} p12c509 it_12bit
#${RT} p12c509 tmr0

${RT} p16f84 sim

${RT} p18f452_ports sim

#${RT} p16f628 p16f628

#${RT} p16f873 sim

${RT} digital_stim sim

${RT} analog_stim sim

${RT} register_stim sim

${RT} p12ce518 sim

${RT} eeprom_wide sim

${RT} interrupts_14bit sim

${RT} macro_test sim

${RT} logic_test sim

${RT} resistor sim

${RT} usart_test sim_pir1v1

${RT} usart_test sim_pir1v2

${RT} txisr_test sim

${RT} tmr0_16bit sim

${RT} switch_test sim

${RT} p18f test1

${RT} p18f test2

${RT} comparator sim_628

${RT} comparator sim_877a

${RT} a2d sim_p16c71

${RT} a2d sim_p16f871

${RT} a2d sim_p16f873a

${RT} a2d sim_p16f874a

${RT} a2d sim_p16f88

${RT} a2d sim_p16f819

${RT} a2d sim_p18f452

${RT} psp sim_p18f452

${RT} psp sim_p16f871

${RT} ttl sim_377

${RT} ccp sim_ccp_877a

${RT} ccp sim_ccp_819

${RT} ccp sim_pwm_877a

${RT} wavegen sim

${RT} spi sim_p16f88

${RT} spi sim_p18f242

${RT} spi sim_p16c62

${RT} i2c sim_p16f88

${RT} i2c sim_p16f819

${RT} i2c sim_p16f876a

${RT} port_stim sim_port_stim

echo "The following tests only pass if CLOCK_EXPERIMENTS"
echo "(in src/clock_phase.cc) is defined."
${RT} p12c509 sim_p12c509_reset

${RT} p16f84 sim_reset

