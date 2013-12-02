#!/bin/bash

if [ $# -ne 5 ]; then
	echo "Wrong number of arguments. Example: ./run.sh opt 300 a1 1 local_search"
	exit -1
fi

BUILD=$1
TLIM=$2
ISET=$3
INUM=$4
HEURISTIC=$5
EXEC=bin/hybrid_heuristic_$BUILD

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
taskset 0x00000003 $EXEC -t $TLIM -p $PROBLEM -i $INPUT -o $OUTPUT -h $HEURISTIC -name
