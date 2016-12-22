/*
 * termCache.cpp
 *
 *      Author: Hao Xu
 */


#include "termCache.h"
#include "hashtable.h"
#include "term.h"
#include "symbols.h"
Region *termMatrixRegion = make_region(0);
Region *termMatrixHashtableRegion = make_region(0);
Region *termCacheRegion = make_region(0);
Hashtable<Term *, -1> termMatrix(TERM_MATRIX_SIZE, termMatrixHashtableRegion);
TermCache globalTermCache;
TermCacheType globalTermCacheType;

Hashtable<TermCacheType *, GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE> *initGlobalTermCacheTypeNodeHashtable() {
	return new Hashtable<TermCacheType *, GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE>(GLOBAL_TERM_CACHE_TYPE_NODE_HASHTABLE_SIZE, termMatrixRegion);
}
Hashtable<TermCacheType *, GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE> *TermCacheType::globalTermCacheTypeNodeHashtable = initGlobalTermCacheTypeNodeHashtable();

void initTermMatrix() {
}

void resetTermMatrix() {
	// todo update
	if(termMatrixRegion!=NULL) {
		region_free(termMatrixRegion);
		termMatrix.clear();
		globalTermCache.clear();
		termMatrixRegion = make_region(0);
	}
}

Term *getFromTermMatrix(Symbol f) {
	return getFromTermMatrix((char *)&f);

}
Term *getFromTermMatrix(Symbol* f, Symbol* &next) {
	char key[MAX_FUNCTION_SYMBOL_DIMENSION * sizeof(Term) +sizeof(Symbol)];
	Symbol *s = (Symbol *) key;
	*s = *f;
	Term **subtermsKey = (Term **) (key + sizeof(Symbol));
	Symbol *nextSubterm = f + 1;
	int n = IS_FUNC_SYMBOL(*s) ? ARITY(*s) : 0;
	for(int i = 0; i < n; i++) {
		subtermsKey[i] = getFromTermMatrix(nextSubterm, nextSubterm);
	}
	next = nextSubterm;
	return getFromTermMatrix(key);
}
Term *getFromTermMatrix(char *key) {
	Symbol s = *(Symbol *) key;
	int n = IS_FUNC_SYMBOL(s) ? ARITY(s) : 0;
	int keySize = sizeof(Symbol) + n * sizeof(Term *);
	Term *t = termMatrix.lookup(key, keySize);
	if(t == NULL) {
		Term **subterms = (Term **) region_alloc(termMatrixRegion, sizeof(Term *) * n);
		memcpy(subterms, key + sizeof(Symbol), sizeof(Term *) * n);
		t = new (termMatrixRegion) Term(s, n, subterms);
		termMatrix.insert(key, t, keySize);
	}
//	char buf[STRING_BUF_SIZE];
//	t->toString(buf, symtable);
//	printf("getTermFromMatrix: %s\n", buf);
	return t;
}
// root node
TermCache::TermCache() :
	key(0),
	super(NULL),
	prev(NULL),
	sibling(NULL),
    funcChild(NULL),
    funcChildTail(NULL),
    term(NULL) {
}

TermCache::TermCache(Symbol key, TermCache *super, Term *term) :
    key(key),
    super(super),
    prev(super->funcChildTail),
    sibling(NULL),
    funcChild(NULL),
    funcChildTail(NULL),
    term(term) {
	if(super->funcChildTail == NULL) {
		super->funcChild = super->funcChildTail = this;
	} else {
        prev->sibling = this;
		super->funcChildTail = this;
	}
}

void TermCache::fillInTerm() {
	Symbol flatterm[MAX_TERM_SIZE];
	Symbol *p = flatterm+MAX_TERM_SIZE;
	TermCache *curr = this;
	while (curr->super != NULL) {
		*(--p) = curr->key;
		curr = curr->super;
	}
	term = getFromTermMatrix(p, p);

}

Term *TermCache::getTerm() {
	if(term == NULL) {
		fillInTerm();
	}
	return term;
}

void TermCache::clear() {
	funcChild = funcChildTail = NULL;
}

TermCacheType::TermCacheType() :
	key(0),
	super(NULL),
	prev(NULL),
	sibling(NULL),
    funcChild(NULL),
    funcChildTail(NULL),
    term(NULL) {
}

TermCacheType::TermCacheType(Symbol key, TermCacheType *super, Term *term) :
    key(key),
    super(super),
    prev(super->funcChildTail),
    sibling(NULL),
    funcChild(NULL),
    funcChildTail(NULL),
    term(term) {
	if(super->funcChildTail == NULL) {
		super->funcChild = super->funcChildTail = this;
	} else {
        prev->sibling = this;
		super->funcChildTail = this;
	}
	HashKeyPtrSymbol hashkey;
	memset(&hashkey, 0, sizeof(HashKeyPtrSymbol));
	hashkey.key = key;
	hashkey.node = super;
	TermCacheType::globalTermCacheTypeNodeHashtable->insert(HASHTABLE_KEY(hashkey), this);
}

void TermCacheType::fillInTerm() {
	Symbol flatterm[MAX_TERM_SIZE];
	Symbol *p = flatterm+MAX_TERM_SIZE;
	TermCacheType *curr = this;
	while (curr->super != NULL) {
		*(--p) = curr->key;
		curr = curr->super;
	}
	//printf("fill in term (size = %d): ", flatterm+MAX_TERM_SIZE-p);
//	for(int i=0;i<flatterm+MAX_TERM_SIZE-p;i++) {
//		printf("%lX(%s) ", p[i], symtable[p[i]]);
//	}
//	printf("\n");
//	fflush(stdout);
	term = getFromTermMatrix(p, p);

}

Term *TermCacheType::getTerm() {
	if(term == NULL) {
		fillInTerm();
	}
	return term;
}

void TermCacheType::clear() {
	funcChild = funcChildTail = NULL;
}

TermCacheType *TermCacheType::lookUpChild(Symbol k) {
	HashKeyPtrSymbol hashkey;
	memset(&hashkey, 0, sizeof(HashKeyPtrSymbol));
	hashkey.key = k;
	hashkey.node = this;
	TermCacheType *ch = (*TermCacheType::globalTermCacheTypeNodeHashtable)[HASHTABLE_KEY(hashkey)];
	if(ch == NULL) {
		// printf("new node %lX\n", k);
		ch = new (termCacheRegion) TermCacheType(k, this);
	}
	return ch;
}



