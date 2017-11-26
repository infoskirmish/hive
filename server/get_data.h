#ifndef __GET_DATA_H
#define __GET_DATA_H

#define GD_PROC_LIST 1
#define GD_IPCONFIG 2
#define GD_NETSTAT_AN 3
#define GD_NETSTAT_RN 4


unsigned char* get_data(int* size, int flag);

#endif

