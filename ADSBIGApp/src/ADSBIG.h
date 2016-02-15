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

#include <epicsTime.h>
#include <epicsTypes.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <cantProceed.h>

#include <asynOctetSyncIO.h>

#include "ADDriver.h"

#define ADSBIGFirstParamString              "ADSBIG_FIRST"
#define ADSBIGStartParamString               "ADSBIG_START"
#define ADSBIGLastParamString               "ADSBIG_LAST"

class ADSBIG : public ADDriver {

 public:
  ADSBIG(const char *portName, int maxBuffers, size_t maxMemory);
  virtual ~ADSBIG();

  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

  virtual void report(FILE *fp, int details);

  void readoutTask(void); 

 private:
  
  //Private functions go here

  //Private static data members

  //Private dynamic data members
  epicsUInt32 m_Acquiring;
  CSBIGCam *p_Cam;
  CSBIGImg *p_Img;
  int m_CamWidth;
  int m_CamHeight;
  bool m_df;
  
  epicsEventId m_startEvent;
  epicsEventId m_stopEvent;

  //Parameter library indices
  int ADSBIGFirstParam;
  #define ADSBIG_FIRST_PARAM ADSBIGFirstParam
  int ADSBIGStartParam;
  int ADSBIGLastParam;
  #define ADSBIG_LAST_PARAM ADSBIGLastParam
  
};

#define NUM_DRIVER_PARAMS (&ADSBIG_LAST_PARAM - &ADSBIG_FIRST_PARAM + 1)

#endif //ADSBIG_H
