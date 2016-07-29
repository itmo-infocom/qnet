#!/bin/bash
echo enabling openvswitch
systemctl enable openvswitch 

echo restarting openvswitch
systemctl restart openvswitch 


echo creating chroots for stunnel
for i in `seq 2 5`; do
  mkdir -p /var/tmp/stunnel-n${i}/var/log/
  chown nobody.nobody /var/tmp/stunnel-n${i}
done
