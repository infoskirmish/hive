/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/XMLParserStack.cpp$
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

/*-----------------  XMLParserStack - FILE DESCRIPTION  -----------------*

Implementation of the XMLParserStack class.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  XMLParserStack - INCLUDES  -------------------------*/

#include "XMLParserStack.h"
#include <stdexcept>

/*-----------------  XMLParserStack - DECLARATIONS ----------------------*/

using std::string;
using std::runtime_error;
using std::pair;

/*==========================================================================*/

XMLParserStack::~XMLParserStack()
{}

void
XMLParserStack::startElement(const char *name, const char **attrs)
{
   if (watchRootTag==name) {
      nestedTargets.push(MemberType(watchRootHandler, watchRootTag));
      watchRootHandler->startElement(name, attrs);
   } else {
      if (nestedTargets.empty()) {
         nestedTargets.push(MemberType(this, string(name)));
      } else {
         XMLParser*  currentParser = nestedTargets.top().first;
         nestedTargets.push(MemberType(currentParser, string(name)));
         // If the watchRootTag is not the real root of the XML document,
         // and we are not within that element, then the topmost member
         // of the stack will be pointing to this object; if that's the
         // case, we don't want to invoke the currentParser's startElement
         // method or we'll create an infinitely recursive loop.
         if (currentParser!=this)
            currentParser->startElement(name, attrs);
      }
   }
}

void
XMLParserStack::endElement(const char *name)
{
   if (nestedTargets.empty()) {
      throw runtime_error(string("Stack underflow encountered in XMLParserStack"));
   }
   
   if (nestedTargets.top().second.compare(name) != 0) {
      string errMsg("Mismatch in element tags encountered in XMLParserStack (\"");
      errMsg += nestedTargets.top().second;
      errMsg += "\" and \"";
      errMsg += name;
      errMsg += "\")";
      throw runtime_error(errMsg);
   }

   nestedTargets.pop();
   if (!nestedTargets.empty()) {
      // As with the startElement case, we need to check to make sure that
      // this object is not the topmost parser when calling endElement to
      // prevent infinite recursion.
      if (nestedTargets.top().first != this)
         nestedTargets.top().first->endElement(name);
   }
}

void
XMLParserStack::characterData(const char *dat, int len)
{
   if (!nestedTargets.empty())
      if (nestedTargets.top().first != this)
         nestedTargets.top().first->characterData(dat, len);
}

#if (defined(_DEBUG) && defined(WIN32))
#include <sstream>

void
XMLParserStack::Dump( CDumpContext& dc ) const
{
   std::stringstream s;

   s << "XMLParserStack, depth=" << size();
   if (size()>0)
      s << ", current tag=\"" << nestedTargets.top().second << '"';
   s << ", watchRootTag=\"" << watchRootTag << '"';

}
#endif
