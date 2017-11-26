/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/ILibraryInterface.h$
$Revision: 1$
$Date: Friday, October 09, 2009 5:04:08 PM$
$Author: timm$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  ILibraryInterface - FILE DESCRIPTION  -----------------*

Declaration of the ILibraryInterface interface.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef	ILibraryInterface_H			// Only process if not already processed
#define	ILibraryInterface_H

/*-----------------  ILibraryInterface - INCLUDES  -------------------------*/

#include "Primitive.h"
#include "ProcessCmdResponse.h"
#include "CustomCommand.h"

/*-----------------  ILibraryInterface - MISCELLANEOUS  --------------------*/

/*-----------------  ILibraryInterface - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary 
{

   /**
      Defines the common interface for Interface Library Module (ILM) instance
      class wrappers. The surface of this interface parallels the "C"-style
      exported endpoint functions described in JY008C615, CutThroat ICD;
      however, the interface methods are defined at a higher abstraction level.
      This provides a basis for an implementation that separates the low-level
      formatting for CT/ILM data transfer from more intelligent (and easier to
      code) functionality at a higher level of ILM functionality.
      
      <see cref="LibraryModuleBase"/> provides an
      implementation of ILibraryInterface.
   */
   class ILibraryInterface
   {
   public:

      /// The virtual destructor is provided to support fully polymorphic behavior.
      virtual ~ILibraryInterface() {}
      
      /**
         Initialize is called in response to the 'initialize' endpoint's
         invocation to perform any tasks associated with the command post module
         establishing a connection with the ILM module.  An inheriting class
         must override this method to instantiate a library module object.
         
         <returns>
         A null string if initialization was successful.  Otherwise, a
         human-readable error message appropriate for the operator of the control
         post application is returned.  </returns>
      */
      virtual String InitializeExt(InterfaceLibrary::uint32 mode, InterfaceLibrary::uint32 version) = 0 ;
      
      
      /**
         ValidatePrimitives is called in response to the 'validatePrimitives'
         endpoint's invocation.  An inheriting class must override this method
         in order to instantiate a library module object.
         
         <returns>
         Provides the connecting module with the list of Primitive
         Validators that describe the command primitives that the ILM supports.
         Note that the set of supported validators may not be constant for a
         given ILM; it may vary based upon the ILM's operating conditions.  </returns>
      */
      virtual vector<Primitive::Validator> ValidatePrimitives(void) const = 0 ;
      
      
      /**
         AddCommands is called in response to the 'addCommands'
         endpoint's invocation.  An inheriting class must override this method
         in order to instantiate a library module object.
         
         <returns>
         An object that contains the list of CustomCommands implemented by the
         ILM.  See AddCommandsResponse.h for the description of this object and
         its contents.  </returns>
      */
      virtual const CustomCommandSet& AddCommands(void) const = 0 ;
      
      /**
         ProcessCmd is called in response to the 'processCmd'
         endpoint's invocation.  An inheriting class must override this method
         in order to instantiate a library module object.
      
         <returns>
         An object that contains the results of the specified primitive command
         executions.  See ProcessCmdResponse.h for the description of this
         object.  </returns>
      */
      virtual ProcessCmdResponse ProcessCmd (vector<Primitive::Activation>&) = 0 ;

   } ;

} ; // namespace InterfaceLibrary
#endif	// ILibraryInterface_H
