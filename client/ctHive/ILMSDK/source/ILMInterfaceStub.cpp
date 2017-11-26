/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/source/ILMInterfaceStub.cpp$
$Revision: 1$
$Date: Tuesday, March 23, 2010 3:20:48 PM$
$Author: sarahs$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  ILMInterfaceStub - FILE DESCRIPTION  -----------------*

Implementation of the classes that model the response from the ILM interface's
AddCommands function.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  ILMInterfaceStub - INCLUDES ----------------------*/

#include "ILMInterfaceStub.h"
#include "LibraryModuleBase.h"
#include <sstream>
#include <stdexcept>

/*-----------------  ILMInterfaceStub - DECLARATIONS -------------------*/

using namespace InterfaceLibrary;
using namespace InterfaceLibrary::Primitive;
using std::stringstream;

/*==========================================================================*/


InterfaceLibrary::String noHandlerDefinedErrMsg("Error: The ILM target has no handler object defined");
InterfaceLibrary::String ilm_version_tag("ilm");
InterfaceLibrary::String sdk_version_tag("sdk");

extern "C" DLL_EXPORT InterfaceLibrary::String initializeExt(InterfaceLibrary::uint32 mode, InterfaceLibrary::uint32 version)
{
   stringstream tagOut;

   LibraryModuleBase* handler = LibraryModuleBase::GetHandlerObject();

   if (handler!=NULL) {
      handler->InitializeExt(mode, version);
      XMLComposer comp;

#ifdef _DEBUG
      comp.beautify = true;
#endif
      comp.StartTag("versions");
      tagOut << handler->GetILMVersion();
      comp.WriteSimpleElement( ilm_version_tag, tagOut.str() );
      tagOut.str("");
      tagOut << SDK_VERSION;
      comp.WriteSimpleElement( sdk_version_tag, tagOut.str() );
      comp.EndTag();
      return comp.GetBytes();
   } else {
      return noHandlerDefinedErrMsg;
   }
}

extern "C" DLL_EXPORT InterfaceLibrary::String validatePrimitives(void)
{
   LibraryModuleBase* handler = LibraryModuleBase::GetHandlerObject();

   if (handler!=NULL) {
      vector<Validator> v = handler->ValidatePrimitives();
      XMLComposer comp;

#ifdef _DEBUG
      comp.beautify = true;
#endif
      comp.StartTag("supportedprimitives");
      for (vector<Validator>::const_iterator i=v.begin(); i!=v.end(); i++) i->XMLCompose(comp);
      comp.EndTag();
      return comp.GetBytes();
   } else {
      return noHandlerDefinedErrMsg;
   }
}

extern "C" DLL_EXPORT InterfaceLibrary::String addCommands(void)
{
   LibraryModuleBase* handler = LibraryModuleBase::GetHandlerObject();

   if (handler!=NULL) {
      XMLComposer comp;

   #ifdef _DEBUG
      comp.beautify = true;
   #endif
      return String(handler->AddCommands().XMLCompose(comp).GetBytes());
   } else {
      return noHandlerDefinedErrMsg;
   }
}

extern "C" DLL_EXPORT InterfaceLibrary::String shutdown(void)
{
	LibraryModuleBase* handler = LibraryModuleBase::GetHandlerObject();

	if (handler!=NULL)
	   handler->Shutdown();
	
	return "";
}

extern "C" DLL_EXPORT InterfaceLibrary::String processCmd(InterfaceLibrary::binary *b)
{
   LibraryModuleBase* handler = LibraryModuleBase::GetHandlerObject();

   if (handler!=NULL) {
      if (b!=NULL) {
         ProcessCmdResponse resp;

         // Since we're dealing with void pointers, and we don't know where they've been,
         // we'll use a try-catch-all construct to make sure that any memory tomfoolery
         // is handled in a defined manner.
         try {
            uint32* vptr = (uint32*)b->data;
            uint32 number_of_primitives = *vptr++;
   
            // Figure out whether this is a custom command
            if ( number_of_primitives == 0 ) {
               // Yes, it's a custom command.
               binary arguments;
               uint32 customRefID = *vptr++;    // First quad-byte after the # of primitives is the custom cmd ID
   
               // Look for the CustomCommand-derived object with that ID; return error message if not found
               CustomCommandSet cmdSet = LibraryModuleBase::GetHandlerObject()->CustomCommands();
               try {
                  CustomCommand& custCmd = cmdSet.find(customRefID);
                  arguments.data = (String*)vptr;   // The arguments start next after the cmd ID
                  arguments.length = b->length - 2*sizeof(uint32);
                  // Process the Custom Command
                  resp = custCmd.Process (arguments);
               } catch (std::logic_error e) {
                  stringstream errmsg;
                  errmsg << "<return><type>Local Failure</type><results><line><text>Unsupported custom command: "
                        << e.what() << "</text></line></results></return>";
                  return errmsg.str();
               }
   
            } else {
               // Not a custom command; just a standard list of primitives.
               vector<Activation> v;
               // Build the list of Primitive activations
               do {
                  uint32 aID = (uint32)(*vptr++);
                  RefTableEntry* tableEntry = handler->primitiveRefTable[aID];
                  // At this level, we check to ensure that each primitive ID is valid.  Other checks,
                  // such as "is the handler function pointer valid?" are done by ProcessCmd.
                  if (tableEntry==NULL) {
                     stringstream errmsg;
                     errmsg << "<return><type>Local Failure</type><results><line><text>Unrecognized command primitive "
                           << aID << " specified in ProcessCmd invocation.</text></line></results></return>";
                     return errmsg.str();
                  }
                  // Adding the Primitive Activation to the list
                  v.push_back(Activation(aID, vptr));
                  vptr = (uint32*)((char*)vptr + tableEntry->packedParamSize);
               } while (--number_of_primitives != 0);
               // Now call the "smart" ProcessCmd override with that list of activations
               resp = handler->ProcessCmd(v);
            }
         } catch (std::exception e) {
            stringstream errmsg;
            errmsg << "<return><type>Local Failure</type><results><line><text>Exception encountered in ILM's processCmd: "
                  << e.what() << "</text></line></results></return>";
            return errmsg.str();
         } catch (...) {
            return "<return><type>Local Failure</type><results><line><text>Unknown exception encountered in ILM's processCmd.</text></line></results></return>";
         }

         XMLComposer comp;
#ifdef _DEBUG
         comp.beautify = true;
#endif
         
         return resp.XMLCompose(comp).GetBytes();
      } else {
         return "<return><type>Local Failure</type><results><line><text>No arguments provided in ProcessCmd invocation.</text></line></results></return>";
      }
   } else {
      return "<return><type>Local Failure</type><results><line><text>The ILM has no command handler object defined.</text></line></results></return>";
   }
}


