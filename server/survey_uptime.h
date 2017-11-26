#ifndef __SURVEYUPTIME_H
#define	__SURVEYUPTIME_H

#include "function_strings.h"

/*!
 *  @brief GetSystemUpTime
 *	
 *	Retrieves the time since the system's last reboot
 *	
 *	@param buf- Character string that returns the system's uptime
 *	@return int - Returns 0 if the functions succeeds and -1 if the function fails
 */

unsigned long GetSystemUpTime( void );

#endif //__SURVEYUPTIME_H
