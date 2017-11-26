
#ifndef ILM_INSTANCE
#define ILM_INSTANCE

#include "SDKDataTypes.h"
#include <stdexcept>

namespace InterfaceLibrary {

class ILMInstance
{
public:
   ILMInstance(const char * sharedLibraryName) throw (std::runtime_error);
   ~ILMInstance(void) ;
   
   String Initialize(void) ;
   String ValidatePrimitives(void) ;
   String AddCommands(void) ;
   String ProcessCmd(InterfaceLibrary::binary *aPrimitives) ;

private:

   void *mLibraryModulePointer ;

   //member variables
   String (*mInitialize)(void) ;
   String (*mValidatePrimitives)(void) ;
   String (*mAddCommands)(void) ;
   String (*mProcessCmd)(InterfaceLibrary::binary *) ;

   ILMInstance(void) ;

  // Dl_info info ;
};

};

#endif

