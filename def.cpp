/*
 * def.cpp
 *
 *      Author: Hao Xu
 */

#include "defs.h"

int universalTermSizeLimit = 0;
#ifdef USE_RELEVANCE
int universalRelevanceLimit = 0;
int universalRelevanceMax = 0;
#endif
int currThreadId;
int globalTimeLimit = 30;
bool globalUseSuperSymbols = false;
bool globalUseBranchThreshold = false;
bool globalUpdateStartingIndex = false;
bool globalSortSubtreesBySize = false;
size_t globalCoroutineStackSize = 16 * 1024 * 1024;
bool globalFlipLiteral = false;
long globalCoroutineStateStackSize = 1024 * 1024;
bool globalRandomClauseLiteral = false;
bool globalRandomClauseInsert = false;
bool globalRandomLexicalOrder = false;
bool globalRandomFlipLiteral = false;
char globalTPTPPath[MAX_PATH_LEN];
ProverStatus globalProverStatus;





