TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

USR_CXXFLAGS += -Wunused-but-set-variable

LIBRARY_IOC += ADSBIGSupport

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# Install devXxxSoft.dbd into <top>/dbd
DBD += ADSBIGSupport.dbd

# Compile and add the code to the support library
ADSBIGSupport_SRCS += ADSBIG.cpp

# Link locally-provided code into the support library,
# rather than directly into the IOC application.
#ADSBIGSupport_SRCS += dbSubExample.c

ADSBIGSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

