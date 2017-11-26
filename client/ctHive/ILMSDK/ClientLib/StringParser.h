/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/StringParser.h$
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

/*-----------------  StringParser - FILE DESCRIPTION  -----------------*

Declaration of basic classes and functionality used by both service and client
sides of the library interface.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef	Interface_Elements_H			// Only process if not already processed
#define	Interface_Elements_H

/*-----------------  StringParser - INCLUDES  -------------------------*/

#include "SDKDataTypes.h"
#include "XMLParser.h"

/*-----------------  StringParser - MISCELLANEOUS  --------------------*/


/*-----------------  StringParser - DECLARATIONS  ---------------------*/


namespace InterfaceLibrary {

   /**
   Description:
      A StringParser will capture the tag name and text content of a simple XML element.
      It has no endElement overload, so it's to be used only as a proxy parser when
      a simple element is expected, but it can nevertheless be very handy in keeping
      deriving classes from having to implement nearly identical startElement and characterData
      overrides.  (Check out the Primitive class in this module for an example of how
      this is applied.)
   */
   class StringParser : public XMLParser
   {
   public:
      StringParser() {}

      virtual ~StringParser();

      std::string tagName;    // The XML tag for the simple text element.
      std::string content;    // The text content between the start and end tags.

      /**
         Override of the XMLParser::startElement method.
      */
      virtual void
      startElement (const char *name, const char **attrs);

      /**
         Override of the XMLParser::characterData method.
      */
      virtual void
      characterData(const char *str, int len);
   };

};

#endif
