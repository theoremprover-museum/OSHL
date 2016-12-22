/*
 * symbols.h
 *
 *      Author: Hao Xu
 */

#ifndef SYMBOLS_H_
#define SYMBOLS_H_

#include "defs.h"
#include "hashtable.h"

extern Symbol varid;
extern Symbol funcid;

extern Symbol functionSymbols[MAX_FUNCTION_SYMBOLS];
extern unsigned int numberOfFunctionSymbols;
extern SymTable symtable;

void initSymbols();
void resetSymbols();
Symbol updateSymTable(const char *buf, unsigned int arity, Symbol &id, bool isNotFunc);
Symbol getNullaryFunctionSymbol();

#endif /* SYMBOLS_H_ */
