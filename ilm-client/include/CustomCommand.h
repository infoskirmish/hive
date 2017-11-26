/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/CustomCommand.h$
$Revision: 1$
$Date: Thursday, October 15, 2009 2:11:49 PM$
$Author: timm$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  CustomCommand - FILE DESCRIPTION  -----------------*

Iation of the classes that model the response from the ILM interface's
AddCommands function.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef	CustomCommand_28593408269237_H			// Only process if not already processed
#define	CustomCommand_28593408269237_H

/*-----------------  CustomCommand - INCLUDES  -------------------------*/

#include <stdexcept>
#include "ProcessCmdResponse.h"

/*-----------------  CustomCommand - MISCELLANEOUS  --------------------*/

using std::vector;

/*-----------------  CustomCommand - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary {

    /**
    * This class models a custom command entity.  A custom command
    * entity is the base class of either a custom command group
    * or a custom command.  This abstraction allows the custom
    * command set to contain groups and/or commands in a flexible
    * way.
   */
   class CustomCommandEntity
   {
   public:
       CustomCommandEntity(){}

        virtual ~CustomCommandEntity() {}
    
        bool isGrp() {return mIsGroup;}
    
        /// Sets title
        void setTitle (std::string title) { mTitle = title; }
        /// Gets the title
        std::string getTitle() { return mTitle; }

        /**
        *Causes this CustomCommandEntity to be serialized by the
        *specified XMLComposer.

         <returns> The s argument.  </returns>
      */
      virtual XMLComposer&
      XMLCompose(XMLComposer& s) const = 0;
        
    protected:
    
        bool mIsGroup;
        std::string mTitle;

   };


   /**
      This class models a custom command sequence as returned by the ILibraryInterface's
      AddCommands function, and also acts as the base class for an ILM's
      implementation of a custom command action. This class can render itself as
      an XML fragment complying with the schema described in JY008C615,
      CutThroat ICD, as the return value for ProcessCmd.
      
      An ILM implementation will normally use the CustomCommand class by
      deriving one class for each of the custom commands that it supports; then,
      one instance of each of these derived types is placed in the
      customCommands member of the LibraryModuleBase instance. Depending upon
      the specific ILM, this may be a one-time affair; alternatively, members of
      the customCommands set may need to be added or removed when new modules
      are loaded or unloaded on the target device.  Whether it's a static or
      dynamic affair depends entirely upon the characteristics of the ILM's
      target.
   */
   class CustomCommand : public CustomCommandEntity
   {
   public:

       /**
       *    Default constructor for a Custom command.
         */
       CustomCommand() { mIsGroup = false; }

      /**
         About as simple as you can get for a "complex" sub-type, an Attribute
         describes the type and size of a required argument for a
         CustomCommand.
      */
      class Attribute
      {
      public:
         /// The XML tag for the type member.
         static const std::string type_tag;
         /// The XML tag for the size member.
         static const std::string size_tag;
         /// The XML tag for the size member.
         static const std::string defaultv_tag;
         /// The "attribute" element tag that defines a CustomCommand::Attribute element in an XML stream.
         static const std::string xmlTagName;
         /// The XML tag for the defaultvalue member in XML-serialized form.
         static const std::string ncseID_tag;
         /// The XML tag for the defaultvalue member in XML-serialized form.
         static const std::string ncseLbl_tag;
         /// The "helptext" element tag
         static const std::string   tag_helpText;
         /// The "typeDesc" element tag
         static const std::string   tag_typeDesc;
         

         /**
            Default constructor for an Attribute.
         */
         Attribute()
         { }

         /**
            Initializing constructor for an Attribute.
         */
         Attribute (
            const char* initType,
            size_t      initSize,
            const char* initHelpTxt = "",
            const char* initDefault = "",
            const char* initNCurseID = "",
            const char* initNCurseLabel = "",
            const char* initTypeDesc = ""
            ) : type(initType), helpText(initHelpTxt),
                defaultValue(initDefault), ncurseID(initNCurseID),
                ncurseLabel(initNCurseLabel), typeDesc(initTypeDesc)
         { size = initSize; }

         /**
            Copy constructor for an Attribute.
         */
         Attribute (
            const Attribute& a
            ) : type(a.type), helpText(a.helpText),
                defaultValue(a.defaultValue), ncurseID(a.ncurseID),
                ncurseLabel(a.ncurseLabel), typeDesc(a.typeDesc)
         { size = a.size; }

         virtual
         ~Attribute() {}

         /**
            Causes this Attribute to be serialized by the specified XMLComposer.

            <returns> The s argument.  </returns>
         */
         virtual XMLComposer&
         XMLCompose(XMLComposer& s) const;

         /// The data type of the attribute.
         std::string type;

         /// The size in memory of the attribute.
         size_t      size;

         /// The help text associated with this attribute
         std::string helpText;

         /// The default value of the attribute
         std::string defaultValue;

         /// The ncurse Identifier of the attribute (must be unique among all attr
         /// and parameters of a command
         std::string ncurseID;

         /// The ncurse Label of the attribute
         std::string ncurseLabel;

         /// A short description of the attribute
         std::string typeDesc;
      };

      /**
         Describes the characteristics of a CustomCommand parameter, which is
         normally an optional command argument denoted by a single-character
         flag.
      */
      class Parameter
      {
      public:
         /// The "parameter" element tag
         static const std::string   xmlTagName;
         /// The "flag" attribute name
         static const char*         attrib_flag;

         /**
            Default constructor for a Parameter.
         */
         Parameter ()
         {}

         /**
            Initializing constructor for flag and (optionally) helpText members.
         */
         Parameter (
            const char* initFlag
            ) : flag(initFlag)
         {}

         /**
            Initializing constructor for all members.
         */
         Parameter (
            const char* initFlag,
            const Attribute&  initAttrib
            ) : flag(initFlag), attribute(initAttrib)
         { }

         /**
            Copy constructor for a Parameter.
         */
         Parameter (
            const Parameter&  p
            ) : flag(p.flag), attribute(p.attribute)
         {}

         /**
            Destructor for a Parameter.
         */
         virtual
         ~Parameter()
         {}

         /**
            Causes this Parameter to be serialized by the specified XMLComposer.

            <returns> The s argument.  </returns>
         */
         virtual XMLComposer&
         XMLCompose(XMLComposer& s) const;

         /// The flag for this parameter
         std::string flag;
         /// The Parameter also carries all of the characteristics of an attribute.
         Attribute   attribute;
      };

      class Column {

      public:

         /// The "column" element tag
         static const std::string   xmlTagName;
         /// The "flag" attribute name
         static const char*         attrib_flag;
         /// The XML tag for the title member.
         static const std::string title_tag;
         /// The XML tag for the element member.
         static const std::string element_tag;

         /**
         *  Default constructor for a Column.
         */
         Column ()
         {}

         /**
         *  Initializing constructor for title.
         */
         Column (
            const char* initTitle
            ) : title(initTitle)
         {}

         /**
            Copy constructor for a Column.
         */
         Column (
            const Column&  p
            ) : title(p.title), elements(p.elements)
         {}

         /**
            Destructor for a Section.
         */
         virtual
         ~Column()
         {}

         /*
         Method for adding an element to the column

         This one takes an attribute as a parameter
         */
         void push_back (Attribute& attrib);

         /*
         Method for adding an element to the column

         This one takes a string as a parameter
         
         */
         void push_back (std::string ncurseID);

         /**
            Causes this Column to be serialized by the specified XMLComposer.

            <returns> The s argument.  </returns>
         */
         virtual XMLComposer&
         XMLCompose(XMLComposer& s) const;

         std::string title;
         std::vector<std::string> elements;
      };

      class Section {

      public:

          /// The "section" element tag
         static const std::string   xmlTagName;
         /// The "flag" attribute name
         static const char*         attrib_flag;

         /**
         *  Default constructor for a Section.
         */
         Section ()
         {}

         /**
            Copy constructor for a Section.
         */
         Section (
            const Section&  p
            ) : columns(p.columns)
         {}

         /**
            Destructor for a Section.
         */
         virtual
         ~Section()
         {}

         /**
            Causes this Section to be serialized by the specified XMLComposer.

            <returns> The s argument.  </returns>
         */
         virtual XMLComposer&
         XMLCompose(XMLComposer& s) const;

         /// The columns in the section
         std::vector<Column> columns;
      };

      // The "cmd" element tag
      static const std::string xmlTagName;
      // The "title" attribute name
      static const char*       attrib_title;
      // The "referenceid" tag
      static const std::string tag_referenceID;
      // The "helptxt" tag
      static const std::string tag_helpTxt;
      /// The "command_attribute" element tag that defines a CustomCommand::Attribute element in an XML stream.
      static const std::string cmdxmlTagName;

      /**
         The destructor for a CustomCommand takes no special action, but is
         provided for extensibility.
       */
      virtual
      ~CustomCommand() {}

      /**
         IXMLStreamable override that causes this CustomCommand to be serialized
         to the specified XMLComposer.

         <returns> The s argument.  </returns>
      */
      XMLComposer&
      XMLCompose(XMLComposer& s) const;

      ///   Read accessor for the CustomCommand's attribute list.
      const vector<Attribute>& GetAttributes() const {return attributes;}

      ///   Read accessor for the CustomCommand's parameter list.
      const vector<Parameter>& GetParameters() const {return parameters;}

      /**
         This method carries out the functionality of the CustomCommand derived
         type.
         
      *  <param name="arguments">  The required arguments for the
      *  command.
         </param>
      * 
         <returns> The results information generated by the command's execution.
         </returns>
       */
      virtual ProcessCmdResponse Process (binary& arguments) = 0;

      /// The unique identifier assigned to this command
      uint32 referenceID;
      /// The help text associated with this command
      std::string helpText;
      /// The list of required attributes used in this command
      vector<Attribute> attributes;
      /// The list of required parameters used in this command
      vector<Parameter> parameters;
      /// The list of sections in the ncurses menu for this command
      vector<Section> sections;
      /// The optional attribute for this command 
      /// (Not a vector because you can only support one)
      Attribute optional_attribute;
      /// The list of optional parameters used in this command
      /// NOTE: All optional parameters MUST have a defaultValue
      vector<Parameter> optional_parameters;
   };

   /**
   *  Container class for CustomCommandEntity objects, while it is
   *  itself a CustomCommandEntity object. The CustomCommandGroup
   *  supplies organized XML serialization capabilities to
   *  effortlessly model a set of custom commands and groups.
      
   *  While it would be nice to simply make CustomCommandGroup a
   *  vector of CustomCommandEntitys, rather than
   *  CustomCommandEntity pointers, this is unfortunately useless
   *  with the abstract class.  The virtual Process method is lost
   *  when the container class attempts to copy objects.  As the
   *  lesser of all possible evils, CustomCommandGroup simply hides
   *  the actual pointer nature of the contained objects and deals
   *  with references at its interface points.
      
      The upshot of this is that an object added to the set must have a lifespan
   *  equal to that of the CustomCommandGroup object itself, or at
   *  least must exist as long as it appears in that container.
   *  Failure to do so will result in a lot of mystifying
   *  segmentation faults.
   */
   class CustomCommandGroup : public CustomCommandEntity
   {
   public:

       /// The default constructor for CustomCommandGroup
       CustomCommandGroup() {parent = NULL; mIsGroup = true;}

      /// The "cmdgrp" element tag
      static const std::string xmlTagName;
      // The "title" attribute name
      static const char*       attrib_title;

      /// Making the size_type of the inherited class public.
      typedef vector<CustomCommandEntity*>::size_type size_type;

      /**
      *  The destructor for a CustomCommandGroup takes no special
      *  action, but is provided for extensibility.
      */
      virtual
      ~CustomCommandGroup() {}

      /**
         The method for adding a CustomCommand to the container.
         
         <param name="cc"> The object to add.  The object MUST have a lifespan
         at least as long as its inclusion in this container instance.
         </param>
         
         \exception
         Throws a logic_error if there's already a member in the set with the
         same referenceID as cc.
       */
      void push_back (CustomCommandEntity& cc) throw (std::logic_error);

      /// Reports the number of CustomCommandEntity objects currently referred to by the container.
      size_type size() const
      {
         return entityVector.size();
      }

      /**
         This method will seek the member of the set with the specified
         referenceID.
         
      *  <returns> A pointer to the specified CustomCommand.  Null if
      *  not found. </returns>
       */
      CustomCommand*
      find(uint32 refID);

      /**
         The index operator accesses the vector by zero-based index position.

      *  <returns> A pointer to the CustomCommandEntity at the
      *  specified location in the set, or NULL if the location
      *  exceeds the size of the set. </returns>

         \exception
         Throws out_of_range if the indx argument is not within the size of the
         vector.
       */
      CustomCommandEntity& operator [] (size_type indx) throw (std::out_of_range);

      /**
         This overload removes the specified CustomCommand from the set.  If the
         object is not a member of the set, the method has no effect.
       */
      void Remove (CustomCommand& cc)
      {
         for (std::vector<CustomCommandEntity*>::iterator i=entityVector.begin(); i!=entityVector.end(); i++) {
             if ((*i)->isGrp()) {
                 Remove(cc);
             } else if (*i==&cc) {
               entityVector.erase(i);
               break;
            }
         }
      }

      /**
         This overload removes the CustomCommand with the specified reference ID
         from the set.  If there's no object in the set with that reference ID,
         the method has no effect.
       */
      void Remove (uint32 refID)
      {
         for (std::vector<CustomCommandEntity*>::iterator i=entityVector.begin(); i!=entityVector.end(); i++) {
            if ((*i)->isGrp()) {
                 Remove(refID);
             } else if (((CustomCommand*)(*i))->referenceID == refID) {
               entityVector.erase(i);
               break;
            }
         }
      }

      /**
      *  This overload clear the entitys from the set.
       */
      void Clear ()
      {
         entityVector.clear();
      }

      /**
      *  Causes this CustomCommandGroup to be serialized by the
      *  specified XMLComposer.

         <returns> The s argument.  </returns>
      */
      XMLComposer&
      XMLCompose(XMLComposer& s) const;

      /// The group that contains this group
      CustomCommandGroup* parent;

   private:

       /// The vector of commands and groups within this group
       vector<CustomCommandEntity*> entityVector;

   };

   /**
   *  Container class for CustomCommandEntity objects.  The
   *  CustomCommandSet supplies organized XML serialization
   *  capabilities to effortlessly model a set of custom commands
   *  and groups.
      
      While it would be nice to simply make CustomCommandSet a vector of
   *  CustomCommandEntity's, rather than CustomCommandEntity
   *  pointers, this is unfortunately useless with the abstract
   *  class.  The virtual Process method is lost when the container
   *  class attempts to copy objects.  As the lesser of all
   *  possible evils, CustomCommandSet simply hides the actual
   *  pointer nature of the contained objects and deals with
   *  references at its interface points.
      
      The upshot of this is that an object added to the set must have a lifespan
      equal to that of the CustomCommandSet object itself, or at least must
      exist as long as it appears in that container.  Failure to do so will
      result in a lot of mystifying segmentation faults.
   */
   class CustomCommandSet : protected vector<CustomCommandEntity*>
   {
   public:
      /// The "custcmdset" element tag
      static const std::string xmlTagName;

      /// Making the size_type of the inherited class public.
      typedef vector<CustomCommandEntity*>::size_type size_type;

      /**
         The destructor for a CustomCommandSet takes no special action, but is
         provided for extensibility.
      */
      virtual
      ~CustomCommandSet() {}

      /**
         The method for adding a CustomCommand to the container.
         
         <param name="cc"> The object to add.  The object MUST have a lifespan
         at least as long as its inclusion in this container instance.
         </param>
         
         \exception
         Throws a logic_error if there's already a member in the set with the
         same referenceID as cc.
       */
      void push_back (CustomCommandEntity& cc) throw (std::logic_error);

      /// Reports the number of CustomCommandEntity objects currently referred to by the container.
      size_type size() const
      {
         return vector<CustomCommandEntity*>::size();
      }

      /**
         This method will seek the member of the set with the specified
         referenceID.
         
         <returns> A reference to the specified CustomCommand.  </returns>

         \exception
         Throws a logic_error if there's no member in the set with the specified
         referenceID.
       */
      CustomCommand&
      find(uint32 refID) throw (std::logic_error);

      /**
         The index operator accesses the vector by zero-based index position.

      *  <returns> A pointer to the CustomCommandEntity at the
      *  specified location in the set, or NULL if the location
      *  exceeds the size of the set. </returns>

         \exception
         Throws out_of_range if the indx argument is not within the size of the
         vector.
       */
      CustomCommandEntity& operator [] (size_type indx) throw (std::out_of_range);

      /**
         This overload removes the specified CustomCommand from the set.  If the
         object is not a member of the set, the method has no effect.
       */
      void Remove (CustomCommand& cc)
      {
         for (iterator i=begin(); i!=end(); i++) {
            if ((*i)->isGrp()) {
                 Remove(cc);
             } else if (*i==&cc) {
               erase(i);
               break;
            }
         }
      }

      /**
         This overload removes the CustomCommand with the specified reference ID
         from the set.  If there's no object in the set with that reference ID,
         the method has no effect.
       */
      void Remove (uint32 refID)
      {
         for (iterator i=begin(); i!=end(); i++) {
            if ((*i)->isGrp()) {
                 Remove(refID);
             } else if (((CustomCommand*)(*i))->referenceID == refID) {
               erase(i);
               break;
            }
         }
      }

      /**
      *  This overload clear the commands from the set.
       */
      void Clear ()
      {
         clear();
      }

      /**
         Causes this CustomCommandSet to be serialized by the specified
         XMLComposer.

         <returns> The s argument.  </returns>
      */
      virtual XMLComposer&
      XMLCompose(XMLComposer& s) const;

   };

};
#endif

