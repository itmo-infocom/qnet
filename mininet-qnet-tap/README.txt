mininet with tap tunnels

requirements: pyyaml, mininet

current config (test.yaml) assumes that qnet subtree from git is at /root/qnet and ctap and keyworker are compiled to binaries
current config file will setup following mininet configuration on a single VM:

in -- s1 - tap0--tap1 - s2 -- out

s1 and s2 openvswitches with default controller,
in and out - virtual nodes with 10.0.0.1 and 10.0.0.2 ip correspondingly

out node additionally runs SimpleHTTPServer at 8000 port which shares /root/qnet/mininet-qnet-tap/data folder

to test connectivity one can use commands of mininet CLI:
- mininet> in ping 10.0.0.2
- mininet> in curl -o /dev/null http://10.0.0.2:8000/test.dat

To exit mininet CLI use ctrl-D, this will stop all virtual infrastructure.

Another way to run commands on nodes is to use switch_to_vhost.sh:
# sh switch_to_vhost.sh in|out
to exit from namespace simply exit bash instance.


OpenFlow:
There is no need for any custom (dynamic) rules in this configuration.
