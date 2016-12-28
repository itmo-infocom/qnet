Initial configuration of "qcrypt" Ryu application must be placed into /var/lib/qcrypt/channels.json. You may find sample of this file in this directory.

Running:

ryu-manager --verbose ryu.app.qcrypt

or for enabling of low level REST-API control for OpenFlow switches:

ryu-manager --verbose ryu.app.qcrypt ryu.app.ofctl_rest

REST-API:
/conf GET -- get current channels configuration
/conf POST -- set current channels configuration
/qchannel/{channel}/{status} GET -- change channels status:
 0 -- scrypt
 1 -- qcrypt
 2 -- reread keys (currently not used)
 3 -- transparent channel without encoding
 4 -- delete flow-entry
 5 -- run crypto_stat
 6 -- run crypto_stat_get
