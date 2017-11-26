#include <stdlib.h>

#include "Utilities.h"
#include "hive.h"

extern "C" {
#include <unistd.h>
#include <sys/types.h>
}

using namespace InterfaceLibrary;
using namespace InterfaceLibrary::Primitive;


//**************************************************************************************
int Utilities::IsRoot( void )
{
	if ( geteuid() == 0 ) return YES;

	return NO;
}

//**************************************************************************************
void Utilities::GenRandomBytes( char *b1, int s1, char *b2, int s2 )
{
    int i;

    // fill the first buffer
    for ( i = 0; i < s1; i++ )
    {
        b1[i] = (char)( rand() % 255 );
    }

    // fill the second buffer
    if ( b2 != NULL )
    {
        for ( i = 0; i < s2; i++ )
        {
            b2[i] = (char)( rand() % 255 );
        }
    }

    return;
}

//**************************************************************************************
bool Utilities::SetPrimitiveSupported( InterfaceLibrary::uint32 primitiveID, bool setSupported)
{
    InterfaceLibrary::Primitive::RefTableEntry * fFoundPrimitive ;
    LibraryModuleBase* handler = LibraryModuleBase::GetHandlerObject();
    fFoundPrimitive = (handler->primitiveRefTable).FindEntry( primitiveID );

    if (fFoundPrimitive != NULL) {
        if (fFoundPrimitive->primitiveID == primitiveID) {
            fFoundPrimitive->currentlySupported = setSupported;
            return true;
        } else {
            return false;
        }
    } else {  // double check
        return false;
    }
}

//*****************************************************************************************
int	Utilities::SetCTStateConnected( void )
{
	/* turn these functions "on" */
	// command execute
	Utilities::SetPrimitiveSupported( 0x08000003, true );

	// command session
	Utilities::SetPrimitiveSupported( 0x08000004, true );

	// command exit
	Utilities::SetPrimitiveSupported( 0x03000008, true );

	// file put
	Utilities::SetPrimitiveSupported( 0x02000003, true );

	// file get
	Utilities::SetPrimitiveSupported( 0x02000009, true );

	//file delete
	Utilities::SetPrimitiveSupported( 0x02000005, true );

	/* turn these functions "off" */
	// connect
	Utilities::SetPrimitiveSupported( 0x03000002, false );
	// listen
	Utilities::SetPrimitiveSupported( 0x03000003, false );
	// trigger
	Utilities::SetPrimitiveSupported( 0x03000004, false );

	// make these options available in the CT interface
	LibraryModuleBase *handler = LibraryModuleBase::GetHandlerObject();
	handler->RefreshCT();

	return 0;
}

int	Utilities::SetCTStateDisconnected( void )
{
	/* turn these functions "off" */
	// command execute
	Utilities::SetPrimitiveSupported( 0x08000003, false );

	// command session
	Utilities::SetPrimitiveSupported( 0x08000004, false );

	// command exit
	Utilities::SetPrimitiveSupported( 0x03000008, false );

	// file put
	Utilities::SetPrimitiveSupported( 0x02000003, false );

	// file get
	Utilities::SetPrimitiveSupported( 0x02000009, false );

	//file delete
	Utilities::SetPrimitiveSupported( 0x02000005, false );

	/* turn these functions "on" */
	// connect
	Utilities::SetPrimitiveSupported( 0x03000002, true );
	// listen
	Utilities::SetPrimitiveSupported( 0x03000003, true );
	// trigger
	Utilities::SetPrimitiveSupported( 0x03000004, true );

	// make these options available in the CT interface
	LibraryModuleBase *handler = LibraryModuleBase::GetHandlerObject();

	handler->RefreshCT();

	return 0;
}
