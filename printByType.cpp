/*
 *      Author: Hao Xu
 */
#include "defs.h"
#include "expansibleList.h"
#include "term.h"
#include "trie.h"
#include "printByType.h"
#include "symbols.h"

void printByType(Version version) {
    printf("%lX", version);
}
void printByType(ClauseTree::Node *node) {
    char buf[STRING_BUF_SIZE];
    if(node->literal == NULL) {
        printf("ClauseTree::Node(leaf = %d)", node->leaf);

    } else {
    node->literal->toString(buf, symtable);
    printf("ClauseTree::Node(literal = %s, sign = %d, leaf = %d)", buf, node->sign, node->leaf);
    }
}
void printByType(ClauseTree *node) {
    char buf[STRING_BUF_SIZE];
    node->toString(buf, symtable);
    printf("ClauseTree: %s", buf);
}
void printByType(TermInstances *ti) {
    printf("%p", ti);
}
void printByType(TermInstanceIterator *tii) {
    printf("%p", tii);
}
void printByType(LazyList<Trie, Sub> *ll) {
    printf("%p", ll);
}
void printByType(Term *term) {
    char buf[STRING_BUF_SIZE];
    term->toString(buf, symtable);
    printf("%s | ", buf);
    if(term->inp != NULL) {
    	term->inp->toString(buf, symtable);
    	printf("%s", buf);
    } else {
    	printf("<<uncompiled>>");
    }

}
void printByType(Sub *sub) {
	if(sub == NULL) {
		printf("<null>");
	} else {
	char buf[STRING_BUF_SIZE];
	while(sub!=NULL && sub->val!=NULL) {
	    sub->val->toString(buf, symtable);
		printf("?%lX(%s) = %s:%ld, ", (long) sub->key - VAR_ID_OFFSET, symtable[sub->key], buf, (long) sub->version);
	    sub = sub->prev;
	}
	}
    printf("\n");

}
void printByType(PMVMInstruction *fs) {
        printf("FlattermSymbol(symbol = %lX, subtermSize = %d, subterm = %p)", fs->symbol, fs->subtermSize, fs->subterm);

}
void printByType(Trie *trie) {
        printf("%p", trie);

}

void printByType(TermCache *termCacheNode) {
	printf("%p", termCacheNode);
}

void printByType(TermCacheType *termCacheNode) {
	printf("%p", termCacheNode);
}

void printByType(LinkedList<Term *>::Node *node) {
	printf("%p", node);
}

void printByType(unsigned int i) {
	printf("%u", i);
}
void printByType(int i) {
	printf("%d", i);
}
void printByType(CoroutineState *thread) {
	if(thread == NULL) {
		printf("CoroutineState(null)");
	}else{
		printf("CoroutineState(%p)(", thread);
		for(int i =0;i<LEVEL_OF_NESTING;i++) {
			printf("ar_bottom_%d = %p",i,thread->arBottoms[i]);
			printf(", ");
		}
		printf("delFlag = %d, ", thread->delFlag);
		printf("threadId = %d, ", thread->threadId);
		printf("anchor = %d, ", thread->anchor);
		printf("prev = %p,", thread->branchPointer);
		printf("super = %p", thread->super);
		//assert(thread->super!=NULL);
		printf(")");

	}
}
/*void printByType(CoroutineDataDependency *thread) {
	if(thread == NULL) {
		printf("CoroutineDataDependency(null)");
	}else{
		printf("CoroutineDataDependency()");
	}
}*/
