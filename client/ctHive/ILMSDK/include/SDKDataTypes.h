/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/SDKDataTypes.h$
$Revision: 1$
$Date: Friday, October 09, 2009 5:04:08 PM$
$Author: timm$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  SDKDataTypes - FILE DESCRIPTION  -----------------*

Declaration of the basic components that appear in the ILM interface.

Author's note (9/4/07): The proliferation of known-length integer types
continues with the addition of the int16/uint16 types.  For the sake of
completeness, the #if directive testing the size of the native int type has been
inserted as an option; however, at the time of writing, it seems extremely
unlikely that any piece of this application will ever see a 16-bit processor.
The real issue is what happens in the other direction: should the compilation
environment change such that the native int type is 64 bits in length, we'll be
screwed, because we'll need two known-length integer types (int16 & int32) and
will have only one standard type (short) available to cover them.  So, here's
hoping that the C/C++ standards committees finally see reason and fix the native
type length issue before this problem rears its head.
*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#ifndef SDK_DATA_TYPES
#define SDK_DATA_TYPES

/*-----------------  SDKDataTypes - INCLUDES  -------------------------*/

#include <climits>
#include <cstring>
#include <string>

/*-----------------  SDKDataTypes - MISCELLANEOUS  --------------------*/

#define DLL_EXPORT __attribute__ ((visibility("default")))
#define DLL_LOCAL  __attribute__ ((visibility("hidden")))

/*-----------------  SDKDataTypes - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary
{
   #if (SHRT_MAX == 32767) 
      typedef unsigned short uint16 ; 
      typedef short int16 ; 
   #elif (INT_MAX == 32767)
      typedef unsigned int uint16 ;
      typedef int int16 ;
   #else 
   #error Cannot determine 16-bit integer types
   #endif 

   #if (INT_MAX == 2147483647)
      typedef unsigned int uint32 ;
      typedef int int32 ;
   #elif (LONG_MAX == 2147483647)
      typedef unsigned long uint32 ;
      typedef long int32 ;
   #else
   #error Cannot determine 32-bit integer types
   #endif

   #if (LONG_MAX == 9223372036854775807)
      typedef unsigned long uint64 ;
      typedef long int64 ;
   #elif (LLONG_MAX == 9223372036854775807)
      typedef unsigned long long uint64 ;
      typedef long long int64 ;
   #else
   #error Cannot determine 64-bit integer types
   #endif

   /**
      This structure describes a raw binary data block that can be returned from
      certain functions in the ILM.  Note that an instance of 'binary' is not
      suitable for marshalled interfaces (the ILM interface is strictly resolved
      in local process memory space.)
   */
	struct binary
	{
      /// Length of the information pointed to by the data member.
		uint32 length ;
      /// Pointer to the raw binary data.
		void *data ;
	} ;

   /**
      Depricated; do not use for new code.  A helper object that assists the
      comparison of two C-style character strings.
   */
   class STLCompare
   {
   public:
      bool operator()(const char *aInput1, const char *aInput2) ;
   };
   

   /**
      This structure is the ILM interface's definition of character string
      entities that are passed as parameters or return types.  While it's
      set up in the explicit-length methodology, if left to its own devices (i.e.,
      the initializing constructor), it will retain the null termination of its
      initializing string for easy default conversion to a C-style const char* form,
      as supported by its explicit conversion operator to that type.
   */
   struct String
   {
   public:
   
      /**
          Default constructor for a String structure.
      */
      String()
      {
         length = 0;
         buffer = NULL;
      }

      /**
         Initializing constructor for a String structure from C-style string.

         <param name="s"> The C-style string to which the new object will be
         made equal.  If NULL, an emptyString will result. </param>
      */
      String (const char* s)
      { 
         NewBuf( ((s == NULL) ? 0 : (uint32)strlen(s)), s);
      }

      /**
         The copy constructor makes a "deep" copy of the specified initializer
         String.

         <param name="s"> The String which this one is to copy. </param>
      */
      String (const String& s)
      { 
         NewBuf(s.length, s.buffer); 
      }

      /**
         The copy constructor makes a "deep" copy of the specified initializer
         string.

         <param name="s"> The string which this one is to copy. </param>
      */
      String (const std::string& s)
      { 
         NewBuf((uint32)(s.length()), s.c_str()); 
      }

      /**
         The destructor for a String.
      */
      ~String()
      {
         // Don't get excited; read your ISO/ANSI C++ standard.  The delete operators
         // do NULL checking so you don't have to.
         delete[] buffer ;
      }

      /**
         C-style string cast operator for a String.
         
         <returns> The buffer member, unless that member is null, in which case
         an empty string is returned.   </returns>
      */
      operator const char*() const
      {
         return (buffer==NULL ? "" : buffer);
      }

      /**
         STL-style string cast operator for a String.
      
         <returns> A new std::string equivalent to this String. </returns>
      */
      operator std::string() const
      { 
         return std::string(buffer, length); 
      }

      /**
         The same-type assignment operator for a String makes a "deep" copy
         of the buffer member.
         
         <param name="s"> The String to which this one is to be made equal. </param>
         <returns> This object. </returns>
      */
      String& operator = (const String& s)
      {
         delete[] buffer;
         NewBuf(s.length, s.buffer);
         return *this;
      }

      /**
         This assignment operator overload copies the specified C-style String.
      
         <param name="s"> The string to which this one is to be made equal.  If
         NULL, this object will be an empty string.   </param>
         
         <returns> This object. </returns>
      */
      String& operator = (const char* s)
      {
         delete[] buffer;
         NewBuf( ((s == NULL) ? 0 : (uint32)strlen(s)), s);
         return *this;
      }

      /**
         The String equality comparison operator returns 

         <param name="s"> The string to which this one is to be compared.  If
               NULL, the standard library's invalid parameter handler will be
               invoked.  </param>
         
         <returns> True if this String's buffer is identical to the C-style
               string argument up to the current length; false otherwise.
               </returns>
      */
      bool operator == (const char* s)
      {
         return (strncmp(buffer, s, (size_t)length)==0);
      }

      /**
         Read accessor for the String length.
      */
      unsigned Length() const
      { 
          return length; 
      }

   protected:

      /// Contains the length of the string in the buffer, not including the null terminator.
      unsigned length;
      
      /// The buffer containing the string.  Will be NULL if the String is zero-length.
      char*    buffer;
      
      /**
         This helper function makes a "deep" copy of the specified String, and
         is used by the assignment operators as well as some non-default
         constructors.
         
         <param name="i"> Length of the string in init, not including the null
            terminator.  If zero, init will be ignored and the buffer member set
            to NULL.  </param>
         <param name="init"> C-style string to which this object is to be set.
            If longer than i, the String's new buffer will be truncated
            accordingly.  </param>
      */
      void NewBuf (uint32 i, const char* init);
   };


};

#endif
