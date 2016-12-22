/*
 *       Author: Hao Xu
 */

#include "term.h"
#include "type.h"
#include "utils.h"
#include "termCache.h"
#ifdef DEBUG_TERM_SIZE
#include "symbols.h"
#endif
#ifdef DEBUG_TERM_SIZE
#include "defs.h"
#endif

Region *termInitRegion = make_region();
Term::Term(u_int64_t symbol) :
	symbol(symbol),
	symbolDimension(0),
	subterms(NULL),
	size(1),
	inp(NULL),
	instances(NULL) {
}
Term::Term(u_int64_t symbol, int symbolDim, Term **subterms) :
    symbol(symbol),
    symbolDimension(symbolDim),
    subterms(subterms),
    size(0),
    inp(NULL),
    instances(NULL) {
}

Term::Term(u_int64_t symbol, int symbolDim, REGION) :
    symbol(symbol),
    symbolDimension(symbolDim),
    subterms((Term **) HEAP_ALLOC(Term *, symbolDim)),
    size(0),
    inp(NULL),
    instances(NULL) {
}
Term::Term() {
}
PMVMInstruction *Term::convertToPMVMInstructions(OffsetList<Symbol, PMVMInstruction *> *firstOccurVar, REGION) {
    int s = termSize();
    PMVMInstruction *ft = HEAP_ALLOC(PMVMInstruction, s+1);
    // printf("flatterm %p\n", ft);
    memset(ft, 0, sizeof(PMVMInstruction) * (s+1)); // this may need to be change to work with alignment
    __convertToFlatterm(ft, firstOccurVar);
    ft[s].subterm = NULL;
    return ft;
}

void Term::compile(REGION) {
	if(inp == NULL) {
	    OffsetList<Symbol, PMVMInstruction *> firstOccurVar;
		inp = convertToPMVMInstructions(&firstOccurVar, CURR_REGION);
	}
}
int Term::termSize() {
#ifdef DEBUG_TERM_SIZE
	char buf[STRING_BUF_SIZE];
	toString(buf, symtable);
	printf("computing term size of term %s\n", buf);
#endif
	if(size == 0) {
    int sum = 1;
#define F(x) sum += x->termSize()
    MAP_APP(F, symbolDimension, subterms);
#undef F
    size = sum;
    return sum;
	} else {
		return size;
	}
}

int Term::varToString(Symbol symbol, char buf[STRING_BUF_SIZE], SymTable &symtable, int offset, char const *pre, char const *post) {
	if(symtable[symbol] == NULL) {
		SPRINT(buf, offset, "%s%lX%s", pre, symbol - VAR_ID_OFFSET, post);
	} else {
		SPRINT(buf, offset, "%s", symtable[symbol]);
	}
	return offset;

}
int Term::toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset, char const *pre, char const *post) {
    if(IS_VAR_SYMBOL(symbol)) { // var
    	return varToString(symbol, buf, symtable, offset, pre, post);
    } else {
        SPRINT(buf, offset, "%s(", symtable[symbol]);
    	// SPRINT(buf, offset, "%s[%X](", symtable[symbol], symbol);
        #define F(x) offset = x->toString(buf, symtable, offset, pre, post); \
                     SPRINT(buf, offset, ",");
        MAP_APP(F, symbolDimension, subterms);
        #undef F
        offset--;
        if(buf[offset] == ',') {
            SPRINT(buf, offset, ")"); // remove trailing ','
        } else {
            buf[offset] = '\0'; // remove trailing '('
        }
    }
    return offset;
}

// Normalize free vars in a term.
// Rename the ith free var (id >= freeVarOffset) to nextFreeVarId+i
// return the next free var id
// precond if var x occurs before var y in clause then id(x) < id(y)
Symbol Term::normalize(Renaming *& varMappings, Symbol freeVarOffset, Symbol nextFreeVarId) {
    Symbol nextFreeVarIdNew = nextFreeVarId;
    Symbol symbol = this->symbol;
    if(IS_VAR_SYMBOL(symbol)) { // variable
        if(symbol >= freeVarOffset) { // may be free
            int varMapped;
            if((varMapped = (*varMappings)[symbol]) == 0) { // free
                varMappings = varMappings->push(symbol, nextFreeVarIdNew ++, termInitRegion); // assign a new normalized variable id,
                                                               // replace variable id,
                                                               // and increase dimnew
            } else { // bound
            }
        } else { // bound
            return nextFreeVarId;
        }
    } else {
    #define F(x) nextFreeVarIdNew = x->normalize(varMappings, freeVarOffset, nextFreeVarIdNew)
        MAP_APP(F, symbolDimension, subterms);
    #undef F
    }
    return nextFreeVarIdNew;
}

Renaming *Term::newVars(Renaming *varMappings, Symbol &freeVarOffset, Symbol &nextNewVarId) {
    Symbol symbol = this->symbol;
    if(IS_VAR_SYMBOL(symbol)) { // variable
        if(symbol >= freeVarOffset) { // free
			varMappings = varMappings->push(symbol, nextNewVarId ++, termInitRegion); // assign a new normalized variable id,
														   // replace variable id,
														   // and increase dimnew
			freeVarOffset = symbol + 1;
        } else { // bound
        }
    } else {
    #define F(x) varMappings = x->newVars(varMappings, freeVarOffset, nextNewVarId)
        MAP_APP(F, symbolDimension, subterms);
    #undef F
    }
    return varMappings;
}


bool Term::operator == (const Term &a) {
    if(symbol != a.symbol) {
        return false;
    } else {
        #define F(x) if(x != a.subterms[INDEX]) return false;
            MAP_APP(F, symbolDimension, subterms);
        #undef F
    }
    return true;
}

bool Term::operator != (const Term &a) {
    return !(*this == a);
}

Term *Term::operator() (Sub *sub, u_int64_t varVersion) {
    if(IS_VAR_SYMBOL(symbol)) {
        Term *t;
        u_int64_t v;
        if((t = (*sub)(symbol | varVersion, v))==NULL) {
        	if(varVersion == 0) {
        		return this;
        	} else {
        		return getFromTermMatrix(symbol | varVersion);
        	}
        } else {
            return (*t)(sub, v);
        }
    } else {
    	char key[sizeof(Symbol) + sizeof(Term *) * MAX_FUNCTION_SYMBOL_DIMENSION];
    	Symbol *f = (Symbol *)key;
    	*f = symbol;
        Term **subtermsNew = (Term **) (key + sizeof(Symbol));
        bool changed = false;
        #define F(x) changed |= ((subtermsNew[INDEX] = (*x)(sub, varVersion)) != x)
            MAP_APP(F, symbolDimension, subterms);
        #undef F
        if(changed) {
            return getFromTermMatrix(key);
        } else {
            return this;
        }
    }
}

Term *Term::operator() (Renaming *renaming) {
    if(IS_VAR_SYMBOL(symbol)) {
        Symbol t;
        if((t = (*renaming)[symbol])==0) {
#ifdef DEBUG_TERM_RENAMING
    	char buf [STRING_BUF_SIZE];
    	this->toString(buf, symtable);
    	printf("renaming var %s to %s\n", buf, buf);
#endif
            return this;
        } else {
#ifdef DEBUG_TERM_RENAMING
    	char buf [STRING_BUF_SIZE];
    	this->toString(buf, symtable);
    	printf("renaming var %s to %s(%lX)\n", buf, symtable[t], t);
#endif
            return getFromTermMatrix(t);
        }
    } else {
    	char key[sizeof(Symbol) + sizeof(Term *) * MAX_FUNCTION_SYMBOL_DIMENSION];
    	Symbol *f = (Symbol *)key;
    	*f = symbol;
        Term **subtermsNew = (Term **) (key + sizeof(Symbol));
        bool changed = false;
        #define F(x) changed |= ((subtermsNew[INDEX] = (*x)(renaming)) != x)
            MAP_APP(F, symbolDimension, subterms);
        #undef F
        if(changed) {
            return getFromTermMatrix(key);
        } else {
            return this;
        }
    }
}

void Term::getKey(char buf[MAX_TERM_SIZE * sizeof(Symbol) +1], int *size) {
	Symbol *bufend = __getKey((Symbol *)buf);
	*size = (bufend - (Symbol *) buf) * sizeof(Symbol);
}

Symbol *Term::__getKey(Symbol *buf) {
	*buf = symbol;
	for(int i=0;i<symbolDimension;i++) {
		buf = subterms[i]->__getKey(buf);
	}
	return buf;
}

PMVMInstruction *Term::__convertToFlatterm(PMVMInstruction *ft, OffsetList<Symbol, PMVMInstruction *> *firstOccurVar) {
    PMVMInstruction *ftnew = ft+1;
#define F(x) ftnew = x->__convertToFlatterm(ftnew, firstOccurVar)
    MAP_APP(F, symbolDimension, subterms);
#undef F
    ft->subterm = this;
    ft->symbol = symbol;
    ft->subtermSize = ftnew - ft;
#ifdef DEBUG_COMPILE
    char buf[STRING_BUF_SIZE];
#endif
    if(IS_VAR_SYMBOL(symbol)) {
		if(firstOccurVar->lookupCheckBound(symbol) == NULL) {
#ifdef DEBUG_COMPILE
			Term::varToString(symbol, buf, symtable);
			printf("%s is a new var\n", buf);
#endif
			ft->repeatedVar = 0;
			ft->newVar = true;
			firstOccurVar->setAndZero(symbol, ft);
//			printf("first occurrence of %lX is at %p\n", ft->symbol, ft);
//			assert((*firstOccurVar)[symbol] == ft);
		} else {
#ifdef DEBUG_COMPILE
			Term::varToString(symbol, buf, symtable);
			printf("%s is not a new var\n", buf);
#endif
//			printf("set repeatedVar of %lX at %p to 1\n", (*firstOccurVar)[symbol]->symbol, (*firstOccurVar)[symbol]);
			(*firstOccurVar)[symbol]->repeatedVar = 1;
			ft->newVar = false;
			ft->repeatedVar = 1; // set to true
		}
		Symbol rep = varTypeEquiv.findEquivRep(symbol);
		ft->varType = typeSymbolToType[HASHTABLE_KEY(rep)];
    }
    return ftnew;
}

Term *Term::ground(Symbol cons) {
    if(IS_VAR_SYMBOL(symbol)) {
		return getFromTermMatrix(cons);
    } else {
    	char key[sizeof(Symbol) + sizeof(Term *) * MAX_FUNCTION_SYMBOL_DIMENSION];
    	Symbol *f = (Symbol *)key;
    	*f = symbol;
        Term **subtermsNew = (Term **) (key + sizeof(Symbol));
        bool changed = false;
        #define F(x) changed |= ((subtermsNew[INDEX] = x->ground(cons)) != x)
            MAP_APP(F, symbolDimension, subterms);
        #undef F
        if(changed) {
            return getFromTermMatrix(key);
        } else {
            return this;
        }
    }
}


bool Term::containsVar(Symbol var) {
    if(var == symbol) {
		return true;
    } else {
        #define F(x) if(x->containsVar(var)) { return true; }
            MAP_APP(F, symbolDimension, subterms);
        #undef F
		return false;
    }
}

void Term::fv(LinkedList<Symbol> *list) {
    if(IS_VAR_SYMBOL(symbol)) {
		if(!list->contains(symbol)) {
			list->append(symbol);
		}
    } else {
        #define F(x) x->fv(list)
            MAP_APP(F, symbolDimension, subterms);
        #undef F
    }
}


LinkedList<Symbol> *Term::fv() {
	LinkedList<Symbol> *list = new LinkedList<Symbol>();
	fv(list);
	return list;
}



