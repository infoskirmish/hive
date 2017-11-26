#ifndef __UTILITIES_H
#define __UTILITIES_H

#include "Primitive_ref.h"
#include "Primitive.h"
#include "LibraryModuleBase.h"

//*********************************************************************************
namespace Utilities {

//*********************************************************************************
int	IsRoot( void );
void GenRandomBytes( char *b1, int s1, char *b2, int s2 );

//*********************************************************************************
/**
*  Updates the Primitive reference table to either set the Primitive
*  supported status to 1 if setSupported is true or 0 if setSupported is false
*  Note: This routine is typically called in passthru mode
*
* @param primitiveID Primitive id number
* @param setSupported desired support status for the specified Primitive
*
* @return true if the primitive status was successfully changed
*/

bool SetPrimitiveSupported( InterfaceLibrary::uint32 primitiveID, bool setSupported );

int	SetCTStateConnected( void );

int SetCTStateDisconnected( void );

}

#endif
