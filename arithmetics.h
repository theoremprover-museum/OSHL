/*
 * arithmetics.h
 *
 *      Author: Hao Xu
 */

#ifndef ARITHMETICS_H_
#define ARITHMETICS_H_

#include "defs.h"
#include "hashtable.h"

struct SaturationTree {
	int key;
	int numberOfArrangements;
	SaturationTree *typeChild;
	SaturationTree *sizeChild;
};

// global data

// problem specific maximum function symbols dimension
extern unsigned int MaxFunctionSymbolDimension;

// number of function symbols of a given dimension
extern unsigned int NumberOfFunctionSymbols[MAX_FUNCTION_SYMBOL_DIMENSION + 1];

// number of distinct terms of a given term size
// initially set of 0 which means it has not been computed
extern unsigned int NumberOfArrangements[MAX_TERM_SIZE + 1][MAX_TERM_SIZE + 1];

unsigned int numberOfDistinctTerms(unsigned int n);

// compute the arrangements for n (same) objects into i (distinctive, nonempty) groups
unsigned int numberOfArrangements(unsigned int n, unsigned int i);

void initArithmetics();

void resetArithmetics();

#endif /* ARITHMETICS_H_ */
