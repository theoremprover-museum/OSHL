/*
 *      Author: Hao Xu
 */
#include <string.h>
#include "arithmetics.h"

// global data

// problem specific maximum function symbols dimension
unsigned int MaxFunctionSymbolDimension;

// number of function symbols of a given dimension
unsigned int NumberOfFunctionSymbols[MAX_FUNCTION_SYMBOL_DIMENSION + 1];

// number of distinct terms of a given term size
// initially set of 0 which means it has not been computed
// compute the arrangements for n (same) objects into i (distinctive, nonempty) groups
unsigned int NumberOfArrangements[MAX_TERM_SIZE + 1][MAX_TERM_SIZE + 1];

// compute number of distinct terms of size n
unsigned int numberOfDistinctTerms(unsigned int n) {
	if(n == 1) {
		return NumberOfFunctionSymbols[0];
	} else {
		unsigned int sum = 0;
		for(unsigned int i = 1;i <= MaxFunctionSymbolDimension; i++) {
			unsigned int numCombinations = numberOfArrangements(n-1, i);
			sum += NumberOfFunctionSymbols[i] * numCombinations;
		}
		return sum;
	}
}

unsigned int numberOfArrangements(unsigned int n, unsigned int i) {
	if(NumberOfArrangements[n][i] == 0) {
		if(i == 1) {
			NumberOfArrangements[n][i] = numberOfDistinctTerms(n);
		} else {
			unsigned int sum = 0;
			for(unsigned int j = 1; j + (i - 1) <= n ; j++) {
				sum += numberOfArrangements(j, 1) * numberOfArrangements(n-j, i-1);
			}
			NumberOfArrangements[n][i] = sum;
		}
	}
	return NumberOfArrangements[n][i];
}

void initArithmetics() {
	memset(NumberOfArrangements, 0, sizeof(unsigned int) * (MAX_TERM_SIZE + 1) * (MAX_TERM_SIZE + 1));
	memset(NumberOfFunctionSymbols, 0, sizeof(unsigned int) * (MAX_FUNCTION_SYMBOL_DIMENSION + 1));
}

void resetArithmetics() {
	memset(NumberOfArrangements, 0, sizeof(unsigned int) * (MAX_TERM_SIZE + 1) * (MAX_TERM_SIZE + 1));
	memset(NumberOfFunctionSymbols, 0, sizeof(unsigned int) * (MAX_FUNCTION_SYMBOL_DIMENSION + 1));
}
