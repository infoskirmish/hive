/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/ProcessCmdResponseX.h$
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

/*-----------------  ProcessCmdResponseX - FILE DESCRIPTION  -----------------*

Declaration of the ProcessCmdResponseX class and the component classes that are
part of its modeling.
*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#if !defined(PROCESSCMDRESPONSEX_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_)
#define PROCESSCMDRESPONSEX_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_

/*-----------------  ProcessCmdResponseX - INCLUDES  -------------------------*/

#include "ProcessCmdResponse.h"
#include "XMLParserStack.h"
#include "IXMLStreamable.h"
#include "StringParser.h"

/*-----------------  ProcessCmdResponseX - MISCELLANEOUS  --------------------*/

/*-----------------  ProcessCmdResponseX - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary {

/**
Description:
   This class models the reply from a ProcessCmd call, and adds XML
   deserialization capability to the ProcessCmdResponse base class.
*/
class ProcessCmdResponseX : public StringParser, public ProcessCmdResponse, public IXMLStreamable
{
public:
   ProcessCmdResponseX()
   {
      elements = new XMLParserStack(this, xmlTagName);
   }

   virtual
   ~ProcessCmdResponseX();

   /**
      This IXMLStreamable override uses the ProcessCmdResponse base class'
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
      IXMLStreamable override returning the 'elements' member, with whom this
      object has been registered as the root element parser.
   */
   virtual XMLParser&
   GetXMLParser ();

   /**
      This XMLParser override allows the ProcessCmdResponseX to detect simple or
      complex sub-elements in an XML stream.
   */
   virtual void
   startElement (
      const char *name,    // The element tag.
      const char **attrs
      );

   /**
      This XMLParser override allows the ProcessCmdResponseX to close out the
      parsing of a sub-element.
   */
   virtual void
   endElement (
      const char *name     // The element tag.
      );

   /**
   Description:
      The CellX class extends the ProcessCmdResponse::Cell class to add XML
      deserialization capabilities.
   */
   class CellX : public StringParser, public Cell
   {
   public:

      virtual
      ~CellX();

      /**
         This XMLParser override allows the CellX to place the previously-parsed
         String::Parser content member into the appropriate member.
      */
	   virtual void
      endElement (
         const char *name     // The element tag.
         );

   };

   /**
   Description:
      The Table class can occur multiple times within a ProcessCmd response, and logically
      consists of a two-dimensional array of Cells, with the dimensions of the array
      given by the rows and columns members.  In reality, since a Table physically
      stores itself as a non-sparse array, the position information in the Cells
      is redundant, so the use of the Cell class is used mainly to assist in
      serialization and deserialization.
   */
   class TableX : public StringParser, public Table
   {
   public:
      /**
         Default constructor for a results Table.  Since the class' XML parsing behavior
         depends on the XMLParserStack's assistance, the SetStack method must be called
         before this object can be used for XML parsing.
      */
      TableX ()
         : Table()
      {
         stack = NULL;
      }

      /**
         This constructor overload relieves the caller of the need to call SetStack.
      */
      TableX (
         XMLParserStack&   s
         ) : Table()
      {
         stack = &s;
      }

      virtual
      ~TableX();

      /**
         This XMLParser override allows the Table to detect simple or complex
         sub-elements in an XML stream.
      */
	   virtual void
      startElement (
         const char *name,    // The element tag.
         const char **attrs
         );

      /**
         This XMLParser override allows the Table to place the previously-parsed
         String::Parser content member into the appropriate member.
      */
	   virtual void
      endElement (
         const char *name     // The element tag.
         );

      /**
         Returns this object, since a TableX acts as its own parser.
      */
      virtual XMLParser&
      GetXMLParser ();

      /**
         If the default constructor is used, SetStack must be called before the
         object can be used for deserializing an XML stream.
      */
      void
      SetStack(
         XMLParserStack&   s
         )
      { stack = &s; }

   protected:

      CellX  cellBeingParsed;  // Used during parsing operations to hold the cell currently being deserialized.
      XMLParserStack*   stack;
   };

   /**
   Description
      Barely qualifying as a "complex type", a Line is an element containing
      verbosity and text sub-elements, both simple strings.
   */
   class LineX : public StringParser, public Line
   {
   public:
      LineX()
      {}

      virtual
      ~LineX();

      /**
         This XMLParser override allows the Line to place the previously-parsed
         String::Parser content member into the appropriate member.
      */
	   virtual void
      endElement (
         const char *name     // The element tag.
         );

   };

protected:
   XMLParserStack*      elements;      // The parser stack used to manage the object heirarchy during parsing.
                                       //    It has to be a pointer since it doesn't have a default constructor
                                       //    and this object is the root parser.

   TableX*  tableBeingParsed;    // Used when a Table is being deserialized.

   LineX*   lineBeingParsed;     // Used when a Line is being deserialized.
};

};

#endif // !defined(PROCESSCMDRESPONSEX_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_)

