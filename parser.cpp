/*
 *      Author: Hao Xu
 */
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include "parser.h"
#include "clause.h"
#include "symbols.h"
#include "arithmetics.h"
#include "termCache.h"
#include "clauseTree.h"

void parseInputFile(const char *filename, LinkedList<Clause *> *t, REGION) {
    char filepath[MAX_PATH_LEN];
    snprintf(filepath, MAX_PATH_LEN, "%s/%s", globalTPTPPath, filename);
	FILE *fp = fopen(filepath, "r");
	if(fp == NULL) {
		printf("cannot open file %s\n", filename);
	} else {
	    fseek(fp, 0, SEEK_END);
	    size_t len = ftell(fp);
	    char *ret = (char *) malloc(len+1);
	    fseek(fp, 0, SEEK_SET);
	    size_t len2 = fread(ret, 1, len, fp);
	    (void) len2;
//	    assert(len == len2);
	    fclose(fp);
	    ret[len] = '\0';
//	    printf("%s", ret);
//	    assert(strlen(ret) == len);

		ParserContext *pc = new ParserContext(CURR_REGION);
		Pointer e(ret);
		parseInput(&e, pc, t, CURR_REGION);
		free(ret);

		Token *token = nextTokenRuleGen(&e, pc, 0);
		if(pc->error || token->type != TK_EOS) {
			printf("parser error, next token %s\n", token->text);
			exit(-1);
		}
	}
}

PARSER_FUNC_BEGIN2(Input, LinkedList<Clause *> *t, REGION)
	LOOP_BEGIN(input)
		TRY(statement)
			NT2(Formula, t, CURR_REGION)
		OR(statement)
			NT2(Include, t, CURR_REGION)
		OR(statement)
			DONE(input)
		END_TRY(statement)
	LOOP_END(input)
PARSER_FUNC_END(Input)

PARSER_FUNC_BEGIN2(Include, LinkedList<Clause *> *t, REGION)
	TTEXT("include");
	TTEXT("(");
	TTYPE(TK_STRING);
	parseInputFile(token->text, t, CURR_REGION);
	TTEXT(")");
	TTEXT(".");
PARSER_FUNC_END(Include)

// precond all unused elements in symtable are set to NULL, all symbols are continuous
PARSER_FUNC_BEGIN2(Formula, LinkedList<Clause *> *clauseTree, REGION)
	char strbuf[STRING_BUF_SIZE];
	Clause *c;
	TTEXT("cnf");
	TTEXT("(");
	TTYPE(TK_TEXT); // formula name
	//printf("parse clause: %s\n", token->text);
	TTEXT(",");
	TTYPE(TK_TEXT); // formula type
	//printf("clause type: %s\n", token->text);
	TTEXT(",");
	NT1(Clause, &c);
	TTEXT(")");
	TTEXT(".");
	c->toString(strbuf, symtable);
	//printf("input clause: %s\n", strbuf);
	clauseTree->append(c);
PARSER_FUNC_END(Formula)

PARSER_FUNC_BEGIN1(Clause, Clause **cRef)
	*cRef = NULL;
	TTEXT("(");
	//printf("clause start\n");

    int dim = 0;
    Term *lits[MAX_NUM_CLAUSE_LITERALS];
    bool signs[MAX_NUM_CLAUSE_LITERALS];
	LOOP_BEGIN(literals)
    	//printf("literal %d start\n", dim);
    	NT2(Literal, &(lits[dim]), &(signs[dim]));
    	//printf("literal %d finished\n", dim);
		dim++;
		TRY(delim)
			TTEXT2(",", "|");
    		//printf("delim %d start\n", dim - 1);
		OR(delim)
			TTEXT(")");
			//printf("clause finished\n");
			DONE(literals);
		END_TRY(delim);
	LOOP_END(literals)

    Clause *c = new (clauseRegion) Clause(dim, 0, -1);
    for(int i=0;i<dim;i++) {
        c->literals[i] = lits[i];
        c->signs[i] = signs[i];
    }

    *cRef = c;
PARSER_FUNC_END(Clause)


PARSER_FUNC_BEGIN2(Literal, Term **termPtr, bool *sign)
	TRY(sign)
		TTEXT("~");
		NT2(Term, termPtr, true);
		*sign = false;
	OR(sign)
		NT2(Term, termPtr, true);
		*sign = true;
	END_TRY(sign)
PARSER_FUNC_END(Literal)

PARSER_FUNC_BEGIN2(Term, Term **termPtr, bool topLevel)
	*termPtr = NULL;
	int dim = 0;
	char fName[1024];
    Symbol i;
	Term *terms[MAX_NUM_CLAUSE_LITERALS];
    char key[MAX_FUNCTION_SYMBOL_DIMENSION * sizeof(Term) +sizeof(Symbol)];
//    Symbol *next;
    TTYPE(TK_TEXT);
    if(isupper(token->text[0])) { // var
		i = updateSymTable(token->text, 0, varid, topLevel);
		//printf("%s:%d\n", buf, i);
		*termPtr = getFromTermMatrix(i);
    } else { // func
		//printf("%s:%d\n", buf, i);
    	strcpy(fName, token->text);
		TRY(paramList)
			TTEXT("(");
			TTEXT(")");
			i = updateSymTable(fName, 0, funcid, topLevel); // arity 0, generate func id
	        *termPtr = getFromTermMatrix(i);
		OR(paramList)
			TTEXT("(");
			LOOP_BEGIN(params)
				NT2(Term, &(terms[dim]), false);
				dim++;
				TRY(delim)
					TTEXT(",");
				OR(delim)
					TTEXT(")");
					DONE(params);
				END_TRY(delim);
			LOOP_END(params)
			// TTEXT(")");
			Symbol *s = (Symbol *) key;
			Term **subtermsKey = (Term **) (key + sizeof(Symbol));
	        i = updateSymTable(fName, dim, funcid, topLevel); // arity dim, generate func id
	        *s = i;
			for(int k = 0;k < dim;k++) {
				subtermsKey[k]=terms[k];
			}
	        *termPtr = getFromTermMatrix(key);
		OR(paramList)
			i = updateSymTable(fName, 0, funcid, topLevel); // arity 0, generate func id
	        *termPtr = getFromTermMatrix(i);
		END_TRY(paramList)
    }
PARSER_FUNC_END(Term)

PARSER_FUNC_BEGIN(Options)
	LOOP_BEGIN(options)
		TRY(option)
			TTEXT2("timeout", "to");
			TTEXT("=");
			TTYPE(TK_INT);
			globalTimeLimit = atoi(token->text);
			printf("timeout=%d\n", globalTimeLimit);
		OR(option)
			TTEXT2("useSuperSymbol", "uss");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalUseSuperSymbols = true;
			OR(bool)
				TTEXT("false");
				globalUseSuperSymbols = false;
			END_TRY(bool)
			printf("useSuperSymbol=%s\n", globalUseSuperSymbols?"true":"false");
		OR(option)
			TTEXT2("useBranchThreshold", "bt");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalUseBranchThreshold = true;
			OR(bool)
				TTEXT("false");
				globalUseBranchThreshold = false;
			END_TRY(bool)
			printf("useBranchThreshold=%s\n", globalUseBranchThreshold?"true":"false");
		OR(option)
			TTEXT2("updateStartingIndex", "usi");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalUpdateStartingIndex = true;
			OR(bool)
				TTEXT("false");
				globalUpdateStartingIndex = false;
			END_TRY(bool)
			printf("updateStartingIndex=%s\n", globalUpdateStartingIndex?"true":"false");
		OR(option)
			TTEXT2("sortSubtreesBySize", "ssbs");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalSortSubtreesBySize = true;
			OR(bool)
				TTEXT("false");
				globalSortSubtreesBySize = false;
			END_TRY(bool)
			printf("sortSubtreesBySize=%s\n", globalSortSubtreesBySize?"true":"false");
		/*OR(option)
			TTEXT2("maxtermsize", "mts");
			TTEXT("=");
			TTYPE(TK_INT);
			universalTermSizeLimit = atoi(token->text);
			printf("maxtermsize=%d\n", universalTermSizeLimit);*/
		OR(option)
			TTEXT2("stacksize", "ss");
			TTEXT("=");
			TTYPE(TK_INT);
			globalCoroutineStackSize = atoi(token->text) * 1024l * 1024;
			printf("stacksize=%ld\n", globalCoroutineStackSize);
		OR(option)
			TTEXT2("statestacksize", "sss");
			TTEXT("=");
			TTYPE(TK_INT);
			globalCoroutineStateStackSize = atoi(token->text) * 1024l * 1024;
			printf("statestacksize=%ld\n", globalCoroutineStateStackSize);
		OR(option)
			TTEXT2("flipLiteral", "fl");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalFlipLiteral = true;
			OR(bool)
				TTEXT("false");
				globalFlipLiteral = false;
			END_TRY(bool)
			printf("flipLiteral=%d\n", globalFlipLiteral);
		OR(option)
			TTEXT2("randomFlipLiteral", "rfl");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalRandomFlipLiteral = true;
			OR(bool)
				TTEXT("false");
				globalRandomFlipLiteral = false;
			END_TRY(bool)
			printf("randomFlipLiteral=%d\n", globalFlipLiteral);
		OR(option)
			TTEXT2("randomLexicalOrder", "rol");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalRandomLexicalOrder = true;
			OR(bool)
				TTEXT("false");
				globalRandomLexicalOrder = false;
			END_TRY(bool)
			printf("randomLexicalOrder=%d\n", globalRandomLexicalOrder);
		OR(option)
			TTEXT2("randomClauseInsert", "rci");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalRandomClauseInsert = true;
			OR(bool)
				TTEXT("false");
				globalRandomClauseInsert = false;
			END_TRY(bool)
			printf("randomClauseInsert=%d\n", globalRandomClauseInsert);
		OR(option)
			TTEXT2("randomClauseLiteral", "rcl");
			TTEXT("=");
			TRY(bool)
				TTEXT("true");
				globalRandomClauseLiteral = true;
			OR(bool)
				TTEXT("false");
				globalRandomClauseLiteral = false;
			END_TRY(bool)
			printf("randomClauseLiteral=%d\n", globalRandomClauseLiteral);

		OR(option)
				TTEXT2("tptppath", "tp");
				TTEXT("=");
				TTYPE(TK_STRING);
				snprintf(globalTPTPPath, MAX_PATH_LEN, "%s", token->text);
				printf("tptppath=%s\n", globalTPTPPath);
		OR(option)
			NEXT_TOKEN;
			printf("unsupported option %s\n", token->text);
			exit(-1);
			ABORT(true);
		END_TRY(option)
		TRY(delim)
			TTEXT(",");
		OR(delim)
			DONE(options);
		END_TRY(delim)
	LOOP_END(options)
PARSER_FUNC_END(Options)

Term *parseTerm(const char *string, REGION) {
	Pointer e(string);
	ParserContext pc(CURR_REGION);
	Term *termPtr;
	parseTerm(&e, &pc, &termPtr, false);
	return termPtr;
}

Clause *parseClause(const char *string, REGION) {
	Pointer e(string);
	ParserContext pc(CURR_REGION);
	Clause *clausePtr;
	parseClause(&e, &pc, &clausePtr);
	return clausePtr;
}
