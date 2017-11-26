/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/XMLParser.cpp$
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

/*-----------------  XMLParser - FILE DESCRIPTION  -----------------*

Implementation of the XMLParser class, which was adapted from EasySoap and
enhanced.  The original module's license declaration is:

 * EasySoap++ - A C++ library for SOAP (Simple Object Access Protocol)
 * Copyright (C) 2001 David Crowley; SciTegic, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: XMLParser.cpp,v 1.13 2006/11/09 07:24:45 dcrowley Exp $
*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  XMLParser - INCLUDES  -------------------------*/

#include "XMLParser.h"
#include <expat.h>
#include <string.h>

/*-----------------  XMLParser - DECLARATIONS ----------------------*/

#define EXPAT_VER(a,b,c) (((((a)*1000)+(b))*1000)+(c))
#define EXPAT_VERSION EXPAT_VER(XML_MAJOR_VERSION,XML_MINOR_VERSION,XML_MICRO_VERSION)

/*==========================================================================*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

XMLParser::~XMLParser()
{
	FreeParser();
}

bool
XMLParser::QuickParse(const char* docTxt, size_t len)
{
   if (len==0) len=strlen(docTxt);
   return XML_Parse(m_parser, docTxt, len, 0) == XML_STATUS_OK;
}

void
XMLParser::FreeParser()
{
	if (m_parser)
	{
		XML_ParserFree(m_parser);
		m_parser = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Initialization and Management
//////////////////////////////////////////////////////////////////////

void
XMLParser::InitParser(const char *encoding)
{
#if EXPAT_VERSION >= EXPAT_VER(1,95,3)
	if (m_parser)
	{
		XML_ParserReset(m_parser, encoding);
	}
	else
#endif
	{
		FreeParser();
		m_parser = (struct XML_ParserStruct*)XML_ParserCreateNS(encoding, '#');
	}
	XML_SetElementHandler(m_parser,
			XMLParser::_startElement,
			XMLParser::_endElement);

	XML_SetCharacterDataHandler(m_parser,
			XMLParser::_characterData);

	XML_SetStartNamespaceDeclHandler(m_parser,
			XMLParser::_startNamespace);

	XML_SetEndNamespaceDeclHandler(m_parser,
			XMLParser::_endNamespace);

	XML_SetUserData(m_parser, this);
}

void *
XMLParser::GetParseBuffer(int size)
{
	if (m_parser)
		return XML_GetBuffer(m_parser, size);
	return 0;
}

bool
XMLParser::ParseBuffer(int size)
{
	if (m_parser)
		return XML_ParseBuffer(m_parser, size, size == 0) != 0;
	return false;
}

const char *
XMLParser::GetErrorMessage()
{
	if (m_parser)
		return XML_ErrorString(XML_GetErrorCode(m_parser));
	return NULL;
}

//////////////////////////////////////////////////////////////////////
// Overridable methods
//////////////////////////////////////////////////////////////////////

void
XMLParser::startElement(const char *, const char **)
{
}

void
XMLParser::endElement(const char *)
{
}

void
XMLParser::characterData(const char *, int)
{
}

void
XMLParser::startNamespace(const char *, const char *)
{
}

void
XMLParser::endNamespace(const char *)
{
}

//////////////////////////////////////////////////////////////////////
// Static methods
//////////////////////////////////////////////////////////////////////

void
XMLParser::_startElement(void *userData, const XML_Char *name, const XML_Char **attrs)
{
	XMLParser *parser = (XMLParser *)userData;
	parser->startElement(name, attrs);
}

void
XMLParser::_endElement(void *userData, const XML_Char *name)
{
	XMLParser *parser = (XMLParser *)userData;
	parser->endElement(name);
}

void
XMLParser::_characterData(void *userData, const XML_Char *str, int len)
{
	XMLParser *parser = (XMLParser *)userData;
	parser->characterData(str, len);
}

void
XMLParser::_startNamespace(void *userData, const XML_Char *prefix, const XML_Char *uri)
{
	XMLParser *parser = (XMLParser *)userData;
	parser->startNamespace(prefix, uri);
}

void
XMLParser::_endNamespace(void *userData, const XML_Char *prefix)
{
	XMLParser *parser = (XMLParser *)userData;
	parser->endNamespace(prefix);
}


const char*
XMLParser::FindAttribute (const char **attrs, const char* attrName)
{
   while (**attrs!=0) {
      if (strcmp(attrName, *attrs++)==0)
         return *attrs;
      else
         attrs++;
   }
   return NULL;
}
