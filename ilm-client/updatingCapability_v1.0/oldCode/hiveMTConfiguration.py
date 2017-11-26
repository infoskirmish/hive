#!/usr/bin/python
# Filename: hiveMTConfiguration.py

callbackIP = "10.3.2.19"
#callbackPort must be in the 1-65535 range
callbackPort = "8000"
#triggerProtocol = [dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq]
triggerProtocol = "raw-udp"
#if triggerProtocol == "raw-udp" or "raw-tcp", remotePort must be in the 1-65535 range
remotePort = 22

oldImplantName = "hived-mikrotik-mipsbe-PATCHED"
newImplantName = "hived-mikrotik-mipsbe-PATCHED"
implantDirectory = "/rw/pckg/"
installationScript = "install_MT_script"

# End of hiveMTConfiguration.py
