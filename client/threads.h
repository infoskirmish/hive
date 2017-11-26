#ifndef __THREADS_H
#define __THREADS_H

int make_thread( void *(*func)(void *), void *args );
int terminate_thread( void );

#endif //__THREADS_H
