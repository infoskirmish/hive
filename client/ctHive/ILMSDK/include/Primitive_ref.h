/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/Primitive_ref.h$
$Revision: 1$
$Date: Tuesday, November 24, 2009 6:16:32 PM$
$Author: timm$
Template: cpp_file.h 3.0
*******************************************************************************

                           CPRCLASS = "SECRET"

*******************************************************************************
*/

/*-----------------  Primitive_ref - FILE DESCRIPTION  -----------------*

Declaration of the types involved in the Primitive reference table.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef PRIMITIVE_REF_483067348496_H
#define PRIMITIVE_REF_483067348496_H

/*-----------------  Primitive_ref - INCLUDES  -------------------------*/

#include <map>
#include "SDKDataTypes.h"

/*-----------------  Primitive_ref - MISCELLANEOUS  --------------------*/

/*-----------------  Primitive_ref - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary {

   class ProcessCmdResponse;

   /**
      This class provides the means to pass information between Primitive
      Activation instances for common commands that require more than one
      command primitive.  CutThroat is not able to repeat the same argument
      across multiple primitives (it's always associated with the first
      primitive in a command sequence), and it's also possible that some
      commands will require the results of a prior primitive as an argument.  To
      resolve both of these problems, a ProcessCmdAccumulator is used to hold
      string representations of named values that are passed along through a
      series of primitive handler functions (see HandlerFcnPtr in this module).
      
      The keys used in a ProcessCmdAccumulator and the associated values
      required by various primitive handler functions are specific to the ILM
      implementation.  However, it is recommended that where practical, keys
      are used which match the argument names appearing in JY008C616 through
      'C630, the Primitive Family inteface descriptions.
    */
   class ProcessCmdAccumulator : public std::map<std::string, std::string>
   {
   public:

      /**
         SupplementArgument provides a shorthand method for a common operation;
         it does its level best to ensure that a value, given by the 'val'
         parameter and identified by keyname, is defined, and that it is
         recorded in this accumulator object.
         
         <param name="val"> The value that must not be an empty string.  If it's
            empty when SupplementArgument is called, the function will search the
            accumulator to see if it contains an entry identified by keyname, and
            if found will copy it into val.  If val is not empty when the
            function is called, it will be left unchanged, and will be recorded
            in the accumulator.
            </param>
         <param name="keyname"> The name that identifies val; used as the index
            into this accumulator.  </param>
         <param name="resp"> If the return value is false, a local failure will
            be recorded in this response. </param>
         
         <returns> True if val was defined at entry, or if an entry indexed by
         keyname could be located in this accumulator; false otherwise.
         </returns>
       */
      bool SupplementArgument (String& val, const std::string& keyname, ProcessCmdResponse& resp);
   };

namespace Primitive
{
   /**
      A Primitive Activation signals the ILM to execute a command primitive with
      the arguments contained within the object.
    */
   class Activation
   {
   public:
      /**
         This constructor creates an Activation for the primitive with an ID
         equal to i, and with an argument blob pointed to by argPtr.
       */
      Activation (uint32 i, void* argPtr = NULL)
      {
         primitiveID = i;
         arguments = argPtr;
      }

      /**
         This constructor creates an Activation from a binary blob that contains
         the primitive ID in the first four bytes, followed immediately by an
         argument blob.
       */
      Activation (uint32* binPtr)
      {
         primitiveID = (*binPtr++);
         arguments = binPtr;
      }

      /**
         Copy constructor for a PrimitiveActivation.
      */
      Activation (const Activation& a)
         : arguments(a.arguments)
      {
         primitiveID = a.primitiveID;
      }

      virtual ~Activation ()
      {
      }


      /// The unique identifier for a Primitive Activation.  Must match
      /// the primitiveID column entry of one of the entries in refTable.
      uint32       primitiveID;
      /// The arguments for the command primitive.
      void*    arguments ;
   } ;

   /**
      The HandlerFcnPtr defines the signature for Activation handler
      functions.  An ILM implementation will define one of these functions for
      each command primitive that it supports, and will relate correct handler
      to the primitive using the Primitive::refTable entry corresponding to that
      Primitive.
      
      <param name="act"> The command Primitive::Activation being
      processed. If not all arguments in the Activation are valid, the handler
      function is expected to search the acc argument for a substitute value. If
      an argument cannot be resolved, the handler function must change the type
      of the resp argument to indicate a local failure.  </param>
      <param name="acc"> Stores argument and result values that may be of
      use to later Activations being processed in the same command sequence.
      The handler function is expected to place a value that can be used by
      another Primitive handler into this object, indexed by a mutually
      agreed-upon name.  </param>
      <param name="resp"> The handler function is to add results information to
      this object. The type member should not be modified to indicate a
      successful outcome, but only to indicate a failure condition or a pending
      results notification.  </param>
      
    */
   typedef void (*HandlerFcnPtr)(Primitive::Activation& act, ProcessCmdAccumulator& acc, ProcessCmdResponse& resp);

   /**
      This is the definition of an entry in the Primitive reference table.  An
      ILM must fill in its Primitives reference table to describe at runtime
      which primitives it supports, what handler functions will handle the
      activations of the command primitives, and any behavioral notes which will
      be displayed to the connecting application's operator.
    */
   typedef struct
   {
      /// The unique identifier for the command primitive.
      const uint32       primitiveID;

      /// The fixed length, in bytes, of the Primitive Activation's parameters when packed in an InterfaceLibrary::binary structure.
      const int      packedParamSize;

      /// A flag indicating that the primitive is supported in the ILM's current mode
      bool           currentlySupported;

      /// The handler function for the primitive.  If currentlySupported is true, this member must be defined (e.g., non-NULL)
      HandlerFcnPtr  handler;

      /// Notes for the CP workstation operator regarding any special considerations for the primitive that are specific to this ILM implementation.
      const char*    behavNote;
   } RefTableEntry;

}
}
#endif
//-----------------------------------------------------------------------------
//
//	SECRET		SECRET		SECRET		SECRET
//
//-----------------------------------------------------------------------------


