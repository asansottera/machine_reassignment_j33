#!/bin/bash

if [ $# -ne 4 ]; then
	echo "Wrong number of arguments. Example: ./run_batch.sh 300 a1 1 local_search"
	exit -1
fi

TLIM=$1
ISET=$2
INUM=$3
HEURISTIC=$4
SEEDS=`seq 10 10 100`
LOG=batch_${ISET}_${INUM}.txt

rm -f $LOG
touch $LOG
for s in $SEEDS; do
	./run_seed.sh opt $TLIM $ISET $INUM $HEURISTIC $s
	./check.sh $ISET $INUM | grep "valid" | sed 's%.*objective cost is.\([0-9]*\)%\1%'  >> $LOG
done
