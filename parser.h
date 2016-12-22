/* 
 * File:   parser.h
 * Author: Hao Xu
 *
 */

#ifndef PARSER_H
#define	PARSER_H

#include "defs.h"
#include "clause.h"
#include "utils.h"
#include "parserstructs.h"
#include "parsergen.h"
#include "stack.h"

struct ClauseTree;

PARSER_FUNC_PROTO2(Input, LinkedList<Clause *> *clauseTree, REGION);
PARSER_FUNC_PROTO2(Formula, LinkedList<Clause *> *clauseTree, REGION);
PARSER_FUNC_PROTO2(Include, LinkedList<Clause *> *t, REGION);
PARSER_FUNC_PROTO1(Clause, Clause **clausePtr);
PARSER_FUNC_PROTO2(Term, Term **termPtr, bool topLevel);
PARSER_FUNC_PROTO2(Literal, Term **termPtr, bool *sign);
PARSER_FUNC_PROTO(Options);

Clause *parseClause(const char *string, REGION);
Term *parseTerm(const char *string, REGION);
void parseInputFile(const char *filename, LinkedList<Clause*> *t, REGION);

#endif	/* PARSER_H */

