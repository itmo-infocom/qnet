#!/bin/sh
flow1=/dev/null
flow2=/dev/null

flow1=/dev/stdout
flow2=/dev/stderr



echo loading keys
./KeyByCURL.out 127.0.0.1:8080/qkey/1  1>$flow1 2>$flow2
./KeyByCURL.out 127.0.0.1:8080/qkey/2  1>$flow1 2>$flow2

