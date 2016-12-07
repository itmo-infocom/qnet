#!/bin/sh
RYUHOST=localhost

curl http://${RYUHOST}:8080/qchannel/1/4
curl http://${RYUHOST}:8080/qchannel/2/4

curl http://${RYUHOST}:8080/qchannel/1/$1
curl http://${RYUHOST}:8080/qchannel/2/$2

