/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/ProcessCmdResponseX.cpp$
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

/*-----------------  ProcessCmdResponse - FILE DESCRIPTION  -----------------*

Implementation of basic classes and functionality used by both service and client
sides of the library interface.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  ProcessCmdResponse - INCLUDES  -------------------------*/

#include "ProcessCmdResponseX.h"

/*-----------------  ProcessCmdResponse - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;

/*==========================================================================*/

//
// CellParser class definitions
//

ProcessCmdResponseX::CellX::~CellX()
{}

void
ProcessCmdResponseX::CellX::endElement ( const char *name )
{
   if (row_tag==name) {
      row = atoi(content.c_str());
   } else if (column_tag==name) {
      column = atoi(content.c_str());
   } else if (text_tag==name) {
      text = content;
   }
}

//
// Table class definitions
//

ProcessCmdResponseX::TableX::~TableX()
{}

void
ProcessCmdResponseX::TableX::startElement ( const char *name, const char **attrs )
{
   if (Cell::xmlTagName == name) {
      if (stack==NULL) throw std::runtime_error("XMLParserStack member not initialized in ProcessCmdResponseX::TableX object");
      stack->SubstituteTop(&cellBeingParsed);
   } else {
      StringParser::startElement(name, attrs);
   }
}


void
ProcessCmdResponseX::TableX::endElement ( const char *name )
{
   if (Cell::xmlTagName == name) {
      // Finished a "cell" subelement
      int index = cellBeingParsed.row * cellBeingParsed.column;
      cells[index] = cellBeingParsed.text;
   } else if (rows_tag==name) {
      // Got a "rows" element
      rows = atoi(content.c_str());
      if (rows>0)
         SetSize(rows, columns);
   } else if (columns_tag==name) {
      // Got a "columns" element
      columns = atoi(content.c_str());
      if (columns>0)
         SetSize(rows, columns);
   } else if (verbosity_tag==name) {
      // Got a "verbosity" element (simple String)
      verbosity = content;
   }
}

XMLParser&
ProcessCmdResponseX::TableX::GetXMLParser ()
{ return *this; }


//
// Line class definitions
//

ProcessCmdResponseX::LineX::~LineX()
{
}

      /**
         This XMLParser override allows the Line to place the previously-parsed
         String::Parser content member into the appropriate member.
      */
void
ProcessCmdResponseX::LineX::endElement ( const char *name )
{
   if (text_tag==name)
      text = content;
   else if (verbosity_tag==name)
      verbosity = (uint32)atol(content.c_str());
}


//
// ProcessCmdResponse class definitions
//


ProcessCmdResponseX::~ProcessCmdResponseX()
{
   delete elements;
}

XMLComposer&
ProcessCmdResponseX::XMLCompose (
   XMLComposer&   dest
   ) const
{
   return ProcessCmdResponse::XMLCompose(dest);
}

XMLParser&
ProcessCmdResponseX::GetXMLParser ()
{
   return *elements;
}

void
ProcessCmdResponseX::startElement (const char *name, const char **attrs)
{
   if (xmlTagName == name) {
      // The "return" root element; ensure that complex subtype models are clear.
      resultsTables.clear();
      resultsLines.clear();
   } else if (Table::xmlTagName == name) {
      // A "table" element
      tableBeingParsed = new TableX(*elements);
      elements->SubstituteTop(tableBeingParsed);
   } else if (Line::xmlTagName == name) {
      // A "line" element
      lineBeingParsed = new LineX();
      elements->SubstituteTop(lineBeingParsed);
   } else {
      // No need to deal explicitly with the type_tag; the inherited StringParser behavior, together
      // with the endElement override, will deal with that.
      StringParser::startElement (name, attrs);
   }
}

void
ProcessCmdResponseX::endElement (const char *name)
{
   if (type_tag == name) {
      type = content;
   } else if (Table::xmlTagName == name) {
      // A "table" element
      resultsTables.push_back(*tableBeingParsed);
      delete tableBeingParsed;
      tableBeingParsed = NULL;
   } else if (Line::xmlTagName == name) {
      // A "line" element
      resultsLines.push_back(*lineBeingParsed);
      delete lineBeingParsed;
      lineBeingParsed = NULL;
   }
}


