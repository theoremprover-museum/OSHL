/*
 * File:   term.h
 * Author: Hao Xu
 *
 */

#ifndef TERM_H
#define	TERM_H

#include "defs.h"
#include "expansibleList.h"
#include "utils.h"
#include "hashtable.h"
#include "linkedList.h"
#include "acyclicGraph.h"
#include "offsetList.h"
#include <string.h> /* memset */
#include <sys/types.h>

#define NEW_VAR new (CURR_REGION) Term(varid ++)
struct PMVMInstruction;
struct Term;
struct Type;
extern SymTable symtable;

// end of flatterm is marked by subterm == NULL
struct PMVMInstruction : RegionAlloc {
    Symbol symbol;
    Term *subterm;
    Type *varType;
    unsigned int subtermSize;
    bool newVar; // if this symbol is a variable, whether this is the first occurrence of the variable in the clause
    int repeatedVar; // if this symbol is a variable and newVar == true, whether this is the first occurrence of a variable that will occur again in the term
                     // if this symbol is a variable and newVar == false, how many prevs needed to get to the corresponding sub node
    
    PMVMInstruction(Symbol symbol, Term *subterm, unsigned int subtermSize) :
        symbol(symbol),
        subterm(subterm),
        subtermSize(subtermSize),
        newVar(false),
        repeatedVar(0)
    {
    }

    void computeBackLevels(OffsetList<Symbol, PMVMInstruction *> *firstOccur, int &nVars, bool countNonRepeatedVars) {
    	// char buf[STRING_BUF_SIZE];
    	// this->toString(buf, symtable);
    	// printf("computing back levels for %s\n", buf);
    	for(PMVMInstruction *p = this;p->subterm != NULL;p++) {
    		if(IS_VAR_SYMBOL(p->symbol)) {
    			if(p->newVar) {
    				if((p->repeatedVar != 0 || countNonRepeatedVars)) {
    					// update repeatedVar to reflect the number of vars, starting from 1
    					// nonrepeated vars will also have value 1 unless they are counted
    					p->repeatedVar = ++nVars;
//        				printf("var %lX has pos %d (nVars = %d)\n", p->symbol, p->repeatedVar, nVars);
    				} else {
//        				printf("var %lX is not repeated (nVars = %d)\n", p->symbol, nVars);
    				}
    			} else {
    				p->repeatedVar = nVars - (*firstOccur)[p->symbol]->repeatedVar;
//    				printf("var %lX has back level %d (nVars = %d)\n", p->symbol, p->repeatedVar, nVars);
    			}
    		}
    	}
    }

    int toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0, char const *pre= "?", char const *post="") {
    	SPRINT(buf, offset, "%p ", this);
        PMVMInstruction *p = this;
        while(p->subterm!=NULL) {
            if(IS_VAR_SYMBOL(p->symbol)) { // var
                SPRINT(buf, offset, "%s%lX%s[newVar:%d, repeatedVar:%d] ", pre, p->symbol - VAR_ID_OFFSET, post, p->newVar, p->repeatedVar);
                // SPRINT(buf, offset, "%s%lX%s%s ", pre, p->symbol - VAR_ID_OFFSET, post, p->newVar?"(free)":"(fixed)");
            } else {
                SPRINT(buf, offset, "%s(%lX) ", symtable[p->symbol], p->symbol);
            }
            p++;
        }
        buf[offset] = '\0';
        return offset;
    }


};

struct Clause;
struct Term : RegionAlloc {
    u_int64_t symbol;
    int symbolDimension;
    Term **subterms;
    int size;
    PMVMInstruction *inp;
    LinkedList<Clause *> *instances; // see model.cpp
    AcyclicGraph<Clause *> *vertex; // see model.cpp
    
    PMVMInstruction *convertToPMVMInstructions(OffsetList<Symbol, PMVMInstruction *> *firstOccurVar, REGION);
    void compile(REGION); // compile this term into a list of pattern matching VM instructions
    
    int termSize();

    int toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0, char const *pre = "?", char const *post = "");

    static int varToString(Symbol symbol, char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0, char const *pre = "?", char const *post = "");

    // Normalize free vars in a term.
    // Rename the ith free var (id >= freeVarOffset) to nextFreeVarId+i
    // return the next free var id
    Symbol normalize(Renaming *& varMappings, Symbol freeVarOffset, Symbol nextFreeVarId);
    
    Renaming *newVars(Renaming *varMappings, Symbol &freeVarOffset, Symbol &nextNewVarId);

    bool operator == (const Term &a);
    
    bool operator != (const Term &a);
    
    bool operator > (const Term &a);

    Term *operator ()(Sub *sub, Version varVersion = 0);
    
    Term *operator ()(Renaming *renaming);

    void getKey(char buf[MAX_TERM_SIZE * sizeof(Symbol) + 1], int *size);

    Term *ground(Symbol cons);

    bool containsVar(Symbol var);

    LinkedList<Symbol> *fv();

    void fv(LinkedList<Symbol> *list);

private:
    Term();

    Term(u_int64_t symbol);

    Term(u_int64_t symbol, int symbolDim, Term **subterms);

    Term(u_int64_t symbol, int symbolDim, REGION);

    PMVMInstruction *__convertToFlatterm(PMVMInstruction *ft, OffsetList<Symbol, PMVMInstruction *> *firstOccurVar);

    Symbol *__getKey(Symbol *buf);

    friend Term *getFromTermMatrix(char *key);
    friend void testTerms();
};

#endif	/* TERM_H */

