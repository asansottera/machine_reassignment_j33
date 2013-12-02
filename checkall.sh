#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Wrong number of arguments. Example: ./checkall.sh a1"
	exit -1
fi

ISET=$1

if [ "$ISET" == "a1" -o "$ISET" == "a2" ]; then
    INUMS=`seq 1 5`
elif [ "$ISET" == "b" ]; then
    INUMS=`seq 1 10`
else
    echo "Unknown instance set $ISET. Example: a1 a2 b"
    exit -1
fi

for i in $INUMS; do
	./check.sh $ISET $i
done
