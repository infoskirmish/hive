#include "hclient.h"
#include "proj_strings.h"  //Required for strings...

/* ******************************************************************************************************************************
 *
 * FreeArgv(char** vector)
 * Description -- function frees the malloc'd memory used to store the argument vector created by BuildArgv()
 * Parameters  -- vector = the argument vector to be freed
 * Return      -- void
 *
 * **************************************************************************************************************************** */

void FreeArgv(char** vector) {
   register char** scan;

   if (vector != NULL) {
      for (scan = vector; *scan != '\0'; scan++) {
         free(*scan);
      }
      free(vector);
   }
}

/* ******************************************************************************************************************************
 *
 * BuildArgv(char* input)
 * Description -- function parses the user's command line into an argument vector similar to argv/argc
 * Parameters  -- input = command line string the user entered
 * Return      -- function returns the argument vector back to the calling function; the calling function must use FreeArgv()
 *                to free the malloc'd memory the pointer is pointing to
 *
 * **************************************************************************************************************************** */

char** BuildArgv(char* input) {
   int argc = 0;
   int squote = 0;
   int dquote = 0;
   char* arg;
   char* copybuf = NULL;
   char** argv = NULL;

   if (*input != '\0') {
      if ((copybuf = (char*)malloc(strlen(input) + 1)) == NULL) {
         //fprintf(stderr, "BuildArgv(): failure to allocate memory for the copy buffer\n");
         fprintf(stderr, "%s", buildArgv1String);
         return NULL;
      }
      if ((argv = (char**)malloc(5 * sizeof(char*))) == NULL) {
         //fprintf(stderr, "BuildArgv(): failure to allocate memory for the argument vector\n");
         fprintf(stderr, "%s", buildArgv2String);
         free(copybuf);
         return NULL;
      }
      while (isspace(*input) != 0) {
         input++;
      }
      while ((*input != '\0') && (argc < 4)) {
         memset(copybuf, 0, sizeof(copybuf));
         argv[argc] = '\0';
         arg = copybuf;
         while (*input != '\0') {
            if ((isspace(*input)) && (!squote) && (!dquote)) {
               break;
            } else {
               if (squote) {
                  if (*input == '\'') {
                     squote = 0;
                  } else {
                     *arg = *input;
                     arg++;
                  }
               } else if (dquote) {
                  if (*input == '"') {
                     dquote = 0;
                  } else {
                     *arg = *input;
                     arg++;
                  }
               } else {
                  if (*input == '\'') {
                     squote = 1;
                  } else if (*input == '"') {
                     dquote = 1;
                  } else {
                     *arg = *input;
                     arg++;
                  }
               }
               input++;
            }
         }
         *arg = '\0';
         argv[argc] = strdup(copybuf);
         argc++;
         while (isspace(*input) != 0) {
            input++;
         }
      }
      free(copybuf);
      while (argc < 5) {
         argv[argc] = '\0';
         argc++;
      }
   }

   return argv;
}
