#ifndef	__HIVE_H
#define __HIVE_H

#include "SDKDataTypes.h"
#include "LibraryModuleBase.h"
#include "Primitive.h"
#include "CustomCommand.h"
#include "SDKDataTypes.h"
#include "Connection.h"
#include "Command.h"
#include "Ilm.h"

#define	YES	1
#define	NO	0

class HiveILM : public InterfaceLibrary::LibraryModuleBase
{
public:
	Ilm::Listener							myListener;
	Command::ShutDown						shutdownCmd2;
	Command::LaunchTrueShell				trueshell;
	InterfaceLibrary::CustomCommandGroup	cGroup;
	InterfaceLibrary::CustomCommandGroup	cGroup_shell;

	Ilm::Trigger myTrigger;

	HiveILM();
	virtual ~HiveILM();
	void AddCustomCmds( void );
	void RemoveCustomCmds( void );

private:
	void RemoveCmdShutDown( void );
	void AddCmdShutDown( void );
	void RemoveCmdShell( void );
	void AddCmdShell( void );

};

#endif
