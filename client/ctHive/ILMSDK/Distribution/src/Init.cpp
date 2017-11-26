/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/Distribution/src/Init.cpp$
$Revision: 1$
$Date: Tuesday, August 25, 2009 2:44:13 PM$
$Author: timm$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
**
**	               (C)  Copyright Northrop Grumman ES/
**                          XETRON Corporation
**                         All rights reserved
*/

/*-----------------  Init - FILE DESCRIPTION  -----------------*

ILM primary object construction and destruction functions.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  Init - INCLUDES  -------------------------*/

#include "LibraryModuleBase.h"

/*-----------------  Init - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;

/// These definitions simply make the extracted function documentation in this module more readable.
#define CONSTRUCT_DECL void __attribute__ ((constructor))
#define DESTRUCT_DECL void __attribute__ ((destructor))

/*==========================================================================*/

/// ILM shared object constructor used to initialize the ILM.  It's called
/// automatically as the shared object is linked by the first executable object.
/// The ILM developer should place any necessary global initialization actions
/// in this function.
CONSTRUCT_DECL ILMConstructor()
{
   // Note to ILM Implementer:
   // Place global Primitive::refTable initialization here

   LibraryModuleBase* myILMInstance = new LibraryModuleBase();

   // Note to ILM Implementer:
   // Populate myILMInstance->customCommands here if necessary

}


/// ILM shared object destructor gets rid of stuff created in ILMConstructor.
/// It's called automatically as the shared object is unlinked by the last
/// executable object.
DESTRUCT_DECL ILMDestructor()
{
   delete LibraryModuleBase::GetHandlerObject();
}


