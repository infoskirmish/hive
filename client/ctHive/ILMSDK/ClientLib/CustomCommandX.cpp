/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/CustomCommandX.cpp$
$Revision: 1$
$Date: Tuesday, August 25, 2009 2:42:11 PM$
$Author: timm$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
**
**	               (C)  Copyright Northrop Grumman ES/
**                          XETRON Corporation
**                         All rights reserved
*/

/*-----------------  CustomCommandX - FILE DESCRIPTION  -----------------*

Implementation of the classes that model the response from the ILM interface's
AddCommands function.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  CustomCommandX - INCLUDES ----------------------*/

#include "CustomCommandX.h"
#include "XMLParserStack.h"

/*-----------------  CustomCommandX - DECLARATIONS -------------------*/

using namespace InterfaceLibrary;
using std::string;

/*==========================================================================*/


void
CustomCommandParser::AttributeParser::endElement(const char *tag)
{
   if (CustomCommand::Attribute::size_tag == tag)
      target.size = (size_t)atoi(content.c_str());
   else if (CustomCommand::Attribute::type_tag == tag)
      target.type = content;
}

void
CustomCommandParser::ParameterParser::startElement (const char *name, const char **attrs)
{
   if (CustomCommand::Parameter::xmlTagName==name) {
      target.flag = FindAttribute(attrs, target.attrib_flag);
   } else if (CustomCommand::Attribute::xmlTagName==name) {
      target.attribute.size = 0;
      target.attribute.type = "";
      myStack.SubstituteTop(&attribParser);
   }
}

void
CustomCommandParser::ParameterParser::endElement(const char *name)
{
   if (CustomCommand::Parameter::tag_helpText==name) target.helpText = content;
}

CustomCommandParser::~CustomCommandParser()
{
   delete attribParser;
   delete paramParser;
}

void CustomCommandParser::startElement(const char* name, const char** attrs)
{
   if (CustomCommand::xmlTagName==name) {
      // Re-initialize the object just in case it's being re-used.
      target.title = target.cmdGroup = target.usage = target.helpText = "";
      target.referenceID = 0;
      ResetParsers();
      target.attributes.clear();
      target.parameters.clear();
      // See if the element start tag included the "title" attribute
      target.title = FindAttribute(attrs, target.attrib_title);
   } else if (CustomCommand::Attribute::xmlTagName==name) {
      target.attributes.push_back(CustomCommand::Attribute());
      ResetParsers();
      attribParser = new AttributeParser(target.attributes.back());
      parserStack.SubstituteTop(attribParser);
   } else if (CustomCommand::Parameter::xmlTagName==name) {
      target.parameters.push_back(CustomCommand::Parameter());
      ResetParsers();
      paramParser = new ParameterParser(parserStack, target.parameters.back());
      parserStack.SubstituteTop(paramParser);
   }
}

void CustomCommandParser::endElement(const char* name)
{
   if (CustomCommand::tag_referenceID==name) {
      target.referenceID = (uint32)atol( content.c_str() );
   } else if (CustomCommand::tag_cmdGrp==name) {
      target.cmdGroup = content;
   } else if (CustomCommand::tag_usage==name) {
      target.usage = content;
   } else if (CustomCommand::tag_helpTxt==name) {
      target.helpText = content;
   }
}

CustomCommandSetX::CustomCommandSetX()
{
   myStack = new XMLParserStack(this, xmlTagName.c_str());
   cmdParser = NULL;
}

CustomCommandSetX::~CustomCommandSetX()
{
   for (iterator i = begin(); i!=end(); i++) {
      delete *i;
   }
   delete cmdParser;
   delete myStack;
}

XMLComposer&
CustomCommandSetX::XMLCompose (
   XMLComposer&   dest
   ) const
{
   return CustomCommandSet::XMLCompose(dest);
}

XMLParser&
CustomCommandSetX::GetXMLParser()
{
   return *myStack;
}

class DummyCommand : public CustomCommand
{
public:
   virtual ProcessCmdResponse Process(String& attributes, binary& params)
   {
      return ProcessCmdResponse();
   }
};

void
CustomCommandSetX::startElement (const char *name, const char **attrs)
{
   if (xmlTagName==name) {
      clear();
   } else if (CustomCommand::xmlTagName==name) {
      vector<CustomCommand*>::push_back(new DummyCommand());
      delete cmdParser;
      cmdParser = new CustomCommandParser(*myStack, *back());
      myStack->SubstituteTop(cmdParser);
      // CustomCommandX requires that its startElement method be called so that it can catch
      // any attributes that may be in the start tag.
      cmdParser->startElement(name, attrs);
   }
}


