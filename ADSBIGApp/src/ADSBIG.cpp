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

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <iocsh.h>
#include <drvSup.h>
#include <registryFunction.h>

#include <libusb.h>

#include "lpardrv.h"
#include "csbigcam.h"
#include "csbigimg.h"

#include "ADSBIG.h"

static void ADSBIGReadoutTaskC(void *drvPvt);
static void ADSBIGPollingTaskC(void *drvPvt);

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
  m_CamWidth = 0;
  m_CamHeight = 0;

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
  createParam(ADSBIGDarkFieldParamString, asynParamInt32,    &ADSBIGDarkFieldParam);
  createParam(ADSBIGPercentCompleteParamString, asynParamFloat64,  &ADSBIGPercentCompleteParam);
  createParam(ADSBIGLastParamString,      asynParamInt32,    &ADSBIGLastParam);

  //Connect to camera here and get library handle
  printf("%s Connecting to camera...\n", functionName);
  p_Cam = new CSBIGCam(DEV_USB1);
  PAR_ERROR cam_err = CE_NO_ERROR;
  if (p_Cam == NULL) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s. CSBIGCam constructor failed. p_Cam is NULL.\n", functionName);
    return;
  }
  if ((cam_err = p_Cam->GetError()) != CE_NO_ERROR) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s. CSBIGCam constructor failed. %s\n", 
	      functionName, p_Cam->GetErrorString(cam_err).c_str());
    return;
  }

  if ((cam_err = p_Cam->EstablishLink()) != CE_NO_ERROR) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s. Failed to establish link to camera. %s\n", 
	      functionName, p_Cam->GetErrorString(cam_err).c_str());
    return;
  }

  printf("%s Successfully connected to camera: %s\n", 
	 functionName, p_Cam->GetCameraTypeString().c_str());

  //Set some default camera modes
  p_Cam->SetActiveCCD(CCD_IMAGING);
  p_Cam->SetReadoutMode(RM_1X1);
  p_Cam->SetExposureTime(1);
  //A lot of defaults are set in the CSBIGCam::Init function as well.
  //FastReadout only seems to apply to the STF-8300.
  p_Cam->SetABGState(ABG_LOW7);
  p_Cam->SetFastReadout(false); 
  p_Cam->SetDualChannelMode(false);

  if ((cam_err = p_Cam->GetFullFrame(m_CamWidth, m_CamHeight)) !=  CE_NO_ERROR) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
  		"%s. Failed to read camera dimensions. %s\n", 
  	      functionName, p_Cam->GetErrorString(cam_err).c_str());
    return;
  } else {
    printf("%s Camera Width: %d\n", functionName, m_CamWidth);
    printf("%s Camera Height: %d\n", functionName, m_CamHeight);
  }

  p_Cam->SetSubFrame(0, 0, m_CamWidth, m_CamHeight);

  

  //Create image object
  p_Img = new CSBIGImg();
  if (p_Img->AllocateImageBuffer(m_CamWidth, m_CamHeight) != TRUE) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
  		"%s. Failed to allocate image buffer for maximum image size.\n", 
  	      functionName);
    return;
  }

  bool paramStatus = true;
  //Initialise any paramLib parameters that need passing up to device support
  paramStatus = ((setStringParam(ADManufacturer, "SBIG") == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam(ADModel, p_Cam->GetCameraTypeString().c_str()) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADMaxSizeX, m_CamWidth) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADMaxSizeY, m_CamHeight) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADSizeX, m_CamWidth) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADSizeY, m_CamHeight) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADBinX, 1) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADBinY, 1) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADMinX, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADMinY, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADImageMode, ADImageSingle) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADTriggerMode, ADTriggerInternal) == asynSuccess) && paramStatus); 
  paramStatus = ((setIntegerParam(ADAcquireTime, 1) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADSBIGDarkFieldParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setDoubleParam(ADSBIGPercentCompleteParam, 0.0) == asynSuccess) && paramStatus);

  if (!paramStatus) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	      "%s Unable To Set Driver Parameters In Constructor.\n", functionName);
    return;
  }

  //Create the thread that reads the data 
  status = (epicsThreadCreate("ADSBIGReadoutTask",
                            epicsThreadPriorityHigh,
                            epicsThreadGetStackSize(epicsThreadStackMedium),
                            (EPICSTHREADFUNC)ADSBIGReadoutTaskC,
                            this) == NULL);
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	      "%s epicsThreadCreate failure for ADSBIGReadoutTask.\n", functionName);
    return;
  }

  //Create the thread that periodically reads the temperature and readout progress
  status = (epicsThreadCreate("ADSBIGPollingTask",
                            epicsThreadPriorityMedium,
                            epicsThreadGetStackSize(epicsThreadStackMedium),
                            (EPICSTHREADFUNC)ADSBIGPollingTaskC,
                            this) == NULL);
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	      "%s epicsThreadCreate failure for ADSBIGPollingTask.\n", functionName);
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
  PAR_ERROR cam_err = CE_NO_ERROR;
  EndExposureParams eep;
  const char *functionName = "ADSBIG::writeInt32";
  
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Entry.\n", functionName);

  getIntegerParam(ADStatus, &adStatus);

  if (function == ADAcquire) {
    if ((value==1) && (adStatus == ADStatusIdle)) {
      m_Acquiring = 1;
      setIntegerParam(ADStatus, ADStatusAcquire);
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Start Event.\n", functionName);
      printf("Sending start event\n");
      epicsEventSignal(this->m_startEvent);
    }
    if ((value==0) && (adStatus != ADStatusIdle)) {
      printf("Aborting acqusition.\n");
      cam_err = p_Cam->EndExposure();
      //SBIGUnivDrvCommand(CC_END_EXPOSURE, &eep, NULL);
      if (cam_err != CE_NO_ERROR) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
  		"%s. CSBIGCam::EndExposure returned an error. %s\n", 
  	      functionName, p_Cam->GetErrorString(cam_err).c_str());
      }
    }
  } else if (function == ADSBIGDarkFieldParam) {
    if (value == 1) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
		"%s Setting Dark Field Mode.\n", functionName);
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
		"%s Setting Light Field Mode.\n", functionName);
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

  if (function == ADAcquireTime) {
    if (value > 0) {
      p_Cam->SetExposureTime(value);
    }
  }
  
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
  PAR_ERROR cam_err = CE_NO_ERROR;

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
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Start Event.\n", functionName);
      acquire = true;
      error = false;
      lock();

      //Sanity checks
      if ((p_Cam == NULL) || (p_Img == NULL)) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s NULL pointer.\n", functionName);
	break;
      }

      printf("%s Time before acqusition: ", functionName);
      epicsTime::getCurrent().show(0);

      //Read what type of image we want - light field or dark field?
      int darkField = 0;
      getIntegerParam(ADSBIGDarkFieldParam, &darkField);

      cam_err = p_Cam->GrabSetup(p_Img, SBDF_DARK_ONLY);
      if (cam_err != CE_NO_ERROR) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
  		"%s. CSBIGCam::GrabSetup returned an error. %s\n", 
  	      functionName, p_Cam->GetErrorString(cam_err).c_str());
      }

      //Do exposure
      unlock();
      if (darkField > 0) {
	printf("Dark Field...\n");
	cam_err = p_Cam->GrabMain(p_Img, SBDF_DARK_ONLY);
      } else {
	printf("Light Field...\n");
	cam_err = p_Cam->GrabMain(p_Img, SBDF_LIGHT_ONLY);
      }
      lock();
      if (cam_err != CE_NO_ERROR) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
  		"%s. CSBIGCam::GrabMain returned an error. %s\n", 
  	      functionName, p_Cam->GetErrorString(cam_err).c_str());
      }

      unsigned short *pData = p_Img->GetImagePointer();

      printf("%s Time after acqusition: ", functionName);
      epicsTime::getCurrent().show(0);

      setDoubleParam(ADSBIGPercentCompleteParam, 100.0);

      //Complete Acquire callback
      setIntegerParam(ADStatus, ADStatusIdle);
      callParamCallbacks();
      setIntegerParam(ADAcquire, 0);
      callParamCallbacks();
      unlock();

      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
		"%s Completed acqusition.\n", functionName);

    } //end of start event

  } //end of while(1)

  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	    "%s: ERROR: Exiting ADSBIGReadoutTask main loop.\n", functionName);

}

/**
 * polling thread function
 */
void ADSBIG::pollingTask(void)
{
  epicsFloat64 timeout = 1.0;
  double temperature = 0.0;
  PAR_ERROR cam_err = CE_NO_ERROR;

  const char* functionName = "ADSBIG::pollingTask";
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Started Polling Thread.\n", functionName);

  while (1) {

    epicsThreadSleep(timeout);

    //Sanity checks
    if ((p_Cam == NULL) || (p_Img == NULL)) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s NULL pointer.\n", functionName);
      break;
    }

    lock();

    GRAB_STATE camState = GS_IDLE;
    double camPercentComplete = 0.0;
    int adStatus = 0;
    getIntegerParam(ADStatus, &adStatus);
    if ((adStatus == ADStatusAcquire || adStatus == ADStatusReadout)) {
      p_Cam->GetGrabState(camState, camPercentComplete);
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
		"%s Cam State: %d. Percent Complete: %f\n", 
		functionName, camState, (camPercentComplete*100.0));
      printf("Cam State: %d, Percent Complete: %f\n", camState, (camPercentComplete*100.0));
      if ((camState == GS_DIGITIZING_DARK) || (camState == GS_DIGITIZING_LIGHT)) {
	setIntegerParam(ADStatus, ADStatusReadout);
      }
      setDoubleParam(ADSBIGPercentCompleteParam, (camPercentComplete*100.0));
    } else {
      if ((cam_err = p_Cam->GetCCDTemperature(temperature)) != CE_NO_ERROR) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		  "%s. CSBIGCam::GetCCDTemperature returned an error. %s\n", 
		  functionName, p_Cam->GetErrorString(cam_err).c_str());
    } else {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
		  "%s Actual Temperature: %f\n", functionName, temperature);
	setDoubleParam(ADTemperatureActual, temperature);
      }
    }

    callParamCallbacks();

    unlock();

  }

  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	    "%s: ERROR: Exiting ADSBIGPollingTask main loop.\n", functionName);

}


//Global C utility functions to tie in with EPICS
static void ADSBIGReadoutTaskC(void *drvPvt)
{
  ADSBIG *pPvt = (ADSBIG *)drvPvt;
  
  pPvt->readoutTask();
}
static void ADSBIGPollingTaskC(void *drvPvt)
{
  ADSBIG *pPvt = (ADSBIG *)drvPvt;
  
  pPvt->pollingTask();
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
