/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/CTInstance.h$
$Revision: 1$
$Date: Friday, October 09, 2009 5:04:08 PM$
$Author: timm$
Template: cpp_file.h 3.0
CPRCLASS = "SECRET"
*******************************************************************************
*/

/*-----------------  CTInstance - FILE DESCRIPTION  -----------------*

Declaration of the CTInstance class.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef	CTInstance_H
#define	CTInstance_H

/*-----------------  CTInstance - INCLUDES  -------------------------*/

#include "SDKDataTypes.h"

/*-----------------  CTInstance - MISCELLANEOUS  --------------------*/

/*-----------------  CTInstance - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary
{

/**
   The CTInstance class contains the information necessary to invoke the
   callback endpoints in the executable module that loaded the ILM.  There is no
   need for more than one instance of this class to ever exist, as all instances
   within an object module will contain exactly the same information.
 */
class CTInstance
{
public:

   /// Initializes CutThroat exported endpoint function pointers
   CTInstance() throw (std::runtime_error);

   ~CTInstance() ;

   /// Calls the primary module's exported RefreshCT endpoint function.
   void RefreshCT(void) ;

   /// Calls the primary module's exported Asynchronous Communications endpoint function.
   void AC(InterfaceLibrary::String aMessage, InterfaceLibrary::uint32 aLostConnection) ;

private:

   /// Contains the pointer to the module which loaded the ILM shared module.
   void *primaryModulePointer ;

   /// Pointer to the RefreshCT endpoint function.
   void (*mRefresh)(void) ;

   /// Pointer to the AC endpoint function.
   void (*mAC)(InterfaceLibrary::String, InterfaceLibrary::uint32) ;
}; 

};

#endif

//-----------------------------------------------------------------------------
//
//	SECRET		SECRET		SECRET		SECRET
//
//-----------------------------------------------------------------------------

