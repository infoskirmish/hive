#ifndef __SURVEYMAC_H
#define	__SURVEYMAC_H

#include "function_strings.h"

/*!
 *  @brief GetMacAddr
 *	
 *	Retrieves the primary ethernet MAC address on the 
 *  host machine
 *	
 *	@param buf- Character string that returns the mac address
 *	@return int - Returns 0 if the functions succeeds and -1 if the function fails
 */

int GetMacAddr( unsigned char* mac );

#endif //__SURVEYMAC_H
