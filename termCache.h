/*
 * termCache.h
 *
 *      Author: Hao Xu
 */

#ifndef TERMCACHE_H_
#define TERMCACHE_H_
#include "term.h"
#include "defs.h"
#include "hashtable.h"

#define TERM_MATRIX_SIZE 0xFFFFFF

extern Region *termMatrixRegion;
extern Region *termMatrixHashtableRegion;

extern Region *termCacheRegion;

    struct TermCache : RegionAlloc {
        Symbol key;
        TermCache *super;
        TermCache *prev;
        TermCache *sibling;
        TermCache *funcChild;
        TermCache *funcChildTail;
        Term *term;

        TermCache();

        TermCache(Symbol key, TermCache *super, Term *term = NULL);

        void fillInTerm();

        Term *getTerm();

        // clear root node
        void clear();

        void toString(char buf[STRING_BUF_SIZE], SymTable &symtable);

        int toStringOffsetIndent(TermCache *root, char buf[STRING_BUF_SIZE], int offset, char indent[MAX_TRIE_DEPTH], int trailing, SymTable &symtable, char const *pre, char const *post);


    };

#define GLOBAL_TERM_CACHE_TYPE_NODE_HASHTABLE_SIZE (1024 * 1024)

    struct TermCacheType : RegionAlloc {
        Symbol key;
        TermCacheType *super;
        TermCacheType *prev;
        TermCacheType *sibling;
        TermCacheType *funcChild;
        TermCacheType *funcChildTail;
        Term *term;
        static Hashtable<TermCacheType *, GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE> *globalTermCacheTypeNodeHashtable;
        TermCacheType();

        TermCacheType(Symbol key, TermCacheType *super, Term *term = NULL);

        void fillInTerm();

        Term *getTerm();

        // clear root node
        void clear();

        void toString(char buf[STRING_BUF_SIZE], SymTable &symtable);

        int toStringOffsetIndent(TermCache *root, char buf[STRING_BUF_SIZE], int offset, char indent[MAX_TRIE_DEPTH], int trailing, SymTable &symtable, char const *pre, char const *post);

        TermCacheType *lookUpChild(Symbol key);

    };

    extern TermCache globalTermCache;

    extern TermCacheType globalTermCacheType;

    Term *getFromTermMatrix(Symbol* f, Symbol* &next);

    Term *getFromTermMatrix(char *key);

    Term *getFromTermMatrix(Symbol s);

    void initTermMatrix();

    void resetTermMatrix();


typedef Triple<TermCacheType *, TermCacheType *, int> TermCacheSeg;

#endif /* TERMCACHE_H_ */
