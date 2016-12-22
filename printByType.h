/* 
 * File:   printByType.h
 * Author: Hao Xu
 *
 */

#ifndef PRINTBYTYPE_H
#define	PRINTBYTYPE_H

#include "trie.h"

#include "termCache.h"
#include "clauseTree.h"
#include "linkedList.h"

struct CoroutineState;

void printByType(Version version);
void printByType(ClauseTree::Node *node);
void printByType(ClauseTree *clauseTree);
void printByType(LazyList<Trie, Sub> *ll);
void printByType(TermInstances *ti);
void printByType(TermInstanceIterator *tii);
void printByType(Term *term);
void printByType(Sub *sub);
void printByType(PMVMInstruction *fs);
void printByType(Trie *trie);
void printByType(TermCache *termCacheNode);
void printByType(TermCacheType *termCacheNode);
void printByType(LinkedList<Term *>::Node *node);
void printByType(CoroutineState *thread);
void printByType(CoroutineDataDependency *thread);

void printByType(unsigned int i);
void printByType(int i);

template<typename T>
void printByType(LinkedStack<T> *node) {
	printf("%p", node);
}
template<typename S, typename T, typename U>
void printByType(Triple<S,T,U> *triple) {
	printf("<");
	printByType(triple->fst);
	printf(",");
	printByType(triple->snd);
	printf(",");
	printByType(triple->third);
	printf(">");
}


#endif	/* PRINTBYTYPE_H */

