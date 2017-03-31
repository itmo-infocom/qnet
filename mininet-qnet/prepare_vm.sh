#!/bin/bash
for i in openvswitch stunnel ryu mininet python-daemon squid tcpmux qcrypt qcrypt-fake-keys; do
   echo installing $i
   yum install $i -y
done

PEM_FILE=/etc/stunnel/stunnel.pem
if [ ! -f $PEM_FILE ]; then
  echo genearting stunnel pem file
  openssl req -new -x509 -days 365 -nodes  -out $PEM_FILE -keyout $PEM_FILE
fi;

echo enabling openvswitch
systemctl enable openvswitch 

echo restarting openvswitch
systemctl restart openvswitch 

echo enabling squid
systemctl enable squid 

echo restarting squid
systemctl restart squid 


echo creating chroots for stunnel
for i in `seq 2 5`; do
  mkdir -p /var/tmp/stunnel-n${i}/var/log/
  chown nobody.nobody /var/tmp/stunnel-n${i} -R
done
