#!/usr/bin/python
# -*- coding: utf-8 -*-

# runcached
# Execute commands while caching their output for subsequent calls. 
# Command output will be cached for $cacheperiod and replayed for subsequent calls
#
# Author Spiros Ioannou sivann <at> inaccess.com
#

cacheperiod=27 #seconds
cachedir="/tmp"
maxwaitprev=5	#seconds to wait for previous same command to finish before quiting
minrand=0		#random seconds to wait before running cmd
maxrand=0

import os
import sys
import errno
import subprocess
import time
import hashlib
import random
import atexit
import syslog


argskip=1

if (len(sys.argv) < 2) or (len(sys.argv) == 2 and sys.argv[1] == '-c'):
	sys.exit('Usage: %s [-c cacheperiod] <command to execute with args>' % sys.argv[0])

if sys.argv[1] == '-c':
	cacheperiod=int(sys.argv[2])
	argskip=3

cmd=" ".join(sys.argv[argskip:])

#hash of executed command w/args
m = hashlib.md5()
m.update(cmd)
cmdmd5=m.hexdigest()


#random sleep to avoid racing condition of creating the pid on the same time
if maxrand-minrand > 0:
	time.sleep(random.randrange(minrand,maxrand))

pidfile=cachedir+os.sep+cmdmd5+'-runcached.pid'
cmddatafile=cachedir+os.sep+cmdmd5+'.data'
cmdexitcode=cachedir+os.sep+cmdmd5+'.exitcode'
cmdfile=cachedir+os.sep+cmdmd5+'.cmd'



def cleanup():
	if os.path.isfile(pidfile):
		os.remove(pidfile)


def myexcepthook(exctype, value, traceback):
	#if exctype == KeyboardInterrupt:
	#	 print "Handler code goes here"
	cleanup()
	syslog.syslog(syslog.LOG_ERR, value.__str__())
	_old_excepthook(exctype, value, traceback)

def runit(cmd,cmddatafile,cmdexitcode,cmdfile):
	f_stdout=open(cmddatafile, 'w')
	#f_stderr=open('stderr.txt', 'w')
	f_stderr=f_stdout
	p = subprocess.Popen(cmd, stdout=f_stdout, stderr=f_stderr,shell=True)
	p.communicate()
	f_stdout.close()
	exitcode=p.returncode
	with open(cmdfile, 'w') as f:
		f.write(cmd)
	with open(cmdexitcode, 'w') as f:
		f.write(str(exitcode))

def file_get_contents(filename):
	with open(filename) as f:
		return f.read()


#install cleanup hook
_old_excepthook = sys.excepthook
sys.excepthook = myexcepthook
atexit.register(cleanup)

#don't run the same command in parallel, wait the previous one to finish
count=maxwaitprev
while os.path.isfile(pidfile):
#remove stale pid if left there without a running process
	prevpid=file_get_contents(pidfile).strip()
	if not os.path.exists("/proc/"+prevpid):
		os.remove(pidfile)
	time.sleep(1)
	count-=1
	if count == 0:
		sys.stderr.write("timeout waiting for '%s' to finish. (pid: %s)\n" % (cmd,pidfile))
		sys.exit (1)

#write pidfile
mypid=os.getpid()
with open(pidfile, 'w') as f:
	f.write(str(mypid))

#if not cached before, run it
if not os.path.isfile(cmddatafile):
	runit(cmd,cmddatafile,cmdexitcode,cmdfile)

#if too old, re-run it
currtime=int(time.time())
lastrun=int(os.path.getmtime(cmddatafile))
diffsec=currtime - lastrun
if (diffsec > cacheperiod):
	runit(cmd,cmddatafile,cmdexitcode,cmdfile)

with open(cmddatafile, 'r') as f:
	sys.stdout.write(f.read())

cleanup()
