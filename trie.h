/* 
 * File:   trie.h
 * Author: Hao Xu
 *
 */

#ifndef TRIE_H
#define	TRIE_H

#include "defs.h"
#include "term.h"
#include "region.h"
#include "hashtable.h"
#include "stack.h"
#include "utils.h"
#include "parser.h"
//#include "clauseTree.h"
// #include "trieUtils.h"

struct SubtermPtr;
struct SubChain;
struct TermInstances;
struct TermInstanceIterator;
struct ClauseTree;

    // a trie node has tree set of subtrees depending on the next symbol in the flatterm
    // func: the next symbol is a functions (constant) symbol
    // varFree: the next symbol is a var which is the first occurrence in this flatterm
    // varBound: the next symbol is a var which is not the first occurrence in this flatterm
    struct Trie : RegionAlloc {
        Symbol key;
        Trie *super;
        Trie *prev;
        Trie *sibling;
        Trie *funcChild;
        Trie *funcChildTail;
        int items;
        SubtermPtr *subtermPtrs;
        bool del; // this field is set to true when a trie node is deleted
        bool visited;  // this node has been added to the trie
        CoroutineState *delDependencyNodes;
        CoroutineState *delDependencyNodesTail;
        CoroutineState *siblingDependencyNodes;
        CoroutineState *siblingDependencyNodesTail;
        static Hashtable<Trie *, GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE> *globalTrieNodeHashtable;
        CoroutineState *delThread;

//        int numRemainingKeys;
//        int *remainingKeys; // when child == NULL
    /* The newNode variable is used to return if the node returned is a generated nod, so that we know when to construct to subtermPointer.
     * Its initial value should be 0.
     * Its value should be set to 1 when a newNode is generated. */
        Trie();

        Trie(Trie *super, Symbol key, bool del = false, bool visited = true);

        Trie *insert(Term *term, REGION);

        Trie *remove(Term *term);

        Trie *lookUp(Term *term);

        Trie *lookUpChild(Symbol key);
        
        Trie *lookUpChildAndGenerateDelNode(Symbol key, REGION);

        void toString(char buf[STRING_BUF_SIZE], SymTable &symtable);

        int toStringOffsetIndent(Trie *root, char buf[STRING_BUF_SIZE], int offset, char indent[MAX_TRIE_DEPTH], int trailing, SymTable &symtable, char const *pre, char const *post);

        int toStringOffsetIndentFromHashtable(Trie *root, char buf[STRING_BUF_SIZE], int offset, char indent[MAX_TRIE_DEPTH], int trailing, SymTable &symtable, char const *pre, char const *post);

        void addForwardEdgeFunc(Symbol s, Trie *node);

        void fillInSubtermPtrs(REGION);

        void fillInSubtermPtrs(Term *term, REGION);

        Term *getTerm(REGION);

        void setDel(bool delNew);
        void updateDelDependency();
        void updateSiblingDependency();
        void static clearDataDependency(Trie *root);

    private:
        Trie *__remove(Term *term);
        Trie *__insert(Term *term, REGION);
        Trie **__fillInSubtermPtrs(Term *term, Trie **ppath, bool &newNode, REGION);

    };

    struct SubtermPtr : RegionAlloc {
        Term *subterm;
        Trie *startTrieNode;
        SubtermPtr *next;
        
        SubtermPtr(Term *subterm, Trie *startTrieNode, SubtermPtr *next) :
        subterm(subterm), startTrieNode(startTrieNode), next(next) {}
    };

    void runTest(int testId, Term *p, ClauseTree *clauseTree, Trie *trie, Trie *negTrie, STACK_DEF, REGION);



    struct SubChain {
        Sub *sub;
        SubChain *next;
        SubChain(Sub *sub) : sub(sub), next(NULL) { }
    };

    // a lazy list
    template <typename S, typename T>
    struct LazyList {
        bool compute;	// whether this node need to be computed
        bool nil;		// whether this node is nil. a nil node should have already been computed
        union {
            struct {
                S *headLink;
                T *head;				// the head elem of a computed node
                LazyList<S, T> *tail;		// the tail of the computed node
            } computed;					// a struct holding a computed node
            ThreadState uncomputed;	// the handle to a suspended coroutine for the uncomputed node
            							// the coroutine should return a value of type T* when it yields
            							// the nil node is indicated by a null pointer
        } node;
        /*LazyList() : compute(false), nil(true) {
        }*/
        LazyList(S *headLink, T *head, LazyList<S, T> *tail) : compute(false), nil(false) {
        	this->node.computed.headLink = headLink;
        	this->node.computed.head = head;
            this->node.computed.tail = tail;
        }
        LazyList(ActivationRecordCommon *arBottomStartFrame, CoroutineState *branchPointer) : compute(true), nil(false) {
            this->node.uncomputed.arBottomStartFrame = arBottomStartFrame;
            this->node.uncomputed.branchPointer = branchPointer;
        }
        void setComputed(S *headLink, T *head, LazyList<S, T> *tail) {
            compute = false;

            this->node.computed.headLink = headLink;
			this->node.computed.head = head;
            this->node.computed.tail = tail;
        }
        void setComputedNil() {
            compute = false;
            nil = true;
        }
    }; 

    struct TermInstanceIterator {
        TermInstances *termInstances;
        LazyList<Trie, Sub> *currSubList;
        TermInstanceIterator(TermInstances *ti);
    };

    // Instances of a term that matches some term in a trie.
    // Each instance is represented by a Substitution which is the mgu of the term and some term in the trie.
    // The variables in the trie are versioned.
    // The initial sub is store in subInit.
    struct TermInstances {
        bool complete;
        Version version;
        Term *term;
        Trie *trie;
        Sub *subInit;
        LazyList<Trie, Sub>* subList;
        TermInstances(Term *term, Trie *trie, Version version, Sub *subInit) :
            complete(false),
            version(version),
            term(term),
            trie(trie),
            subInit(subInit),
            subList(NULL) {
        }
        TermInstanceIterator *iterator() {
            return new TermInstanceIterator(this);
        }
    };
    
    void appendDep(CoroutineState *&dep, CoroutineState *&depTail, CoroutineState *curr);

#endif	/* TRIE_H */

