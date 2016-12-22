/*
 * Author: Hao Xu
 */

#include "utils.h"
#include "term.h"

#include "symbols.h"
#include "termCache.h"
#include "offsetList.h"
#include <string.h>


/* bool occur(int vsymbol, Term *t) {
    if(t->symbol == vsymbol) {
        return true;
    } else {
#define F(x) \
        if(occur(vsymbol, x)) { \
            return true; \
        }
        MAP_APP(F, t->symbolDimension, t->subterms);
#undef F
        return false;
    }
}

Sub *unify(Term *s, Term *t, Sub *sub) {
    Sub *subNew = sub;
    if(IS_VAR_SYMBOL(s->symbol)) {
        Term *v;
        if((v = sub->lookup(s->symbol)) == NULL) {
            if(occur(s->symbol, t)) {
                return NULL;
            } else {
                subNew = new Sub(s->symbol, t, subNew);
            }
        } else {
            unify(v, t, subNew);
        }
        
    } else if(IS_VAR_SYMBOL(t->symbol)) {
        Term *v;
        if((v = sub->lookup(t->symbol)) == NULL) {
            if(occur(t->symbol, s)) {
                return NULL;
            } else {
                subNew = new Sub(t->symbol, s, subNew);
            }
        } else {
            unify(s, v, subNew);
        }
        
    } else {
        if(s->symbol != t->symbol) {
            return NULL;
        }
        // assume that one symbol can only have fixed dimension
    #define F(x) \
        subNew = unify(x, t->subterms[INDEX], subNew); \
        if(subNew == NULL) { \
            return NULL; \
        }

        MAP_APP(F, s->symbolDimension, s->subterms);
    #undef F
    }
}*/

bool occur(Symbol a, Term *b, Sub *sub, Version versionB) {
    if(IS_VAR_SYMBOL(b->symbol)) {
        if((b->symbol | versionB) == a) {
            return true;
        } else {
            Term *t;
            Version version;
            t = (*sub)(b->symbol | versionB, version);
            if(t == NULL) { // var b is free
                return false;
            } else {
                return occur(a, t, sub, version);
            }
        }
    } else {
        int i;
        for(i=0;i<b->symbolDimension;i++) {
            if(occur(a, b->subterms[i], sub, versionB)) {
                return true;
            }
        }
        return false;
    }
}

bool isRenaming(Sub *sub) {
    OffsetList<Symbol, bool> mask;
    while(sub->prev != NULL) {
        if(IS_VAR_SYMBOL(sub->val->symbol)) {
        	// offset list does not check for out of bound error
            if(mask.offset > sub->val->symbol || mask.top <= sub->val->symbol || mask[sub->val->symbol] == 0) {
                mask.set(sub->val->symbol, 1);
            } else {
                return false;
            }
        } else {
            return false;
        }
        sub = sub->prev;
    }
    return true;
}
bool outOfDom(Sub *sub, Term *t) {
	LinkedList<Symbol> *ss = t->fv();
	for(LinkedList<Symbol>::Node *curr = ss->head; curr!=NULL; curr=curr->next) {
		if((*sub)[curr->value] != NULL) {
			delete ss;
			return false;
		}
	}
	delete ss;
    return true;
}
// This function does not reset sub if unification fails.
Sub *unify(bool signa, Term *a, bool signb, Term *b, Sub *sub, Region *r) {
    if(signa!=signb) {
        return NULL;
    } else {
        return unify(a, b, sub, r);
    }
}
// This function does not reset sub if unification fails.
Sub *unify(Term *a, Term *b, Sub *sub, Region *r, Symbol versionA, Symbol versionB) {
    Term *t;
    u_int64_t version;
    if(IS_VAR_SYMBOL(a->symbol)) { // term a is a var
        if((b->symbol | versionB) == (a->symbol | versionA)) { // var a == term b
            return sub;
        } else {
            if((t = (*sub)(a->symbol | versionA, version)) == NULL) { // var a is free
                if(!occur(a->symbol | versionA, b, sub, versionB)) {
                    return sub->push(a->symbol | versionA, b, r, versionB);
                } else {
                    return NULL;
                }
            } else { // var a is mapped to a term
                return unify(t, b, sub, r, version, versionB);
            }
        }
    } else if(IS_VAR_SYMBOL(b->symbol)) { // term a is not a var; term b is a var
        if((t = (*sub)(b->symbol | versionB, version)) == NULL) { // var b is free
            if(!occur(b->symbol | versionB, a, sub, versionA)) {
                return sub->push(b->symbol | versionB, a, r, versionA);
            } else {
                return NULL;
            }
        } else { // var b is mapped to a term
            return unify(a, t, sub, r, versionA, version);
        }
    } else if(b->symbol == a->symbol) { // none of term a or term b are vars, but they have the same head symbol
        int i;
        for(i=0;i<a->symbolDimension;i++) {
            if((sub = unify(a->subterms[i], b->subterms[i], sub, r, versionA, versionB))==NULL) {
                return NULL;
            }
        }
        return sub;
    } else {
        return NULL;
    }
}

// This function does not reset sub if unification failed.
// precond: vars in b have accending ids
// there maybe overlap between FV(a) and FV(b), they are different variables
bool variant(Term *a, Term *b, Renaming *&sub, Symbol &minFreeVarIdB, Region *r) {
    u_int64_t t;
    if(IS_VAR_SYMBOL(a->symbol)) { // term a is a var
    	if(!IS_VAR_SYMBOL(b->symbol)) { // b needs to be a var
    		return false;
    	} else {
#ifdef DEBUG_VARIANT
		printf("comparing %lX and minFreeVarId\n", a->symbol);
#endif
			if(b->symbol < minFreeVarIdB) { // var b is not first occurrence
	#ifdef DEBUG_VARIANT
				printf("var %lX is less then minFreeVarId\n", a->symbol);
	#endif
				Symbol am = (*sub)[a->symbol];
				return am != 0 && b->symbol == am;
			} else // var b is first occurrence
			if((t = (*sub)[a->symbol]) == 0) { // var a is free
				// it is impossible to have two variables mapped to the same variable
				sub = sub->push(a->symbol, b->symbol, r);
				minFreeVarIdB = b->symbol + 1; // update minFreeVarIdB
				return true;

			} else { // var a is not free
				return false;
			}
    	}

    } else if(IS_VAR_SYMBOL(b->symbol)) { // term a is not a var; term b is a var
        return false;
    } else if(b->symbol == a->symbol) { // none of term a or term b are vars, but they have the same head symbol
        int i;
        for(i=0;i<a->symbolDimension;i++) {
            if(!variant(a->subterms[i], b->subterms[i], sub, minFreeVarIdB, r)) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

// any var x in term b whose id < minFreeVarIdB has occurred before.
// therefore, the var y in the corresponding position in a must be a var such that y in dom(sub) and sub(y) = x
bool variant(bool signa, Term *a, bool signb, Term *b, Renaming *&sub, Symbol &minFreeVarIdB, Region *r) {
    if(signa!=signb) {
        return false;
    } else {
    	bool ret = variant(a,b,sub, minFreeVarIdB, r);
#ifdef DEBUG_VARIANT
    	char buf1[STRING_BUF_SIZE], buf2[STRING_BUF_SIZE];
    	a->toString(buf1, symtable);
    	b->toString(buf2, symtable);
    	printf("variant(%s, %s, %lX) = %d\n", buf1, buf2, minFreeVarId, ret);
#endif
    	return ret;
//        return variant(a, b, sub, mask);
    }
    
}
/*Term *dupTermFromSub(Term *a, Sub *sub, REGION) {
    Term *dupa = new (CURR_REGION) Term(a->symbol, a->symbolDimension, CURR_REGION);
    #define F(t) IS_VAR_SYMBOL(t->symbol) ? extractTermFromSub(t->symbol, sub, CURR_REGION) : dupTermFromSub(t, sub, CURR_REGION)
    MAP_ARR(F, a->symbolDimension, a->subterms, dupa->subterms);
    #undef F
    return dupa;
}
*/
Term *extractTermFromSub(int a, Sub *sub) {
	Version version;
    Term *suba = (*sub)(a, version);
    if(suba == NULL) {
        return getFromTermMatrix(a);
    } else {
        return (*suba)(sub, version);
    }
}


void truncate(char *buf, int len) {
	strcpy(buf + len - 3, "...");
}

Symbol minVarId(Term *term) {
	if(IS_VAR_SYMBOL(term->symbol)) {
		return term->symbol;
	} else {
		Symbol m = MAX_VAR_ID + 1;
		for(int i = 0;i<term->symbolDimension;i++) {
			m = min(minVarId(term->subterms[i]), m);
		}
		return m;
	}
}

// precond: VAR_ID_OFFSET - 1 doesn't underflow, so that it can be used as a tight upper bound for an empty set of var ids
Symbol maxVarId(Term *term) {
#ifdef DEBUG_MAX_VAR_ID
	char buf[STRING_BUF_SIZE];
	term->toString(buf, symtable);
#endif
	if(IS_VAR_SYMBOL(term->symbol)) {
#ifdef DEBUG_MAX_VAR_ID
		printf("max var id of term %s is %lX\n", buf, term->symbol);
#endif
		return term->symbol;
	} else {
		Symbol m = VAR_ID_OFFSET - 1;
		for(int i = 0;i<term->symbolDimension;i++) {
			m = max(maxVarId(term->subterms[i]), m);
		}
#ifdef DEBUG_MAX_VAR_ID
		printf("max var id of term %s is %lX\n", buf, m);
#endif
		return m;
	}
}
// precond: MAX_VAR_ID + 1 doesn't overflow, so that it can be used as a lower bound for an empty set of var ids
Symbol minVarId(Term **terms, int n) {
	Symbol m = MAX_VAR_ID + 1;
	for(int i = 0;i<n;i++) {
		m = min(minVarId(terms[i]), m);
	}
	return m;
}
char formatSizeBuf[1024];
char *formatSize(size_t size) {
	if(size < 1024 * 1024) {
		sprintf(formatSizeBuf, "%fK", size/(1024.0));
	} else {
		sprintf(formatSizeBuf, "%fM", size/(1024 * 1024.0));
	}
	return formatSizeBuf;
}


