/*
 *      Author: Hao Xu
 */

#include <time.h>
#include "coroutines.h"
#include "trie.h"
#include "term.h"
#include "utils.h"
#include "stack.h"
#include "hashtable.h"
#include "utils.h"
#include "parser.h"
#include "symbols.h"
#include "printByType.h"
#include "arithmetics.h"
#include "model.h"
#include "ordering.h"
#include "superSymbol.h"
#include "type.h"
#include "memory.h"
#include "times.h"
#include "region.h"

#include <assert.h>
// #define assert(x)
    CoroutineState *threads[(MAX_TERM_SIZE + 1) *( MAX_RELEVANCE + 1)];
    CoroutineState *relevanceThreads; // this is the nodes for extending max relevance
    //LinkedList<CoroutineState *> relDependencyNodes;
#ifdef INSERT_THREAD_HEAD
    CoroutineState *threadsHead[MAX_TERM_SIZE + 1];
#endif
    CoroutineState *failureThreads = NULL;
    CoroutineState *idleThreads = NULL;
    CoroutineState *dataThread = NULL; // this thread contains all "saved states" that are used
                                // for the purpose of keeping stack allocated data (sub nodes) from being overwritten
                                // without these "saved states", when lookupWrapper returns, the data become invalidated
    							// the "saved states" are automatically deleted by STACK_TRUNCATE
Region *subRegion = make_region(0);
Region *instStackRegion = make_region(0);
#ifdef STAT_SUPER_SYMBOLS
int superSymbolHitCounter = 0;
int superSymbolMissCounter = 0;
#endif
void printProverStatus(const char *status, int counter, long startTime, long currentTime) {
	printf("%s\n", status);
	printf("number of instances generated = %i\n", counter);
	printf("proof time = %f\n", (((double)currentTime - startTime)/CLOCKS_PER_SEC));
#ifdef PROFILE_MODEL
	printf("model time = %f\n", (((double)modelTime)/CLOCKS_PER_SEC));
#endif
	printf("inference rate = %f clauses/sec\n", counter/(((double)currentTime - startTime)/CLOCKS_PER_SEC));
#ifdef STAT_SUPER_SYMBOLS

	printf("super symbol acceleration = %d out of %d times", superSymbolHitCounter, superSymbolHitCounter + superSymbolMissCounter);
#endif


}
int constructInstanceClause(LinkedStack<Term *> *instStackNew, Term **lits, bool *ss, int *relevance, Sub *retsub, ClauseTree::Node *retClauseTreeNode) {
	int nLits = 0;
#ifdef USE_RELEVANCE
	int rel = 0;
#endif
	 // add all instances of negative literals, which are already in the trie
	while(instStackNew!=NULL) {
		// printf("instStack: %p\n", instStackNew);
		lits[nLits] = instStackNew->val;
		ss[nLits++] = false;
#ifdef USE_RELEVANCE
		rel = max(rel, instStackNew->val->vertex->value->relevance);
#endif
		instStackNew = instStackNew->prev;
	}
#ifdef USE_RELEVANCE
	rel++;
	if(rel > universalRelevanceMax) {
		universalRelevanceMax = rel;
//		for(LinkedList<CoroutineState *>::Node *curr = relDependencyNodes.head; curr!=NULL; curr=curr->next) {
//			undeleteAndEnableDependencySubtree(curr->value);
//		}
//		relDependencyNodes.clear();
	}
	*relevance = rel;
#endif
#ifdef DEBUG_INSTANCE_CLAUSE
		printf("instance clause: \n");
#endif
	while(retClauseTreeNode->literal!=NULL) {
		// subterm pointers always point to the largest subterm first
		if(retClauseTreeNode->sign) { // only sub positive literals
			lits[nLits] = (*retClauseTreeNode->literal)(retsub);
			ss[nLits++] = true;
		}
		if(globalUpdateStartingIndex) {
			retClauseTreeNode->super->startingIndex = retClauseTreeNode->index;
		}
#ifdef DEBUG_INSTANCE_CLAUSE
		char buf[STRING_BUF_SIZE];
		retClauseTreeNode->literal->toString(buf, symtable);
		printf("%s%s | ", retClauseTreeNode->sign?"":"~", buf);
		retClauseTreeNode->inp->toString(buf, symtable);
		printf("%s\n", buf);
#endif
		retClauseTreeNode = retClauseTreeNode->super;
	}
#ifdef DEBUG_INSTANCE_CLAUSE
		printf("\ninstance sub: ");
		printByType(retsub);
		printf("\n");
#endif
		return nLits;

}

int removeRepeats(Term **lits, bool *ss, int nLits) {
	int j = 1, k = 1;
	while(k<nLits) {
		if(lits[k] != lits[k-1]) {
			if(j != k) {
				lits[j] = lits[k];
				ss[j] = ss[k];
			}
			j++;
		}
		k++;
	}
	return j;

}
Term *lits[MAX_NUM_CLAUSE_LITERALS_RESOLVENT];
bool ss[MAX_NUM_CLAUSE_LITERALS_RESOLVENT];
Clause *processClauseInstance(LinkedStack<Term *> *instStackNew, Sub *retsub, ClauseTree::Node *retClauseTreeNode) {
	int nLits;
	int rel;
	nLits = constructInstanceClause(instStackNew, lits, ss, &rel, retsub, retClauseTreeNode);
	// sort
	quicksort(lits, ss, nLits);
	// remove repeats
	nLits = removeRepeats(lits, ss, nLits);
	// construct new clause instance object
	return new (clauseRegion) Clause(nLits, lits, ss, rel, 0);

}
Trie *auxLookUpChild(Trie *curr, Symbol s) {
//	char buf[STRING_BUF_SIZE];
//	if(curr != NULL){curr->toString(buf, symtable);} else {strcpy(buf, "<null>");}
//	printf("auxLookupChild %s, %s\n", buf, symtable[s]);
	if(curr == NULL) {
		return NULL;
	} else {
		Trie *ch = curr->lookUpChild(s);
		if(ch == NULL || ch->del) {
//			printf("not found!\n");
			return NULL;
		} else {
//			printf("found!\n");
			return ch;
		}
	}
}

void runTest(int testId, Term *p, ClauseTree *clauseTree, Trie *trie, Trie *negTrie, STACK_DEF, REGION) {

//        printf("stack depth = %d\n", i);

    Trie *retnode;
    Trie *cnode;
    TermCacheType *tctnode;
    Sub *subNew;
    Term *subterm;
    LinkedStack<TermCacheSeg *> *subSegNew;
    Version version;
    Term *retterm;
    SubtermPtr *subtermPtr;
    TermInstances *ti = NULL;
    TermInstanceIterator *tiItr = NULL;
    Sub *retsub;
    ClauseTree::Node *retClauseTreeNode;
    int counter;
    char buf[STRING_BUF_SIZE];
    Symbol s, rep;
    int arity;
// variable used in the main loop
	int i;
	int k;
	Clause *inst;
	clock_t startTime;
	clock_t maxTime;
	clock_t currentTime;
	CoroutineState *gbp_anchor; // this is the anchor branch record that is used when restore_state is called and there is no saved state below threshold or simple no saved state.
								// it is anchor because normal saved states are marked as deleted once they are restored
								// this state much be kept to prevent future "fall through"s
	int restartSize;
    int nNew, nInput;
    LinkedList<Term *>::Node * termPatternHead;
    unsigned int indexNew;
    LinkedStack<Term *> *instStackNew;
    PMVMInstruction *cinp;

    CoroutineState *bp;
	bool noInst;
	bool noInstSet;
#ifdef PROFILE_MODEL
	long modelStart, modelEnd;
#endif

// macros

// backtrack if any term generated will be too large.
#ifdef USE_THREADS
#ifdef USE_RELEVANCE
#define CHECK_TERM_SIZE(n, rp) \
		if(n > universalTermSizeLimit) { \
			int threadId = (n*(MAX_RELEVANCE + 1) + universalRelevanceLimit); \
			CoroutineState *&threadRef = threads[threadId]; \
			SAVE_STATE_WITH_THREAD(rp, threadRef, -threadId, NULL, del); \
			RESTORE_STATE; \
		} \

#else
#define CHECK_TERM_SIZE(n, rp) \
		if(n > universalTermSizeLimit) { \
			int threadId = n; \
			CoroutineState *&threadRef = threads[threadId]; \
			SAVE_STATE_WITH_THREAD(rp, threadRef, -threadId, NULL, del); \
			RESTORE_STATE; \
		} \

#endif
#define CHECK_TERM_SIZE_NS(n, rp) \
		if(n > universalTermSizeLimit) { \
			RESTORE_STATE; \
		} \

#else
#define CHECK_TERM_SIZE(n, rp) \
if(n > universalTermSizeLimit) { \
	SAVE_STATE_WITH_THRESHOLD_AND_THREAD(rp, n, BRANCH_POINTER); \
	RESTORE_STATE; \
} \

#endif

#define LEVEL_main 1
#define LEVEL_mainLoop 1

#define LEVEL_TermInstanceIterator 1
#define LEVEL_unfoldLazyList 1
#define LEVEL_TermInstances_new 1
#define LEVEL_lookupInverse 1
#define LEVEL_branchInverse 1
#define LEVEL_branchInverseTryFuncSymbols 2
#define LEVEL_TermInstanceIterator_next 1
#define LEVEL_LazySubList_compute 1

#define LEVEL_ClauseTree_Node_traverse_subtrees 2
#define LEVEL_lookupInverseType 1
#define LEVEL_generateInverseType 1
#define LEVEL_branchInverseType 1
//#define LEVEL_branchInverseTypeTryTermPatterns 2
#define LEVEL_lookupWrapper 1
#define LEVEL_lookup 1
#define LEVEL_branch 1
#define LEVEL_branchTryTargetNodes 2
//#define LEVEL_ClauseTree_Node_traverse_subtreesAdapter 1
#define LEVEL_ClauseTree_Node_traverse 1
#define LEVEL_ClauseTree_Node_handle_return 2

#define LEVEL_ClauseTree_Node_traverse_subtreesNS 2
#define LEVEL_lookupInverseTypeNS 1
#define LEVEL_generateInverseTypeNS 1
#define LEVEL_branchInverseTypeNS 1
#define LEVEL_branchInverseTypeTryTermPatternsNS 2
#define LEVEL_lookupWrapperNS 1
#define LEVEL_lookupNS 1
#define LEVEL_branchNS 1
#define LEVEL_branchTryTargetNodesNS 2
#define LEVEL_ClauseTree_Node_traverse_subtreesAdapterNS 1
#define LEVEL_ClauseTree_Node_traverseNS 1
#define LEVEL_ClauseTree_Node_traverse_dispatchNS 1

	PROTO(mainLoop, (ClauseTree *))
    PROTO(main, (int, Term *, ClauseTree *, Trie *))
    PROTO(lookupInverseType, (int, Trie *, PMVMInstruction *, Sub *, bool), Trie *, Sub *, int)
	PROTO(branchInverse, (
    		int, TermCacheType *, Trie *, int
    	), TermCacheType *, Trie *, int)
	PROTO(branchInverseTryFuncSymbols, (
			TermCacheType *, int
		), TermCacheType *, Trie *, int)
    PROTO(lookupWrapper, (
    		bool, int, PMVMInstruction *, Sub *, int
    	), Trie *, Sub *)
	PROTO(branchInverseType, (
			int, TermCacheType *, Trie *, PMVMInstruction *
		), TermCacheType *, Trie *, int)
	PROTO(lookup, (
			int, Trie *, PMVMInstruction *, Sub *
		), Trie *, Sub *, int)
	PROTO(branch, (
    		int, Trie *, Trie *, int
    	), Trie *, Term *, int)
	PROTO(branchTryTargetNodes, (
			Trie *
		), Trie *, Term *, int)
	PROTO(ClauseTree_Node_traverse_subtrees, (
			ClauseTree::Node *, Sub *, LinkedStack<Term *> *
		), ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
	PROTO(generateInverseType, (
			int, TermCacheType *, Trie *, PMVMInstruction *, LinkedStack<TermCacheSeg *> *
		), TermCacheType*, Trie *, int)
	PROTO(ClauseTree_Node_traverse,
    (
        ClauseTree::Node *,
        Sub *,
        LinkedStack<Term *> *
    ),
    ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
    /*PROTO(branchInverseType,
	(
			int,
			TermCacheType *,
			Trie *,
			PMVMInstruction *
	), TermCacheType *, Trie *, int)
	PROTO(branchInverseType,(
			int, TermCacheType *, Trie *, PMVMInstruction *
	), TermCacheType *, Trie *, int)*/
//	PROTO(branchInverseTypeTryTermPatterns, (
//		LinkedList<Term *>::Node *
//	), TermCacheType *, Trie *, int)

    PROTO(ClauseTree_Node_traverse_subtreesNS, (unsigned int), ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
    PROTO(ClauseTree_Node_traverse_subtreesAdapterNS, (ClauseTree::Node *,Trie *,Version,Sub *), ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
    PROTO(lookupInverseTypeNS, (int, Trie *, PMVMInstruction *, Sub *, Version, Version, bool), Trie *, Sub *, int)
    PROTO(lookupWrapperNS, (
    		bool, int, Trie *, PMVMInstruction *, Sub *, Version, Version
    	), Trie *, Sub *)
	PROTO(branchInverseTypeNS, (
			int, TermCacheType *, Trie *, PMVMInstruction *
		), TermCacheType *, Trie *, int)
	PROTO(lookupNS, (
			int, Trie *, PMVMInstruction *, Sub *, Version, Version
		), Trie *, Sub *, int)
	PROTO(branchNS, (
    		int, Trie *, Trie *, int
    	), Trie *, Term *, int)
	PROTO(branchTryTargetNodesNS, (
			Trie *
		), Trie *, Term *, int)
	PROTO(ClauseTree_Node_traverse_dispatchNS, (
			ClauseTree::Node *, Trie *, Sub *, Version, LinkedStack<Term *> *
		), ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
	PROTO(generateInverseTypeNS, (
			int, TermCacheType *, Trie *, PMVMInstruction *, LinkedStack<TermCacheSeg *> *
		), TermCacheType*, Trie *, int)
	PROTO(ClauseTree_Node_traverseNS,
    (
        ClauseTree::Node *,
        Trie *,
        Version,
        Sub *,
        LinkedStack<Term *> *
    ),
    ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
    PROTO(branchInverseTypeNS,
	(
			int,
			TermCacheType *,
			Trie *,
			PMVMInstruction *
	), TermCacheType *, Trie *, int)
	PROTO(branchInverseTypeNS,(
			int, TermCacheType *, Trie *, PMVMInstruction *
	), TermCacheType *, Trie *, int)
	PROTO(branchInverseTypeTryTermPatternsNS, (
		LinkedList<Term *>::Node *
	), TermCacheType *, Trie *, int)
PROTO(ClauseTree_Node_handle_return,
	(
			Trie *,
			Sub *
	), ClauseTree::Node *, Sub *, LinkedStack<Term *> *)



    SEC_BEGIN(main, (testId, p, clauseTree, trie))

    int tempDelFlag;

#ifdef DEBUG_VERIFY_THREAD_GEN_NO_INST
#define VERIFY_SEARCH_RESULT \
	noInstSet = false; \
    BRANCH_POINTER = gbp_anchor; /* reset branch_pointer to prevent inconsistency in threads */ \
	SAVE_STATE(PASTE(rp, __LINE__)); \
	bp = BRANCH_POINTER; \
	FUNC_CALL(ClauseTree_Node_traverse_subtreesAdapterNS, (VAR(clauseTree)->root, &modelTrieRoot, (Version)0, new Sub()), retClauseTreeNode, retsub, instStackNew ); \
	inst = processClauseInstance(instStackNew, retsub, retClauseTreeNode); \
	inst->toString(buf, symtable, 0); \
	printf("%d: expected [%d] %s\n", counter, inst->clauseSize(), buf); \
	fflush(stdout); \
	noInst = false; \
	noInstSet = true; \
	CoroutineState *bpNew = BRANCH_POINTER; \
	while(bpNew != bp) { \
		bpNew->delFlag = 0x8; /* mark as garbage */ \
		bpNew = bpNew->branchPointer; \
	} \
	RESTORE_STATE; \
PASTE(rp, __LINE__): \
	if(!noInstSet) { \
		noInst = true; \
		noInstSet = true; \
	} \
	gbp_anchor->delFlag = 0; \
	gbp_anchor->threadId = -1024; \

#else
#define VERIFY_SEARCH_RESULT \

#endif
#define FUN mainLoop
    PARAMS(
            ClauseTree *, clauseTree)
    RETURNS()
    BEGIN
    DEFS()
    	globalProverStatus = RUNNING;
    	gbp_anchor = NULL;
    	counter = 0;
    	// this optimization allows restart searching for an instance from a term size that is larger than 1
    	// if the smallest counterexample to a model composed of eligible literal set A is a clause which generate eligible literal L,
    	// then the smallest counterexample to a model A + {L} - B should has a size >= min(|L|,min({|D| : D in B}))
    	restartSize = VAR(clauseTree)->root->branchMinLiteralSize;
    	startTime = clock();
    	maxTime = startTime + globalTimeLimit * CLOCKS_PER_SEC;
		dataThread = NULL;
#ifdef PROFILE_MODEL
    	modelTime = 0;
#endif

		// initialize the stack at first call
		SAVE_STATE_ANCHOR(rp_mainLoop);
		gbp_anchor = reinterpret_cast<CoroutineState *>(GLOBAL_BRANCH_POINTER);
		gbp_anchor->threadId = -1024;
		// memset(threadHeads, 0, sizeof(CoroutineState) * MAX_TERM_SIZE);
		for(int j=0; j<(MAX_TERM_SIZE+1)*MAX_RELEVANCE; j++) {
			threads[j] = gbp_anchor;
#ifdef INSERT_THREAD_HEAD
			threadsHead[j] = NULL;
#endif
		}
		universalTermSizeLimit = restartSize;
#ifdef USE_RELEVANCE
		universalRelevanceLimit = 0;
#ifdef DEBUG_RELEVANCE
		printf("s%d, r%d\n", universalTermSizeLimit, universalRelevanceLimit);
#endif
		currThreadId = 1 * (MAX_RELEVANCE + 1);
#else
		currThreadId = universalTermSizeLimit;
#endif

#ifdef DEBUG_RESTART_SIZE
		printf("restart size = %d\n", restartSize);
#endif
#ifdef DEBUG_INSTANCE_CLOCK
		currentTime = clock();
		printf("inst clock = %ld\n", currentTime);
#endif

#ifdef DEBUG_TERM_SIZE_LIMIT
		printf("term size limit = %d\n", universalTermSizeLimit);
#endif
		currentTime = clock();
#ifdef DEBUG_CLOCK
		printf("clock = %ld\n", currentTime);
#endif
		if(currentTime >= maxTime) {
			globalProverStatus = TIMED_OUT;
			printProverStatus("time out", counter, startTime, currentTime);
			RETURN();
		}

		// verify search result
		VERIFY_SEARCH_RESULT;
#ifdef DEBUG_UNIVERSAL_TERM_SIZE_LIMIT
			printf("set universal literal size limit = %d\n", universalTermSizeLimit);
#endif

		// if we use the threshold, we only need to restore the state
		// because all branches that may have potential instances but are too large are also saved
		// because we don't reenter the mainloop, we can use global variables in the mainloop
		// and we need to use global variables to ensure that the values used in the mainloop are not reset when RESTORE_STATE is called

			// memory alloc sub root
		FUNC_CALL(ClauseTree_Node_traverse_subtrees, (VAR(clauseTree)->root, new (subRegion) Sub(), (LinkedStack<Term *> *) NULL), retClauseTreeNode, retsub, instStackNew );
		counter ++;
		// increment trie version
		// All computation that are out dated has to be redone
		// update starting index and

		inst = processClauseInstance(instStackNew, retsub, retClauseTreeNode);
#ifdef DEBUG_INSTANCE
		// print instance
		inst->toString(buf, symtable, 0);
		printf("%d: instance [s%d, r%d] %s\n", counter, inst->clauseSize(),
#ifdef USE_RELEVANCE
				inst->relevance,
#else
				0,
#endif
				buf);
		currentTime = clock();
		printf("%d: instance clock = %f\n", counter, (currentTime - startTime)/((double)CLOCKS_PER_SEC));
		fflush(stdout);
#endif
#ifdef DEBUG_VERIFY_GENERATED_INSTANCE
		printf("verify instance\n");
		checkInstance(inst, false);
#endif
#ifdef DEBUG_CHECK_THREADS
		printf("post inst generation checking thread [%d]\n", currThreadId);
		printf("global_bp = %p\n", GLOBAL_BRANCH_POINTER);
		printThread(threads[currThreadId]);
		checkThread(threads[currThreadId]);
		printf("no error\n");
#endif

		// ordered resolution
		orderedResolve(&inst);
#ifdef DEBUG_RESOLVENT
		inst->toString(buf, symtable, 0);
		printf("%d: resolvent %s\n", counter, buf);
#endif
		if(inst->numberOfLiterals == 0) { // empty clause found
			globalProverStatus = PROOF_FOUND;
			currentTime = clock();
			printProverStatus("proof found\n", counter, startTime, currentTime);
			RETURN();
		} else {

#ifdef INSERT_THREAD_HEAD
			// clear to force recalculation of all thread heads
			for(int j=0; j<=MAX_TERM_SIZE; j++) {
				threadsHead[j] = NULL;
			}
#endif
#ifdef DEBUG_VERIFY_RESOLVENT
			printf("verify resolvent\n");
			checkInstance(inst, false);
#endif
#ifdef PROFILE_MODEL
			modelStart = clock();
#endif
			insertClauseInstanceIntoModel(inst);
			/*restartSize = */eagerCascadingDeleteClauseFromModel(inst, inst->literals[0]);
#ifdef PROFILE_MODEL
			modelEnd = clock();
			modelTime += (modelEnd - modelStart);
#endif
#ifdef DEBUG_VERIFY_EL_CLAUSES
			checkEligibleLiterals();
#endif
#ifdef DEBUG_ELIGIBLE_LITERALS
			printEligibleLiterals();
#endif
		}
#ifdef DEBUG_STACK_USAGE
		printf("\ninstance stack usage: %fK\n", (STACK_TOP - STACK_BOTTOM)/1024.0);
#endif

		if(GLOBAL_BRANCH_POINTER == NULL) {
			globalProverStatus = SATURATED;
			currentTime = clock();
			printProverStatus("saturated", counter, startTime, currentTime);
			RETURN();
		} else {
			universalTermSizeLimit = restartSize;
#ifdef USE_RELEVANCE
			universalRelevanceLimit = 0;
#ifdef DEBUG_RELEVANCE
			printf("s%d, r%d\n", universalTermSizeLimit, universalRelevanceLimit);
#endif
			currThreadId = 1 * (MAX_RELEVANCE + 1);
#else
			currThreadId = universalTermSizeLimit;
#endif
			// verify search result
			VERIFY_SEARCH_RESULT;

			BRANCH_POINTER = threads[currThreadId];
#ifdef DEBUG_CHECK_THREADS
			printf("switch to thread [%d]:\n", currThreadId);
			printf("global_bp = %p\n", GLOBAL_BRANCH_POINTER);
			printThread(BRANCH_POINTER);
			checkThread(threads[currThreadId]);
#endif

			currentTime = clock();
	#ifdef DEBUG_CLOCK
			printf("clock = %ld\n", currentTime);
	#endif
			if(currentTime >= maxTime) {
				globalProverStatus = TIMED_OUT;
				printProverStatus("time out", counter, startTime, currentTime);
				RETURN();
			}
			if(TOTAL_MEMORY_USAGE > globalCoroutineStackSize) {
				printf("memory full, restart\n");
				printMemoryUsage(false);
				// try to restart
				//Region *newClauseRegion = make_region();
				assert(LEVEL_OF_NESTING >= 1);
				ActivationRecordCommon *arbs[LEVEL_OF_NESTING];
				memset(arbs, 0, sizeof(ActivationRecordCommon *) * LEVEL_OF_NESTING);
				arbs[0] = AR_BOTTOM_N(1);
				for(int i = 1; i < LEVEL_OF_NESTING; i++) {
					arbs[i] = NULL;
				}
				gc(arbs);
				AR_BOTTOM_N(1) = arbs[0];
				printf("after restarting\n");
				printMemoryUsage(false);
				if(TOTAL_MEMORY_USAGE > globalCoroutineStackSize) {

					globalProverStatus = MAX_MEMORY;
					printProverStatus("max memory", counter, startTime, currentTime);
					RETURN();
				}
			}

			RESTORE_STATE;
		}
	rp_mainLoop:
		// must reset thread to gbp_anchor no need to reset thread id and gbp_anchor, since it is an anchor state
		threads[currThreadId] = gbp_anchor;
#ifdef USE_RELEVANCE
		if(universalRelevanceLimit < universalRelevanceMax) {
			universalRelevanceLimit++;
		} else {
#ifdef DEBUG_VERIFY_THREAD_GEN_NO_INST
		if(!noInst) {
			printf("skipping instant of size %d\n", universalTermSizeLimit);
			exit(-1);
		}
#endif
			universalRelevanceLimit = 0;
#endif
			universalTermSizeLimit ++;
			if(universalTermSizeLimit > MAX_TERM_SIZE) {
				globalProverStatus = MAXED_OUT;
				currentTime = clock();
				printProverStatus("max term size", counter, startTime, currentTime);
				RETURN();
			}
#ifdef USE_RELEVANCE
		}
#endif
#ifdef USE_RELEVANCE
		#ifdef DEBUG_RELEVANCE
		printf("s%d, r%d\n", universalTermSizeLimit, universalRelevanceLimit);
#endif
		currThreadId = universalTermSizeLimit * (MAX_RELEVANCE+1) + universalRelevanceLimit;
#else
		currThreadId = universalTermSizeLimit;
#endif
		if(GLOBAL_BRANCH_POINTER == NULL) { // if there is no more saved state left
			globalProverStatus = SATURATED;
			currentTime = clock();
			printProverStatus("saturated", counter, startTime, currentTime);
			RETURN();
		} else { // if there are still save state with higher branch threshold
			// restore anchor state
			// switch to thread i
			BRANCH_POINTER = threads[currThreadId];
#ifdef DEBUG_CHECK_THREADS
			printf("switch to thread [%d]:\n", currThreadId);
			printf("global_bp = %p\n", GLOBAL_BRANCH_POINTER);
			printThread(BRANCH_POINTER);
			checkThread(threads[currThreadId]);
#endif
#ifdef DEBUG_THREAD
			printf("try thread %d\n", currThreadId);
#endif
			currentTime = clock();
	#ifdef DEBUG_CLOCK
			printf("clock = %ld\n", currentTime);
	#endif
			if(currentTime >= maxTime) {
				globalProverStatus = TIMED_OUT;
				printProverStatus("time out", counter, startTime, currentTime);
				RETURN();
			}
			if(TOTAL_MEMORY_USAGE > globalCoroutineStackSize) {
				printf("memory full, restart\n");
				printMemoryUsage(false);
				// try to restart
				//Region *newClauseRegion = make_region();
				assert(LEVEL_OF_NESTING == 2);
				ActivationRecordCommon *arbs[2] = { AR_BOTTOM_N(1), AR_BOTTOM_N(2) };
				gc(arbs);
				AR_BOTTOM_N(1) = arbs[0];
				AR_BOTTOM_N(2) = arbs[1];
				printf("after restarting\n");
				printMemoryUsage(false);
				if(TOTAL_MEMORY_USAGE > globalCoroutineStackSize) {

					globalProverStatus = MAX_MEMORY;
					printProverStatus("max memory", counter, startTime, currentTime);
					RETURN();
				}
			}
			RESTORE_STATE;
		}
	END
#undef FUN

    #define FUN main
    PARAMS(
            int, testId,
            Term *, p,
            ClauseTree *, clauseTree,
            Trie *, trie)
    RETURNS()
    BEGIN
    switch(VAR(testId)) {
    case 1: // term + trie
//        universalTermSizeLimit = 100;
//        VAR(p)->toString(buf, symtable);
//        printf("finding instances of %s\n", buf);
//        NEW(TermInstances, (false, VAR(p), VAR(trie), (Version)0, new Sub()), ti);
//        tiItr = ti->iterator();
//        counter = 0;
//        while(1) {
//            FUNC_CALL(TermInstanceIterator_next, (tiItr), retsub);
//            if(retsub == NULL) {
//                break;
//            }
//            counter ++;
//            printByType(retsub);
//        }
//
//        assert(counter == 7);
//
//        printf("finding more instances of %s\n", buf);
//        counter = 0;
//        while(1) {
//            FUNC_CALL(TermInstanceIterator_next, (tiItr), retsub);
//            if(retsub == NULL) {
//                break;
//            }
//            counter ++;
//            printByType(retsub);
//        }
//
//        assert(counter == 0);
//
//        printf("finding instances of %s from start again\n", buf);
//        tiItr = ti->iterator();
//        counter = 0;
//        while(1) {
//            FUNC_CALL(TermInstanceIterator_next, (tiItr), retsub);
//            if(retsub == NULL) {
//                break;
//            }
//            counter ++;
//            printByType(retsub);
//        }
//
//        assert(counter == 7);
//
//        version = VAR_VERSION_OFFSET;
//
//        printf("finding instances of %s from start, with version %lX\n", buf, version);
//        NEW(TermInstances, (false, VAR(p), VAR(trie), version, new Sub()), ti);
//        tiItr = ti->iterator();
//        counter = 0;
//        while(1) {
//            FUNC_CALL(TermInstanceIterator_next, (tiItr), retsub);
//            if(retsub == NULL) {
//                break;
//            }
//            counter ++;
//            printByType(retsub);
//        }
//
//        assert(counter == 7);
//
//        printf("finding more instances of %s, with version %lX\n", buf, version);
//        counter = 0;
//        while(1) {
//            FUNC_CALL(TermInstanceIterator_next, (tiItr), retsub);
//            if(retsub == NULL) {
//                break;
//            }
//            counter ++;
//            printByType(retsub);
//        }
//
//        assert(counter == 0);
//
//        printf("finding instances of %s from start, unfoldLazyList\n", buf);
//        NEW(TermInstances, (false, VAR(p), VAR(trie), version, new Sub()), ti);
//        counter = 0;
//        FUNC_CALL(unfoldLazyList, (ti->subList), retnode, retsub);
//        if(retsub != NULL) {
//            counter ++;
//            printByType(retsub);
//            RESTORE_STATE;
//        }
//
//        assert(counter == 7);
//
//        printf("branching pointer = %p\n", BRANCH_POINTER);
//        assert(BRANCH_POINTER == 0);
//
//        printf("finding more instances of %s, unfoldLazyList\n", buf);
//        printf("finding instances of %s from start again, unfoldLazyList\n", buf);
//        counter = 0;
//        FUNC_CALL(unfoldLazyList, (ti->subList), retnode, retsub);
//        if(retsub != NULL) {
//            counter ++;
//            printByType(retsub);
//            RESTORE_STATE;
//        }
//
//        assert(counter == 7);
//        // assert(counter == 0);
        break;
    case 2: // inverse term + trie
//    	{
//    	int numOfTermsAdj[6];
//    	numOfTermsAdj[1]= 0;
//    	numOfTermsAdj[2]= 0;
//    	numOfTermsAdj[3]= 3; // numberOfDistinctTerms(1) * numberOfDistinctTerms(1) * 2 - 1
//    	numOfTermsAdj[4]= 9; // numOfTermsAdj[3] + numberOfDistinctTerms(1) * numberOfDistinctTerms(2) * 2 - 2;
//    	numOfTermsAdj[5]= numOfTermsAdj[4] + numberOfDistinctTerms(1) * numberOfDistinctTerms(3) * 2 + numberOfDistinctTerms(2) * numberOfDistinctTerms(2) - 3;
//    	for(int i=1; i<=5; i++) {
//    		// raise literal size limit
//			universalTermSizeLimit = i;
//			int expected = numOfTermsAdj[i];
//			printf("set universal literal size limit = %d, expected = %d\n", i, expected);
//			VAR(p)->toString(buf, symtable);
//			printf("finding instances of %s\n", buf);
//			NEW(TermInstances, (true, VAR(p), VAR(trie), (Version)0, new Sub()), ti);
//			tiItr = ti->iterator();
//			counter = 0;
//			while(1) {
//				FUNC_CALL(TermInstanceIterator_next, (tiItr), retsub);
//				if(retsub == NULL) {
//					break;
//				}
//				counter ++;
//				printByType(retsub);
//			}
//
//			printf("%d == %d\n", counter, expected);
//			assert(counter == expected);
//
//			printf("finding instances of %s from start, with version %lX\n", buf, version);
//			NEW(TermInstances, (true, VAR(p), VAR(trie), version, new Sub()), ti);
//			tiItr = ti->iterator();
//			counter = 0;
//			while(1) {
//				FUNC_CALL(TermInstanceIterator_next, (tiItr), retsub);
//				if(retsub == NULL) {
//					break;
//				}
//				counter ++;
//				printByType(retsub);
//			}
//
//			assert(counter == expected);
//    	}
//    	}
		break;
    case 3: // clause tree + trie
    	universalTermSizeLimit = 100;
    	counter = 0;
    	SAVE_STATE(rp_main_3);
    	subNew = new (subRegion) Sub();
    	FUNC_CALL(ClauseTree_Node_traverse_subtrees, (VAR(clauseTree)->root, subNew, (LinkedStack<Term *> *) NULL), retClauseTreeNode, retsub, instStackNew );
		counter ++;
		printByType(retsub);
		RESTORE_STATE;
	rp_main_3:
    	assert(counter == 26);
    	break;
    case 4: // clause tree + trie
    	universalTermSizeLimit = 4;
    	counter = 0;
    	SAVE_STATE(rp_main_4);
    	subNew = new (subRegion) Sub();
    	FUNC_CALL(ClauseTree_Node_traverse_subtrees, (VAR(clauseTree)->root, subNew, (LinkedStack<Term *> *) NULL), retClauseTreeNode, retsub, instStackNew );
		counter ++;
		// print clause
		while(retClauseTreeNode->literal!=NULL) {
			char buf[STRING_BUF_SIZE];
			Term *t = retClauseTreeNode->literal;
			t = (*t)(retsub);
			t->toString(buf, symtable);
			printf("%s%s ", retClauseTreeNode->sign?"":"~", buf);
			retClauseTreeNode = retClauseTreeNode ->super;
		}
		printf("\n");
		printByType(retsub);

		RESTORE_STATE;
	rp_main_4:
    	assert(counter == 31);
    	break;
    case 100:
    	FUNC_CALL(mainLoop, (VAR(clauseTree)));
    	break;
    }
	printf("\n\nstack usage: %fK\n", (STACK_TOP - STACK_BOTTOM)/1024.0);
    END
    #undef FUN
    typedef LazyList<Trie, Sub> LazyList_Trie_Sub;


#include "fun.traverse.h"
#include "fun.lookup.h"
#include "fun.inverse.lookup.h"
#include "fun.traverse.noState.h"
#include "fun.lookup.noState.h"
#include "fun.inverse.lookup.noState.h"
	/*    // precond: varnode != NULL
	    #define FUN lookupTryTargetVarNodes
	        PARAMS(
	        		int, n,
	                Trie *, varcnode,
	                PMVMInstruction *, inp,
	                Sub *, sub,
	                Version, varVersionSource,
	                Version, varVersionTarget,
	                int, numberOfPlaceholders)
	        RETURNS(Trie *, Sub *)
	    BEGIN

	    	int subtermSize;
	        subterm = VAR(sub)->lookup(VAR(varcnode)->key, version); // look up the key of the target var node
	        if(subterm != NULL) { // target var is bound
	            // unify current source term with the term that the target var is bound to
	            subNew = unify(VAR(inp)->subterm, subterm, VAR(sub), VAR(varVersionSource), version);
	            subtermSize = subterm->termSize(); // inefficient & inaccurate, needs revising
	        } else { // target var is free
	            subNew = VAR(sub)->push(VAR(varcnode)->key, VAR(inp)->subterm, VAR(varVersionSource));
	            subtermSize = VAR(inp)->subtermSize; // inaccurate, needs revising
	        }
	        if(VAR(varcnode)->sibling==NULL) { // no more vars, no more backtracking
	            if(subNew!=NULL) {
	                FUNC_TAIL_CALL(lookup,
						(
							VAR(n) - subtermSize,
							VAR(varcnode),
							VAR(inp)+VAR(inp)->subtermSize,
							subNew,
							VAR(varVersionSource),
							VAR(varVersionTarget),
							VAR(numberOfPlaceholders) - 1
						)
					);
	            } else {
	                RESTORE_STATE;
	            }
	        } else {
	            if(subNew!=NULL) {
	                SAVE_STATE(rp2);
	                FUNC_TAIL_CALL(lookup,
						(
							VAR(n) - subtermSize,
							VAR(varcnode),
							VAR(inp)+VAR(inp)->subtermSize,
							subNew,
							VAR(varVersionSource),
							VAR(varVersionTarget),
							VAR(numberOfPlaceholders) - 1
						)
					);
	            }
	        rp2:
	            // try other target var nodes
	            FUNC_TAIL_CALL(lookupTryTargetVarNodes,
					(
						VAR(n),
						VAR(varcnode)->sibling,
						VAR(inp),
						VAR(sub),
						VAR(varVersionSource),
						VAR(varVersionTarget),
						VAR(numberOfPlaceholders)
					)
				);
	        }

	    END
	    #undef FUN*/
/********************************************************
 * currently not in use                                 *
 ********************************************************/
/*#define FUN CONSTRUCTOR(TermInstances)
    PARAMS(
    		bool, inverse,
            Term *, term,
            Trie *, trie,
            Version, varVersionTarget,
            Sub *, subInit
            )
	RETURNS(TermInstances *)
    BEGIN
    {
        TermInstances *ti = new TermInstances(VAR(term), VAR(trie), VAR(varVersionTarget), VAR(subInit));
        ActivationRecordCommon *arBottomStartFrame;
		CoroutineState *branchPointer;
        FUNC_CALL_INIT(arBottomStartFrame, branchPointer,
                lookupWrapper, (
                		VAR(inverse),
						(int) ti->term->termSize(),
						ti->trie,
						ti->term->inp, // need to be updated to use the separately stored inp in ClauseTree::Node
						ti->subInit,
						(Version)0,
						VAR(varVersionTarget),
						(LinkedStack<CoroutineState *> *) NULL

				)
		);
        ti->subList = new LazyList_Trie_Sub(arBottomStartFrame, branchPointer);
        RETURN(ti);
    }
    END
#undef FUN
    // Unfold a lazy list
    #define FUN unfoldLazyList
    PARAMS(
    		LazyList_Trie_Sub *, currSubList)
    RETURNS(Trie *, Sub *)
    BEGIN
        if(VAR(currSubList)->compute) {
            FUNC_CALL(LazySubList_compute, (VAR(currSubList)),
                    retnode, subNew);
        }
        if(VAR(currSubList)->nil) {
            RETURN((Trie *) NULL, (Sub *) NULL);
        } else {
            SAVE_STATE(rp_unfoldLazyList);
            RETURN(VAR(currSubList)->node.computed.headLink, VAR(currSubList)->node.computed.head);
        rp_unfoldLazyList:
            FUNC_TAIL_CALL(unfoldLazyList, (VAR(currSubList)->node.computed.tail));
        }
    END
    #undef FUN

    #define FUN TermInstanceIterator_next
        PARAMS(
                TermInstanceIterator *, iterator
                )
        RETURNS(Sub *)
    BEGIN
        if(VAR(iterator)->currSubList->compute) {
            FUNC_CALL(LazySubList_compute, (VAR(iterator)->currSubList),
                    retnode, subNew);
        }
        if(VAR(iterator)->currSubList->nil) {
            RETURN((Sub *) NULL);
        } else {
            subNew = VAR(iterator)->currSubList->node.computed.head;
            VAR(iterator)->currSubList = VAR(iterator)->currSubList->node.computed.tail;
            RETURN(subNew);
        }
    END
    #undef FUN

    #define FUN LazySubList_compute
        PARAMS(
                LazyList_Trie_Sub *, subList
                )
        RETURNS(Trie *, Sub *)
    BEGIN
    {
        ActivationRecordCommon *arBottomStartFrame;
        CoroutineState *branchPointer;
        arBottomStartFrame = VAR(subList)->node.uncomputed.arBottomStartFrame;
        branchPointer = VAR(subList)->node.uncomputed.branchPointer;
        FUNC_CALL_RESUME(arBottomStartFrame, branchPointer, arBottomStartFrame, branchPointer,
                retnode, subNew);
        if(subNew == NULL) { // retnode maybe NULL in inverse lookup
            VAR(subList)->setComputedNil();
        } else {
            VAR(subList)->setComputed(retnode, subNew, new LazyList_Trie_Sub(arBottomStartFrame, branchPointer));
        }
    }
    END
    #undef FUN
*/

    SEC_END;
    return;
}

