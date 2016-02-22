#!../../bin/linux-x86_64/example

## You may have to change example to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/example.dbd"
example_registerRecordDeviceDriver pdbbase

#################################################
# Set up the SBIG driver

ADSBIGConfig("S1",-1,-1)

#################################################
# Set up the areaDetector plugins

NDStdArraysConfigure("S1.ARR1", 10, 0, "S1", 0, -1, 0, 0)

NDStatsConfigure("S1.STATS1", 10, 0, "S1", 0, -1, -1, 0, 0)

NDFileTIFFConfigure("S1.TIFF1", 10, 0, "S1", 0, 0, 0, 0)

#################################################
# autosave

epicsEnvSet IOCNAME example
epicsEnvSet SAVE_DIR /tmp/example

save_restoreSet_Debug(0)

### status-PV prefix, so save_restore can find its status PV's.
save_restoreSet_status_prefix("BL99:CS:ADSBIG")

set_requestfile_path("$(SAVE_DIR)")
set_savefile_path("$(SAVE_DIR)")

save_restoreSet_NumSeqFiles(3)
save_restoreSet_SeqPeriodInSeconds(600)
set_pass0_restoreFile("$(IOCNAME).sav")
set_pass1_restoreFile("$(IOCNAME).sav")

#################################################

## Load record instances
dbLoadRecords "db/example.db"

cd ${TOP}/iocBoot/${IOC}
iocInit

# Create request file and start periodic 'save'
epicsThreadSleep(5)
makeAutosaveFileFromDbInfo("$(SAVE_DIR)/$(IOCNAME).req", "autosaveFields")
create_monitor_set("$(IOCNAME).req", 10)


