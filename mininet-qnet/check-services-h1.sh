#!/bin/bash

for i in 1 3; do
   echo n${i}
   ssh 10.0.0.$i 'ss -nlp|grep LISTEN'
   echo
done
