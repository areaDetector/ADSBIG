
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

//Epics headers
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <iocsh.h>
#include <drvSup.h>
#include <registryFunction.h>

#include "ADSBIG.h"

static void ADSBIGReadoutTaskC(void *drvPvt);

/**
 * Constructor
 */
ADSBIG::ADSBIG(const char *portName, int maxBuffers, size_t maxMemory) : 
  ADDriver(portName, 1, NUM_DRIVER_PARAMS, 
	   maxBuffers, maxMemory, 
	   asynInt32Mask | asynInt32ArrayMask | asynDrvUserMask,
	   asynInt32Mask | asynFloat64Mask, 
	   ASYN_CANBLOCK | ASYN_MULTIDEVICE,
	   1, 0, 0)
{
  int status = 0;
  const char *functionName = "ADSBIG::ADSBIG";

  m_Acquiring = 0;

  //Create the epicsEvent for signaling the data task.
  //This will cause it to do a poll immediately, rather than wait for the poll time period.
  m_startEvent = epicsEventMustCreate(epicsEventEmpty);
  if (!m_startEvent) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for start event.\n", functionName);
    return;
  }
  m_stopEvent = epicsEventMustCreate(epicsEventEmpty);
  if (!m_stopEvent) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for stop event.\n", functionName);
    return;
  }

  //Add the params to the paramLib 
  //createParam adds the parameters to all param lists automatically (using maxAddr).
  createParam(ADSBIGFirstParamString,     asynParamInt32,    &ADSBIGFirstParam);
  createParam(ADSBIGStartParamString,     asynParamInt32,    &ADSBIGStartParam);
  createParam(ADSBIGLastParamString,     asynParamInt32,    &ADSBIGLastParam);

  //Connect to camera here and get library handle
  printf("%s Connecting to camera...\n", functionName);

  //Create the thread that reads the data 
  status = (epicsThreadCreate("ADSBIGReadoutTask",
                            epicsThreadPriorityMedium,
                            epicsThreadGetStackSize(epicsThreadStackMedium),
                            (EPICSTHREADFUNC)ADSBIGReadoutTaskC,
                            this) == NULL);
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	      "%s epicsThreadCreate failure for ADSBIGReadoutTask.\n", functionName);
    return;
  }

  printf("%s Created OK.\n", functionName);
}

/**
 * Destructor. Should never get here.
 */
ADSBIG::~ADSBIG()
{
  printf("ERROR: ADSBIG::~ADSBIG Called.\n");
}

/**
 *
 */
void ADSBIG::report(FILE *fp, int details)
{
  printf("TODO\n");
}


/**
 * writeInt32. Write asyn integer values.
 */
asynStatus ADSBIG::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  asynStatus status = asynSuccess;
  int function = pasynUser->reason;
  int addr = 0;
  int adStatus = 0;
  const char *functionName = "ADSBIG::writeInt32";
  
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Entry.\n", functionName);

  getIntegerParam(ADStatus, &adStatus);

  if (function == ADAcquire) {
    if ((value==1) && (adStatus == ADStatusIdle)) {
      m_Acquiring = 1;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Start Event.\n", functionName);
      printf("Sending start event\n");
      epicsEventSignal(this->m_startEvent);
    }
    if ((value==0) && (adStatus != ADStatusIdle)) {
      printf("Aborting acqusition.\n");
    }
  }

  if (status != asynSuccess) {
    callParamCallbacks(addr);
    return asynError;
  }

  status = (asynStatus) setIntegerParam(addr, function, value);
  if (status!=asynSuccess) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s Error Setting Parameter. Asyn addr: %d, asynUser->reason: %d, value: %d\n", 
              functionName, addr, function, value);
    return(status);
  }

  //Do callbacks so higher layers see any changes 
  callParamCallbacks(addr);

  return status;
}


/**
 * writeFloat64. Write asyn float values.
 */
asynStatus ADSBIG::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  asynStatus status = asynSuccess;
  int function = pasynUser->reason;
  int addr = 0;
  int adStatus = 0;
  const char *functionName = "ADSBIG::writeFloat64";
  
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Entry.\n", functionName);

  getIntegerParam(ADStatus, &adStatus);


  //Logic goes here


  if (status != asynSuccess) {
    callParamCallbacks(addr);
    return asynError;
  }

  status = (asynStatus) setDoubleParam(addr, function, value);
  if (status!=asynSuccess) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s Error Setting Parameter. Asyn addr: %d, asynUser->reason: %d, value: %f\n", 
              functionName, addr, function, value);
    return(status);
  }

  //Do callbacks so higher layers see any changes 
  callParamCallbacks(addr);

  return status;
}


/**
 * Readout thread function
 */
void ADSBIG::readoutTask(void)
{
  asynStatus status = asynError;
  epicsEventWaitStatus eventStatus;
  epicsFloat64 timeout = 0.001;
  bool error = false;
  bool acquire = false;

  const char* functionName = "ADSBIG::readoutTask";
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Started Readout Thread.\n", functionName);

  while (1) {

    //Wait for a stop event, with a short timeout, to catch any that were done after last one.
    eventStatus = epicsEventWaitWithTimeout(m_stopEvent, timeout);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Event Before Start Event.\n", functionName);
    }

    lock();
    setIntegerParam(ADAcquire, 0);
    if (!error) {
      setStringParam(ADStatusMessage, "Idle");
    }
    callParamCallbacks();
    unlock();

    eventStatus = epicsEventWait(m_startEvent);          
    if (eventStatus == epicsEventWaitOK) {
      printf("Got start event.");
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Start Event.\n", functionName);
      acquire = true;
      error = false;
      lock();

      

      //Complete Start callback
      callParamCallbacks();
      setIntegerParam(ADSBIGStartParam, 0);
      callParamCallbacks();
      unlock();

    } //end of start event

  } //end of while(1)

  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	    "%s: ERROR: Exiting ADSBIGReadoutTask main loop.\n", functionName);

}


//Global C utility functions to tie in with EPICS
static void ADSBIGReadoutTaskC(void *drvPvt)
{
  ADSBIG *pPvt = (ADSBIG *)drvPvt;
  
  pPvt->readoutTask();
}


/*************************************************************************************/
/** The following functions have C linkage, and can be called directly or from iocsh */

extern "C" {

/**
 * Config function for IOC shell. It instantiates an instance of the driver.
 * @param portName The Asyn port name to use
 * @param maxBuffers Used by asynPortDriver (set to -1 for unlimited)
 * @param maxMemory Used by asynPortDriver (set to -1 for unlimited)
 */
  asynStatus ADSBIGConfig(const char *portName, int maxBuffers, size_t maxMemory)
  {
    asynStatus status = asynSuccess;
    
    /*Instantiate class.*/
    try {
      new ADSBIG(portName, maxBuffers, maxMemory);
    } catch (...) {
      printf("Unknown exception caught when trying to construct ADSBIG.\n");
      status = asynError;
    }
    
    return(status);
  }


 /* Code for iocsh registration */
  
  /* ADSBIGConfig */
  static const iocshArg ADSBIGConfigArg0 = {"Port name", iocshArgString};
  static const iocshArg ADSBIGConfigArg1 = {"Max Buffers", iocshArgInt};
  static const iocshArg ADSBIGConfigArg2 = {"Max Memory", iocshArgInt};
  static const iocshArg * const ADSBIGConfigArgs[] =  {&ADSBIGConfigArg0,
                                                         &ADSBIGConfigArg1,
                                                         &ADSBIGConfigArg2};
  
  static const iocshFuncDef configADSBIG = {"ADSBIGConfig", 3, ADSBIGConfigArgs};
  static void configADSBIGCallFunc(const iocshArgBuf *args)
  {
    ADSBIGConfig(args[0].sval, args[1].ival, args[2].ival);
  }

  static void ADSBIGRegister(void)
  {
    iocshRegister(&configADSBIG, configADSBIGCallFunc);
  }
  
  epicsExportRegistrar(ADSBIGRegister);

} // extern "C"
