#!/bin/sh
#
# A meta-script that invokes each of the individual
# regression tests.

RT=./rt.sh
# Instruction set simulation of the mid-range devices

${RT} instructions_14bit instructions_14bit

${RT} node_test node_test

#${RT} p12c509 it_12bit
#${RT} p12c509 tmr0

${RT} p16f84 p16f84

#${RT} p16f628 p16f628

#${RT} p16f873 p16f873
