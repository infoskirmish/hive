#ifndef	__COMMAND_H
#define __COMMAND_H

#include "LibraryModuleBase.h"
#include "Primitive.h"
#include "CustomCommand.h"
#include "ProcessCmdResponse.h"

namespace Command {

void Execute( InterfaceLibrary::Primitive::Activation& actvn, InterfaceLibrary::ProcessCmdAccumulator& acc, InterfaceLibrary::ProcessCmdResponse& resp );

void Session( InterfaceLibrary::Primitive::Activation& actvn, InterfaceLibrary::ProcessCmdAccumulator& acc, InterfaceLibrary::ProcessCmdResponse& resp );

void Exit( InterfaceLibrary::Primitive::Activation& actvn, InterfaceLibrary::ProcessCmdAccumulator& acc, InterfaceLibrary::ProcessCmdResponse& resp );


// Custom Commands are declared as classes
	class ShutDown : public InterfaceLibrary::CustomCommand
	{
	public:
		ShutDown();
		virtual InterfaceLibrary::ProcessCmdResponse Process( InterfaceLibrary::binary &arguments );
	};

	class LaunchTrueShell : public InterfaceLibrary::CustomCommand
	{
	public:
		LaunchTrueShell();
		virtual InterfaceLibrary::ProcessCmdResponse Process( InterfaceLibrary::binary &arguments );
	};

}

#endif
