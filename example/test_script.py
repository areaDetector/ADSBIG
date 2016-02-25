#!/usr/bin/python 

import sys
import os
import time

from epics import caget, caput

base_pv = "BL99:Det:SBIG:"

def main():
    
    print "Running SBIG tests..."

    print "Current detector state: " + str(caget(base_pv+"StatusMessage_RBV", as_string=True))

    caput(base_pv+"AcquireTime", 1, wait=True)

    for i in range(3):
        print "ReadoutMode: " + str(i)
        caput(base_pv+"ReadoutMode", i, wait=True)

        for image in range(10):
            print "Acquire: " + str(i)
            caput(base_pv+"Acquire", 1, wait=True, timeout=20)
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
