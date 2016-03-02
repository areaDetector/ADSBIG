#!../../bin/linux-x86_64/example

## You may have to change example to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

epicsEnvSet("SBIG_PV", "BL99:Det:SBIG")

## Register all support components
dbLoadDatabase "dbd/example.dbd"
example_registerRecordDeviceDriver pdbbase

#################################################
# Set up the SBIG driver

ADSBIGConfig("S1",-1,-1)

#################################################
# Set up the areaDetector plugins

NDROIConfigure("S1.ROI1", 10, 0, "S1", 0, -1, -1, 0, 0)

NDStdArraysConfigure("S1.ARR1", 10, 0, "S1.ROI1", 0, -1, 0, 0)

NDStatsConfigure("S1.STATS1", 10, 0, "S1", 0, -1, -1, 0, 0)

NDFileTIFFConfigure("S1.TIFF1", 10, 0, "S1", 0, 0, 0, 0)

#################################################
# autosave

epicsEnvSet IOCNAME example
epicsEnvSet SAVE_DIR /tmp/example

save_restoreSet_Debug(0)

### status-PV prefix, so save_restore can find its status PV's.
save_restoreSet_status_prefix("BL99:CS:SBIG:")

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
create_monitor_set("$(IOCNAME).req", 10)

# Enable plugins at startup (settings that are not autosaved)
dbpf $(SBIG_PV):ArrayCallbacks 1
dbpf $(SBIG_PV):TIFF1:EnableCallbacks 1
dbpf $(SBIG_PV):Stats1:EnableCallbacks 1
dbpf $(SBIG_PV):ROI1:EnableCallbacks 1
dbpf $(SBIG_PV):Array1:EnableCallbacks 1

# Set up ROI binning by default for the array plugin
# We also enable scaling (divide by 4 because we are doing 2x2 binning).
# We need to force the output data type to be UInt16, otherwise the plugin 
# will convert to floating point.
dbpf $(SBIG_PV):ROI1:BinX 2
dbpf $(SBIG_PV):ROI1:BinY 2
dbpf $(SBIG_PV):ROI1:DataTypeOut 3
dbpf $(SBIG_PV):ROI1:EnableScale 1
dbpf $(SBIG_PV):ROI1:Scale 4

# Set up TIFF plugin auto increment
dbpf $(SBIG_PV):TIFF1:AutoIncrement 1
dbpf $(SBIG_PV):TIFF1:AutoSave 1

# Set the NDAttributesFile paths
dbpf $(SBIG_PV):NDAttributesFile "/home/controls/epics/ADSBIG/master/example/sbig_cam.xml"
dbpf $(SBIG_PV):TIFF1:NDAttributesFile "/home/controls/epics/ADSBIG/master/example/sbig_tiff.xml"

# After IOC startup, set the frame sizes. This needs to be done after
# we have set the binning (via autosave).
dbpf $(SBIG_PV):MinX.PROC 1
dbpf $(SBIG_PV):MinY.PROC 1
dbpf $(SBIG_PV):SizeX.PROC 1
dbpf $(SBIG_PV):SizeY.PROC 1

