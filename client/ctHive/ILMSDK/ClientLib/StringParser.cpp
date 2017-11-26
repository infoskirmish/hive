/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/StringParser.cpp$
$Revision: 1$
$Date: Tuesday, August 25, 2009 2:42:12 PM$
$Author: timm$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
**
**	               (C)  Copyright Northrop Grumman ES/
**                          XETRON Corporation
**                         All rights reserved
*/

/*-----------------  StringParser - FILE DESCRIPTION  -----------------*

Implementation of basic classes and functionality used by both service and client
sides of the library interface.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  StringParser - INCLUDES  -------------------------*/

#include "StringParser.h"

/*-----------------  StringParser - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;
using std::string;

/*==========================================================================*/


StringParser::~StringParser()
{}

void StringParser::startElement(const char *n, const char **attrs)
{ tagName = n; content.clear(); }

void StringParser::characterData(const char *str, int len)
{ content.append(str, len); }

