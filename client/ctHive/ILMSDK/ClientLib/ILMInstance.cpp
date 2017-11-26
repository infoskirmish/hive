
#include "ILMInstance.h"
#include <dlfcn.h>

using namespace InterfaceLibrary;
using std::string;

ILMInstance::~ILMInstance(void)
{

}

ILMInstance::ILMInstance(const char * sharedLibraryName) throw (std::runtime_error)
{
	mLibraryModulePointer = dlopen(sharedLibraryName, RTLD_NOW) ;

	if(mLibraryModulePointer == NULL) {
      string errMsg("Failed to load ILM shared library ");
      errMsg += dlerror();
      throw std::runtime_error(errMsg);
   }

   mInitialize = (InterfaceLibrary::String (*)(void)) dlsym(mLibraryModulePointer, "initialize") ;
   mValidatePrimitives = (InterfaceLibrary::String (*)(void)) dlsym(mLibraryModulePointer, "validatePrimitives") ;
   mAddCommands = (InterfaceLibrary::String (*)(void)) dlsym(mLibraryModulePointer, "addCommands") ;
   mProcessCmd = (InterfaceLibrary::String (*)(InterfaceLibrary::binary *)) dlsym(mLibraryModulePointer, "processCmd") ;

   //     dladdr((void *)fCreateInstance->mAddCommands, &fCreateInstance->info) ;

   if (mInitialize == NULL ||
         mValidatePrimitives == NULL ||
         mAddCommands == NULL ||
         mProcessCmd == NULL ) {
      string errMsg("Failed to resolve one or more module endpoints in ILM shared library ");
      errMsg += dlerror();
      throw std::runtime_error(errMsg);
   }
}
		
InterfaceLibrary::String ILMInstance::Initialize(void)
{
	return (*mInitialize)() ;
}

InterfaceLibrary::String ILMInstance::ValidatePrimitives(void)
{
	return (*mValidatePrimitives)() ;
}

InterfaceLibrary::String ILMInstance::AddCommands(void)
{
	return (*mAddCommands)() ;

}

InterfaceLibrary::String ILMInstance::ProcessCmd(InterfaceLibrary::binary *aPrimitives)
{
	return (*mProcessCmd)(aPrimitives) ;
}

