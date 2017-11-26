/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/source/LibraryModuleBase.cpp$
$Revision: 1$
$Date: Wednesday, March 17, 2010 4:26:17 PM$
$Author: sarahs$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  LibraryModuleBase - FILE DESCRIPTION  -----------------*

Implementation of the LibraryModuleBase class and the endpoint handler redirection
functions.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  LibraryModuleBase - INCLUDES  -------------------------*/

#include <vector>
#include <sstream>
#include "LibraryModuleBase.h"
#include "CustomCommand.h"

/*-----------------  LibraryModuleBase - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;
using namespace InterfaceLibrary::Primitive;

static String noHandlerDefinedErrMsg("Error: The ILM target has no handler object defined");

extern "C" DLL_EXPORT InterfaceLibrary::String initializeExt(InterfaceLibrary::uint32 mode, InterfaceLibrary::uint32 version) ;

/*==========================================================================*/


LibraryModuleBase* LibraryModuleBase::handlerObject = NULL;
CTInstance* LibraryModuleBase::callback = NULL;
Dl_info LibraryModuleBase::sharedObjectInfo;

LibraryModuleBase::LibraryModuleBase()
{
   if(handlerObject==NULL) {
      // The first instance created has some housekeeping to do

      handlerObject = this;
    
      dladdr((void *)( initializeExt), &sharedObjectInfo) ;

      callback = new CTInstance();
   }
}

LibraryModuleBase::~LibraryModuleBase()
{
   if (handlerObject==this) {
      delete callback;
      handlerObject=NULL;
   }
}

String LibraryModuleBase::InitializeExt(uint32 mode, uint32 version)
{
    ctMode = mode;
    ctVersion = version;

   return "";
}

String LibraryModuleBase::Shutdown(void)
{
	return "";
}

vector<Validator> LibraryModuleBase::ValidatePrimitives(void) const
{
    return primitiveRefTable.GetSupportedPrimitives();
}

const CustomCommandSet& LibraryModuleBase::AddCommands(void) const
{
    return customCommands;
}

ProcessCmdResponse LibraryModuleBase::ProcessCmd (vector<Activation>& activations)
{
   ProcessCmdResponse retVal;
   ProcessCmdAccumulator accum;

   for (size_t i=0; i<activations.size() && retVal.type.empty(); i++) {
      RefTableEntry* primRef = primitiveRefTable[activations[i].primitiveID];
      if (primRef->handler==NULL) {
         std::stringstream errmsg;
         errmsg << "The ILM has not defined a handler function for command primitive "
               << primRef->primitiveID << "; this primitive was skipped.";
         retVal.resultsLines.push_back(ProcessCmdResponse::Line(0, errmsg.str().c_str()));
         retVal.type = ProcessCmdResponse::TYPE_Local_Failure;
      } else {
         // Once again in this God-forsaken interface definition, we must deal with
         // void pointers (hidden in the activations' arguments).  Catching everything
         // so as to deal with problems as gracefully as possible.
         // TODO: Will probably want to catch more types; specifically, we'll probably
         // want to make exception types for Local and Remote failures, and give the
         // handlers the option of throwing these for convenience.
         try {
            primRef->handler(activations[i], accum, retVal);
         } catch (...) {
            std::stringstream errmsg;
            errmsg << "Unknown error encountered when calling handler for primitive "
                  << primRef->primitiveID;
            retVal.resultsLines.push_back(ProcessCmdResponse::Line(0, errmsg.str().c_str()));
            retVal.type = ProcessCmdResponse::TYPE_Local_Failure;
         }
      }
   }
   // If nobody set the type element, go ahead and assume "Success"
   if (retVal.type.empty()) {
      retVal.type = ProcessCmdResponse::TYPE_Success;
   }
   return retVal;
}


