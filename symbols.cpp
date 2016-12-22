/*
 * symbols.cpp
 *
 *      Author: Hao Xu
 */

#include "symbols.h"
#include "arithmetics.h"

Symbol varid = VAR_ID_OFFSET;
Symbol funcid = FUNC_ID_OFFSET;

Symbol functionSymbols[MAX_FUNCTION_SYMBOLS];
unsigned int numberOfFunctionSymbols = 0;
bool idUsed[MAX_FUNCTION_SYMBOLS];

SymTable symtable;

void initSymbols() {
	memset(idUsed, 0, sizeof(bool[MAX_FUNCTION_SYMBOLS]));
}

void resetSymbols() {
	funcid = FUNC_ID_OFFSET;
	varid = VAR_ID_OFFSET;
	numberOfFunctionSymbols = 0;
	symtable.clear();
	initSymbols();
}

Symbol updateSymTable(const char *buf, unsigned int arity, Symbol &id, bool isNotFunc)
{
	Symbol i;
    if((i = symtable[buf]) != 0) {
    	return i;
    }
    if(IS_FUNC_SYMBOL(id)) {
    	if(globalRandomLexicalOrder) {
    		Symbol start = rand() % MAX_FUNCTION_SYMBOLS;
    		i = start;
			while(idUsed[i]) {
		    	i++;
		    	if(i>=MAX_FUNCTION_SYMBOLS) {
		    		i -= MAX_FUNCTION_SYMBOLS;
		    	}
		    	if(i == start) {
		    		return 0; // error
		    	}
			}
			i = i * (FUNC_ID_ARITY_MASK + 1) | arity;
			id = i;
    	} else {
			i = id | arity;
			id += FUNC_ID_ARITY_MASK + 1;
    	}
    	if(!isNotFunc) {
			functionSymbols[numberOfFunctionSymbols++] = i;

			NumberOfFunctionSymbols[arity]++;
			if(MaxFunctionSymbolDimension < arity) {
				MaxFunctionSymbolDimension = arity;
			}
			// printf("add func %lX %s\n", i, buf);
    	} else {
        	// printf("add pred or super %lX %s\n", i, buf);

    	}
    } else {
    	i = id++;
    }
	symtable.insert(i, strdup(buf));
	return i;
}

Symbol getNullaryFunctionSymbol() {
	for(unsigned int i = 0;i < numberOfFunctionSymbols;i++) {
		Symbol func = functionSymbols[i];
		if(ARITY(func) == 0) {
			return func;
		}
	}
	return 0;
}





