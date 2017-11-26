/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/source/XMLComposer.cpp$
$Revision: 1$
$Date: Friday, October 09, 2009 5:04:22 PM$
$Author: timm$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  XMLComposer - FILE DESCRIPTION  -----------------*

Implementation of the XMLComposer class.  This module was shamelessly plagiarized
from the open-source EasySoap project, thought it has been augmented with
some functionality that will make it easier to use as a stand-alone XML
generator, and some of the higher-level namespace management capabilities
have been jettisoned in the interest of simplicity since they were not
needed for the immediate application.

The license statement from the module from which this one is originally
adapted is:

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
 * $Id: XMLComposer.cpp,v 1.6 2006/11/09 07:24:45 dcrowley Exp $
 */
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  XMLComposer - INCLUDES  -------------------------*/
#include <string>
#include "XMLComposer.h"


/*-----------------  XMLComposer - DECLARATIONS ----------------------*/

using std::string;
using std::stringstream;
using std::runtime_error;

XMLComposer::~XMLComposer()
{
}


string
XMLComposer::GetSymbol(const char *prefix)
{
   stringstream s;
   s << prefix << ++gensym;
	return s.rdbuf()->str();
}

void
XMLComposer::Reset(bool addDecl)
{
	inStart = false;
   while (!elementTags.empty()) elementTags.pop();
   gensym = 0;
   this->rdbuf()->pubseekpos(0);

	if (addDecl)
	{
		StartPI("xml");
		AddAttr("version", "1.0");
		AddAttr("encoding", "UTF-8");
		EndPI();
	}
}


void
XMLComposer::StartTag(const char *tag)
{
	EndStart();
	*this << '<' << tag;
   elementTags.push(string(tag));
	inStart = true;
}

void
XMLComposer::EndStart()
{
   if (inStart) {
	   *this << '>';
	   if (beautify) *this << "\n" << string(elementTags.size()*indent, ' ');
	   inStart = false;
   }
}

void
XMLComposer::AddAttr(const char *attr, const char *value) throw (runtime_error)
{
	if (!inStart)
      throw runtime_error("XML serialization error: Adding attribute when not in start tag.");

	*this << ' ' << attr << "=\"";
	WriteEscaped(value);
	*this << '"';
}

void
XMLComposer::AddXMLNS(const char *prefix, const char *ns)
{
	if (beautify)
		*this << "\n" << string(elementTags.size()*indent, ' ');
	else
		*this << ' ';

	*this << "xmlns";
	if (prefix) *this << ':' << prefix;
	*this << "=\"";
	WriteEscaped(ns);
	*this << '"';
}

void
XMLComposer::EndTag() throw (runtime_error)
{
   if (elementTags.empty()) throw runtime_error("XML serialization errror: Element tag stack underflow detected in EndTag.");

   string tagName = elementTags.top();
   elementTags.pop();
	if (inStart)
	{
		*this << "/>";
      if (beautify) *this << "\n" << string(elementTags.size()*indent, ' ');
	}
	else
	{
      // If we're beautifying, we'll have already gone in one indent too far, so
      //  we'll need to back out one indent before putting in the close tag
      if (beautify) this->rdbuf()->pubseekoff(-indent, ios_base::cur, ios_base::out);
      *this << "</" << tagName << '>';
      if (beautify) *this << "\n" << string(elementTags.size()*indent, ' ');
	}
	inStart = false;
}

void
XMLComposer::WriteSimpleElement(const char* tagName, const char* content)
{
   StartTag(tagName);
   WriteValue(content);
   // Doing things manually instead of calling EndTag to keep an unnecessary
   // newline from being added when beautifying.
   elementTags.pop();
   *this << "</" << tagName << '>';
   if (beautify) *this << "\n" << string(elementTags.size()*indent, ' ');
}

void
XMLComposer::WriteSimpleElement(const char* tagName, const string& content)
{
   StartTag(tagName);
   WriteValue(content);
   // Doing things manually instead of calling EndTag to keep an unnecessary
   // newline from being added when beautifying.
   elementTags.pop();
   *this << "</" << tagName << '>';
   if (beautify) *this << "\n" << string(elementTags.size()*indent, ' ');
}

void
XMLComposer::WriteSimpleElement(const char* tagName, long content)
{
   StartTag(tagName);
   WriteValue(content);
   // Doing things manually instead of calling EndTag to keep an unnecessary
   // newline from being added when beautifying.
   elementTags.pop();
   *this << "</" << tagName << '>';
   if (beautify) *this << "\n" << string(elementTags.size()*indent, ' ');
}

void
XMLComposer::WriteSimpleElement(const char* tagName, double content)
{
   StartTag(tagName);
   WriteValue(content);
   // Doing things manually instead of calling EndTag to keep an unnecessary
   // newline from being added when beautifying.
   elementTags.pop();
   *this << "</" << tagName << '>';
   if (beautify) *this << "\n" << string(elementTags.size()*indent, ' ');
}

void
XMLComposer::WriteEscaped(const char *str)
{
	if (str!=NULL)
	{
		while (*str!=0)
		{
			char c = *str++;
			switch (c) {
            case '&':   *this << "&amp;";    break;
            case '<':   *this << "&lt;";     break;
            case '>':   *this << "&gt;";     break;
            case '\'':  *this << "&apos;";   break;
            case '"':   *this << "&quot;";   break;
            case '\r':  *this << "&#xd;";    break;
            default:    *this << c;          break;
         }
		}
	}
}

