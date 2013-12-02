#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Wrong number of arguments. Example: ./check.sh a1 1"
	exit -1
fi

EXEC=../solution_checker/solution_checker
ISET=$1
INUM=$2

if [[ $ISET = a1 ]]; then
	IDIR=../data_A_first_part
elif [[ $ISET = a2 ]]; then
	IDIR=../data_A_second_part
elif [[ $ISET = b ]]; then
	IDIR=../data_B
fi

PROBLEM=$IDIR/model_${ISET}_${INUM}.txt
INPUT=$IDIR/assignment_${ISET}_${INUM}.txt
OUTPUT=sol_${ISET}_${INUM}.txt

echo "Checking instance $INUM of set $ISET"
$EXEC $PROBLEM $INPUT $OUTPUT
