/*
 * File:   defs.h
 * Author: Hao Xu
 *
 */

#ifndef DEFS_H
#define DEFS_H

#include "region.h"

#define USE_TYPES
#define USE_THREADS

//#define DEBUG_LOOKUP
//#define DEBUG_INVERSE_LOOKUP
#define DEBUG_INSTANCE
//#define DEBUG_INSTANCE_CLAUSE
//#define DEBUG_ORDERED_RESOLVE
//#define DEBUG_RESOLVENT
//#define DEBUG_ELIGIBLE_LITERALS
//#define DEBUG_RESTART_SIZE
//#define DEBUG_CLOCK
//#define DEBUG_INSTANCE_CLOCK
//#define DEBUG_STACK_USAGE
//#define DEBUG_TERM_SIZE_LIMIT
//#define DEBUG_LOOKUP_CLAUSE_TREE
//#define DEBUG_GENERATE_TERM
//#define DEBUG_VERIFY_LINKED_STACK
//#define DEBUG_STACK_ALLOC
//#define DEBUG_VAR_TYPE
//#define DEBUG_TRIE
//#define DEBUG_VERIFY_THREAD_GEN_NO_INST
//#define DEBUG_VERIFY_GENERATED_INSTANCE
//#define DEBUG_VERIFY_RESOLVENT
//#define DEBUG_VERIFY_EL_CLAUSES
//#define DEBUG_CHECK_THREADS
//#define DEBUG_ENABLE_DISABLE
//#define DEBUG_MODEL
//#define DEBUG_RESTART
//#define DEBUG_TERM_SIZE
//#define DEBUG_TERM_RENAMING
//#define DEBUG_COMPILE
//#define DEBUG_VARIANT
//#define DEBUG_TYPE


//#define printf(...)

#ifdef DEBUG_STACK_ALLOC
#define DEBUG_STACK_ALLOC_SWITCH true
#else
#define DEBUG_STACK_ALLOC_SWITCH false
#endif
#define DEBUG_STACK_SIZE false
#define DEBUG_STACK_SIZE_SHOW false
#define STACK_DEBUG false
#define STACK_STATE_DEBUG false
#define DEBUG_AR_BOTTOM_SET false
#define DEBUG_BRANCH_THRESHOLD false
#define DEBUG_DEALLOC false
//#define DEBUG_TRACE_DEALLOC false
//#define DEBUG_STACK_SIZE true
//#define DEBUG_STACK_SIZE_SHOW true
//#define STACK_DEBUG true
//#define STACK_STATE_DEBUG true
//#define DEBUG_AR_BOTTOM_SET true
//#define DEBUG_BRANCH_THRESHOLD true
//#define DEBUG_DEALLOC true
#define DEBUG_TRACE_DEALLOC true

#define HEAP_STACK
//#define INSERT_THREAD_HEAD
#define USE_RELEVANCE

//#define PROFILE_MODEL

#define INCREMENTAL_GENERATION

enum ProverStatus {
	RUNNING,
	TIMED_OUT,
	MAX_MEMORY,
	PROOF_FOUND,
	SATURATED,
	MAXED_OUT
};

typedef unsigned long int u_int64_t;
typedef u_int64_t Symbol;
typedef u_int64_t Version;
#define CURR_REGION __r
#define REGION Region *CURR_REGION
#define MAKE_REGION Region *CURR_REGION = make_region(0)
#define FREE_REGION region_free(CURR_REGION)

// bit     0 ~ 7        8 ~ 15      16 ~ 23     24 ~ 31
// func    |func arity|
//         |func id                       |
// var                                          |var version|
//         |var id                                          |
// address space:
// 0 to 7FFFFF: function symbol 2^14 (16K)
// 800000 to FFFFFF: var symbol 2^22 (4M)
#define FUNC_ID_ARITY_MASK ((Symbol) 0xFF)
#define FUNC_ID_OFFSET FUNC_ID_ARITY_MASK + 1
#define VAR_ID_OFFSET ((Symbol)0x800000)
#define MAX_VAR_ID_MASK ((Symbol) 0xFFFFFF)
#define VAR_VERSION_OFFSET ((Symbol)0x1000000)
#define MAX_VAR_VERION_MASK ((Symbol)0xFF000000)
#define MAX_VAR_ID ((Symbol)0xFFFFFFFF) // VAR_VERION_MASK | MAX_VAR_ID_MASK
#define MAX_NUM_CLAUSE_LITERALS 1024
#define MAX_NUM_CLAUSE_LITERALS_RESOLVENT 256 * 1024
#define MAX_NUM_CLAUSE_VARS 1024
#define MAX_TERM_SIZE 127
#define MAX_FUNCTION_SYMBOL_DIMENSION 255
#define MAX_FUNCTION_SYMBOLS 1024
#define STRING_BUF_SIZE 1024 * 16
#define CLAUSE_STRING_TRUNCATE_THRESHOLD 30


#define IS_FUNC_SYMBOL(x) ((x) < VAR_ID_OFFSET)
#define IS_VAR_SYMBOL(x) ((x) >= VAR_ID_OFFSET)

#define ARITY(x) ((unsigned int) ((x) & FUNC_ID_ARITY_MASK))

#define HEAP_DEF(t,n) t *n = (t *)region_alloc(CURR_REGION, sizeof(t))
#define HEAP_ALLOC(t,l) ((l)==0?NULL:(t *)region_alloc(CURR_REGION, sizeof(t)*(l)))
#define MAP_ARR(f, l, a1, a2) {int _index; for(_index=0;_index<(l);_index++) { (a2)[_index] = f((a1)[_index]); }}
#define MAP_APP(f, l, a) {int _index; for(_index=0;_index<(l);_index++) { f((a)[_index]); }}
#define INDEX _index

#define SPRINT(buf, offset, format, ...) snprintf((buf) + offset, STRING_BUF_SIZE - offset, format , ##__VA_ARGS__); \
                                     offset += strlen((buf)+offset)

//#define MAX(a,b) ((a)>(b)?(a):(b))

#define GLOBAL_TRIE_NODE_HASHTABLE_SIZE 1000000
#define MAX_TRIE_DEPTH 1024

#define MAX_RELEVANCE (16 * 1024 - 1)


#define MAX_PATH_LEN 1024

extern int universalTermSizeLimit;
extern int globalTimeLimit;
extern bool globalUseSuperSymbols;
extern bool globalUseBranchThreshold;
extern bool globalUpdateStartingIndex;
extern bool globalSortSubtreesBySize;
extern size_t globalCoroutineStackSize;
extern bool globalFlipLiteral;
extern bool globalRandomClauseLiteral;
extern bool globalRandomClauseInsert;
extern bool globalRandomLexicalOrder;
extern bool globalRandomFlipLiteral;
extern long globalCoroutineStateStackSize;
extern char globalTPTPPath[MAX_PATH_LEN];
extern ProverStatus globalProverStatus;
#ifdef USE_RELEVANCE
extern int universalRelevanceMax;
extern int universalRelevanceLimit;
#endif
extern int currThreadId;

#endif	/* DEFS_H */

