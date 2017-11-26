
/** \file debug.h */
#ifndef _TRIGGER_DEBUG_H
#define _TRIGGER_DEBUG_H

#ifdef DEBUG

#include <stdio.h>

//#define DEBUG_FORMAT "\n[%s] %s:%5d %s): "
#define DEBUG_FORMAT "\n[%s]%s): "
#define debug_log(str, log_type, format, ...) \
do { \
    fprintf(str, DEBUG_FORMAT, log_type,  __func__); \
    fprintf(str, format, ## __VA_ARGS__); \
 } while (0);
/* do { \ */
/*     fprintf(str, DEBUG_FORMAT, log_type, __FILE__, __LINE__, __func__); \ */
/*     fprintf(str, format, ## __VA_ARGS__); \ */
/* } while (0) */

#define info(format, ...) debug_log(stderr, "i", format, ## __VA_ARGS__)
#define debug(format, ...) debug_log(stderr, "d", format, ## __VA_ARGS__)

#define error(format, ...) debug_log(stderr, "E", format, ## __VA_ARGS__)

#define debug_printBuf(buf, len) do { \
    debug("################################## debugbuf (%zu)\n" ,len);  \
    if (buf && len > 0) { \
        size_t xj3=0; char rj3=0; \
        for(xj3=0;xj3<len;xj3++) \
        {  \
            if (buf[xj3] >= 32 && buf[xj3] <= 126)  \
                rj3=buf[xj3];  \
            else  \
                rj3='.'; \
            fprintf(stderr, "%c", rj3); \
        } \
        fprintf(stderr, "\r\n");  \
    } else { debug("buf to print is null\n"); } \
  } while(0);

#define debug_printhex(buf, len) do { \
    debug("################################## hex (%zu)" ,len);  \
    if (buf && len > 0) { \
        size_t xj3=0; \
        for(xj3=0;xj3<len;xj3++) \
        {  \
            if (xj3%8==0) fprintf(stderr, "\r\n"); \
            fprintf(stderr, "0x%08x ", buf[xj3]); \
        } \
        fprintf(stderr, "\r\n"); \
    } else { debug("buf to print is null\n"); } \
  }  while(0);

//#define debug_printBuf(x, y) { }

#else /* DEBUG not defined - get rid of debug messages */

#define info(format, ...) { }
#define debug(format, ...) { }
#define error(format, ...) { }
#define debug_printBuf(x, y) { }
#define debug_printhex(x, y) { }

#endif /* #ifdef DEBUG */
#endif /* #ifndef DEBUG_H */
