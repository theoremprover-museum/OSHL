/*
 * Author: Hao Xu
 */
#include "trie.h"
#include "term.h"
#include "utils.h"
#include "stack.h"
#include "hashtable.h"
#include "utils.h"
#include "parser.h"
#include "symbols.h"
//#include "basic.h"
#include "printByType.h"
#include "arithmetics.h"

#include <assert.h>

//#define DEBUG_VERIFY_LOOKUP_CHILD
//#define DEBUG_SHOW_TRIE_BY_HASHTABLE

TermInstanceIterator::TermInstanceIterator(TermInstances* ti) :
    termInstances(ti),
    currSubList(ti->subList) {
}

Region *globalTrieHashtableRegion = make_region(0);
Hashtable<Trie *, GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE> *initGTNH() {
	return new Hashtable<Trie *, GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE>(GLOBAL_TRIE_NODE_HASHTABLE_SIZE, globalTrieHashtableRegion);
}
Hashtable<Trie *, GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE> *Trie::globalTrieNodeHashtable = initGTNH();

// root node
Trie::Trie() : 
	key(0),
	super(NULL),
	prev(NULL),
	sibling(NULL),
    funcChild(NULL),
    funcChildTail(NULL),
    items(0),
    subtermPtrs(NULL),
    del(false),
    visited(true),
    delDependencyNodes(NULL),
    delDependencyNodesTail(NULL),
    siblingDependencyNodes(NULL),
    siblingDependencyNodesTail(NULL),
    delThread(NULL) {
}

Trie::Trie(Trie *super, Symbol key, bool del, bool visited) :
    key(key),
    super(super),
    prev(super->funcChildTail),
    sibling(NULL),
    funcChild(NULL),
    funcChildTail(NULL),
    items(0),
    subtermPtrs(NULL),
    del(del),
    visited(visited),
    delDependencyNodes(NULL),
    delDependencyNodesTail(NULL),
    siblingDependencyNodes(NULL),
    siblingDependencyNodesTail(NULL),
    delThread(NULL) {
	if(!del) {
    	if(prev == NULL) {
    		super->funcChild = super->funcChildTail = this;
    	} else {
            prev->sibling = this;
    		super->funcChildTail = this;
    	}
	}
    	HashKeyPtrSymbol hashkey;
    	memset(&hashkey, 0, sizeof(HashKeyPtrSymbol));
    	hashkey.key = key;
    	hashkey.node = super;
    	globalTrieNodeHashtable->insert(HASHTABLE_KEY(hashkey), this);
/*
    node->numRemainingKeys = numRemainingKeys;
    node->remainingKeys = numRemainingKeys == 0? NULL: HEAP_ALLOC(int, numRemaingKeys);
*/
}

//
void Trie::addForwardEdgeFunc(Symbol s, Trie *node) {
    HashKeyPtrSymbol hashkey;
    // Set memory to 0 in case there is padding to align the data,
    // which can not be cleared by assigning values to fields.
    memset(&hashkey, 0, sizeof(HashKeyPtrSymbol));
    hashkey.node = this;
    hashkey.key = s;
	globalTrieNodeHashtable->insert(reinterpret_cast<const char *>(&hashkey), node);
}
Trie *Trie::insert(Term *term, REGION) {
    items ++;
#ifdef DEBUG_TRIE
    char buf[STRING_BUF_SIZE];
    term->toString(buf, symtable);
    printf("insert %s\n", buf);
#endif
    Trie *curr = __insert(term, CURR_REGION);
    curr -> fillInSubtermPtrs(term, CURR_REGION);
#ifdef DEBUG_TRIE
    this->toString(buf, symtable);
    printf("%s", buf);
#endif
    return curr;
}

/* How newNode works:
 * Suppose that we have f(f(a),g(b)) in the trie.
 * We want to add f(f(a),g(a)) in the trie.
 * We already have the following subterm pointers:
 * f(f(a),g(b))
 * ^        |
 * |________|
 * f(f(a),g(b))
 *        ^ |
 *        |_|
 * f(f(a),g(b))
 *          ^ (point to itself)
 *          |
 * f(f(a),g(b))
 *   ^ |
 *   |_|
 * f(f(a),g(b))
 *     ^ (point to itself)
 *     |
 * We must only add subterm pointers originating from the g(a) part of the new term, i.e., pointers orginating from the newly created trie branch.
 * Because the pointer is created by the caller, we need to know whether the callee ends up on a new branch.
 * newNode is used to return that information from the callee.
 * Property: if one callee ends up on a new branch, then all subsequent callees (not necessarily of the same caller) also end up on a new branch.
 * By this property, the value of newNode can only go from 0 to 1, not conversely.
 * Therefore, we only need one variables to store the value.
 */
Trie *Trie::__insert(Term *term, REGION) {
    Symbol key = term->symbol;

    Trie *cnode = lookUpChild(key);

    if(cnode != NULL) {
    	if(cnode->del) {
#ifdef DEBUG_INSERT_REMOVE
    		printf("MARK node %p as undeleted\n", cnode);
#endif
    		cnode->setDel(false);
    		if(!cnode->visited) {
    			cnode->visited = true;
    		// add node to the funcChild
#ifdef DEBUG_INSERT_REMOVE
            	printf("APPEND node %p to funcChild %p\n", cnode, funcChildTail);
#endif
                if(funcChildTail == NULL) {
                	cnode->prev = cnode->sibling = NULL;
                    funcChild = funcChildTail = cnode;
                } else {
                	funcChildTail->updateSiblingDependency();
                	cnode->prev = funcChildTail;
                	funcChildTail->sibling = cnode;
                	cnode->sibling = NULL;
                    funcChildTail = cnode;
                }
    		}
    	}
        cnode->items ++;
        #define F(x) cnode = cnode->__insert(x, CURR_REGION)
        MAP_APP(F, term->symbolDimension, term->subterms);
        #undef F
    } else {
    	if(funcChildTail != NULL) {
			// if funcChild is NULL, it mean that the node "this" is also newly added,
    		// or pregenerated, in either case there has been attempt to visited its subtrees
    		// because no term can be a proper prefix of another term
			funcChildTail->updateSiblingDependency();
    	}
    	cnode = new (CURR_REGION) Trie (this, key);
        cnode->items ++;
        #define F(x) cnode = cnode->__insert(x, CURR_REGION)
        MAP_APP(F, term->symbolDimension, term->subterms);
        #undef F

    }
    return cnode;
}

Trie* Trie::remove(Term *term) {
    items --;
#ifdef DEBUG_TRIE
    char buf[STRING_BUF_SIZE];
    term->toString(buf, symtable);
    printf("remove %s\n", buf);
#endif
    Trie *cnode = __remove(term);
#ifdef DEBUG_TRIE
    this->toString(buf, symtable);
    printf("%s", buf);
#endif
    return cnode;
}

Trie* Trie::__remove(Term *term) {

#ifdef DEBUG_INSERT_REMOVE
	printf("REMOVE: visiting node %p\n", this);
#endif
    Symbol key = term->symbol;
    Trie *cnode = lookUpChild(key);

    if(--cnode->items == 0) {
//        if(cnode->prev != NULL) {
//            if(cnode == funcChildTail) {
//                cnode->prev->sibling = NULL;
//                funcChildTail = cnode->prev;
//            } else {
//                cnode->prev->sibling = cnode->sibling;
//                cnode->sibling->prev = cnode->prev;
//            }
//        } else {
//                if(cnode->sibling == NULL) {
//                    funcChild = funcChildTail = NULL;
//                } else {
//                    cnode->sibling->prev = NULL;
//                    funcChild = cnode->sibling;
//                }
//        }

        // delete cnode
/*        HashKeyTrieNode hashkey;
        memset(&hashkey, 0, sizeof(HashKeyTrieNode)); 
        hashkey.node = this;
        hashkey.key = key;
        globalTrieNodeHashtable->deleteKey((char *)&hashkey);*/
#ifdef DEBUG_INSERT_REMOVE
        printf("MARK node %p as deleted\n", cnode);
#endif
        cnode->setDel(true);
//        Trie *cnode2 = cnode->funcChild;
//        cnode->funcChild = cnode->funcChildTail = NULL;
        // delete all nodes under cnode
//        while(cnode2!=NULL) {
/*            hashkey.node = cnode;
            hashkey.key = cnode->funcChild->key;
            globalTrieNodeHashtable->deleteKey((char *)&hashkey);*/
//        	cnode = cnode2;
#ifdef DEBUG_INSERT_REMOVE
        	printf("MARK node %p as deleted\n", cnode);
#endif
//            cnode->setDel(true);
//            cnode->items = 0;
//            cnode2 = cnode->funcChild;
//            cnode->funcChild = cnode->funcChildTail = NULL;
        }

//        //cnode->updateDataDependency();
//        return NULL;
//    } else {
        int i;
        for(i=0;i<term->symbolDimension;i++) {
            cnode = cnode->__remove(term->subterms[i]);
//            if(cnode == NULL) {
//                return NULL;
//            }
        }
        return cnode;
//    }
}

Trie *Trie::lookUpChildAndGenerateDelNode(Symbol key, REGION) {
	assert(this != NULL);
	Trie *node = lookUpChild(key);
	if(node == NULL) {
		// this node has not been visited yet, so when it is added to the funcChildTail, the sibling data dependencies has to be updated
		node = new (CURR_REGION) Trie(this, key, true, false);
	}
	return node;

}
Trie *Trie::lookUpChild(Symbol key) {
    HashKeyPtrSymbol hashkey;
    memset(&hashkey, 0, sizeof(HashKeyPtrSymbol)); 
    hashkey.key = key;
    hashkey.node = this;
#ifdef DEBUG_VERIFY_LOOKUP_CHILD
    char buf[STRING_BUF_SIZE];
    toString(buf, symtable);
    printf("looking up key %s from:\n%s\n", symtable[key], buf);
    Trie *ret = (*globalTrieNodeHashtable)[(char *)&hashkey];
    printf("hashtable pointer %p\n", ret);
    Trie *cnode = funcChild;
    while(cnode!=NULL) {
    	printf("try func child with key %s\n", symtable[cnode->key]);
        if(cnode->key == key) {
        	printf("matched\n");
            assert(ret == cnode && !ret->del);
            break;
        }
        cnode = cnode->sibling;
    }
    if(cnode == NULL) {
		cnode = varChild;
		while(cnode!=NULL) {
			if(cnode->key == key) {
				assert(ret == cnode && !ret->del);
				break;
			}
			cnode = cnode->sibling;
		}
    }

    if(cnode == NULL) {
    	assert( ret == NULL || ret->del );
    }
#endif
    return (*globalTrieNodeHashtable)[(char *)&hashkey];
}

Trie *Trie::lookUp(Term *t) {
	Trie *cnode = lookUpChild(t->symbol);
	if(cnode == NULL) {
		return NULL;
	}
	for(int i =0;i<t->symbolDimension;i++) {
		cnode = cnode ->lookUp(t->subterms[i]);
		if(cnode == NULL) {
			return NULL;
		}
	}
	return cnode;
}

void Trie::toString(char buf[STRING_BUF_SIZE], SymTable &symtable) {
    char indent[MAX_TRIE_DEPTH];
    indent[0] = '\0';
    int offset = toStringOffsetIndent(this, buf, 0, indent, 0, symtable, "?", "");
#ifdef DEBUG_SHOW_TRIE_BY_HASHTABLE
    toStringOffsetIndentFromHashtable(this,buf,offset,indent,0,symtable,"?","");
#endif
}

int Trie::toStringOffsetIndentFromHashtable(Trie *root, char buf[STRING_BUF_SIZE], int offset, char indent[MAX_TRIE_DEPTH], int trailing, SymTable &symtable, char const *pre, char const *post) {
    int ind = strlen(indent);
    if(ind!=0) {
        if(trailing) {
            indent[ind-1] = '\300';
        } else {
            indent[ind-1] = '\303';
        }
        SPRINT(buf, offset, "%s", indent);
        if(trailing) {
            indent[ind-1] = ' ';
        } else {
            indent[ind-1] = '\263';
        }
    }
    if(this == root) {
        SPRINT(buf, offset, "[root]");
    } else
    if(IS_VAR_SYMBOL(key)) { // var
        SPRINT(buf, offset, "%s%ld%s", pre, key, post);
    } else {
        SPRINT(buf, offset, "%s", symtable[key]);
    }
    SPRINT(buf, offset, "[%d]", items);
    SPRINT(buf, offset, "(%p)", this);
    SPRINT(buf, offset, "(funcChildTail=%p)", this->funcChildTail);
    char termBuf[STRING_BUF_SIZE];
    SubtermPtr *sp = subtermPtrs;
    while(sp != NULL){
        sp->subterm->toString(termBuf, symtable);
        //SPRINT(buf, offset, "->%s{%p}", termBuf, sp->startTrieNode);
        SPRINT(buf, offset, "->%s", termBuf);
        sp=sp->next;
    }
    SPRINT(buf, offset, "\n");
    indent[ind] = '\263';
    indent[ind+1] = '\0';
    HashKeyPtrSymbol hashkey;
    memset(&hashkey, 0, sizeof(HashKeyPtrSymbol));
    hashkey.node = this;
    Trie *cnode;
    for(int i=0; i < numberOfFunctionSymbols; i ++) {
    	Symbol currFuncSymbol = functionSymbols[i];
        hashkey.key = currFuncSymbol;
        cnode = (*globalTrieNodeHashtable)[reinterpret_cast<char *>(&hashkey)];
		if(cnode !=NULL && !cnode->del) {
#ifdef DEBUG_VERIFY_LOOKUP_CHILD
			Trie *cnode2 = funcChild;
			while(cnode2!=NULL) {
				if(cnode2->key == cnode->key) {
					if(cnode != cnode2 || cnode->del) {
						SPRINT(buf, offset, "%p ", cnode);
						SPRINT(buf, offset, "%p ", cnode2);
						SPRINT(buf, offset, "%d ", cnode->del);
						SPRINT(buf, offset, "%s", "!!!missing!!!");
					}
					break;
				}
				cnode2 = cnode2->sibling;
			}
			if(cnode2 == NULL) {
				if(cnode != NULL && !cnode->del) {
					SPRINT(buf, offset, "%s", "!!!phatom!!!");
				}
			}
#endif
			offset = cnode->toStringOffsetIndentFromHashtable(root, buf, offset, indent, cnode->sibling == NULL, symtable, pre, post);
		}
    }
    // we don't consider var child here
    indent[ind] = '\0';

    buf[offset] = '\0'; // remove trailing '('
    return offset;
}

int Trie::toStringOffsetIndent(Trie *root, char buf[STRING_BUF_SIZE], int offset, char indent[MAX_TRIE_DEPTH], int trailing, SymTable &symtable, char const *pre, char const *post) {
    int ind = strlen(indent);
    if(ind!=0) {
        if(trailing) {
            indent[ind-1] = '\300';
        } else {
            indent[ind-1] = '\303';
        }
        SPRINT(buf, offset, "%s", indent);
        if(trailing) {
            indent[ind-1] = ' ';
        } else {
            indent[ind-1] = '\263';
        }
    }
    if(this == root) {
        SPRINT(buf, offset, "[root]");
    } else
    if(IS_VAR_SYMBOL(key)) { // var
        SPRINT(buf, offset, "%s%ld%s", pre, key, post);
    } else {
        SPRINT(buf, offset, "%s", symtable[key]);
    }
    SPRINT(buf, offset, "[%d]", items);
    SPRINT(buf, offset, "(%p)", this);
    SPRINT(buf, offset, "(funcChildTail=%p)", this->funcChildTail);
    SPRINT(buf, offset, "(del=%d)", this->del);
    SPRINT(buf, offset, "(delDeps=");
    for(CoroutineState *curr = this->delDependencyNodes; curr != NULL; curr = curr->next) {
    	SPRINT(buf, offset, "%p(delf=%d,tid=%d)", curr, curr->delFlag, curr->threadId);
    }
    SPRINT(buf, offset, ")");
    char termBuf[STRING_BUF_SIZE];
    SubtermPtr *sp = subtermPtrs;
    while(sp != NULL){
        sp->subterm->toString(termBuf, symtable);
        //SPRINT(buf, offset, "->%s{%p}", termBuf, sp->startTrieNode);
        SPRINT(buf, offset, "->%s", termBuf);
        sp=sp->next;
    }
    SPRINT(buf, offset, "\n");
    indent[ind] = '\263';
    indent[ind+1] = '\0';
    Trie *cnode = funcChild;
    while(cnode!=NULL) {
        offset = cnode->toStringOffsetIndent(root, buf, offset, indent, cnode->sibling == NULL, symtable, pre, post);
        cnode = cnode ->sibling;
    }
    indent[ind] = '\0';

    buf[offset] = '\0'; // remove trailing '('
    return offset;
}


void Trie::fillInSubtermPtrs(Term *term, REGION) {
	if(subtermPtrs == NULL) {
		Trie *path[MAX_TERM_SIZE];
		Trie **ppath = path + MAX_TERM_SIZE;

		Trie *curr = this;
		while (curr->super != NULL) {
			*(--ppath) = curr;
			curr = curr->super;
		}
		*(--ppath) = curr; // root
	//	printf("fill in term: ");
	//	for(int i=0;i<flatterm+MAX_TERM_SIZE-p;i++) {
	//		printf("%lX(%s) ", p[i], symtable[p[i]]);
	//	}
	//	printf("\n");
		bool newNode;
		__fillInSubtermPtrs(term, ppath, newNode, CURR_REGION);
	}
}

void Trie::fillInSubtermPtrs(REGION) {
	if(subtermPtrs == NULL) {
		Trie *path[MAX_TERM_SIZE];
		Trie **ppath = path + MAX_TERM_SIZE;
		Symbol flatterm[MAX_TERM_SIZE];
		Symbol *p = flatterm+MAX_TERM_SIZE;

		Trie *curr = this;
		while (curr->super != NULL) {
			*(--p) = curr->key;
			*(--ppath) = curr;
			curr = curr->super;
		}
		*(--ppath) = curr; // root
	//	printf("fill in term: ");
	//	for(int i=0;i<flatterm+MAX_TERM_SIZE-p;i++) {
	//		printf("%lX(%s) ", p[i], symtable[p[i]]);
	//	}
	//	printf("\n");
		Term *term = getFromTermMatrix(p, p);
		bool newNode;
		__fillInSubtermPtrs(term, ppath, newNode, CURR_REGION);
	}

}

// newNode whether the returned node is a new node therefore needs to fill in subtermptrs
Trie **Trie::__fillInSubtermPtrs(Term *term, Trie **ppath, bool &newNode, REGION) {
	Trie **pcurr = ppath + 1; // skip top symbol in term

	if(term->symbolDimension == 0) {
		newNode = subtermPtrs == NULL;
	} else {
		for(int i=0;i<term->symbolDimension;i++) {
			pcurr = __fillInSubtermPtrs(term->subterms[i], pcurr, newNode, CURR_REGION);
		}
	}
	if(newNode) {
		(*pcurr)->subtermPtrs = new (CURR_REGION) SubtermPtr(term, *ppath, (*pcurr)->subtermPtrs);
	}
	return pcurr;
}

Term *Trie::getTerm(REGION) {
	fillInSubtermPtrs(CURR_REGION);
	return subtermPtrs->subterm;
}

void Trie::setDel(bool delNew) {
	if(del != delNew) {
		del = delNew;
		updateDelDependency();
	}
}


void checkDep(CoroutineState *dep) {
//	printf("checking dep ");
//	while(dep != NULL) {
//		printf("%p, ", dep);
//		dep = dep->directDataDependencyNext;
//	}
//	printf("\n");
//	fflush(stdout);
}
void deleteCurr(CoroutineState *&dep, CoroutineState *&depTail, CoroutineState *prev, CoroutineState *curr) {
	if(prev == NULL) {
		if(curr->directDataDependencyNext == NULL) {
			dep = depTail = NULL;
		} else {
			dep = curr->directDataDependencyNext;
		}
	} else {
		if(curr->directDataDependencyNext == NULL) {
			depTail = prev;
			prev->directDataDependencyNext = NULL;
		} else {
			prev->directDataDependencyNext = curr->directDataDependencyNext;
		}
	}
	checkDep(dep);
}

void appendDep(CoroutineState *&dep, CoroutineState *&depTail, CoroutineState *curr) {
	curr->directDataDependencyNext = NULL;
	if(dep == NULL) {
		dep = curr;
	} else {
		depTail->directDataDependencyNext = curr;
	}
	depTail = curr;

	checkDep(dep);
}

void clearDep(CoroutineState *&dep, CoroutineState *&depTail) {
	dep = depTail = NULL;

}

void Trie::updateDelDependency() {
		for(CoroutineDataDependency *prev = NULL, *curr = delDependencyNodes; curr != NULL; ) {
			if((curr->delFlag & DEL_FLAG_PERM_DEL) != 0) {
				CoroutineDataDependency *next = curr->directDataDependencyNext;
				deleteCurr(delDependencyNodes, delDependencyNodesTail, prev, curr);

				SET_DELETE_AND_ADD_TO_FREE_LIST(curr);
				if(DEBUG_TRACE_DEALLOC) state_dealloc_counter++;
				if(DEBUG_DEALLOC) printf("%d: state %p dealloced\n", state_dealloc_counter, curr);

				curr = next;
			} else {
				flip(curr);
				prev = curr;
				curr = curr->directDataDependencyNext;
			}
		}
	//dataDependencyNodes.clear();
}
void Trie::updateSiblingDependency() {
	//printf("update sibling dependency %p\n", this);
	for(CoroutineDataDependency *curr = siblingDependencyNodes; curr != NULL;curr = curr->directDataDependencyNext) {
		undeleteAndEnableDependencySubtree(curr);
	}
	clearDep(siblingDependencyNodes, siblingDependencyNodesTail);
}
void Trie::clearDataDependency(Trie *root) {
	clearDep(root->delDependencyNodes, root->delDependencyNodesTail);
	clearDep(root->siblingDependencyNodes, root->delDependencyNodesTail);
	LinkedList<const char *> *keys = globalTrieNodeHashtable->keys();
	for(LinkedList<const char *>::Node *k = keys->head; k!=NULL; k=k->next) {
		Trie *node = globalTrieNodeHashtable->lookup(k->value);
		clearDep(node->siblingDependencyNodes, node->siblingDependencyNodesTail);
		clearDep(node->delDependencyNodes, node->delDependencyNodesTail);
	}
	delete keys;
}
