#!/usr/bin/python

import pexpect, struct, fcntl, termios, signal, sys
from datetime import datetime

def sigwinch_passthrough (sig, data):
	s = struct.pack("HHHH", 0, 0, 0)
	a = struct.unpack('HHHH', fcntl.ioctl(sys.stdout.fileno(), termios.TIOCGWINSZ, s))
	global p
	p.setwinsize(a[0], a[1])

#p = pexpect.spawn('/usr/bin/gnome-terminal -e /bin/bash')  #Note this is global and used in sigwinch_passthrough.

now=datetime.now()
logFileName="gTerminal_"+now.strftime('%Y%m%d_%H%M%S')+".log"
commandLine="/usr/bin/gnome-terminal -e "+"\""+"/usr/bin/script -c /bin/bash "+logFileName+"\""
p = pexpect.spawn(commandLine)  #This actually does start up a gnome-terminal with bash and creates the ./testLog.txt...

#p = pexpect.spawn('/usr/bin/gnome-terminal -e "/usr/bin/script -c /bin/bash testLog.txt" ')  #This actually does start up a gnome-terminal with bash and creates the ./testLog.txt...

signal.signal(signal.SIGWINCH, sigwinch_passthrough)
#p.interact()
p.expect(pexpect.EOF, timeout=600)


