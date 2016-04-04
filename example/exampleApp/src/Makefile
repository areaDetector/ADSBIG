TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC application

PROD_IOC = example
# example.dbd will be created and installed
DBD += example.dbd

USR_LDFLAGS += -L/usr/local/lib

# example.dbd will be made up from these files:
example_DBD += commonDriverSupport.dbd
example_DBD += ADSBIGSupport.dbd
example_DBD += devIocStats.dbd
example_DBD += sseqRecord.dbd

# example_registerRecordDeviceDriver.cpp derives from example.dbd
example_SRCS += example_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
example_SRCS_DEFAULT += exampleMain.cpp
example_SRCS_vxWorks += -nil-

# Link in the code from the support library
include $(ADCORE)/ADApp/commonDriverMakefile
example_LIBS += ADSBIGSupport
example_LIBS += devIocStats
SYS_PROD_LIBS += sbigudrv

# Finally link to the EPICS Base libraries
example_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

