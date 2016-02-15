TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

USR_CXXFLAGS += -Wunused-but-set-variable

#LIBUSB1 is set in configure/CONFIG_SITE
USR_INCLUDES += -I$(LIBUSB1)
USR_INCLUDES += -I$(ADSBIG_VENDOR)

LIBRARY_IOC += ADSBIGSupport

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# Install devXxxSoft.dbd into <top>/dbd
DBD += ADSBIGSupport.dbd

# Compile and add the code to the support library
ADSBIGSupport_SRCS += ADSBIG.cpp

USR_OBJS += $(ADSBIG_VENDOR)/csbigcam.o
USR_OBJS += $(ADSBIG_VENDOR)/csbigimg.o

#ADSBIGSupport_SRCS += $(ADSBIG_VENDOR)/csbigcam.cpp
#ADSBIGSupport_SRCS += $(ADSBIG_VENDOR)/csbigimg.cpp

ADSBIGSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

