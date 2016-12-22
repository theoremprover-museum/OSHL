/*
 *      Author: Hao Xu
 */
#include <stdlib.h>
#include <stdio.h> /* snprintf */
#include <string.h> /* strlen */
#include "clause.h"
#include "defs.h"
#include "utils.h"

Region *clauseRegion = make_region(0);
Region *clauseComponentRegion = make_region(0);

// ObjectPool<Clause, MAX_NUM_CLAUSES> globalClauseObjectPool;

/*    Clause::Clause(int nol, int dim, bool *signs, Term **lits) :
    numberOfLiterals(nol),
        dimension(dim),
        signs(signs),
        literals(lits),
        listOfNodes(NULL) {
    }*/

    Clause::Clause(int nol, int rel, int dim)  :
		numberOfLiterals(nol),
        dimension(dim),
        relevance(rel),
        signs((bool *)region_alloc(clauseComponentRegion, sizeof(bool) * nol)),
        literals((Term **) region_alloc(clauseComponentRegion, sizeof(Term) *nol)),
        listOfNodes(NULL) {
    }

    Clause::Clause(int nol, Term **lits, bool *ss, int rel, int dim)  :
		numberOfLiterals(nol),
        dimension(dim),
        relevance(rel),
        signs((bool *)region_alloc(clauseComponentRegion, sizeof(bool) * nol)),
        literals((Term **) region_alloc(clauseComponentRegion, sizeof(Term) *nol)),
        listOfNodes(NULL) {
		memcpy(literals, lits, nol * sizeof(Term *));
		memcpy(signs, ss, nol * sizeof(bool));

    }

    int Clause::clauseSize() {
		int instSize = 0;
		for(int k = 0; k < numberOfLiterals; k ++) {
			instSize = max(instSize, literals[k]->termSize());
		}
		return instSize;

    }

    Clause::~Clause() {
    	SET_DELETE(signs) ;
    	SET_DELETE(literals);
    }


    int Clause::toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset) {
    	if(this == NULL) {
            SPRINT(buf, offset, "<null>");
    		offset += 6;
    		return offset;
    	}
        SPRINT(buf, offset, "[");
        #define F(x) SPRINT(buf, offset, "%s", signs[INDEX]? "": "~"); \
                     offset = x->toString(buf, symtable, offset, "?", ""); \
                     SPRINT(buf, offset, "|");
        MAP_APP(F, numberOfLiterals, literals);
        #undef F
        if(numberOfLiterals > 0) {
        	offset--;
        }
        SPRINT(buf, offset, "]"); // removing trailing "|"
        offset++;
        return offset;
    }
    // Normalize the variable indices in a clause to 0, 1, ..., n, based on their first occurrence in the clause,
    // and set the dimension field.
    int Clause::normalize(Symbol nextFreeVarId) {
    	Renaming varMappingsObj;
    	Renaming *varMappings = &varMappingsObj;

        const int nextFreeVarId0 = nextFreeVarId;


#define F(x) nextFreeVarId = x->normalize(varMappings, 0, nextFreeVarId); x = (*(x))(varMappings);
        MAP_APP(F, numberOfLiterals, literals);
#undef F

        dimension = nextFreeVarId - nextFreeVarId0;
        deleteToNodeExclusive(varMappings, &varMappingsObj);
        return nextFreeVarId;
    }
    void Clause::sortBySign() {
    	Term *sorted[MAX_NUM_CLAUSE_LITERALS];
    	numberOfNegativeLiterals = 0;
    	for(int i=0;i<numberOfLiterals;i++) {
    	    		if(!signs[i]) { // negative literal
    	    			numberOfNegativeLiterals++;
    	    		}
    	    	}


    	Term **p = sorted + numberOfNegativeLiterals;
    	Term **n = sorted;
    	for(int i=0;i<numberOfLiterals;i++) {
    		if(!signs[i]) { // negative literal
    			*(n++) = literals[i];
    		}
    		else { // positive literal
				*(p++) = literals[i];
			}
    	}
    	// update signs
    	for(int i=0;i<numberOfNegativeLiterals;i++) {
    		signs[i] = false;
    	}
    	for(int i =numberOfNegativeLiterals;i<numberOfLiterals;i++) {
    		signs[i] = true;
    	}
    	// update literals
    	memcpy(literals, sorted, sizeof(Term*) * numberOfLiterals);
    }
    // precond: must be sorted
    void Clause::removeRedundantLiterals() {
        for(int i=0, j=0, n = numberOfLiterals-1;i<n;i++) {
            if(i!=j) {
                literals[j] = literals[i];
                signs[j] = signs[i];
            }
            if(signs[i] != signs[i+1] || *literals[i] != *literals[i+1]) {
                j++;
            } else {
            	numberOfLiterals--;
            }
        }
        
    }
    void Clause::flipLiterals(Hashtable<bool, sizeof(Symbol)> &flip) {
    	for(int i=0;i<numberOfLiterals;i++) {
    		Symbol s = literals[i]->symbol;
    		if(flip[HASHTABLE_KEY(s)]) {
    			signs[i] = !signs[i];
    		}
    	}
    }

    /*void Clause::sort() {
        
    }
    
    Clause *Clause::operator ()(Sub *sub) {
        Clause *c;

        sort();
        removeRedundantLiterals();
        return c;
    }
    Clause *Clause::operator ()(Renaming *sub) {
        Clause *c;
        sort();
        removeRedundantLiterals();
        return c;
    }*/
