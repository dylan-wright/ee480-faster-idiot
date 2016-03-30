#!/bin/bash

for f in *.idiot
do
    base=${f%.idiot}
    text=${base}.text.vmem
    data=${base}.data.vmem
    echo "-> assembling $base"
    echo $f | ./aik.py > $text 2> $data
done
