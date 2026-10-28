# Bind the CA server to the VirtualBox Host-only interface.
# Update this address if the VM Host-only IP changes.
epicsEnvSet("EPICS_CAS_INTF_ADDR_LIST", "192.168.56.105")

dbLoadRecords("ioc/db/esp32-real.db")
iocInit()
