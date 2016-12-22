/*
 * superSymbol.h
 *
 *      Author: Hao Xu
 */

#include "defs.h"
#include "offsetList.h"
#include "term.h"
#include "region.h"

extern OffsetList<Symbol, Symbol*> superSymbolToSequenceOfSymbols;

extern Symbol numSuperSymbol /* initially = 0 */;

Symbol addSuperSymbol(Symbol *sequence);
PMVMInstruction *generateSuperSymbols(PMVMInstruction *inp, OffsetList<Symbol, PMVMInstruction *> *firstOccurVar, REGION);

// set super symbol arity to 255 which unlikely for a normal symbol
// use arity as a marker for fast super symbol detection
#define SUPER_SYMBOL_ARITY ((Symbol)0xff)
#define IS_SUPER_SYMBOL(x) (ARITY(x)==SUPER_SYMBOL_ARITY)






