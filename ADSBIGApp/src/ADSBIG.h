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

#ifndef ADSBIG_H
#define ADSBIG_H

#include <stddef.h>

#include "ADDriver.h"

class ADSBIG : public ADDriver {

 public:
  ADSBIG(const char *portName, int maxBuffers, size_t maxMemory);
  virtual ~ADSBIG();

  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual void report(FILE *fp, int details);

 private:
  
  //Parameter library indices
  int ADSBIGFirstParam;
  #define ADSBIG_FIRST_PARAM ADSBIGFirstParam

  int ADSBIGLastParam;
  #define ADSBIG_LAST_PARAM ADSBIGLastParam
  
};

#define NUM_DRIVER_PARAMS (&ADSBIG_LAST_PARAM - &ADSBIG_FIRST_PARAM + 1)

#endif //ADSBIG_H
