/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/XMLParser.h$
$Revision: 1$
$Date: Tuesday, August 25, 2009 2:42:12 PM$
$Author: timm$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
**
**	               (C)  Copyright Northrop Grumman ES/
**                          XETRON Corporation
**                         All rights reserved
*/

/*-----------------  XMLParser - FILE DESCRIPTION  -----------------*

Declaration of the XMLParser class.  This module was shamelessly plagiarized
from the open-source EasySoap project, thought it has been augmented with
some functionality that will make it easier to use as a stand-alone XML
parser, and some of the higher-level namespace management capabilities
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
 * $Id: XMLParser.h,v 1.6 2003/06/03 17:32:19 dcrowley Exp $
 */
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#if !defined(XMLPARSER_H__751545FF_EF84_42BC_9622_A6CE624F1F14__INCLUDED_)
#define XMLPARSER_H__751545FF_EF84_42BC_9622_A6CE624F1F14__INCLUDED_

/*-----------------  XMLParser - INCLUDES  -------------------------*/

#include <stddef.h>

/*-----------------  XMLParser - MISCELLANEOUS  --------------------*/

#define PARSER_NS_SEP "#"

/*-----------------  XMLParser - DECLARATIONS  ---------------------*/

struct XML_ParserStruct;


/**
   XMLParser is the base class for extensions that reconstitute an object from
   an XML stream. XMLParser is based on the open-source Expat XML parsing
   library, and is a class wrapper for the most commonly used capabilities of
   that library.  XMLParser allows a much more reasonable distribution of
   parsing intelligence among classes representing complex types within
   an XML stream, rather than having to have one "uber-parser" that
   must be able to interpret an entire XML document.

   XMLParser has no pure virtual methods, and is thus instantiable as is
   (though it won't do much by itself.)  This is a boon to derived classes
   that don't need all of the overrides; for example, many applications
   may not care about the namespace declarations, and would thus not need
   to override the startNamespace and endNamespace methods.

<example>
   There are two ways to use an XMLParser.  The "quick" way is good when you
   have the luxury of waiting until the entire XML stream is available before
   you begin parsing.  This is frequently the case when you're using XMLParser
   as a SOAP-like mechanism for serializing objects.  The long way is better for
   piecemeal work, when you need to start parsing a stream before it's all
   available, or if it's too big to conveniently handle in a single chunk.

   For both approaches, you need to derive a class that extends XMLParser, and
   overrides at least one of its protected virtual methods:

<pre>
   class MyParserClass : public XMLParser
   {
   public:
      double dblVal;
      int    intVal;
   protected:
      string charBuf;

      virtual void endElement (
            const char *name)
      {
         if (strcmp(name, "dblVal")==0) {
            dblVal = atof(charBuf.c_str());
         } else if (strcmp(name, "intVal")==0) {
            intVal = atoi(charBuf.c_str());
         }
      }

      virtual void characterData (
            const char *str, int len)
      {
         charBuf = string(str, (string::size_type)len);
      }
   };
</pre>

   In this case, we've created a class which overrides two of the three common
   data recovery methods in XMLParser.  MyParserClass acts as its own XMLParser,
   and it deserializes its two data members from elements whose XML tag names
   happen to be the same as the member names.  It uses a single string member,
   charBuf, as a temporary holding location to save element contents until we
   know which member to parse them into.  (An alternative solution would have
   been to override startElement instead of endElement, set an enumerated member
   to indicate which if any member's element is being parsed, and interpret the
   data in characterData; either approach is equally valid, since we get the
   same tag name argument in both startElement and endElement methods.)
   MyParserClass will work against an XML fragment that looks like this:

<pre>
   <obj>
     <dblVal>3.14159</dblVal>
     <intVal>42</intVal>
   </obj>
</pre>

   The <obj> tags are ignored by MyParserClass, but are needed to make a valid
   XML document (which must have exactly one root element.)  In practice, it's
   typical for a serialized object not to be responsible for its delimiting
   element tags, but instead to leave them up to a containing object.

   The short approach will simply look like this:

<pre>
   char* xmlText = "<obj> <dblVal>3.14159</dblVal> <intVal>42</intVal> </obj>"
   MyParserClass c;
   c.QuickParse(xmlText);
</pre>

   While QuickParse can be called multiple times with different segments of the
   XML stream, it shines in cases where the whole thing can be done in one shot,
   especially when there's no need to explicitly allocate the memory for it. By
   contrast, the long approach would look something like this:

<pre>
   fstream fs("myStream.xml", ios_base::in);
   MyParserClass prsr;
   prsr.InitParser();

   while (!fs.eof()) {
      char* buffer = (char*)prsr.GetParseBuffer(40);
      fs.getline(buffer, 40, '>');
      int i=strlen(buffer);
      if (!fs.eof() && i!=0) {
         buffer[i]='>';  // Since this is getline delimiter, have to put it back
         if (!prsr.ParseBuffer(i+1)) {
            cout << prsr.GetErrorMessage() << endl;
            break;
         }
      }
   }
</pre>

   While the long approach obviously requires more lines of code, and is going
   to go back to the memory well a number of times for buffer allocation, it
   should be apparent that it's much better at dealing with a continuous stream
   of information.  In our example, we've keyed our getline function off of the
   tag end delimiter, '>', rather than an end-of-line.  This is a good strategy
   for parsing a serialized object that doesn't have any long element text,
   since it's impervious to unbeautified input.  You need only guarantee that
   the specified buffer size is long enough to contain the longest element text
   plus the longest end tag.

   </example>
*/
class XMLParser
{
public:
   /**
      The constructor for an XMLParser will automatically call InitParser with
      the specified character set.  If a different character set is needed,
      another call to InitParser can safely be made before any other methods are
      called.
   */
	XMLParser(const char *charset = NULL)
   {
      m_parser = NULL;
      InitParser(charset);
   }

   /**
      The destructor frees any resources that may still be held by the object.
   */
	virtual ~XMLParser();

   /**
      If the entire XML document is available in a single shot, this method will
      perform all necessary parsing functions in a single call.
      
      <param name="docTxt"> The XML text to be parsed   </param>
      <param name="len"> The length of the text to parse.  Must be less than or
      equal to the actual string length; if zero (default), the entire string
      will be parsed up to the terminating nul character.  </param>
      
      <returns> True if the parsing came off without an error; false otherwise.
      If false, GetErrorMessage can be used to determine the cause of the
      parsing failure. </returns>
   */
   bool QuickParse(const char* docTxt, size_t len = 0);

   /**
      Prepares this object for parsing operations.  Called automatically by the
      constructor, but may be safely called again before other methods are
      called if the character set can't be determined at the time of
      construction.
      
      <param name="charset"> The language character set of the XML document to
      be parsed.  The default is a UTF-8 encoding.   </param>
   */
	void InitParser(const char *charset = 0);

   /**
      This poorly-named method allocates a working buffer for the C-based Expat
      parsing functions of the specified size.

      <returns>
      The newly-allocated buffer, whose address is also stored in the m_parser
      structure.  (The operation that follows a call to GetParseBuffer is
      usually to copy some XML stream data into the buffer.)  </returns>
   */
	void *GetParseBuffer(int size);

   /**
      Proving that ParseBuffer is a verb as well as a noun, this method causes
      the buffer allocated in GetParseBuffer to be interpreted, resulting in
      a God-awful number of invocations of this object's startElement, endElement,
      and characterData methods.
      
      <param name="size">  The valid amount of data in the buffer.
      </param>
      
      <returns> True if the operation came off without a hitch; false if an
      error was encountered, in which case GetErrorMessage may be used to see
      what went wrong.    </returns>
   */
   bool ParseBuffer(int size);

   /**
      If a previous ParseBuffer call returned false, this method returns a string
      describing the error that it encountered.
   */
	const char *GetErrorMessage();

   /**
      This helper method is useful in startElement overrides to locate in the
      attribute list the value of an XML element attribute with the specified name.

      <returns>
      The value of the attribute if located; NULL otherwise.    </returns>
   */  
   static const char*
   FindAttribute (
      const char **attrs,
      const char* attrName
      );

protected:
   /**
      The parser invokes this method when an element start tag is
      encountered.
      
      <param name="name">  The element tag name.    </param>
      <param name="attrs">  If non-null, this array contains alternating
         name/value pairs for the attributes that appeared in the start tag.
      </param>
   */
	virtual void startElement (const char *name, const char **attrs);

   /**
      The parser invokes this method when an element end tag is encountered.

      <param name="name">  The element tag name.    </param>
   */
	virtual void endElement (const char *name);

   /**
      The parser invokes this method for a section of text that appears between
      element tags.  In general, it's possible for several characterData calls
      to happen between startElement and endElement calls.  (Newline characters
      in particular will cause extra invocations all by themselves.)

      <param name="str">  A segment of element content, not nul terminated; the
         len argument must be used to determine the meaningful length.
      </param>
      <param name="len">  The valid length of the str argument.
      </param>
   */
   virtual void characterData ( const char *str, int len  );

   /**
      The parser invokes this method when the start
      of a namespace declaration is encountered.

      <param name="prefix">  The namespace prefix.     </param>
      <param name="uri">  The URI declared for the namespace.   </param>
   */
	virtual void startNamespace(const char *prefix, const char *uri);

   /**
      The parser invokes this method when the end
      of a namespace declaration is encountered.
   
      <param name="prefix">  The namespace prefix.     </param>
   */
	virtual void endNamespace(const char *prefix);

   friend class XMLParserStack;

protected:
   /**
      Used by the InitParser method and the class destructor to free
      up the m_parser structure.
   */
	void FreeParser();

   /// The Expat parser descriptor structure.
	struct XML_ParserStruct *m_parser;

private:
   typedef char XML_Char;
   static void _startElement(void *userData, const XML_Char *name, const XML_Char **attrs);
   static void _endElement(void *userData, const XML_Char *name);
   static void _characterData(void *userData, const XML_Char *str, int len);
   static void _startNamespace(void *userData, const XML_Char *prefix, const XML_Char *uri);
   static void _endNamespace(void *userData, const XML_Char *prefix);
};

#endif // !defined(XMLPARSER_H__751545FF_EF84_42BC_9622_A6CE624F1F14__INCLUDED_)

