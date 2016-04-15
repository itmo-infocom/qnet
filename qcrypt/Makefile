all:
	echo Choose type of setup: n1 n2 n4 n5 s1 s2

common:
	ln -sf /opt/qcrypt/etc/init.d/* /etc/init.d/
	ln -sf /opt/qcrypt/etc/xinetd.d/crypt /etc/xinetd.d/
	ln -sf /opt/qcrypt/etc/stunnel/* /etc/stunnel/
	grep -q quncrypt /etc/services || cat /opt/qcrypt/etc/services >> /etc/services
	chkconfig --add stunneld
	chkconfig --add ryu
	chkconfig --add ofsetup

stunnel:
	chkconfig --level 345 stunneld on
	service stunneld restart

ryu:
	chkconfig --level 345 ryu on
	service ryu restart

ofsetup:
	chkconfig --level 345 ofsetup on
	service ofsetup restart

httpd:
	chkconfig --level 345 httpd on
	service httpd restart

stunnel_server:
	ln -sf /opt/qcrypt/etc/sysconfig/stunnel.server /etc/sysconfig/stunnel

stunnel_client:
	ln -sf /opt/qcrypt/etc/sysconfig/stunnel.client /etc/sysconfig/stunnel

s1_setup: 
	ln -sf /opt/qcrypt/share/of/setup.s1 /opt/qcrypt/share/of/setup

s2_setup:
	ln -sf /opt/qcrypt/share/of/setup.s2 /opt/qcrypt/share/of/setup

n2_setup: 
	ln -sf /opt/qcrypt/etc/xinetd.d/crypt.n2 /etc/xinetd.d/crypt

s1: common s1_setup stunnel_client stunnel ofsetup
	echo Setup s1

s2: common s2_setup ofsetup ryu
	echo Setup s2

n1: 
	echo Setup n1

n2: common n2_setup stunnel_server stunnel httpd
	echo Setup n2

n4: common stunnel_server stunnel
	echo Setup n4

n5: common stunnel_client stunnel 
	echo Setup n5
	
