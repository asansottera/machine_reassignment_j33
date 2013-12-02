#!/bin/bash

if [ $# -ne 4 ]; then
	echo "Wrong number of arguments. Example: ./runall.sh opt 300 a1 local_search"
	exit -1
fi

BUILD=$1
TLIM=$2
ISET=$3
INUMS=0
HEURISTIC=$4

if [ "$ISET" == "a1" -o "$ISET" == "a2" ]; then
    INUMS=`seq 1 5`
elif [ "$ISET" == "b" ]; then
    INUMS=`seq 1 10`
else
    echo "Unknown instance set $ISET. Example: a1 a2 b"
    exit -1
fi

for i in $INUMS; do
    ./run.sh $BUILD $TLIM $ISET $i $HEURISTIC
done

