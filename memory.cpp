/*
 * memory.cpp
 *
 *      Author: Hao Xu
 */
#include "stdlib.h"
#include "memory.h"
#include "utils.h"
#include "stack.h"
size_t total_memory_usage;


void printMemoryUsage(bool verbose) {
	if(verbose) {
	printf("===========\n");

	printf("Sub region size total %s\n", formatSize(region_data_size(subRegion)));
	printf("InstStack region size total %s\n", formatSize(region_data_size(instStackRegion)));
	printf("TermMatrix region size total %s\n", formatSize(region_data_size(termMatrixRegion)));
	printf("TermMatrix hashtable region size total %s\n", formatSize(region_data_size(termMatrixHashtableRegion)));
	printf("TermCache region size total %s\n", formatSize(region_data_size(termCacheRegion)));
	printf("Clause region size total %s\n", formatSize(region_data_size(clauseRegion)));
	printf("Clause component region size total %s\n", formatSize(region_data_size(clauseComponentRegion)));
	printf("Trie node region size total %s\n", formatSize(region_data_size(modelTrieRegion)));
	printf("Trie hashtable region size total %s\n", formatSize(region_data_size(globalTrieHashtableRegion)));
	printf("CoroutineState region size total %s\n", formatSize(region_data_size(coroutineStateRegion)));
	printf("First Gen Stack region size total %s\n", formatSize(region_data_size(firstGenStackRegion)));
	printf("Second Gen Stack region size total %s\n", formatSize(region_data_size(secondGenStackRegion)));
	printf("Curr Gen Stack region size total %s\n", formatSize(region_data_size(stackRegion)));

	printf("===========\n");
	}
	printf("Total memory pool size %s\n", formatSize(regionPagePool.size));
	printf("Total memory pool in use %s\n", formatSize(regionPagePool.inUse));
	printf("Total memory allocation in use %s\n", formatSize(TOTAL_MEMORY_USAGE));

}

ActivationRecordCommon *copyActivationRecord(int level, ActivationRecordCommon *c, Region *gen1, Region *gen2, Region *newGen2) {
#ifdef DEBUG_RESTART
	printf("%d: start copying %p\n", level, c);
	fflush(stdout);
#endif
	if(c==NULL) {
#ifdef DEBUG_RESTART
		printf("%d: done copying %p\n", level, c);
		fflush(stdout);
#endif
		return NULL;
	}
	if(REGIONDESCOF(c)->r == gen1) { // do not copy if already in new region
		return c;
	}
	if(DELETED(c)) {
		// if an activation record is deleted then it must have been copied earlier,
		// now the pointer points to the new address
#ifdef DEBUG_RESTART
		printf("copy %p, del = %d, it has already been copied to %p\n", c, REGIONDESCOF(c)->del, *reinterpret_cast<ActivationRecordCommon **>(c));
		assert((*reinterpret_cast<ActivationRecordCommon **>(c))->nestingLevel == 1 || (*reinterpret_cast<ActivationRecordCommon **>(c))->nestingLevel == 2);
		printf("%d: done copying %p\n", level, c);
		fflush(stdout);
#endif
		return *reinterpret_cast<ActivationRecordCommon **>(c);
	}
	ActivationRecordCommon **ar = c->arBottoms;
	for(int i = 0; i < LEVEL_OF_NESTING; i++) {
		ar[i] = copyActivationRecord(level +1, ar[i], gen1, gen2, newGen2);
	}

	ActivationRecordCommon *newPtr = (REGIONDESCOF(c)->r == gen2) ?
			(ActivationRecordCommon *) region_alloc(gen1, SIZE(c)) :
			(ActivationRecordCommon *) region_alloc(newGen2, SIZE(c));
	// now copy it self because there is no cyclic stack, we don't need to test DELETED(c) again here
	memcpy(newPtr, c, SIZE(c));
	SET_DELETE(c);
	// record the new address
	*reinterpret_cast<ActivationRecordCommon **>(c) = newPtr;
#ifdef DEBUG_RESTART
	printf("copy %p to %p\n", c, newPtr);
	printf("%d: done copying %p\n", level, c);
	fflush(stdout);
#endif

	return newPtr;

}

void gc(ActivationRecordCommon **arbs) {
	{
	// clause component
	Region *newClauseComponentRegion = make_region();

	FIRST_OBJ_REGION(clauseRegion, x, o);
	while(o != NULL) {
		if(!DELETED(o)) {
			Clause *c = (Clause *) o;
#ifdef DEBUG_RESTART
			char buf[STRING_BUF_SIZE];
			c->toString(buf, symtable);
			printf("copy clause %s\n", buf);
#endif
			Term **literals = c->literals;
			bool *signs = c->signs;
			c->literals = (Term **) region_alloc(newClauseComponentRegion, sizeof(Term *)*c->numberOfLiterals);
			c->signs = (bool *) region_alloc(newClauseComponentRegion, sizeof(bool)*c->numberOfLiterals);
			memcpy(c->literals, literals, sizeof(Term *)*c->numberOfLiterals);
			memcpy(c->signs, signs, sizeof(bool) *c->numberOfLiterals);
		}
		NEXT_OBJ_REGION(x, o);
	}
	region_free(clauseComponentRegion);
	clauseComponentRegion = newClauseComponentRegion;
	}{
	// stack
	Region *newStackRegion = make_region();
	Region *newGen2 = make_region();

	for(int i = 0; i < LEVEL_OF_NESTING; i++) {
		arbs[i] = copyActivationRecord(0, arbs[i], firstGenStackRegion, secondGenStackRegion, newGen2);
	}

	FIRST_OBJ_REGION(coroutineStateRegion, x, o);
	while(o != NULL) {
		if(!DELETED(o)) {
			CoroutineState *c = (CoroutineState *) o;
			if(c->threadId!=0) { // it is a data dependency record if threadId = 0
#ifdef DEBUG_RESTART
			printf("copy activation record from coroutine state");
			printByType(c);
			printf("\n");
#endif
			ActivationRecordCommon **ar = c->arBottoms;
			for(int i = 0; i < LEVEL_OF_NESTING; i++) {
				ar[i] = copyActivationRecord(0, ar[i], firstGenStackRegion, secondGenStackRegion, newGen2);
			}
			}
		}
		NEXT_OBJ_REGION(x, o);
	}
	region_free(stackRegion);
	region_free(secondGenStackRegion);
	secondGenStackRegion = newGen2;
	stackRegion = newStackRegion;
	}

}
