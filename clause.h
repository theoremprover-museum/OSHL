/*
 * File:   clause.h
 * Author: Hao Xu
 *
 */

#ifndef CLAUSE_H
#define	CLAUSE_H

#include "term.h"
#include "defs.h"
#include "hashtable.h"
#include "linkedList.h"
#include "acyclicGraph.h"
#include <string.h>

//#define MAX_NUM_CLAUSES 0x1000000
//extern ObjectPool<Clause, MAX_NUM_CLAUSES> globalClauseObjectPool;
extern Region *clauseRegion;

//    typedef struct clause Clause;

struct Clause : RegionAllocType<Clause> {
    int numberOfLiterals;
    int numberOfNegativeLiterals;
    int dimension;
    int relevance;
    bool *signs;
    Term **literals;
    LinkedList<LinkedList<Clause *>::Node *> *listOfNodes; // see model.cpp

    
    // Clause(int nol, int dim, bool *signs, Term **lits);

    Clause(int nol, int rel, int dim);
    Clause(int nol, Term **lits, bool *ss, int rel, int dim);
    ~Clause();


    int toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0);
    // Normalize the variable indices in a clause to 0, 1, ..., n, based on their first occurance in the clause,
    // and set the dimension field.
    int normalize(Symbol nextFreeVarId = 0);
    // precond: must be sorted
    void removeRedundantLiterals();
    int clauseSize();

    void sortBySign();
    void flipLiterals(Hashtable<bool, sizeof(Symbol)> &);

    Clause *operator ()(Sub *sub);
    Clause *operator ()(Renaming *sub);
};

#endif	/* CLAUSE_H */

