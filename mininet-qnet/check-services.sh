#!/bin/bash

for i in `seq 1 5`; do
   echo n${i}
   ssh 10.0.0.$i 'ss -nlp|grep LISTEN'
   echo
done
