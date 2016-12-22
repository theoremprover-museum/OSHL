/* 
 * File:   coroutines.h
 * Author: Hao Xu
 *
 */
#include "stack.h"
#ifndef COROUTINES_H
#define	COROUTINES_H

extern    CoroutineState *threads[(MAX_TERM_SIZE + 1) * (MAX_RELEVANCE + 1)];
extern    CoroutineState *threadsHead[MAX_TERM_SIZE + 1]; // the node next to gpb_anchor
extern    CoroutineState *failureThreads;
extern    CoroutineState *idleThreads;
extern    CoroutineState *dataThread; // this thread contains all "saved states" that are used



    
    


#endif	/* COROUTINES_H */

