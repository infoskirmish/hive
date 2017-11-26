/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/XMLComposer.h$
$Revision: 1$
$Date: Friday, October 09, 2009 5:04:08 PM$
$Author: timm$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  XMLComposer - FILE DESCRIPTION  -----------------*

Declaration of the XMLComposer class.  This module was shamelessly plagiarized
from the open-source EasySoap project, thought it has been augmented with
some functionality that will make it easier to use as a stand-alone XML
generator, and some of the higher-level namespace management capabilities
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
 * $Id: XMLComposer.h,v 1.3 2006/11/09 07:24:38 dcrowley Exp $
 */
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#if !defined(XMLCOMPOSER_H__DB61E902_B4A0_4AA3_A0F7_479D6295BD06__INCLUDED_)
#define XMLCOMPOSER_H__DB61E902_B4A0_4AA3_A0F7_479D6295BD06__INCLUDED_

/*-----------------  XMLComposer - INCLUDES  -------------------------*/

#include <stack>
#include <stdexcept>
#include <exception>
#include <sstream>

/*-----------------  XMLComposer - MISCELLANEOUS  --------------------*/

/*-----------------  XMLComposer - DECLARATIONS  ---------------------*/


/**
An instance of the XMLComposer class can be used as the basis for serializing objects as
XML streams.  Creating XML is not terribly difficult; far less so in fact than
parsing it.  However, to do so in a manner that follows good Object-Oriented
design principals, where the knowledge of how to serialize an object lies in the
object's class or a designated proxy, one can readily benefit from some
assistance in maintaining the syntax of XML and the state of the XML stream
heirarchy.  It is exactly this gap which XMLComposer is intended to fill.

<h3>Example</h3>

Let's assume two classes, one of which contains a collection of the other, and
both of which have a method that makes good use of an XMLComposer:
<pre>

class A {
private:
   int intMember;
   std::string strMember;
public:
   A(int i, const char* s) : intMember(i), strMember(s) {} // Initializing constructor

   XMLComposer& XMLCompose(XMLComposer& c) {
      c.WriteSimpleElement("intMember", intMember);
      c.WriteSimpleElement("strMember", strMember.c_str());
      return c;
   }
   ...
}

class B {
public:
   double dblMember;
   std::vector&lt;A&gt;   objects;
   XMLComposer& XMLCompose(XMLComposer& c) {
      c.StartTag("Class_B");
      c.WriteSimpleElement("dblMember", dblMember);
      for (int i=0; i<object.length(); i++) {
         c.StartTag("Class_A");
         objects[i].XMLCompose(c);
         c.EndTag();
      }
      c.EndTag();
      return c;
   }
   ...
}

</pre>
Since the data members of class A are private, we really can't do without
something like the XMLCompose method if an object of that type is going to be
serialized as XML, or as anything else for that matter.  The code fragment
<pre>

B  bObj;
bObj.dblMember = -3.14159;
bObj.objects[0] = A(6, "blah blah blah");
bObj.objects[1] = A(3, "supercalifragilisticexpialidocious");

XMLComposer c;
c.beautify = true;
c.Reset(true);
c.StartTag("rootElement");
c.AddAttr("minorTweak", "true");
bObj.XMLCompose(c);
c.StartTag("part2");
c.AddAttr("name", "NakedAObject");
A(83, "Wikipedia is cool.").XMLCompose(c);
c.EndTag();
c.EndTag();
clog << c.GetBytes().c_str();

</pre>
produces the following output in the standard log:
\verbatim

<?xml version="1.0" encoding="UTF-8" ?>
<rootElement minorTweak=true>
  <Class_B>
    <dblMember>-3.14159</dblMember>
    <Class_A>
      <intMember>6</intMember>
      <strMember>blah blah blah</strMember>
    </Class_A>
    <Class_A>
      <intMember>3</intMember>
      <strMember>supercalifragilisticexpialidocious</strMember>
    </Class_A>
  </Class_B>
  <part2 name=NakedAObject>
    <intMember>83</intMember>
    <strMember>Wikipedia is cool.</strMember>
  </part2>
</rootElement>

\endverbatim
Note that regardless of the nesting level of an A object, its rendering always
obeys correct XML syntax.  Also see that for data types that can be inserted
into an ostream, such as B's dblMember, XMLComposer's inheritance from
stringstream supplies all that's needed to format those types.

One thing you may notice if you look at the example in detail is that A's
XMLCompose method was not responsible for writing the object's own XML entity
tag.  This is because in actual practice, especially in the case of a container
object like B, the tag may need to be varied in order to assure the ability to
uniquely identify the run-time type.  Also, when XML is being parsed, a class
like A will probably not be given control until <i>after</i> its element start
tag has been detected; thus, to preserve the symmetry of the operations,
composition of the start tag has also been "bumped up" to the containing
functions.


<h3>Implementation Notes</h3>

EasySoap's original XMLComposer didn't inherit from anything, and did its own
manual buffer management and all of that.  To provide a bit of extra functionality
for free, and to simplify the code, this author elected instead to base the
class on a stringstream, providing an insertion operator as well as all sorts of
handy buffer access functions.  It also seemed superfluous (and even a little
error-prone) to have the EndTag method require the tag name as a parameter; the
class should be smart enough to remember this information, and now it is.

*/
class XMLComposer : protected std::stringstream
{
public:
   enum {
      defaultIndent = 2   // The default value for the indent member.
   };

   /**
      Constructor for an XMLComposer.  Note that by default, an XMLComposer is
      <i>not</i> set to beautify its composed stream.
   */
   XMLComposer()
   {
      beautify = inStart = false;
      indent = defaultIndent;
      gensym = 0;
   }

	/**
      While an XMLComposer's destructor doesn't explicitly do anything, its
      'virtual' modifier allows for full polymorphic behavior of inherited
      classes which want to take advantage of the stringstream's formatting
      capabilities.
   */
   virtual ~XMLComposer();

	/**
      Resets the members of XMLComposer and clears the composed string.  Alternatively,
      it can also add a default processing instruction declaration.  (Note: the
      beautify member is not altered by Reset.)
      
      <param name="addDecl">If true, an xml processing instruction declaration
      will be used to initialize the composed stream.    </param>
   */
   void
   Reset (bool addDecl);

   /**
      Returns the composed XML string.  For a C-style const char* result, use
      c_str() on the returned object.
   */
	std::string
   GetBytes() const
   { return this->rdbuf()->str(); }

   /**
      Returns the current length of the composed XML string.
   */
	unsigned int
   GetLength() const
   { return (unsigned int)(this->rdbuf()->str().length()); }

   /**
      Inserts the start tag of a processing instruction into the XML stream.
      
      <param name="name">The processing instruction tag.</param>
   */
	void StartPI(const char *name)
   {
	   EndStart();
	   *this << "<?" << name;
	   inStart = true;
   }

   /**
      Inserts the end tag of a processing instruction into the XML stream.
   */
   void EndPI()
   {
	   *this << "?>";
	   if (beautify) *this << "\n";
	   inStart = false;
   }

   /**
      Inserts the start tag of an element into the XML stream.

      <param name="tag">The element name tag.</param>
   */
	void StartTag(const char *tag);

   /**
      Add an attribute to an element tag.  A call to this method must immediately
      follow a call to StartTag, StartPI, or another AddAttr call.

      \exception
      Throws runtime_error if the XML stream's insertion point is not currently
      inside of an element start tag.
   */
	void AddAttr (
      const char *attr,    // The name of the XML attribute
      const char *value    // The text representing the attribute
      ) throw (std::runtime_error);

   /**
      Inserts the namespace declaration into the XML stream.
   */
	void AddXMLNS(const char *prefix, const char *ns);

   /**
      Inserts the end tag of an element into the XML stream.

      \exception
      Throws runtime_error if no start tag exists to close out (i.e., the elementTags
      stack member was underrun.)
   */
   void
   EndTag() throw (std::runtime_error);

   /**
      Writes the text in the content of an element.  Must be called in between
      StartTag and EndTag calls for the element.
   */
	void WriteValue(const char *val)
   {
	   if (inStart) {
		   *this << '>';
		   inStart = false;
	   }
	   WriteEscaped(val);
   }

   /**
      Overload for writing a standard string as the content of an element.  Must
      be called in between StartTag and EndTag calls for the element.
   */
	void WriteValue(const std::string& val)
   {
	   WriteValue(val.c_str());
   }

   /**
      Overload for writing an integer as the content of an element.  Must
      be called in between StartTag and EndTag calls for the element.
   */
	void WriteValue(long val)
   {
      if (inStart) {
         *this << '>';
         inStart = false;
      }
      *this << val;
   }

   /**
      Overload for writing a double-precision number as the content of an
      element. Must be called in between StartTag and EndTag calls for the
      element.
   */
	void WriteValue(double val)
   {
      if (inStart) {
         *this << '>';
         inStart = false;
      }
      *this << val;
   }

   /**
      A shorthand method for a common operation, that of writing an element with simple
      string content between its begin and end tags.
   */
   void WriteSimpleElement(const char* tagName, const char* content);

   /**
      Overload for writing an element with std::string content between its begin
      and end tags.
   */
   void WriteSimpleElement(const char* tagName, const std::string& content);

   /**
      Overload for writing an element with simple integer value content between
      its begin and end tags.
   */
   void WriteSimpleElement(const char* tagName, long content);

   /**
      Overload for writing an element with simple floating point value content
      between its begin and end tags.
   */
   void WriteSimpleElement(const char* tagName, double content);

   /// Indicates that whitespace should be used to make the stream more readable.  False by default.
	bool		beautify;

   /// When 'beautify' is true, elements are indented this number of spaces from their enclosing
   /// elements; value is defaultIndent by default.
   unsigned char indent;

protected:

   /// Contains the stack of nested element tags currently in effect (StartTag pushes, EndTag pops).
   std::stack<std::string> elementTags;

   /**
      Generates and returns a symbol unique to the current composition starting
      with the specified prefix and having a suffix of numerical digits.
   */
	std::string
   GetSymbol(const char *prefix);

   /**
      Terminates a start tag, and if the 'beautify' flag is set it then adds the apprporiate
      indent for the next line.
   */
	void EndStart();

   /**
      All character string content written to the XML stream goes through this
      function, which converts any special characters meaningful in XML syntax
      into their appropriate substitution strings before writing them to the
      stream.
      
      <param name="str"> The string content to be written to the stream.  As one
      might expect, a str argument that contains no special XML characters will
      get written to the stream unscathed. </param>
      
      \note  Some class methods write content to the stream without going
      through WriteEscaped.  This is legal, so long as their rendered content
      (such as that of an integer value) cannot by definition contain special
      XML characters.
   */
   virtual void
   WriteEscaped(const char *str);

	/** This flag indicates that an element's start tag has been written
       to the stream, but not yet terminated.   */
   bool        inStart;

   /** A counter that is incremented by the GetSymbol method, used to
       generate unique symbols.  */
	unsigned int	gensym;

};

/**
   This class extends XMLComposer to allow the insertion of preformatted XML
   text into the formatted stream.  It should only be employed in cases where a
   significant section of material exists that is known to conform to the target
   schema, and which cannot reasonably be parsed and then recomposed.
 */
class UnsafeXMLComposer : public XMLComposer
{
public:

   /**
      The WriteRaw method is what the XMLComposer base class moved Heaven and
      Earth to avoid, this being the injection of uncontrolled material into the
      formatted text stream's current insertion point.
      
      <param name="str"> The preformatted XML text to insert into the stream.
      No translation of special characters; the text is inserted exactly as is.
      The caller is responsible for any necessary translation, as well as for
      ensuring that the element heirarchy is consistent.  </param>
    */
   void WriteRaw(const char* str)
   {
      *this << str;
   }
};

#endif // !defined(XMLCOMPOSER_H__DB61E902_B4A0_4AA3_A0F7_479D6295BD06__INCLUDED_)

