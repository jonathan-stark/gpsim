#!/bin/bash
#
# A meta-script that invokes each of the individual
# regression tests.

RT=./rt.sh
# Instruction set simulation of the mid-range devices

${RT} instructions_14bit/instructions_14bit

${RT} node_test/node_test

${RT} p16f84/p16f84
