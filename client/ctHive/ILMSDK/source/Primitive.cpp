/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/source/Primitive.cpp$
$Revision: 1$
$Date: Tuesday, November 24, 2009 6:15:50 PM$
$Author: timm$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  Primitive - FILE DESCRIPTION  -----------------*

Implementation of the Primitive class and its attendant components.
*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  Primitive - INCLUDES  -------------------------*/

#include "Primitive.h"
#include "XMLComposer.h"

/*-----------------  PrimitiveCT - DECLARATIONS ----------------------*/

using std::string;
using namespace InterfaceLibrary;
using namespace InterfaceLibrary::Primitive;

/*==========================================================================*/

const string   Primitive::Validator::xmlTagName("primitive");
const string   Primitive::Validator::tag_primitiveID("primitiveid");
const string   Primitive::Validator::tag_behavioralNote("behavioralnote");

XMLComposer& Primitive::Validator::XMLCompose (XMLComposer& composer) const
{
   composer.StartTag(xmlTagName.c_str()) ;
   composer.WriteSimpleElement(tag_primitiveID.c_str(), (long)primitiveID);
   if (behavioralNote!=NULL) {
      composer.WriteSimpleElement(tag_behavioralNote.c_str(), behavioralNote);
   }
   composer.EndTag() ;
   
   return composer ;
}

Primitive::ReferenceTable Primitive::ReferenceTable::Static(true);

Primitive::ReferenceTable::ReferenceTable(RefTableEntry m[])
{
   int tableSize;

   for (tableSize=0; m[tableSize].primitiveID!=ENDOFREFTABLEID; tableSize++);
   tableSize++;
   myTableInstance = new RefTableEntry[tableSize];
   memcpy(myTableInstance, m, tableSize*sizeof(RefTableEntry));
}

Primitive::RefTableEntry*
Primitive::ReferenceTable::FindEntry (uint32 primID)
{
   RefTableEntry* e;
   for (e = &myTableInstance[0]; e->primitiveID < primID; e++);
   return (primID==e->primitiveID ? e : NULL);
}

std::vector<Primitive::Validator>
Primitive::ReferenceTable::GetSupportedPrimitives() const
{
   std::vector<Primitive::Validator> v;

   for (RefTableEntry* e = &myTableInstance[0]; e->primitiveID != ENDOFREFTABLEID; e++) {
      if (e->currentlySupported) {
         v.push_back(Validator(e->primitiveID, e->behavNote));
      }
   }
   return v;
}



