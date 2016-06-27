#!/usr/bin/env python

import pexpect
import sys
import time

filename = sys.argv[1]
child = pexpect.spawn(filename)
child.logfile = sys.stdout
child.expect('\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}: Hello World', timeout=60)
child.sendcontrol('c')
child.wait()
