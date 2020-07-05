# get_mac
Get full MAC of a BT device using the LAP and a dictionary file of OUI<br />
##Requirements
BlueZ installed<br />

##Usage
gcc -o get_mac get_mac.c -lbluetooth<br />get_mac -d[oui file] -l[lap]<br />./get_mac -dapl.oui -l11:22:33
