/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/IXMLStreamable.h$
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

/*-----------------  IXMLStreamable - FILE DESCRIPTION  -----------------*

Declaration of the IXMLStreamable class.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#if !defined(IXMLSTREAMABLE_H__62DCBB27_A5EF_4549_BD8A_F6B89AE2FC76__INCLUDED_)
#define IXMLSTREAMABLE_H__62DCBB27_A5EF_4549_BD8A_F6B89AE2FC76__INCLUDED_

/*-----------------  IXMLStreamable - INCLUDES  -------------------------*/

#include "XMLComposer.h"
//#include "XMLParser.h"

/*-----------------  IXMLStreamable - MISCELLANEOUS  --------------------*/


/*-----------------  IXMLStreamable - DECLARATIONS  ---------------------*/

/**
Description:
IXMLStreamable provides the basis for container-oriented objects to use SOAP-like
capabilities for modeling data that is transferred in XML documents.  It's an
interface that defines two capabilities: for the inheriting class to serialize
itself into an XML text stream composer (the XMLCompose method), and to
provide access to a parser which understands the inheriting class and can
populate an instance by deserializing an XML text stream the GetXMLParser method.)

Implementation Notes:
Inheriting classes will normally have default constructors available
*/
class IXMLStreamable
{
public:

   virtual ~IXMLStreamable()
   {}

   /**
   This method causes the IXMLStreamable object to serialize itself into the
   specified XML text stream composer.

   Returns:
   The dest argument.
   */
   virtual XMLComposer&
   XMLCompose (
      XMLComposer&   dest
      ) const = 0;

   /**
   Returns a reference to a parser which can parse an XML text stream and
   populate an object based on that stream's contents.

   Usage Notes:
   The referenced returned may be an instance of the inheriting class itself,
   more specfically the object for which the method override is invoked and
   into which the XML text will be deserialized.  Having a class that acts
   as its own parser is convenient with respect to being able to access
   protected members and the like.  Thus, if an IXMLStreamable-inheriting
   class uses a "helper" class as a parser, it is likely that this helper class
   will have to be declared as a friend class of the class which it serves.
   */
   //virtual XMLParser&
   //GetXMLParser () = 0;
};

#endif // !defined(IXMLSTREAMABLE_H__62DCBB27_A5EF_4549_BD8A_F6B89AE2FC76__INCLUDED_)
