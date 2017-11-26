/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/source/CustomCommand.cpp$
$Revision: 1$
$Date: Thursday, October 15, 2009 2:12:21 PM$
$Author: timm$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  CustomCommand - FILE DESCRIPTION  -----------------*

Implementation of the classes that model the response from the ILM interface's
AddCommands function.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  CustomCommand - INCLUDES  -------------------------*/

#include <stdexcept>
#include <sstream>
#include "CustomCommand.h"
#include "ProcessCmdResponse.h"

/*-----------------  CustomCommand - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;
using std::string;

/*==========================================================================*/

int currentAttrib = 0;

const string CustomCommand::Attribute::type_tag("type");    // The XML tag for the type member in XML-serialized form.
const string CustomCommand::Attribute::size_tag("size");    // The XML tag for the size member in XML-serialized form.
const string CustomCommand::Attribute::defaultv_tag("defaultvalue");    // The XML tag for the defaultvalue member in XML-serialized form.
const string CustomCommand::Attribute::ncseID_tag("ncurseid");    // The XML tag for the defaultvalue member in XML-serialized form.
const string CustomCommand::Attribute::ncseLbl_tag("ncurselabel");    // The XML tag for the defaultvalue member in XML-serialized form.
const string CustomCommand::Attribute::tag_helpText("helptext");
const string CustomCommand::Attribute::tag_typeDesc("typedesc");
const string CustomCommand::Attribute::xmlTagName("attribute");  // The XML tag that defines a CustomCommand::Attribute element in an XML stream.
const string CustomCommand::cmdxmlTagName("command_attribute");  // The XML tag that defines a CustomCommand::Attribute element in an XML stream.

XMLComposer&
CustomCommand::Attribute::XMLCompose(XMLComposer& s) const
{
   s.StartTag(xmlTagName.c_str());
   std::stringstream tempName;
   if ( ncurseID == "" ) {
       currentAttrib++;
       tempName << "Argument" << currentAttrib;
       s.WriteSimpleElement(ncseID_tag.c_str(), tempName.str());
       s.WriteSimpleElement(ncseLbl_tag.c_str(), "Custom Attribute");
   } else {
       s.WriteSimpleElement(ncseID_tag.c_str(), ncurseID);
       s.WriteSimpleElement(ncseLbl_tag.c_str(), ncurseLabel);
   }
   s.WriteSimpleElement(type_tag.c_str(), type);
   s.WriteSimpleElement(size_tag.c_str(), (long)size);
   s.WriteSimpleElement(tag_typeDesc.c_str(), typeDesc);
   if ( !defaultValue.empty() ) {
       s.WriteSimpleElement(defaultv_tag.c_str(), defaultValue);
   }
   if ( !helpText.empty() ) {
       s.WriteSimpleElement(tag_helpText.c_str(), helpText.c_str());
   }
   s.EndTag();
   return s;
}

const string   CustomCommand::Parameter::xmlTagName("parameter");
const char*    CustomCommand::Parameter::attrib_flag = "flag";

XMLComposer&
CustomCommand::Parameter::XMLCompose(XMLComposer& s) const
{
   s.StartTag(xmlTagName.c_str());
   s.AddAttr(attrib_flag, flag.c_str());
   attribute.XMLCompose(s);
   s.EndTag();
   return s;
}

const string   CustomCommand::Section::xmlTagName("section");

XMLComposer&
CustomCommand::Section::XMLCompose(XMLComposer& s) const
{
   s.StartTag(xmlTagName.c_str());
   if ( columns.size() > 0 ) {
       for (vector<Column>::const_iterator i = columns.begin(); i!=columns.end(); i++) {
           i->XMLCompose(s);
       }
   }
   s.EndTag();
   return s;
}

const string   CustomCommand::Column::xmlTagName("column");
const string   CustomCommand::Column::title_tag("title");
const string   CustomCommand::Column::element_tag("element");

XMLComposer&
CustomCommand::Column::XMLCompose(XMLComposer& s) const
{
   s.StartTag(xmlTagName.c_str());
   s.WriteSimpleElement(title_tag.c_str(), title);
   if ( elements.size() > 0 ) {
       for (vector<std::string>::const_iterator i = elements.begin(); i!=elements.end(); i++) {
           s.WriteSimpleElement(element_tag.c_str(), i->c_str());
       }
   }
   s.EndTag();
   return s;
}

void CustomCommand::Column::push_back (Attribute& attrib)
{
    push_back(attrib.ncurseID);
}

void CustomCommand::Column::push_back (std::string ncurseID)
{
    elements.push_back(ncurseID);
}

XMLComposer&
CustomCommand::XMLCompose(XMLComposer& s) const
{
   s.StartTag(xmlTagName.c_str());
   s.AddAttr(attrib_title, mTitle.c_str());
   s.WriteSimpleElement(tag_referenceID.c_str(), (long)referenceID);
   s.WriteSimpleElement(tag_helpTxt.c_str(), helpText);

   s.StartTag("attributes");
   if ( attributes.size() > 0 ) {
       s.StartTag("required_attributes");
       for (vector<Attribute>::const_iterator i = attributes.begin(); i!=attributes.end(); i++) {
           s.StartTag(cmdxmlTagName.c_str());
           i->XMLCompose(s);
           s.EndTag();
       }
       s.EndTag();
   }
   if ( optional_attribute.type != "" ) {
       s.StartTag("optional_attributes");
       s.StartTag(cmdxmlTagName.c_str());
       optional_attribute.XMLCompose(s);
       s.EndTag();
       s.EndTag();
   }
   s.EndTag(); // Attributes

   s.StartTag("parameters");
   if ( parameters.size() > 0 ) {
       s.StartTag("required_parameters");
       for (vector<Parameter>::const_iterator i = parameters.begin(); i!=parameters.end(); i++)
          i->XMLCompose(s);
       s.EndTag();
   }
   if ( optional_parameters.size() > 0 ) {
       s.StartTag("optional_parameters");
       for (vector<Parameter>::const_iterator i = optional_parameters.begin(); i!=optional_parameters.end(); i++)
          i->XMLCompose(s);
       s.EndTag();
   }
   s.EndTag(); // parameters

   s.StartTag("ncurseslayout");
   if ( sections.size() > 0 ) {
       for (vector<Section>::const_iterator i = sections.begin(); i!=sections.end(); i++) {
           i->XMLCompose(s);
       }
   }
   s.EndTag(); // ncurseslayout

   s.EndTag(); // xmlTagName
   return s;
}

const string CustomCommand::xmlTagName("cmd");
const char*  CustomCommand::attrib_title = "title";
const string CustomCommand::tag_referenceID("referenceid"); 
const string CustomCommand::tag_helpTxt("helptext");

const string CustomCommandGroup::xmlTagName("cmdgrp");
const char*  CustomCommandGroup::attrib_title = "title";

void CustomCommandGroup::push_back (CustomCommandEntity& cc) throw (std::logic_error)
{
   for (std::vector<CustomCommandEntity*>::iterator i=entityVector.begin(); i!=entityVector.end(); i++) {
       if (!cc.isGrp()) {
           CustomCommand* tempCmd = (CustomCommand*)&cc;
           if (((CustomCommand*)(*i))->referenceID==tempCmd->referenceID) {
             std::stringstream ss;
             ss << "Attempting to duplicate command reference ID " << tempCmd->referenceID << " in CustomCommandSet::push_back";
             throw std::logic_error(ss.str());
           }
       }
   }
   entityVector.push_back(&cc);
}

CustomCommand*
CustomCommandGroup::find(uint32 refID)
{
    CustomCommand* ret = NULL;

   for (std::vector<CustomCommandEntity*>::iterator i=entityVector.begin(); i!=entityVector.end(); i++) {
       if ((*i)->isGrp()) {
           ret = ((CustomCommandGroup*)(*i))->find(refID);
           if (ret) {
               return ret;
           }
       }
       if (((CustomCommand *)(*i))->referenceID==refID)
         return (CustomCommand *)(*i);
   }

   return NULL;
}

CustomCommandEntity& CustomCommandGroup::operator [] (size_type indx) throw (std::out_of_range)
{
   if (indx<size()) {
      return *entityVector.operator [] (indx);
   } else {
      std::stringstream ss;
      ss << "Index " << indx << " out of range in CustomCommandSet::operator[]";
      throw std::out_of_range(ss.str());
   }
}

XMLComposer&
CustomCommandGroup::XMLCompose(XMLComposer& s) const
{
   s.StartTag(xmlTagName.c_str());
   s.AddAttr(attrib_title, mTitle.c_str());
   for (std::vector<CustomCommandEntity*>::const_iterator i = entityVector.begin(); i!=entityVector.end(); i++) {
      (*i)->XMLCompose(s);
   }
   s.EndTag();
   return s;
}

const string CustomCommandSet::xmlTagName("custcmdset");

void CustomCommandSet::push_back (CustomCommandEntity& cc) throw (std::logic_error)
{
   for (iterator i=begin(); i!=end(); i++) {
       if (!cc.isGrp()) {
           CustomCommand* tempCmd = (CustomCommand*)&cc;
           if (((CustomCommand*)(*i))->referenceID==tempCmd->referenceID) {
             std::stringstream ss;
             ss << "Attempting to duplicate command reference ID " << tempCmd->referenceID << " in CustomCommandSet::push_back";
             throw std::logic_error(ss.str());
           }
       }
   }
   vector<CustomCommandEntity*>::push_back(&cc);
}

CustomCommand&
CustomCommandSet::find(uint32 refID) throw (std::logic_error)
{
    CustomCommand* ret = NULL;

   for (iterator i=begin(); i!=end(); i++) {
       if ((*i)->isGrp()) {
           ret = ((CustomCommandGroup*)(*i))->find(refID);
           if (ret) {
               return *ret;
           }
       }
       if (((CustomCommand *)(*i))->referenceID==refID)
         return *((CustomCommand *)(*i));
   }
   std::stringstream ss;
   ss << "CustomCommand with referenceID=" << refID << " not found in CustomCommandSet::find";
   throw std::logic_error(ss.str());
}

CustomCommandEntity& CustomCommandSet::operator [] (size_type indx) throw (std::out_of_range)
{
   if (indx<size()) {
      return *vector<CustomCommandEntity*>::operator [] (indx);
   } else {
      std::stringstream ss;
      ss << "Index " << indx << " out of range in CustomCommandSet::operator[]";
      throw std::out_of_range(ss.str());
   }
}

XMLComposer&
CustomCommandSet::XMLCompose(XMLComposer& s) const
{
   s.StartTag(xmlTagName.c_str());
   for (const_iterator i = begin(); i!=end(); i++) {
      (*i)->XMLCompose(s);
   }
   s.EndTag();
   return s;
}


