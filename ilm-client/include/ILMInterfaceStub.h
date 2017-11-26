/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/ILMInterfaceStub.h$
$Revision: 1$
$Date: Tuesday, March 23, 2010 3:20:38 PM$
$Author: sarahs$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  ILMInterfaceStub - FILE DESCRIPTION  -----------------*

Declaration of the exported callback endpoints for the ILM.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef ILM_INTERFACE_STUB_H
#define ILM_INTERFACE_STUB_H

/*-----------------  ILMInterfaceStub - INCLUDES  -------------------------*/

#include "SDKDataTypes.h"

/*-----------------  ILMInterfaceStub - MISCELLANEOUS  --------------------*/

/*-----------------  ILMInterfaceStub - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary {

   /**
      The initialize endpoint will invoke the Initialize overload of the handler
      object provided by the LibraryModuleBase class.
      
      <returns> The value returned by the handler object's Initialize method.
      </returns>
   */
   extern "C" DLL_EXPORT InterfaceLibrary::String initializeExt(InterfaceLibrary::uint32 mode, InterfaceLibrary::uint32 version) ;

   /**
      The shutdown endpoint will invoke the Shutdown overload of the handler
      object provided by the LibraryModuleBase class.
      
      <returns> The value returned by the handler object's Shutdown method.
      </returns>
   */
   extern "C" DLL_EXPORT InterfaceLibrary::String shutdown(void);


   /**
      Invokes the ValidatePrimitives overload of the handler object provided by
      the LibraryModuleBase class.

      <returns> The value returned by the handler object's ValidatePrimitives
      method, rendered into XML. </returns>
   */
   extern "C" DLL_EXPORT InterfaceLibrary::String validatePrimitives(void) ;

   /**
      Invokes the AddCommands overload of the handler object provided by
      the LibraryModuleBase class.

      <returns> The value returned by the handler object's AddCommands
      method, rendered into XML. </returns>
   */
   extern "C" DLL_EXPORT InterfaceLibrary::String addCommands(void) ;
   /// See JY008C615, CutThroat ICD, for the description of this function's argument and return value.

   /**
      Translates the cmdInfo argument into an ordered list of
      Primitive::Activations, then invokes the ProcessCmd overload of the
      handler object provided by the LibraryModuleBase class.
      
      <param name="cmdinfo">  </param>
      <returns> The value returned by the handler object's AddCommands method,
      rendered into XML. </returns>
   */
   extern "C" DLL_EXPORT InterfaceLibrary::String processCmd(InterfaceLibrary::binary *cmdinfo) ;

};

#endif
