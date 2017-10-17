#!/bin/bash
for i in openvswitch stunnel ryu mininet python-daemon squid tcpmux KeyByCURL of-qnet pythoncrypt httpd; do
   echo installing $i
   yum install $i -y
done

#pip install nifpga

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

echo configuring firewall

firewall-cmd --permanent --zone=public --add-port=6633/tcp
firewall-cmd --permanent --direct --add-rule ipv4 filter INPUT 0 -p gre -j ACCEPT
firewall-cmd --permanent --zone=public --add-port=8080/tcp
firewall-cmd --permanent --zone=public --add-port=3128/tcp
firewall-cmd --reload


