/*
 *      Author: Hao Xu
 */
#include "stack.h"
#include "coroutines.h"
#include "linkedList.h"

unsigned char retVals[RETVAL_BUF_SIZE];
int save_state_counter = 0;
int restore_state_counter = 0;
int ar_dealloc_counter = 0;
int ar_alloc_counter = 0;
int state_dealloc_counter = 0;
int state_alloc_counter = 0;
int dd_dealloc_counter = 0;
int dd_alloc_counter = 0;
size_t dealloc_size_total = 0;
size_t alloc_size_total = 0;
size_t realloc_size_total = 0;

Region *firstGenStackRegion = make_region();
Region *secondGenStackRegion = make_region();

Region *stackRegion = make_region();
Region *coroutineStateRegion = make_region();


void breakHere() {
	printf("break");
}

void flip(CoroutineState *node) {
	if((node->delFlag & 0x1) == 0) {
		// delete
		deleteAndDisableDependencySubtree(node);
	} else {
		// undelete
		undeleteAndEnableDependencySubtree(node);
	}
}
void deleteAndDisableDependencySubtree(CoroutineState *node) {
#ifdef DEBUG_ENABLE_DISABLE
	printf("disable direct dd ");
		printByType(node);
		printf("\n");
#endif
		node->delFlag |= 0x1;
		if((node->delFlag & 0x2) == 0) { // need to mark the subtrees as disabled only if this node was not disabled
			CoroutineState *p = node->child;
			while(p != NULL) {
				markDataDependencySubtreeAsDisabled(p);
				p = p->next;
			}
		}
}
void enableCommon(CoroutineState* node) {
	CoroutineState* p = node->child;
	if (node->delFlag == 0) {
		if (p == NULL && node->threadId > 0) { // when threadId < 0 this node has already been added to a thread or should not be threaded at all
												// when threadId == 0 this node is a data dependency node
		// only leaves are added back to the threads
#ifdef INSERT_THREAD_HEAD
			if(threadsHead[node->threadId] != NULL) {
				node->branchPointer = threadsHead[node->threadId]->branchPointer;
				threadsHead[node->threadId]->branchPointer = node;
				threadsHead[node->threadId] = node;
				node->threadId = -node->threadId;
			} else {
				// look for new thread head
				CoroutineState *h = threads[node->threadId];
				CoroutineState *hprev = h->branchPointer;
				if(hprev == NULL) {
					// empty thread
					node->branchPointer = threads[node->threadId];
					threads[node->threadId] = threadsHead[node->threadId] = node;
					node->threadId = -node->threadId;
				} else {
					while(hprev->branchPointer != NULL) {
						h = hprev;
						hprev = hprev->branchPointer;
					}
					node->branchPointer = hprev;
					h->branchPointer = node;
					threadsHead[node->threadId] = node;
					node->threadId = -node->threadId;
				}
			}
#else
			node->branchPointer = threads[node->threadId];
			threads[node->threadId] = node;
			node->threadId = -node->threadId;
#endif
#ifdef DEBUG_CHECK_THREADS
			printf("checkThread from enableCommon\n");
			printThread(threads[-node->threadId]);
			checkThread (threads[-node->threadId]);
			printf("no error\n");
#endif
		} else {

			while (p != NULL) {
				markDataDependencySubtreeAsEnabled(p);
				p = p->next;
			}
		}
	}
}

void undeleteAndEnableDependencySubtree(CoroutineState *node) {
#ifdef DEBUG_ENABLE_DISABLE
		printf("enable direct dd ");
		printByType(node);
		printf("\n");
#endif
		node->delFlag &= ~(unsigned int)0x1; // the subtree must have been disabled because this node was marked deleted
		enableCommon(node);
}
// this function marks dependency subtree as disabled because of it is super node is disabled. set 2nd bit to 1
void markDataDependencySubtreeAsDisabled(CoroutineState *node) {
	if((node->delFlag & 0x2) == 0) { // disable only if not already disabled
#ifdef DEBUG_ENABLE_DISABLE
		printf("disable super dd ");
		printByType(node);
		printf("\n");
#endif
		node->delFlag |= 0x2;
		CoroutineDataDependency *p = node->child;
#ifdef HEAP_STACK
		// dec ref count
//		if(node->isCoroutineState) {
//			ActivationRecordCommon *arbs[LEVEL_OF_NESTING];
//			CoroutineState *state = (CoroutineState *) node;
//			int nestingLevel = state->nestingLevel;
//			memcpy(arbs, state->arBottoms, sizeof(ActivationRecordCommon *) * LEVEL_OF_NESTING);
//			ActivationRecordCommon *arb;
//			for(;;) {
//				arb = arbs[nestingLevel];
//				if(arb == NULL) {
//					break;
//				}
//				DEC_REF_COUNT_F(arb);
//				if(REF_COUNT_IS_ZERO(arb)) {
//					nestingLevel = arb->callerNestingLevel;
//					arbs[nestingLevel] = arb->arBottoms[arb->callerNestingLevel];
//					STACK_DEALLOC(arb);
//				} else {
//					break;
//				}
//			}
//		}
#endif
		while(p != NULL) {
			markDataDependencySubtreeAsDisabled(p);
			p = p->next;
		}
	}

}

// this function marks dependency subtree as not disabled because of it is super node is disabled. set 2nd bit to 1
void markDataDependencySubtreeAsEnabled(CoroutineState *node) {
	if((node->delFlag & 0x2) != 0) { // enable only if not already enabled
#ifdef DEBUG_ENABLE_DISABLE
		printf("enable super dd ");
		printByType(node);
		printf("\n");
#endif
		node->delFlag &= ~(unsigned int) (0x2);
		enableCommon(node);
	}

}

void printThread(CoroutineState *thread) {
	LinkedList<CoroutineState *> visited;
	printf("\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\n");
	while(thread!=NULL) {
		printByType(thread);
		printf("\n");
		if(visited.contains(thread)) {
			printf("<circular>\n");
			break;
		}
		visited.append(thread);
		thread = thread->branchPointer;
	}
	printf("/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\n");
	fflush(stdout);
}
// true if it is circular
void checkThread(CoroutineState *thread) {
	CoroutineState *p = thread;
	LinkedList<CoroutineState *> visited;
	while(p!=NULL) {
		if(p->threadId > 0) {
			printThread(thread);
			printf("error: positive thread id\n");
			exit(-1);
		}
		if(visited.contains(p)) {
			printThread(thread);
			printf("error: circular thread\n");
			exit(-1);
		} else {
			visited.append(p);
			p = p->branchPointer;
		}
	}
}


#ifdef DEBUG_STACK_ALLOC
LinkedStack<int> *stackAllocatedBlock = NULL;
LinkedStack<int> *stackInvalidatedBlock = NULL;
LinkedStack<int> *stackOverwrittenBlock = NULL;
#endif

CoroutineState::CoroutineState() : delFlag(0), super(NULL), child(NULL), prev(NULL), next(NULL), branchPointer(), threadId(0) {}


ActivationRecordCommon::ActivationRecordCommon() : returnProgramPointer(NULL), returnBranchPointer(NULL), nestingLevel(1), callerNestingLevel(1)
{
	for(int i = 0; i<LEVEL_OF_NESTING; i++) {
		arBottoms[i] = NULL;
	}
}
