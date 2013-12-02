#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Wrong number of arguments. Example: ./analyze.sh a1 1"
	exit -1
fi

EXEC=bin/hybrid_heuristic_opt
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

echo "Analyzing instance $INUM of set $ISET"
$EXEC -p $PROBLEM -i $INPUT -o $OUTPUT -a
