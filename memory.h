/*
 * memory.h
 *
 *      Author: Hao Xu
 */

#ifndef MEMORY_
#define MEMORY_

#include "clause.h"
#include "symbols.h"
#include "stack.h"
#include "memoryPool.h"

// extern ObjectPool<Clause, MAX_NUM_CLAUSES> globalClauseObjectPool;

// init only
// symtable region is symtable.region
extern Region *superSymbolRegion;
extern Region *typeRegion;

// growing
extern Region *modelTrieRegion;
extern Region *termMatrixRegion;
extern Region *termMatrixHashtableRegion;
extern Region *termCacheRegion;
extern Region *globalTrieHashtableRegion;
extern Region *clauseRegion;
extern Region *clauseComponentRegion;
extern Region *subRegion;
extern Region *instStackRegion;
extern Region *firstGenStackRegion;
extern Region *secondGenStackRegion;
extern Region *stackRegion;
extern Region *coroutineStateRegion;

extern MemoryPool<DEFAULT_BLOCK_SIZE> regionPagePool;

extern size_t total_memory_usage;

#define TOTAL_MEMORY_USAGE \
		total_memory_usage \

//		+ clauseRegion->alloc_size \
//		+ termMatrixRegion->alloc_size \
//		+ termCacheRegion->alloc_size \
//		+ termMatrixHashtableRegion->alloc_size \
//		+ subRegion->alloc_size \
//		+ instStackRegion->alloc_size \
//		+ globalTrieHashtableRegion->alloc_size \
//		+ modelTrieRegion->alloc_size \

void printMemoryUsage(bool verbose = true);
void gc(ActivationRecordCommon **arbs);

#endif /* MEMORY_ */
