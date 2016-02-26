#!/usr/bin/python 

import sys
import os
import time

from epics import caget, caput

base_pv = "BL99:Det:SBIG:"
exposures = [0.1, 0.5, 1, 2, 10]
readout_modes = [0, 1, 2]
images = 20

def main():
    
    print "Running SBIG tests..."

    print "Current detector state: " + str(caget(base_pv+"StatusMessage_RBV", as_string=True))

    for readout in readout_modes: 
        print "ReadoutMode: " + str(readout)
        caput(base_pv+"ReadoutMode", readout, wait=True)

        for exposure in exposures:
            print "AcquireTime: " + str(exposure)
            caput(base_pv+"AcquireTime", exposure, wait=True)

            #Take a dark field
            print "Taking a dark field..."
            caput(base_pv+"DarkField", 1, wait=True)
            caput(base_pv+"Acquire", 1, wait=True, timeout=30)
            status = caget(base_pv+"DetectorState_RBV")
            if (status != 0):
                print "ERROR!"
                print base_pv+"DetectorState_RBV="+str(status)
                print str(caget(base_pv+"StatusMessage_RBV", as_string=True))
                sys.exit(1)
            print "Done."
            caput(base_pv+"DarkField", 0, wait=True)

            #Take light fields
            print "Taking " + str(images) + " images..."
            for image in range(images):
                print "Acquire: " + str(image)
                caput(base_pv+"Acquire", 1, wait=True, timeout=30)
                status = caget(base_pv+"DetectorState_RBV")
                if (status != 0):
                    print "ERROR!"
                    print base_pv+"DetectorState_RBV="+str(status)
                    print str(caget(base_pv+"StatusMessage_RBV", as_string=True))
                    sys.exit(1)

    print "Complete."
    sys.exit(0)

if __name__ == "__main__":
    main()
