#!/bin/bash

ADDTHRESH=16

if [[ "$1" =~ [0-9]{1,3} ]]; then
    ADDTHRESH="$1"
fi

M1=0
while [ $M1 -lt 256 ]; do
    M2=0
    while [ $M2 -lt 256 ]; do
        cat <<EOT > params.inc
ALGTHRESH       equ     07fh
NUMERATOR       equ     $M1
DENOMINATOR     equ     $M2

EOT
        printf "%d,%d" $M1 $M2
        make run | egrep '(^[A2]|CYCLS)' | sed -r -e 's/^.*CYCLS\]=\[//' -e 's/\]\[ *([0-9]*) .s.*$/ \1/' | awk '/^[A2]/{printf(",%s",$1);}/^0x/{printf(",%d,%d\n",$1,$2);}'
        M2=$((M2+1))
    done
    M1=$((M1+1))
done
