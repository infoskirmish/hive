/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/CustomCommandX.h$
$Revision: 1$
$Date: Tuesday, August 25, 2009 2:42:11 PM$
$Author: timm$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
**
**	               (C)  Copyright Northrop Grumman ES/
**                          XETRON Corporation
**                         All rights reserved
*/

/*-----------------  CustomCommandX - FILE DESCRIPTION  -----------------*

Declaration of the classes that model the response from the ILM interface's
AddCommands function, with XML parsing capabilities for use by the ILM's client.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef	CustomCommandX_H			// Only process if not already processed
#define	CustomCommandX_H

/*-----------------  AddCommandsResponse - INCLUDES  -------------------------*/

#include "CustomCommand.h"
#include "IXMLStreamable.h"
#include "StringParser.h"

/*-----------------  AddCommandsResponse - MISCELLANEOUS  --------------------*/

class XMLParserStack;

/*-----------------  AddCommandsResponse - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary {

   /**
   Description:
      The specialized parser for the CustomCommand class.
   */
   class CustomCommandParser : public StringParser
   {
   public:

      /**
      Description:
         XML parser for a CustomCommand::Attribute.
      */
      class AttributeParser : public StringParser
      {
      public:
         /**
            Default constructor for an AttributeX.
         */
         AttributeParser(
            CustomCommand::Attribute& a   // The Attribute that contains the parsed content.
            ) : target(a)
         {}

         /**
            The AttributeX class overrides the String::Parser's endElement
            method so that it can transfer the parsed content member to the
            appropriate member.
         */
	      virtual void
         endElement(const char *tag);

      protected:
         CustomCommand::Attribute&  target;
      };

      /**
      Description:
         XML parser for a CustomCommand::Parameter.
      */
      class ParameterParser : public StringParser
      {
      public:
         ParameterParser (
            XMLParserStack& s,
            CustomCommand::Parameter& p
            ) : myStack(s), target(p), attribParser(p.attribute)
         {}

         /**
            Override of the String::Parser's startElement method.

         Usage Notes:
            *** VERY IMPORTANT !!! ***
            Parameter's startElement method must be invoked so that it can catch its
            own attributes (XML attributes, that is) in the element start tag.  (CustomCommand's
            handler does this.)
         */
	      virtual void
         startElement (
            const char *name,    // The element tag
            const char **attrs   // If non-null, name/value pairs for the attributes which appeared in the start tag.
            );

         /**
            The Parameter class overrides the String::Parser's endElement method
            so that it can transfer the parsed content member to the appropriate
            Parameter member string for simple string elements.
         */
	      virtual void
         endElement(const char *tag);

      protected:
         XMLParserStack&   myStack;
         CustomCommand::Parameter&  target;
         AttributeParser   attribParser;
      };

      /**
         The CustomCommand constructor can be used to initialize the parser stack
         member if the object is to be used for XML deserialization.
      */
      CustomCommandParser (
         XMLParserStack&   s,
         CustomCommand&    c
         ) : parserStack(s), target(c)
      {
         paramParser = NULL;
         attribParser = NULL;
      }

      virtual
      ~CustomCommandParser();

      /**
         Override of the String::Parser's startElement method.

      Usage Notes:
         *** VERY IMPORTANT !!! ***
         CustomCommand's startElement method must be invoked so that it can catch its
         own attributes in the element start tag.  (CustomCommandSet's handler does
         this.)
      */
	   virtual void
      startElement (
         const char *name,    // The element tag
         const char **attrs   // If non-null, name/value pairs for the attributes which appeared in the start tag.
         );

      /**
         The Primitive class overrides the String::Parser's endElement method
         so that it can transfer the parsed content member to the appropriate
         Primitive member string.
      */
	   virtual void
      endElement(const char *tag);

   protected:
      void
      ResetParsers()
      {
         delete paramParser;
         delete attribParser;
         paramParser = NULL;
         attribParser = NULL;
      }

      XMLParserStack&   parserStack;
      CustomCommand&    target;
      ParameterParser*  paramParser;
      AttributeParser*  attribParser;
   };

   /**
   Description:
      Container class for CustomCommand objects with XML deserialization
      (parsing) capabilities. CustomCommandSetX extends CustomCommandSet by
      adding the parsing half of the IXMLStreamable interface (CustomCommandSet
      already implements the composing half).  It uses an XMLParserStack with
      the CustomCommandSet itself registered as the root, and uses the stack to
      hand off parsing of the CustomCommandParser elements during
      deserialization.
   */
   class CustomCommandSetX : public CustomCommandSet, public XMLParser, public IXMLStreamable
   {
   public:
      /**
         The default CustomCommandSet constructor allocates the XMLParserStack member that
         is the object's parser.
      */
      CustomCommandSetX();

      /**
         The destructor deallocates the XMLParserStack member.
      */
      virtual
      ~CustomCommandSetX();

      /**
         This IXMLStreamable override uses the CustomCommandSet base class'
         XMLCompose method to serialize itself into the specified XML text stream
         composer.  It must be re-declared here in order to satisfy the
         IXMLStreamable interface.
   
      Returns:
         The dest argument.
      */
      virtual XMLComposer&
      XMLCompose (
         XMLComposer&   dest
         ) const;

      /**
         IXMLStreamable override that returns the myStack member that
         acts as the command set's parser.
      */
      virtual XMLParser&
      GetXMLParser();

      /**
         This override fires during ParseBuffer processing when an element
         start tag is encountered.  If the tag is "custcmdset", the members collection
         is cleared; if the tag is "cmd", a CustomCommand is added to the collection
         and given control of parsing until its end tag is encountered.
      */
	   virtual void
      startElement (
         const char *name,    // The element tag
         const char **attrs   // If non-null, name/value pairs for the attributes which appeared in the start tag.
         );

   protected:
      XMLParserStack* myStack;   // The XML parser for this object.  Has to be a pointer allocated
                                 // explicitly in the constructor to prevent a "chicken & egg" problem.

      CustomCommandParser* cmdParser;  // The parser for the CustomCommand currently being parsed,
                                       //    which will always appear at the end of the vector container.
   };

};
#endif


