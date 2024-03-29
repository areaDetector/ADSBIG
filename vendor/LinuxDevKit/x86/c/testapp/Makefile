####################################################################################
# The Makefile of SBIG Linux USB testing application.
# Author: Matt Longmire, SBIG
#         Jan Soldan, Consultant at Aplegen/SBIG
# History:
# 2012-11-24: JS updated to the latest driver utilizing the libusb 1.0.X library.
# 2011-11-08: MT & JS updated to the latest driver utilizing the libusb 0.1 library.
####################################################################################
####################################################################################
# Note: Adapt this Makefile to your needs, ie.:

# APP_OUT_DIR
# APP_OUT_DIR
# APP_OUT_DIR

# then run:
 
# make clean -f Makefile
# make -f Makefile

# Note: Linker uses shared libraries, so you can put *.so library to the 
# /usr/local/lib/ directory and setenv LD_LIBRARY_PATH to it. 
####################################################################################
APP_OUT_DIR = ./
APP_NAME    = testapp
S_APP       = testmain.cpp csbigcam.cpp csbigimg.cpp
O_APP       = testmain.o csbigcam.o csbigimg.o
####################################################################################
APP_SRC    = $(S_APP)
APP_OBJ    = $(O_APP)

INCLUDE    = -I .ls -I /usr/include/libusb-1.0/ -I /data/fits/cfitsio
LIBRARY    = -L /usr/local/lib -L /data/fits/cfitsio/lib/

APP_CFLAGS = -fPIC $(INCLUDE) $(LIBRARY) -Wall
APP_CC     = g++
####################################################################################
all: $(APP_NAME)
install: all
####################################################################################
# APP_NAME
####################################################################################
$(APP_NAME): $(APP_OBJ)
	  $(APP_CC) $(APP_CFLAGS) -o $(APP_OUT_DIR)$(APP_NAME) $(APP_OBJ) -lsbigudrv -lm
	  chmod a+x $(APP_OUT_DIR)$(APP_NAME)

testmain.o : testmain.cpp csbigcam.h csbigimg.h
	  $(APP_CC) $(APP_CFLAGS) -o testmain.o -c testmain.cpp

csbigcam.o : csbigcam.cpp csbigcam.h csbigimg.h
	  $(APP_CC) $(APP_CFLAGS) -o csbigcam.o -c csbigcam.cpp

csbigimg.o : csbigimg.cpp csbigimg.h
	  $(APP_CC) $(APP_CFLAGS) -o csbigimg.o -c csbigimg.cpp
####################################################################################
clean:
		rm -f *.o
		rm -f *.so
		rm -f *.a
####################################################################################
