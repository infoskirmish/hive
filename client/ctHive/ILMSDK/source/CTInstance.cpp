/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/source/CTInstance.cpp$
$Revision: 1$
$Date: Friday, October 09, 2009 5:04:22 PM$
$Author: timm$
Template: cpp_file.cpp 3.0
CPRCLASS = "SECRET"
*******************************************************************************
*/

/*-----------------  CTInstance - FILE DESCRIPTION  -----------------*

Implementation of the CTInstance class.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  CTInstance - INCLUDES  -------------------------*/

#include <stdexcept>
#include <dlfcn.h>
#include "CTInstance.h"

/*-----------------  CTInstance - DECLARATIONS ----------------------*/

using namespace std;
using namespace InterfaceLibrary;

/*==========================================================================*/


CTInstance::CTInstance() throw (runtime_error)
{
   primaryModulePointer = dlopen(NULL, RTLD_NOW) ;
   if(primaryModulePointer == NULL) throw runtime_error(string("Failed to load primary module in CTInstance: ")+dlerror());
   mRefresh = (void(*)())dlsym(primaryModulePointer, "refreshCT") ;
   if(mRefresh == NULL) throw runtime_error(string("Could not find endpoint in CTInstance: ")+dlerror());
   mAC = (void(*)(String, uint32))dlsym(primaryModulePointer, "AC") ;
   if(mAC == NULL) throw runtime_error(string("Could not find endpoint in CTInstance: ")+dlerror());
}

CTInstance::~CTInstance()
{
    dlclose(primaryModulePointer) ;
}


void CTInstance::RefreshCT(void)
{
    void (*fRefresh)(void) = (void (*) (void))mRefresh ;
    (*fRefresh)() ;
}

void CTInstance::AC(InterfaceLibrary::String aMessage, InterfaceLibrary::uint32 aLostConnection)
{
    void (*fAC)(InterfaceLibrary::String, InterfaceLibrary::uint32) = (void (*)(InterfaceLibrary::String, InterfaceLibrary::uint32)) mAC ;
    (*fAC)(aMessage, aLostConnection) ;
}

//-----------------------------------------------------------------------------
//
//	SECRET		SECRET		SECRET		SECRET
//
//-----------------------------------------------------------------------------

