/*
 * model.h
 *
 *      Author: Hao Xu
 */

#ifndef MODEL_H_
#define MODEL_H_

#ifdef USE_RELEVANCE
extern Trie *modelTrieRoot[];
#else
extern Trie modelTrieRoot;
#endif
extern Region *modelTrieRegion;

void initModel();
void resetModel();
void insertClauseInstanceIntoModel(Clause *inst);
void deleteClauseInstanceFromModel(Clause *inst);
int eagerCascadingDeleteClauseFromModel(Clause *inst, Term *elilit);
void orderedResolve(Clause **instPtr);
void printEligibleLiterals();
void checkInstance(Clause *inst, bool elit);
void checkEligibleLiterals();

#endif /* MODEL_H_ */
