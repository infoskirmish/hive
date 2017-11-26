#ifndef	__FILE_H
#define __FILE_H

#include "LibraryModuleBase.h"
#include "Primitive.h"

namespace File {

void Delete( InterfaceLibrary::Primitive::Activation& actvn, InterfaceLibrary::ProcessCmdAccumulator& acc, InterfaceLibrary::ProcessCmdResponse& resp );

void Get( InterfaceLibrary::Primitive::Activation& actvn, InterfaceLibrary::ProcessCmdAccumulator& acc, InterfaceLibrary::ProcessCmdResponse& resp );

void Put( InterfaceLibrary::Primitive::Activation& actvn, InterfaceLibrary::ProcessCmdAccumulator& acc, InterfaceLibrary::ProcessCmdResponse& resp );

}

#endif
