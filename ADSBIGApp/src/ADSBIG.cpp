
/**
 * Area detector driver for SBIG Astronomical Instruments cameras.
 * 
 * This driver was developed using a ST-8300 and version
 * 1.31 of their class library on RHEL6/7.
 *
 * Matt Pearson
 * Feb 2016
 *
 */

#include "ADSBIG.h"

/**
 * Constructor
 */
ADSBIG::ADSBIG(const char *portName, int maxBuffers, size_t maxMemory) : 
  ADDriver(portName, 1, NUM_DRIVER_PARAMS, 
	   maxBuffers, maxMemory, 
	   asynInt32Mask | asynFloat64Mask | asynDrvUserMask,
	   asynInt32Mask | asynFloat64Mask,
	   ASYN_CANBLOCK | ASYN_MULTIDEVICE,
	   1, 0, 0)
{
  const char *functionName = "ADSBIG::ADSBIG";


  printf("%s Created OK.\n", functionName);
}

/**
 * Destructor. Should never get here.
 */
ADSBIG::~ADSBIG()
{
  printf("ERROR: ADSBIG::~ADSBIG Called.\n");
}
