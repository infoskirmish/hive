/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/ClientLib/XMLParserStack.h$
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

/*-----------------  XMLParserStack - FILE DESCRIPTION  -----------------*

Declaration of the XMLParserStack class.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#if !defined(XMLPARSERSTACK_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_)
#define XMLPARSERSTACK_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_

/*-----------------  XMLParserStack - INCLUDES  -------------------------*/

#include "XMLParser.h"
#include <string>
#include <stack>
#if (defined(_DEBUG) && defined(WIN32))
#include <afx.h>
#endif

/*-----------------  XMLParserStack - MISCELLANEOUS  --------------------*/


/*-----------------  XMLParserStack - DECLARATIONS  ---------------------*/


/**
Description:
XMLParserStack extends the functionality of XMLParser by adding hierarchical
management to complex objects that use XMLParsers for deserialization.
While XMLParser (the Expat wrapper that was pretty much stolen from EasySOAP)
is all well and good, it still leaves something to be desired from an object-oriented
standpoint.  Namely, one XMLParser object must still be totally knowledgeable
of the entire document's structure, and the parsing of XML complex types cannot
be wholly delegated to suitable corresponding model classes as one might like.
XMLParserStack relieves this limitation by providing a mechanism to substitute
appropriate parsers as XML elements are encountered, and then restoring the
previous parser that was in effect for the enclosing element.  In this way,
horrendously complex XML documents can be modeled while continuing to contain
structural details in bite-sized C++ classes that represent the complex types
therein; this also restores a sense of symmetry between the deserialization
operation and the strict hierarchical behavior of the serialization operation
through an XMLComposer object.

In addition to the OO encapsulation and modeling capabilities fostered by
XMLParserStack, the class also verifies than an XML structure is well-formed
without additional effort by the application.

How It Works:

A top-level class that models a complex XML document -- let's call it Bob -- would
probably implement the IXMLStreamable interface, and have as a member an
XMLParserStack.  A Bob instance would respond with that member in its GetXMLParser
method, and would also have registered itself as the "root" class with this parser,
providing as it did so the tag name of the XML element that it models.  This
element need not be the root element in the strict XML definition; XMLParserStack
will work on any fragment, so long as only one instance of the element registered
with XMLParserStack as the "root" occurs in the stream.

When a Bob object's XMLParserStack sees the root name in an element start tag,
it will push the Bob instance onto the stack and invoke its startElement
method.  The stack will then route all subsequent startElement and characterData
calls that it receives to the parser that appears on the top of the stack.  As it
receives startElement activations, the XMLParserStack will push a duplicate of the
pointer of the top-of-stack, paired with the element tag name, onto the stack, a
mechanism that it uses to check the start/end tag matches.  An XMLParserStack-aware
object has the option, when it receives a startElement call, of substituting a
different XMLParser for its own top-of-stack entry (remember, by definition that object
still has a pointer in the next entry down in the stack).  So, when a Bob object's
startElement override is called, and it recognizes a start tag for a complicated
sub-element for which there's another XMLParser model class (call it Frank), Bob will
instantiate a Frank object and substitute its pointer for the top-of-stack; the Frank
will now receive the parsing function calls.

On the way out, the XMLParserStack's operating rules retain all the information
needed to call the endElement method of the correct object.  When the stack
receives an endElement call, it checks the tag name against the start tag currently
on the top-of-stack; they should always match for well-formed XML.  Then, it
unceremoniously pops the top-of-stack, discarding it, and calls the endElement method
of the new top-of-stack (which, by definition, points to the object that substituted
the one just discarded, if it was different.)  Note that this convention means that
a parser does not receive startElement and endElement calls for its own tag unless
its containing object decides that it should.

TODO: Add an example.
*/
class XMLParserStack : public XMLParser
#if (defined(_DEBUG) && defined(WIN32))
      // Needed to help with a knotty debugging issue.
      , public CObject
#endif
{
public:
   // This is the type of a stack object, which relates a parser object to an element tag name.
   typedef std::pair<XMLParser*, std::string> MemberType;

   /**
   The constructor for an XMLParserStack requires the root parser object and the
   element tag that begins its XML representation to be specified.
   */
   XMLParserStack (
         XMLParser*  rootHandler,      // Initializer for the watchRootHandler member.
         const std::string&  rootTag   // Initializer for the watchRootTag member.
         ) : watchRootTag(rootTag), watchRootHandler(rootHandler)
   { }

   virtual
   ~XMLParserStack(void);

   /**
      A parser will use the SubstituteTop method to replace the topmost pair
      on the stack with a different parser.  This is usually in response to
      a startElement invocation, where a parsing object sees an element
      tag for a complex subtype that has its own parser.

   Usage Notes:
      SubstituteTop does not automatically call the newTop object's startElement
      override.  If this is necessary, for things such as allowing a parser
      to pick up its own custom attributes in the element start tag, the
      object performing the substitution must make this call explicitly
      after the SubstituteTop call.
   */
   void
   SubstituteTop (
      XMLParser*  newTop
      )
   {
      MemberType nt(newTop, nestedTargets.top().second);
      nestedTargets.pop();
      nestedTargets.push(nt);
   }

   /**
      Reports the current depth of parsing in the XML document.
   */
   size_t
   size() const
   { return nestedTargets.size(); }

#if (defined(_DEBUG) && defined(WIN32))
   /**
      Since XMLParserStack will often contain pointers to itself, the Visual Studio
      debugger has an issue with trying to display an infinitely recursive
      description in the variable display windows.  This Dump override provides
      a much more succinct description of the object that avoids this problem.
   */
   virtual void Dump(
      CDumpContext& dc 
   ) const;
#endif


protected:
   std::stack<MemberType> nestedTargets;  // The element stack containing the element tags and the objects that parse them.

   std::string watchRootTag;     // Element tag that will cause the watchRootHandler to be given control of parsing.

   XMLParser*  watchRootHandler; // Pointer to the object that interprets the logical root element of the XML document
                                 // or fragment that we're interested in.

   /**
      startElement fires during ParseBuffer processing when an element
      start tag is encountered.  It will push a new pair onto the stack
      to designate the current parser in effect according to the following
      rules:
      - If the stack is empty, and the name argument is not the root tag
      that was designated at the time of the stack's creation, a new member
      with this object as the parser will be pushed onto the stack.  This
      allows a root tag to be designated which is not actually the root
      of the XML document to be used, while maintaining tag match checking.
      - If the name string is the root tag that was registered at the time
      of creation, the corresponding root handler will be pushed onto the
      stack, and its startElement method invoked with the same arguments.
      (Note that this is the only instance where the new top-of-stack member's
      startElement method is called implicitly, a privilege reserved for the root handler.)
      - Otherwise, a new member is pushed onto the stack with the new element
      tag and a parser pointing to the same object as the old top of the stack;
      that object's startElement method is then invoked with the same arguments.
   */
	virtual void
   startElement (
      const char *name,    // The element tag
      const char **attrs   // If non-null, name/value pairs for the attributes which appeared in the start tag.
      );

   /**
      This method fires during ParseBuffer processing when an element
      end tag is encountered.  It will pop the topmost member off of the
      stack, verifying the tag name as it does so; then, if the stack
      is not empty, it will invoke the endElement method of the new
      topmost member parser.

      Exceptions:
      Will throw a runtime_error if there's a stack underflow (more end tags
      than start tags in the XML document), or if the name argument does
      not match the tag name given in the startElement call.
   */
	virtual void endElement (
      const char *name     // The element tag.
      );

	/**
      This method fires during ParseBuffer processing for each token
      that appears between element start and end tags.  It will redirect the call to the
      XMLParser object pointed to by the topmost member of the stack.
   */
   virtual void
   characterData (
      const char *str,  // A token of element content, not nul terminated.
      int len           // The valid length of str.
      );

};

#endif // !defined(XMLPARSERSTACK_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_)
