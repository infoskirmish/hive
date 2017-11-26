/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/Primitive.h$
$Revision: 1$
$Date: Tuesday, November 24, 2009 6:16:32 PM$
$Author: timm$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  Primitive - FILE DESCRIPTION  -----------------*

Declaration of the Primitive class and its attendant components.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef PRIMITIVE_483067348496_H
#define PRIMITIVE_483067348496_H

/*-----------------  Primitive - INCLUDES  -------------------------*/

#define ENDOFREFTABLEID 0xffffffff

#include <vector>
#include "SDKDataTypes.h"
#include "Primitive_ref.h"

/*-----------------  Primitive - MISCELLANEOUS  --------------------*/


class XMLComposer;

/*-----------------  Primitive - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary 
{

   namespace Primitive
   {

      /**
         An object of the Validator class is used by the ILM to communicate the
         description of a supported Primitive to CutThroat.
      */
      class Validator
      {
      public:
         /**
            Constructs a Validator for the specified primitive ID with an
            optional behavioral note.
            */
         Validator ( uint32 pID, const char * aBehavioralNote = NULL)
            : primitiveID(pID)
         {
            behavioralNote = aBehavioralNote;
         }

         /**
            Causes the Validator to be serialized to the specified XMLComposer.
            <returns> The c argument. </returns>
          */
         XMLComposer& XMLCompose (XMLComposer& c) const ;

         /// The identifier of the command primitive reported by this Validator.
         uint32    primitiveID;

         /// The behavioral note for this primitive as applied to the ILM implementation.
         const char * behavioralNote ;

         static const std::string   xmlTagName;
         static const std::string   tag_primitiveID;
         static const std::string   tag_behavioralNote;
      } ;

      /** This is the reference table that describes pretty much everything that
          the ILM implementation does with command primitives.  It's a fixed
          array containing one entry for each predefined Primitive, regardless
          of whether that Primitive is supported by the ILM or not.  (Support is
          indicated by setting the entry's currentlySupported flag.)
         
          For easier use, the ReferenceTable class contains a "Static" object
          that provides a wrapper for this global table.
      */
      extern RefTableEntry  refTable[];

      /**
         ReferenceTable is a class wrapper for Primitive reference tables.  It
         serves three important purposes:  generating at construction time a
         copy of the static refTable, providing an easy way to index by
         Primitive ID, and generating a snapshot vector of supported Primitives
         that's used in the ValidatePrimitives override.
       */
      class ReferenceTable
      {
      public:

         /**
            The ReferenceTable constructor creates a copy of the specified
            array of table entries.
            
            <param name="m"> The array that will be used to initialize the
               object.  If not specified, the static Primitive::refTable array will
               be used.  </param>
          */
         ReferenceTable(RefTableEntry m[] = refTable);

         ~ReferenceTable()
         {
            if (myTableInstance!=refTable) delete[] myTableInstance;
         }

         /// The index operator returns the entry corresponding to the specified
         /// primitive identifier.  Returns null if an invalid primID is used.
         RefTableEntry* operator[](uint32 primID) {return FindEntry(primID);}

         /**
            Returns the entry corresponding to the specified primitive
            identifier, or null if an invalid primID is used.
         */
         RefTableEntry*
         FindEntry (uint32 primID);

         /**
            Runs through the object and creates a Validator
            object for each Primitive whose currentlySupoprted flag is set.

            <returns> A list of Validators, one for each Primitive currently
            supported. </returns>
         */
         std::vector<Primitive::Validator>
         GetSupportedPrimitives() const;

         /// This object is a wrapper around the global refTable array.
         static ReferenceTable Static;

      private:
         RefTableEntry*  myTableInstance;

         /// Used exclusively to build the Static member.
         explicit ReferenceTable(bool isStatic)
         {
            myTableInstance = refTable;
         }
      };

   };
};

#endif //PRIMITIVE_483067348496_H
