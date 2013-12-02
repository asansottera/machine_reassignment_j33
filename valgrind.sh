#!/bin/bash

if [ $# -ne 4 ]; then
	echo "Wrong number of arguments. Example: ./valgrind.sh 300 a1 1 local_search"
	exit -1
fi

TLIM=$1
ISET=$2
INUM=$3
HEURISTIC=$4
EXEC=bin/hybrid_heuristic_dbg

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

echo "Running on instance $INUM of set $ISET"
valgrind $EXEC -t $TLIM -p $PROBLEM -i $INPUT -o $OUTPUT -h $HEURISTIC -name
