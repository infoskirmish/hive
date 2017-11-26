/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/LibraryModuleBase.h$
$Revision: 1$
$Date: Tuesday, March 23, 2010 3:20:39 PM$
$Author: sarahs$
Template: cpp_file.h 3.0
CPRCLASS = "SECRET"
*******************************************************************************
*/

/*-----------------  LibraryModuleBase - FILE DESCRIPTION  -----------------*

Declaration of the LibraryModuleBase class.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef	LibraryModuleBase_H			// Only process if not already processed
#define	LibraryModuleBase_H

/// The version of the SDK
#define SDK_VERSION 2

/*-----------------  LibraryModuleBase - INCLUDES  -------------------------*/

#include <dlfcn.h>
#include "ILibraryInterface.h"
#include "Primitive.h"
#include "CTInstance.h"

/*-----------------  LibraryModuleBase - MISCELLANEOUS  --------------------*/

using namespace std;

/*-----------------  LibraryModuleBase - DECLARATIONS  ---------------------*/


namespace InterfaceLibrary {
   class CustomCommandSet;
   class ProcessCmdResponse;

   /**
      The base class for the implementation of an Interface Library Module (ILM).
      As an ILibraryInterface inheritor, LibraryModuleBase provides a class wrapper
      for the interface endpoints, with virtual handler methods that are matched
      1-for-1 with those exported functions.  In addition, it provides XML parsing
      and generation services, with more intelligent overloads for the interface
      service functions that will speed the development of ILMs.

      An ILM will create a single instance of a LibraryModuleBase-derived class,
      and this object will automatically become the target for the C-declared
      exported interface endpoints. (The LibraryModuleBase constructor will set
      the handlerObject member to point to the first created instance, and the
      endpoint functions have access to this pointer to invoke the overridden
      member functions.)
   */
   class LibraryModuleBase : private ILibraryInterface
   {
   public:

      /**
         Constructor for a LibraryModuleBase.  The handlerObject member will be
         set to the first instance created.
      */
      LibraryModuleBase();

      /**
         The destructor for LibraryModuleBase will set handlerObject to NULL if
         it is currently set to the instance being destroyed.
      */
      virtual ~LibraryModuleBase();

      /**
      *  The InitializeExt override of the handlerObject is called
      *  when the 'initializeExt' endpoint is invoked by the control
      *  post client module. The default LibraryModuleBase
      *  implementation loads the callback endpoints from the calling
      *  module.
       
         An inheriting class can override this method to perform any
         implementation-specific initialization tasks associated with the
         establishment of a control post connection.  If this is done, the
         implementation should call the base class' Initialize first, and then
         proceed with its own actions, for example:  <pre>
       
         class MyILM : public LibraryModuleBase {
      *     virtual String InitializeExt() {
      *        String returnVal = LibraryModuleBase::InitializeExt();
               // ... Your implementation-specific initialization stuff here
               // If you encounter any errors, append the message(s) to returnVal
               return returnVal;
            }
            ...
         };
       
         </pre>
 
       <returns>
          A null string if initialization was successful.  Otherwise, a
          human-readable error message appropriate for the operator of the
          connecting application is returned.
       </returns>
       */
       virtual String InitializeExt(InterfaceLibrary::uint32 mode, InterfaceLibrary::uint32 version);

       virtual String Shutdown();

      /**
         This ValidatePrimitives override is called when the
         'validatePrimitives' endpoint is invoked by the control post client
         module.  Lower-level functions convert the raw binary data passed
         across the interface into a vector of Primitive Activation objects;
         this default implementation will call the handlers for each of these
         objects, using the handler function pointers located in the
         Primitive::refTable.

         An inheriting class may override this method if necessary; however, it
         is recommended that an ILM implementation perform any ILM-specific
         actions in the Activation handler functions rather than in an override
         for this method.

      <returns> An object that provides the connecting application with the
         results of the specified primitive command executions.  See
         ProcessCmdResponse.h for the description of this object. </returns>
      */
      virtual std::vector<Primitive::Validator> ValidatePrimitives(void) const;

      /**
         AddCommands is called when the 'addCommands' endpoint is invoked by the
         control post client module.  An inheriting class may override this
         method if there's some cause for special implementation-specific
         handling, such as a target whose set of custom commands varies
         radically and often as modules are loaded and unloaded.  For the most
         part, though, it is recommended that the ILM implementation manage the
         information returned through the customCommands member.
      
         <returns> The customCommands member. </returns>
      */
       virtual const CustomCommandSet& AddCommands(void) const;

      /**
         This ProcessCmd override is called when the 'processCmd' endpoint is
         invoked by the control post client module.  Lower-level functions
         convert the raw binary data passed across the interface into a vector
         of Primitive::Activation objects; this default implementation will call
         the handlers for each of these objects, using the handler function
         pointer located in the Primitive::refTable.

         An inheriting class may override this method if necessary; however, it
         is recommended that an ILM implementation perform any ILM-specific
         actions in the Activation handler functions rather than in an override
         for this method.
      
      <returns> An object that provides the connecting application with the
         results of the specified primitive command executions.  See
         ProcessCmdResponse.h for the description of this object. </returns>
      */
      virtual ProcessCmdResponse ProcessCmd (std::vector<Primitive::Activation>&);

      /**
         Read accessor that provides a pointer to the LibraryModuleBase instance
         (there should only be one).
      */
      static LibraryModuleBase* GetHandlerObject()
      {
         return handlerObject;
      }

      /// This member contains a copy of the global refTable as it existed at the
      /// time of the LibraryModuleBase object's creation.  If this class is extended
      /// for the convenience of multi-mode activity, this member can be used to
      /// maintain the behavior of the mode represented by this particular
      /// LibraryModuleBase instance.
      Primitive::ReferenceTable primitiveRefTable;

      /**
         Provides a reference to the dynamic linkage information for this
         module.
       */
      static const Dl_info& GetSharedObjectInfo()
      {
         return sharedObjectInfo;
      }

      /**
      *  Provides the mode of CutThroat connecting to the ILM.
       */
      uint32 GetCTMode()
      {
         return ctMode;
      }

      /**
      *  Provides the version of CutThroat connecting to the ILM.
       */
      uint32 GetCTVersion()
      {
         return ctVersion;
      }

      /**
      *  Sets the version of the ILM.
       */
      void SetILMVersion(uint32 version)
      {
         ilmVersion = version;
      }

      /**
      *  Provides the version of the ILM.
       */
      uint32 GetILMVersion()
      {
         return ilmVersion;
      }

      /**
         Invokes the primary module's RefreshCT callback function.
       */
      void RefreshCT()
      {
         callback->RefreshCT();
      }

      /**
      *  Invokes the primary module's AC callback function.
       */
      void AC(ProcessCmdResponse msg, uint32 lostConnection)
      {
          XMLComposer fComposedMessage ;

          msg.XMLCompose(fComposedMessage) ;

          callback->AC(fComposedMessage.GetBytes(), lostConnection);
      }

      /**
         The accessor for the customCommands member is a synonym for the
         AddCommands method, except that it lacks the const qualifiers.
       */
      CustomCommandSet& CustomCommands()
      {
         return customCommands;
      }

   protected:

      /// This member can be used by a LibraryModuleBase derivation to store
      /// the custom commands supported by the ILM implementation.  It's the
      /// container returned by the default AddCommands implementation.
      CustomCommandSet customCommands;

      /** This instance pointer is the object whose handlers will receive the
         calls from the C-declared library endpoints.  It's set to
         the first LibraryModuleBase object constructed.  */
      static LibraryModuleBase* handlerObject;

   private:

      /// Dynamic linkage information for the module, returned by GetSharedObjectInfo.
      static Dl_info  sharedObjectInfo;

      /// Contains the CT callback functions
      static CTInstance*   callback;

      /// The mode of CutThroat connecting to the ILM
      uint32 ctMode;

      /// The version of CutThroat connecting to the ILM
      uint32 ctVersion;

      /// The version of the ILM
      uint32 ilmVersion;

   };


};
#endif	// LibraryModuleBase_H
//-----------------------------------------------------------------------------
//
//	SECRET		SECRET		SECRET		SECRET
//
//-----------------------------------------------------------------------------

