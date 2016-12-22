/*
 * superSymbol.cpp
 *
 *      Author: Hao Xu
 */

// #define DEBUG_GEN_SUPER_SYMBOLS

#include "defs.h"
#include "offsetList.h"
#include "superSymbol.h"
#include "symbols.h"
#include "hashtable.h"

/* A super symbol represents a sequence function symbols or predicate symbols.
 * The main motivation of introducing the super symbol is
 * so that looking up a fixed sequence of N function symbols or predicate symbols
 * can be replace by looking up a superSymbol, thereby improving the efficiency.
 * For example, if we have literal p(f(X),g(a,Y)) in the input clause, we can combine
 * p and f into a new super symbol, which we denote by S, and g and a into a new super symbol, which we denote by V.
 * The arity of a super symbol is determined by the minimum number of symbols needed to make a complete term.
 * In our example, S has arity 2, and V has arity 1.
 * In a trie, a super symbol is treated in the same way as a normal function symbol or predicate symbol, except that
 * the edge marked with a super symbol points to a descendant rather then a direct child, making a trie a DAG rather than a tree.
 * The trie is built as a normal trie, until looking up of a super symbol takes place.
 * When a super symbol, say S, representing a sequence of normal symbols, f1, ..., fn, is being looked up from a trie node,
 * the lookup procedure first look up S,
 *     if S is not found, it tries to look up the normal symbols,
 *     	   if a descendant is found, the lookup procedure adds and edge labelled with the super symbol coming from to the current node to the descendant.
 *     	   if a descendant is not found, the lookup procedure does nothing
 *     if S is found, it tries to detect if the descendant is deleted,
 *         if the descendant is deleted, the lookup procedure does nothing
 *         if the descendant is not deleted, the lookup procedure returns the descendant.
 *
 * To support this algorithm, we need a map from super symbols to (finite) sequences of symbols.
 * We generate the ids of super symbols continuously. Therefore, we can use the offsetList data structure.
 * The sequence is zero-terminated.
 */

OffsetList<Symbol, Symbol*> superSymbolToSequenceOfSymbols;

Region *superSymbolRegion = make_region(0);

Hashtable<Symbol, -1> sequenceOfSymbolsToSuperSymbol(10000, superSymbolRegion);

Symbol numSuperSymbol = 0;

template<typename T>
int length(T *seq) {
	T *p = seq;
	while(*p != (T) 0) { p++; }
	return p - seq;

}

Symbol addSuperSymbol(Symbol *sequence) {
	char key[10000]; // a very large array
	char *pKey=key;
	Symbol *pSymbols = sequence;

	while(*pSymbols != (Symbol) 0) {
		const char *symbolName = symtable[*pSymbols];
		strcpy(pKey, symbolName);
		pKey += strlen(symbolName);
		*pKey='|';
		pKey ++;
		pSymbols++;
	}

	*pKey = '\0';

	// printf("looking for super symbol by key %s\n", key);
	Symbol id;
	if((id = sequenceOfSymbolsToSuperSymbol.lookup(key))!=0) {
		return id;
	}
	char buf[1024];
	snprintf(buf, 1024, "SuperSymbol%ld", numSuperSymbol);


	numSuperSymbol++;
	id = updateSymTable(buf, SUPER_SYMBOL_ARITY, funcid, true); // set arity to SUPER_SYMBOL_ARITY

	int len = length(sequence);
	Symbol *sequenceNew = new Symbol[len + 1];
	memcpy (sequenceNew, sequence, sizeof(Symbol) * (len + 1));
	superSymbolToSequenceOfSymbols.set(id, sequenceNew);
	sequenceOfSymbolsToSuperSymbol.insert(key, id);
	return id;
}



/* the subtermSize field of the new PMVMInstruction is invalid */
PMVMInstruction *generateSuperSymbols(PMVMInstruction *inp, OffsetList<Symbol, PMVMInstruction *> *firstOccurVar, REGION) {
	Symbol seq[MAX_TERM_SIZE];
	PMVMInstruction *pp;
	for(pp=inp;pp->subterm!=NULL;pp++) {}

	PMVMInstruction *inst = HEAP_ALLOC(PMVMInstruction, pp-inp+1); // may allocate more space than needed
	int instIndex = 0;
	PMVMInstruction *p = inp, *start = inp;
	for(;;) {
		if(p->subterm==NULL || !IS_FUNC_SYMBOL(p->symbol)) {
			if(p == start) {
			} else if(p == start + 1) {
				inst[instIndex++] = *start;
				start ++;
			} else if(p > start + 1) {
				int i = 0;
				while(start!=p) {
					seq[i++] = start->symbol;
					start ++;
				}
				seq[i] = (Symbol) 0;
				printf("super symbol ");
				for(int k = 0; k<i; k++) {
					printf("%s|", symtable[seq[k]]);
				}
				printf("\n");

				Symbol id = addSuperSymbol(seq);
				inst[instIndex].newVar = false;
				inst[instIndex].subterm = (p-1)->subterm; // must be nonnull, invalid cannot be used
				inst[instIndex].subtermSize = (p-1)->subtermSize; // invalid cannot be used
				inst[instIndex].symbol = id;
				instIndex++;
			}
			if(p->subterm == NULL) {
				break;
			}
			// start must point to a var symbol now
			if(firstOccurVar != NULL && start->newVar) {
				// update firstOccurVar
				firstOccurVar->set(start->symbol, inst + instIndex);
			}
			inst[instIndex++] = *start;
			start ++;
		}
		p++;
	}

	inst[instIndex].subterm = NULL;
#ifdef DEBUG_GEN_SUPER_SYMBOLS
	char buf1[STRING_BUF_SIZE], buf2[STRING_BUF_SIZE];
	inp->toString(buf1, symtable);
	inst->toString(buf2, symtable);
	printf("%s ---> %s\n", buf1, buf2);
#endif
	return inst;
}






