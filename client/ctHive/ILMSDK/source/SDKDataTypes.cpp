/*-****************************************************************************
$Archive: /ControlPost/ILM_SDK/source/SDKDataTypes.cpp $
$Revision: 1 $
$Date: 3/24/08 3:01p $
$Author: joshk $
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  SDKDataTypes - FILE DESCRIPTION  -----------------*

Implementation of the SDKDataTypes components.

*/
/* $NoKeywords: $ (No rcs replacement keywords below this point) */

/*-----------------  SDKDataTypes - INCLUDES  -------------------------*/

#include "SDKDataTypes.h"

/*-----------------  SDKDataTypes - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;

bool STLCompare::operator()(const char *aInput1, const char *aInput2)
{
   return(strcasecmp(aInput1, aInput2) < 0) ;
}

void String::NewBuf (uint32 i, const char* init)
{
   length = i;
   if (i!=0) {
      buffer = strncpy(new char[i+1], init, (size_t)i);
      buffer[i]=0; // Force NULL termination, automatically truncating init if necessary
   } else {
      buffer = NULL;
   }
}

         
