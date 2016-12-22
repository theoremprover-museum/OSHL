/*
 * File:   main.c
 * Author: Hao Xu
 *
 */

#define MINGW

#ifdef MINGW
#else
#include <sys/resource.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "term.h"
#include "defs.h"
#include "parser.h"
#include "clause.h"
#include "stack.h"
#include "printByType.h"
#include "arithmetics.h"
#include "symbols.h"
#include "acyclicGraph.h"
#include "model.h"
#include "type.h"
#include "ordering.h"
#include "superSymbol.h"
#include "memory.h"
#define DECL_TEST(t) void test##t()
#define TEST_BEGIN(t) \
		DECL_TEST(t) { \
			printf("====================\n"); \
			printf("test " #t "\n"); \
			printf("====================\n"); \

#define TEST_END(t) \
			printf("\n====================\n"); \
			printf("successful " #t "\n"); \
			printf("====================\n"); \
		} \

#define RUN_TEST(t) \
		if(strcmp(testToRun, "alltests") == 0 || strcmp(testToRun, #t) == 0) { \
			test##t(); \
		} \

void testFrameSet1(STACK_DEF);
void testNestedCoroutines1(STACK_DEF);
void testThreshold1(STACK_DEF);
void testSavedState1(STACK_DEF);
void randomTermGenerator(int maxLength, char *buf, int bufsize);
void processInfo();

DECL_TEST(Stack);
DECL_TEST(Terms);
DECL_TEST(Parser);
DECL_TEST(Trie);
DECL_TEST(InverseTrie);
DECL_TEST(SavedState);
DECL_TEST(FrameSet);
DECL_TEST(NestedCoroutines);
DECL_TEST(Threshold);
DECL_TEST(SuperSymbol);
DECL_TEST(EquivRel);
DECL_TEST(LiteralListToType);
DECL_TEST(ClauseTree);
DECL_TEST(ClauseTreeNormalizeVariant);
DECL_TEST(ClauseTreeNormalizeVariant2);
DECL_TEST(ClauseTreeNormalizeVariant3);
DECL_TEST(ClauseTreeNewVar);
DECL_TEST(MatchClauseTree);
DECL_TEST(MatchClauseTreeFull);
DECL_TEST(Arithmetics);
DECL_TEST(Hashtable);
DECL_TEST(GetFromTermMatrix);
DECL_TEST(AcyclicGraph);
DECL_TEST(Model);
DECL_TEST(Quicksort);
DECL_TEST(MainLoop);
DECL_TEST(MainLoop2);
DECL_TEST(MainLoop3);
DECL_TEST(MainLoop4);
DECL_TEST(MainLoop5);
DECL_TEST(MainLoop6);
DECL_TEST(MainLoop7);
DECL_TEST(MainLoop8);
DECL_TEST(MainLoop9);

void runProver(const char *, const char *);
int main(int argc, char** argv) {

        
	processInfo();
	initTermMatrix();
	initArithmetics();
	initModel();
	if(argc <= 2) {
	//    testParser();
		const char *testToRun;
		if(argc == 1) {
			testToRun = "alltests";
		} else {
			testToRun = argv[1];
		}
	    RUN_TEST(Stack);
	    RUN_TEST(SavedState);
	    RUN_TEST(FrameSet);
	    RUN_TEST(NestedCoroutines);
	    RUN_TEST(Threshold);
		RUN_TEST(Hashtable);
		RUN_TEST(Trie);
		RUN_TEST(InverseTrie);
		RUN_TEST(SuperSymbol);
		RUN_TEST(EquivRel);
		RUN_TEST(LiteralListToType);
		RUN_TEST(ClauseTree);
		RUN_TEST(ClauseTreeNewVar);
		RUN_TEST(ClauseTreeNormalizeVariant);
		RUN_TEST(ClauseTreeNormalizeVariant2);
		RUN_TEST(ClauseTreeNormalizeVariant3);
		RUN_TEST(MatchClauseTree);
		RUN_TEST(MatchClauseTreeFull);
		RUN_TEST(Arithmetics);
		RUN_TEST(GetFromTermMatrix);
		RUN_TEST(AcyclicGraph);
		RUN_TEST(Model);
		RUN_TEST(Quicksort);
		RUN_TEST(MainLoop);
		RUN_TEST(MainLoop2);
		RUN_TEST(MainLoop3);
		RUN_TEST(MainLoop4);
		RUN_TEST(MainLoop5);
		RUN_TEST(MainLoop6);
		RUN_TEST(MainLoop7);
		// RUN_TEST(MainLoop8);
		RUN_TEST(MainLoop9);
	} else {
		runProver(argv[1], argv[2]);
	}
        processInfo();
        fflush(stdout);

    return (EXIT_SUCCESS);
}
void processInfo()
{
#ifdef MINGW
    printf("\n\nprocess info not supported on MinGW\n");
#else
	int ret;
	struct rusage usage, *p = &usage;

	ret=getrusage(RUSAGE_SELF,&usage);
	printf("\n\n user time used                   %f\n",  p->ru_utime.tv_sec+p->ru_utime.tv_usec/1000000.0);
	printf(" system time used                 %f\n",  p->ru_stime.tv_sec+p->ru_stime.tv_usec/1000000.0);
	printf(" integral shared memory size      %ld\n",  p->ru_ixrss           );
	printf(" integral unshared data           %ld\n",  p->ru_idrss           );
	printf(" integral unshared stack          %ld\n",  p->ru_isrss           );
	printf(" page reclaims                    %ld\n",  p->ru_minflt          );
	printf(" page faults                      %ld\n",  p->ru_majflt          );
	printf(" swaps                            %ld\n",  p->ru_nswap           );
	printf(" block input operations           %ld\n",  p->ru_inblock         );
	printf(" block output operations          %ld\n",  p->ru_oublock         );
	printf(" messages sent                    %ld\n",  p->ru_msgsnd          );
	printf(" messages received                %ld\n",  p->ru_msgrcv          );
	printf(" signals received                 %ld\n",  p->ru_nsignals        );
	printf(" voluntary context switches       %ld\n",  p->ru_nvcsw           );
	printf(" involuntary                      %ld\n",  p->ru_nivcsw          );
#endif

}

TEST_BEGIN(SavedState)
        UserStack *st = new UserStack();
        testSavedState1(st);
        testSavedState1(st);
        delete st;
TEST_END(SavedState)

void testSavedState1(STACK_DEF) {
#define LEVEL_fun1 1
#define LEVEL_fun2 1
PROTO(fun1, (int))
PROTO(fun2, (int))
    SEC_BEGIN(fun1,(1))

#define FUN fun1
    PARAMS(int, p)
    RETURNS()
    BEGIN
    DEFS(int, a)
    VAR(a) = 1;
    FUNC_CALL(fun2, (VAR(a)));
    printf("a0 = %d\n", VAR(a));
    VAR(a) = 2;
    RETURN();
    END
#undef FUN

#define FUN fun2
    PARAMS(int, a)
    RETURNS()
    BEGIN
    printf("a = %d\n", VAR(a));
    SAVE_STATE(rp);
    RETURN();
    rp:
    printf("a2 = %d\n", VAR(a));
    RETURN();
    END
#undef FUN
    SEC_END
}

TEST_BEGIN(FrameSet)
    UserStack *st = new UserStack();
    testFrameSet1(st);
    delete st;
    
TEST_END(FrameSet)

void testFrameSet1(STACK_DEF) {
	PROTO(fun2, (int))
	PROTO(fun1, (int))
	#define LEVEL_fun2 1
	#define LEVEL_fun1 1
    SEC_BEGIN(fun1, (1))
#define FUN fun2
    PARAMS(int, a)
    RETURNS()
BEGIN
    DEFS()
    printf("a = %d\n", VAR(a));
    SAVE_STATE(rp);
    RETURN();
    rp:
    printf("a2 = %d\n", VAR(a));
    SAVE_STATE(rp2);
    RETURN();
    rp2:
    printf("a3 = %d\n", VAR(a));
    RETURN();
END
#undef FUN

#define FUN fun1
    PARAMS(int, p)
    RETURNS()
BEGIN
    DEFS(int, a,
         ThreadState, fs)
    VAR(a) = 1;
//    FUNC_CALL_INIT(VAR(fs).arBottomStartFrame, VAR(fs).branchPointer, fun2, (VAR(a)));
//    while(VAR(fs).branchPointer!=NULL) {
//    	FUNC_CALL_RESUME(VAR(fs).arBottomStartFrame, VAR(fs).branchPointer, VAR(fs).arBottomStartFrame, VAR(fs).branchPointer);
//    }
END
#undef FUN
    SEC_END
#undef LEVEL_fun2
#undef LEVEL_fun1
}

bool testFinished;
TEST_BEGIN(NestedCoroutines)
	testFinished = false;
	UserStack *st = new UserStack();
	testNestedCoroutines1(st);
	delete st;
	assert(testFinished);

TEST_END(NestedCoroutines)

void testNestedCoroutines1(STACK_DEF) {
	PROTO(fun1, (int))
	PROTO(fun2, (int))
	PROTO(fun2tail, (int))
	PROTO(fun1tail, (int))
	#define LEVEL_fun1 1
	#define LEVEL_fun2 2
	#define LEVEL_fun2tail 2
	#define LEVEL_fun1tail 1
	SEC_BEGIN(fun1, (1))

#define FUN fun1
	PARAMS(int, a1)
	RETURNS()
BEGIN
	DEFS(int, b1)
	VAR(b1) = 2;
	printf("fun1: a1 = %d, b1 = %d\n", VAR(a1), VAR(b1));
	assert(VAR(a1) == 1);
	assert(VAR(b1) == 2);
	// fun2 will perform a tail call to level 1.
	// This tests if the ret pointer is properly set across levels.
	FUNC_CALL(fun2, ((int)3)); // call fun2 at level 2
	assert(VAR(a1) == 1);
	assert(VAR(b1) == 2);
	FUNC_TAIL_CALL(fun2tail, ((int)5)); // tail call fun2tail at level 2
END
#undef FUN

#define FUN fun2
	PARAMS(int, a2)
	RETURNS()
BEGIN
	DEFS(int, b2)
	VAR(b2) = 4;
	printf("fun2: a1 = %d, b1 = %d, a2 = %d, b2 = %d\n", VAR_F1(fun1, a1), VAR_F1(fun1, b1), VAR(a2), VAR(b2));
	assert(VAR_F1(fun1, a1) == 1);
	assert(VAR_F1(fun1, b1) == 2);
	assert(VAR(a2) == 3);
	assert(VAR(b2) == 4);
	FUNC_TAIL_CALL(fun1tail, (VAR(a2)));
END
#undef FUN

#define FUN fun2tail
	PARAMS(int, a2)
	RETURNS()
BEGIN
	DEFS(int, b2)
	VAR(b2) = 6;
	printf("fun2tail: a1 = %d, b1 = %d, a2 = %d, b2 = %d\n", VAR_F1(fun1, a1), VAR_F1(fun1, b1), VAR(a2), VAR(b2));
	assert(VAR_F1(fun1, a1) == 1);
	assert(VAR_F1(fun1, b1) == 2);
	assert(VAR(a2) == 5);
	assert(VAR(b2) == 6);
	testFinished = true;
END
#undef FUN


#define FUN fun1tail
	PARAMS(int, a1)
	RETURNS()
BEGIN
	DEFS(int, b1)
	VAR(b1) = 4;
	printf("fun1tail: a1 = %d, b1 = %d\n", VAR(a1), VAR(b1));
	assert(VAR(a1) == 3);
	assert(VAR(b1) == 4);
END
#undef FUN
	SEC_END

}

// redefined branch threshold
#ifdef BRANCH_THRESHOLD
#undef BRANCH_THRESHOLD
#endif

#define BRANCH_THRESHOLD bt_threshold
int bt_threshold = 0;

TEST_BEGIN(Threshold)
	testFinished = false;
	UserStack *st = new UserStack();
	testThreshold1(st);
	delete st;
	assert(testFinished);
TEST_END(Threshold)

void testThreshold1(STACK_DEF) {
int n[] = {1, 3, 3};
int i = 0;
int bt_sum = 0;
PROTO(thresholdFunc1,())
PROTO(thresholdFunc2,())
#define LEVEL_thresholdFunc1 1
#define LEVEL_thresholdFunc2 1
SEC_BEGIN(thresholdFunc1, ())
#define FUN thresholdFunc1
	PARAMS()
	RETURNS()
	BEGIN
		FUNC_CALL(thresholdFunc2, ());
		if(i<3) {
			bt_threshold = n[i];
			i++;
			RESTORE_STATE;
		}
		printf("sum is %d\n", bt_sum);
		assert(bt_sum == 6620);
		testFinished = true;
	END
#undef FUN
#define FUN thresholdFunc2
	PARAMS()
	RETURNS()
	BEGIN
	/* doesn't work for the current version */
		SAVE_STATE_WITH_THREAD(rp_thresholdFunc_1, BRANCH_POINTER, universalTermSizeLimit, NULL, del);
		SAVE_STATE_WITH_THREAD(rp_thresholdFunc_2, BRANCH_POINTER, universalTermSizeLimit, NULL, del);
		SAVE_STATE_WITH_THREAD(rp_thresholdFunc_3, BRANCH_POINTER, universalTermSizeLimit, NULL, del);
		RETURN();
		rp_thresholdFunc_3:
			printf("rp 3 \n");
			bt_sum ++;
			bt_sum *= 30;
			RETURN();
		rp_thresholdFunc_2:
			printf("rp 2 \n");
			bt_sum ++;
			bt_sum *= 20;
			RETURN();
		rp_thresholdFunc_1:
			printf("rp 1 \n");
			bt_sum ++;
			bt_sum *= 10;
			RETURN();
	END
#undef FUN
	SEC_END
}



void randomTermGenerator( int maxLength, char *buf, int bufsize) {
    if(maxLength <= 1) {
        buf[0] = '\0';
        return;
    }
    int depth = 0;
    int a;
    int i;
    int s;
    int length = 0;
    int mode = 0;
    for(i=0;i<bufsize/2;i++) {
        switch(mode) {
            case 0: // term
                a = rand()%26;
                buf[i] = (char)('a'+a);
                mode = 1;
                length ++;
                break;
            case 1: // after symbol
                if(depth == 0) {
                    buf[i] = '(';
                    mode = 0;
                    depth++;
                } else {
                    s = rand()%3;
                    if(length > maxLength) {
                        s = 2;
                    }
                    switch(s) {
                        case 0:
                            buf[i] = ',';
                            mode = 0;
                            break;
                        case 1:
                            buf[i] = '(';
                            mode = 0;
                            depth++;
                            break;
                        case 2:
                            buf[i] = ')';
                            depth --;
                            mode = 2;
                            break;

                    }
                }
                break;
            case 2: // after term
                s = rand()%2;
                if(length > maxLength) {
                    s = 1;
                }
                switch(s) {
                    case 0:
                        if(depth == 0) {
                            buf[i] = '\n';
                        } else {
                            buf[i] = ',';
                        }
                        mode = 0;
                        break;
                    case 1:
                        if(depth == 0) {
                            length = 0;
                            buf[i] = '\n';
                            mode = 0;
                        } else {
                            buf[i] = ')';
                            depth --;
                            mode = 2;
                        }
                        break;

                }
                break;
        }
    }
        switch(mode) {
            case 0: // term
                a = rand()%26;
                buf[i++] = 'a'+a;
                break;
            case 1: // after symbol
            case 2: // after term
                break;
        }
        int k;
        for(k=0;k<depth;k++) {
            buf[i++] = ')';
        }
        buf[i++] = '\0';
}
Trie *constructTrie(const char *str, int &counter, REGION) {
    char buf[STRING_BUF_SIZE];
    Trie *trie = new (CURR_REGION) Trie();
    ParserContext pc(CURR_REGION);
    Pointer e(str);

    printf("inserting terms\n");
    int counter2 = 0;
    Term *t;
    while(true) {
        parseTerm(&e, &pc, &t, false);
        if(t == NULL) {
        	break;
        }
        t->toString(buf, symtable);
        counter2++;
        t->compile(CURR_REGION);
        //if(findInstanceFromTrie(t->flatterm, trie, NULL, &ssbuf)==NULL && counter2 > 83 && counter2 < 94) {
            counter ++;
            printf("inserting %s\n", buf);
            t->inp->toString(buf, symtable);
            printf("flatterm: %s\n", buf);
            trie->insert(t, CURR_REGION);
            trie->toString(buf, symtable);
            printf("%s\n", buf);
        //} else {
        //    printf("skipped %s\n", buf);
        //}

        //printf("inserted\n", buf);
        //trieToString(trie, buf, symtable);
        //printf("%s", buf);
    }
    printf("%d inserted, %d items\n", counter, trie->items);
    return trie;

}
void deleteTrie(const char *str, Trie* trie, REGION) {
    char buf[STRING_BUF_SIZE];

    ParserContext pc(CURR_REGION);
    Pointer e(str);
        printf("deleting terms\n");
        Term *t;
    while(true) {
        parseTerm(&e, &pc, &t, false);
        if(t == NULL) {
        	break;
        }
        t->toString(buf, symtable);
        printf("deleting %s\n", buf);
        trie->remove(t);
        printf("deleted %s\n", buf);
        trie->toString(buf, symtable);
        printf("%s", buf);
    }

}
TEST_BEGIN(Trie)
    MAKE_REGION;

//	const int bufsize = 60000;
//	char tbuf[bufsize];
//	randomTermGenerator(10, tbuf, bufsize);
//	printf("%d, %s\n", (int)strlen(tbuf), tbuf);

	resetTermMatrix();
	resetSymbols();
	resetArithmetics();
    symtable.insert(0, "<root>");
    char const *str = "f(a,b) g(a,b) g(a,f(a,b)) g(a,f(b,a)) g(f(a,a),f(b,a)) g(f(b,b),a) g(W,Z) g(W,W)";
    int counter = 0;
    Trie *trie = constructTrie(str, counter, CURR_REGION);
    assert(counter == trie->items);

    char buf[STRING_BUF_SIZE];
    char const *pattern = "g(X, Y)";
    Term *p = parseTerm(pattern, CURR_REGION);
    p->compile(CURR_REGION);
    

    trie->toString(buf, symtable);
    printf("%s", buf);

    UserStack *st = new UserStack();
    runTest(1, p, NULL, trie, NULL, st, CURR_REGION);
    
    printf("Hashtable Load Balance = %f, %d\n", Trie::globalTrieNodeHashtable->balance(), Trie::globalTrieNodeHashtable->len);
    //getchar();
    deleteTrie(str, trie, CURR_REGION);
    assert(0 == trie->items);
    FREE_REGION;
TEST_END(Trie)

TEST_BEGIN(InverseTrie)
    MAKE_REGION;
    resetSymbols();
	resetArithmetics();
	resetTermMatrix();

    symtable.insert(0, "<root>");
    char const *str = /*terms; //*/ "g(h(a),a) g(a,h(b)) f(a,b) g(a,b) g(a,f(a,b)) g(a,f(b,a)) g(f(a,a),f(b,a)) g(f(b,b),a)";
    int counter = 0;
    Trie *trie = constructTrie(str, counter, CURR_REGION);
    assert(counter == trie->items);

    char buf[STRING_BUF_SIZE];
    char const *pattern = "g(X, Y)";
    Term *p = parseTerm(pattern, CURR_REGION);
    p->compile(CURR_REGION);


    trie->toString(buf, symtable);
    printf("%s", buf);

    UserStack *st = new UserStack();
    runTest(2, p, NULL, trie, NULL, st, CURR_REGION);

    printf("Hashtable Load Balance = %f, %d\n", Trie::globalTrieNodeHashtable->balance(), Trie::globalTrieNodeHashtable->len);
    //getchar();
    deleteTrie(str, trie, CURR_REGION);
    assert(0 == trie->items);
    FREE_REGION;
TEST_END(InverseTrie)

TEST_BEGIN(Parser)
    MAKE_REGION;
	resetTermMatrix();
    char const *input= "(a(X,Y),~x(y,z()))";

    char buf[1024];
    parseTerm(input+1, CURR_REGION)->toString(buf, symtable);
    // printf("%s\n", buf);
    assert(strcmp(buf, "a(X,Y)") == 0);
    Clause *c = parseClause(input, CURR_REGION);
    c->toString(buf, symtable);
    // printf("%s\n", buf);
    assert(strcmp(buf, "[a(X,Y)|~x(y,z)]") == 0);
    FREE_REGION;
TEST_END(Parser)

TEST_BEGIN(SuperSymbol)
	MAKE_REGION;

	resetTermMatrix();
	resetSymbols();
	resetArithmetics();
	char const *str = "p(f(X,b,Z), g(a,Y,b))";

//	char buf[STRING_BUF_SIZE];
	Term *p = parseTerm(str, CURR_REGION);
	p->compile(CURR_REGION);
	PMVMInstruction *newInp = generateSuperSymbols(p->inp, NULL, CURR_REGION);

	printf("number of super symbols = %ld\n", numSuperSymbol);
	assert(numSuperSymbol == 2);

	PMVMInstruction *pInp = newInp;
	while(pInp->subterm!=NULL) pInp ++;
	assert(pInp - newInp == 7);

	// test that no repeated super symbol is generated
	generateSuperSymbols(p->inp, NULL, CURR_REGION);

	printf("number of super symbols = %ld\n", numSuperSymbol);
	assert(numSuperSymbol == 2);

	FREE_REGION;
TEST_END(SuperSymbol)

int cmpAdd(int a, int b, int c);

void cmpStack1(int a, int b) {
    cmpAdd(a, b, 0);
}

int cmpAdd(int a, int b, int c) {
    int c2;

    if(a==b) {
        return a + c;
    } else {
        c2 = cmpAdd(a+1, b, a+c);

        return c2;
    }
}
int testStack1(int a, int b, STACK_DEF) {
    int ret;
#define LEVEL_print 1
#define LEVEL_add 1
PROTO(print, (int, int), int)
PROTO(add, (int, int, int), int)
    SEC_BEGIN(print, (a, b), ret);
#define FUN print
    PARAMS(int, a, int, b)
    RETURNS(int)
    BEGIN
    DEFS(int, c)

    FUNC_CALL(add, (VAR(a), VAR(b), (int)0),VAR(c));
    //printf("%d+...+%d=%d\n", VAR(a), VAR(b), VAR(c));
    RETURN(VAR(c));
    END
#undef FUN

#define FUN add
    PARAMS(int, a, int, b, int, c)
    RETURNS(int)
    BEGIN


    if(VAR(a) == VAR(b)) {
        RETURN(VAR(a) + VAR(c));
    } else {
//        FUNC_CALL(3, add, (VAR(a)+1, VAR(b), VAR(a)+VAR(c)),c2);
        
//        FUNC_RET(c2);
        FUNC_TAIL_CALL(add, (VAR(a)+1, VAR(b), VAR(a)+VAR(c)));
        // RETURN(ret);
    }
    END
#undef FUN

    SEC_END;
    return ret;
}

TEST_BEGIN(Stack)
    UserStack *st = new UserStack();
    time_t stime, ftime;
    time(&stime);
    int i;
    int n = 1;//00000;
    int a = 1;
    int b = 10000;
    for(i=0;i<n;i++) {
        int ret = testStack1(a, b, st);
        if(i==0) {
            printf("%d+...+%d=%d\n", a, b, ret);
        }
    }
    time(&ftime);
    printf("time stack ave = %f\n", (ftime - stime) / (double)n);
    delete st;

    time(&stime);
    for(i=0;i<n;i++) {
        cmpStack1(a, b);
    }
    time(&ftime);
    printf("time cmp ave = %f\n", (ftime - stime) / (double)n);


TEST_END(Stack)

TEST_BEGIN(Terms)

	resetTermMatrix();
    MAKE_REGION;
    char buf[STRING_BUF_SIZE];
    Term *s = new (CURR_REGION) Term(-2, 1, CURR_REGION);
    s->subterms[0] = new (CURR_REGION) Term(-1);
    Term *t = new (CURR_REGION) Term(-3, 2, CURR_REGION);
    t->subterms[0] = new (CURR_REGION) Term(2);
    t->subterms[1] = new (CURR_REGION) Term(1);
    Term *t2 = new (CURR_REGION) Term(-3, 2, CURR_REGION);
    t2->subterms[0] = new (CURR_REGION) Term(1);
    //t2->subterms[1] = offsetVarId(t,10, CURR_REGION);
    Clause *c = new (clauseRegion) Clause(3, 0, 3);
    c->signs[0] = -1;
    c->signs[1] = 1;
    c->signs[2] = -1;
    c->literals[0] = s;
    c->literals[1] = t;
    c->literals[2] = t2;
    //TermFunc *tf = newTermFunc(t, CURR_REGION);
    //TermFunc *rtf = newReverseTermFunc(t, CURR_REGION);

    SymTable symtable;
    symtable.insert(1, "a");
    symtable.insert(2, "f");
    symtable.insert(3, "g");

    //offsetVarId(t, 10, CURR_REGION)->toString(buf, symtable);
    //printf("%s\n", buf);

    c->toString(buf, symtable);
    printf("%s\n", buf);

    //termFuncToString(tf, buf, symtable);
    //printf("%s\n", buf);

    //termFuncToString(rtf, buf, symtable);
    //printf("%s\n", buf);

    c->normalize(varid);
    c->toString(buf, symtable);
    printf("%s\n", buf);

    Sub *sub = new (CURR_REGION) Sub();
    if(unify(t, t2, sub, CURR_REGION)) {
        extractTermFromSub(0, sub)->toString(buf, symtable);
        printf("%s\n", buf);
        extractTermFromSub(1, sub)->toString(buf, symtable);
        printf("%s\n", buf);
    }

    FREE_REGION;

TEST_END(Terms)

TEST_BEGIN(ClauseTreeNewVar)
    MAKE_REGION;
	resetTermMatrix();
	resetSymbols();
    Clause *c;
    ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
    resetSymbols();
    const char *str[] = {
        "(~p(X, Y), ~q(X, W))",
        "(~p(X, Y), ~q(W, W))",
        "(q(X), p(X, Y))",
        NULL
    };
    int i = 0;
    char strbuf[STRING_BUF_SIZE];
    try {
        while(str[i]!=NULL) {
                c = parseClause(str[i], CURR_REGION);
                c->toString(strbuf, symtable);
				printf("%s\n", strbuf);
                clauseTree->addClause(c, CURR_REGION);
                i++;
        }
        clauseTree->newVars();
        clauseTree->compile(CURR_REGION);
        clauseTree->toString(strbuf, symtable);
        puts(strbuf);
        ClauseTree::Node *node = clauseTree->root->subtrees[0], *node0, *node1, *node2;
        assert((node->inp+1)->newVar);
        assert((node->inp+2)->newVar);
        node0=node->subtrees[0];
        node1=node->subtrees[1];
        node2=clauseTree->root->subtrees[1]->subtrees[0];
        assert(!(node0->inp+1)->newVar);
        assert((node0->inp+2)->newVar);
        assert((node1->inp+1)->newVar);
        assert(!(node1->inp+2)->newVar);
        assert(!(node2->inp+1)->newVar);
		assert((node2->inp+2)->newVar);

    } catch(const char *p) {
        puts(str[i]);
        while(p--!=str[i])
            putchar(' '); 
        puts("^");
    }
    FREE_REGION;
TEST_END(ClauseTreeNewVar)

TEST_BEGIN(EquivRel)
	MAKE_REGION;
	resetTermMatrix();
	resetSymbols();
	resetArithmetics();
	EquivalenceRelation<Symbol> equiv;
	LinkedList<Term *> pos;
	LinkedList<Term *> neg;

	const char *strpos[] = {
		"p(Z, Y)",
		"q(W, W)",
		"r(X1, Y1, f(Z))",
		NULL
	};
	const char *strneg[] = {
		"p(Y2, X2)",
		"q(W2, W2)",
		"r(g(X2), Y2, f(Z2))",
		NULL
	};
	for(int i =0;strpos[i]!=NULL;i++) {
		Term *t = parseTerm(strpos[i], CURR_REGION);
		pos.append(t);
	}
	for(int i =0;strneg[i]!=NULL;i++) {
		Term *t = parseTerm(strneg[i], CURR_REGION);
		neg.append(t);
	}
	literalListToEquiv(&equiv, &pos, &neg);
	char strbuf[STRING_BUF_SIZE];
	equiv.toString(strbuf, symtable);
	printf("%s\n", strbuf);
	assert(equiv.findEquivRep(symtable["Y"]) == symtable["X2"]);
	assert(equiv.findEquivRep(symtable["Z"]) == symtable["Z2"]);
	assert(equiv.findEquivRep(symtable["Y2"]) == symtable["Z2"]);
	assert(equiv.findEquivRep(symtable["W"]) == symtable["W2"]);
	assert(equiv.findEquivRep(symtable["Y1"]) == symtable["Z2"]);
	assert(equiv.findEquivRep(symtable["W2"]) == symtable["W2"]);
	assert(equiv.findEquivRep(symtable["X2"]) == symtable["X2"]);
	assert(equiv.findEquivRep(symtable["Z2"]) == symtable["Z2"]);
	FREE_REGION;
TEST_END(EquivRel)

TEST_BEGIN(LiteralListToType)
	MAKE_REGION;
	resetTermMatrix();
	resetSymbols();
	resetArithmetics();
	resetTypes();
	EquivalenceRelation<Symbol> equiv;
	LinkedList<Term *> pos;
	LinkedList<Term *> neg;
	LinkedList<Symbol> vars;

	const char *strpos[] = {
		"p(Z, Y)",
		"q(W, W)",
		"r(X1, Y1, f(Z))",
		NULL
	};
	const char *strneg[] = {
		"p(Y2, X2)",
		"q(W2, W2)",
		"r(g(X2), Y2, f(Z2))",
		NULL
	};
	for(int i =0;strpos[i]!=NULL;i++) {
		Term *t = parseTerm(strpos[i], CURR_REGION);
		pos.append(t);
	}
	for(int i =0;strneg[i]!=NULL;i++) {
		Term *t = parseTerm(strneg[i], CURR_REGION);
		neg.append(t);
	}

	vars.append(symtable["Z"]);
	vars.append(symtable["Y"]);
	vars.append(symtable["W"]);
	vars.append(symtable["X1"]);
	vars.append(symtable["Y1"]);
	vars.append(symtable["X2"]);
	vars.append(symtable["Y2"]);
	vars.append(symtable["Z2"]);
	vars.append(symtable["W2"]);
	literalListToTypes(&pos, &neg, &vars);
	char strbuf[STRING_BUF_SIZE];
	Symbol var = symtable["X1"];
	Type *t = typeSymbolToType[HASHTABLE_KEY(var)];
	t->toString(strbuf, symtable);
	printf("%s: %s\n", "X1", strbuf);
	var = symtable["X2"];
	t = typeSymbolToType[HASHTABLE_KEY(var)];
	t->toString(strbuf, symtable);
	printf("%s: %s\n", "X2", strbuf);
//	var = symtable["Z2"];
//	t = typeSymbolToType[HASHTABLE_KEY(var)];
//	t->toString(strbuf, symtable);
//	printf("%s: %s\n", "Z2", strbuf);
	var = symtable["Z"];
	t = typeSymbolToType[HASHTABLE_KEY(var)];
	assert(t == NULL);

	FREE_REGION;
TEST_END(LiteralListToType)

TEST_BEGIN(ClauseTree)
	MAKE_REGION;
	resetTermMatrix();
	resetSymbols();
	resetArithmetics();
	Clause *c;
	ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
	const char *str[] = {
		"(~p(X, Y), ~q(Z, W))",
		"(~p(X, Y), ~q(a, W))",
		"(~q(Z, b), ~p(X, Y))",
		"(~p(Z, X), ~q(W, a))",
		"(~p(Z, X), ~q(X, a))", // not a repeated clause
		"(~q(X, c), ~p(Z, X))",
		"(~p(X, X), ~p(Z, X))",
		"(~p(X, X), ~p(Z, X), ~r(X, Y))",
		"(~p(X, Y), ~q(a, W))",
		"(~p(X, Y), q(a, W))",
		"(p(X, Y), ~q(a, W))",
		"(p(X, Y), ~q(a, W), r(X, Y))",
		NULL
	};
	int i = 0;
	char strbuf[STRING_BUF_SIZE];
	try {
		while(str[i]!=NULL) {
				c = parseClause(str[i], CURR_REGION);
				c->toString(strbuf, symtable);
				printf("%s\n", strbuf);
				clauseTree->addClause(c, CURR_REGION);
				i++;
		}
		clauseTree->newVars();
		clauseTree->compile(CURR_REGION);
		clauseTree->toString(strbuf, symtable);
		puts(strbuf);
		assert (strcmp(strbuf,
			"<root>[2]\n"
			" ~p(X,Y) | p(102) ?0 ?1 [8]\n"
			"  ~q(Z,W) | q(202) ?2 ?3 *[0]\n"
			"  ~q(a,?4) | q(202) a(300) ?4 *[0]\n"
			"  ~q(?5,b) | q(202) ?5 b(400) *[0]\n"
			"  ~q(?6,a) | q(202) ?6 a(300) *[0]\n"
			"  ~q(Y,a) | q(202) ?1 a(300) *[0]\n"
			"  ~q(Y,c) | q(202) ?1 c(500) *[0]\n"
			"  ~p(Y,Y) | p(102) ?1 ?1 *[1]\n"
			"   ~r(Y,?7) | r(602) ?1 ?7 *[0]\n"
			"  q(a,?8) | q(202) a(300) ?8 *[0]\n"
			" ~q(a,?9) | q(202) a(300) ?9 [1]\n"
			"  p(?A,?B) | p(102) ?A ?B *[1]\n"
			"   r(?A,?B) | r(602) ?A ?B *[0]\n"
			) == 0);
	} catch(const char *p) {
		puts(str[i]);
		while(p--!=str[i])
			putchar(' ');
		puts("^");
	}
	FREE_REGION;
TEST_END(ClauseTree)

TEST_BEGIN(ClauseTreeNormalizeVariant)
	MAKE_REGION;
	resetTermMatrix();
	Clause *c;
	ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
	resetSymbols();
	const char *str[] = {
			"(~p(P))",
			"(h(H), ~p(P))",
//		"( ~ person(Person)| lives(Person,house_1)| lives(Person,house_2)| lives(Person,house_3)| lives(Person,house_4)| lives(Person,house_5) )",
//		"( ~ person(Person)| drinks(Person,orange)| drinks(Person,coffee)| drinks(Person,tea)| drinks(Person,milk)| drinks(Person,water) )",
//		"( ~ person(Person)| drives(Person,masserati)| drives(Person,saab)| drives(Person,porsche)| drives(Person,honda)| drives(Person,jaguar) )",
//		"( ~ person(Person)| owns(Person,dog)| owns(Person,snails)| owns(Person,horse)| owns(Person,fox)| owns(Person,zebra) )",
//		"( is_color(H,yellow)| ~ person(Person)| ~ drives(Person,masserati)| ~ house(H)| ~ lives(Person,H) )",
//		"( next_to(House_1,House_2)| ~ person(Person_1)| ~ owns(Person_1,fox)| ~ house(House_1)| ~ lives(Person_1,House_1)| ~ person(Person_2)| ~ drives(Person_2,saab)| ~ house(House_2)| ~ lives(Person_2,House_2) )",
//		"( owns(P,snails)| ~ person(P)| ~ drives(P,porsche) )",
//		"( drinks(P,orange)| ~ person(P)| ~ drives(P,honda) )",
//		"( next_to(House_1,House_2)| ~ person(Person_1)| ~ drives(Person_1,masserati)| ~ house(House_1)| ~ lives(Person_1,House_1)| ~ person(Person_2)| ~ owns(Person_2,horse)| ~ house(House_2)| ~ lives(Person_2,House_2))",
//		"(~p(X, Y), q(a, W))",
//		"(p(X, Y), ~q(a, W))",
//		"(p(X, Y), ~q(a, W), r(X, Y))",
		NULL
	};
	int i = 0;
	char strbuf[STRING_BUF_SIZE];
	try {
		while(str[i]!=NULL) {
				c = parseClause(str[i], CURR_REGION);
				c->toString(strbuf, symtable);
				printf("%s\n", strbuf);
				// c->sortBySign();
				// c->normalize(VAR_ID_OFFSET);
				// c->toString(strbuf, symtable);
				// printf("normalized: %s\n", strbuf);
				// sortBySign must be called before normalize in addClause, otherwise this test will fail
				clauseTree->addClause(c, CURR_REGION);
				i++;
		}
		clauseTree->newVars();
		clauseTree->compile(CURR_REGION);
		clauseTree->toString(strbuf, symtable);
		puts(strbuf);
		assert (strcmp(strbuf,
				"<root>[1]\n"
				" ~p(P) | p(101) ?0 *[1]\n"
				"  h(H) | h(201) ?1 *[0]\n") == 0);
	} catch(const char *p) {
		puts(str[i]);
		while(p--!=str[i])
			putchar(' ');
		puts("^");
	}
	FREE_REGION;
TEST_END(ClauseTreeNormalizeVariant)

TEST_BEGIN(ClauseTreeNormalizeVariant2)
	MAKE_REGION;
	resetTermMatrix();
	Clause *c;
	ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
	resetSymbols();
	const char *str[] = {
			"(~p(P))",
			"(~h(H), ~p(P))",
		NULL
	};
	int i = 0;
	char strbuf[STRING_BUF_SIZE];
	try {
		while(str[i]!=NULL) {
				c = parseClause(str[i], CURR_REGION);
				c->toString(strbuf, symtable);
				printf("%s\n", strbuf);
				// c->sortBySign();
				// c->normalize(VAR_ID_OFFSET);
				// c->toString(strbuf, symtable);
				// printf("normalized: %s\n", strbuf);
				// sortBySign must be called before normalize in addClause, otherwise this test will fail
				clauseTree->addClause(c, CURR_REGION);
				i++;
		}
		clauseTree->newVars();
		clauseTree->compile(CURR_REGION);
		clauseTree->toString(strbuf, symtable);
		puts(strbuf);
		assert (strcmp(strbuf,
				"<root>[1]\n"
				" ~p(P) | p(101) ?0 *[1]\n"
				"  ~h(H) | h(201) ?1 *[0]\n") == 0);
	} catch(const char *p) {
		puts(str[i]);
		while(p--!=str[i])
			putchar(' ');
		puts("^");
	}
	FREE_REGION;
TEST_END(ClauseTreeNormalizeVariant2)

	TEST_BEGIN(ClauseTreeNormalizeVariant3)
		MAKE_REGION;
		resetTermMatrix();
		Clause *c;
		ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
		resetSymbols();
		const char *str[] = {
				"(~p(X,Y), q(Y,X))",
				"(~p(X,Y), q(X,Y))",
			NULL
		};
		int i = 0;
		char strbuf[STRING_BUF_SIZE];
		try {
			while(str[i]!=NULL) {
					c = parseClause(str[i], CURR_REGION);
					c->toString(strbuf, symtable);
					printf("%s\n", strbuf);
					clauseTree->addClause(c, CURR_REGION);
					i++;
			}
			clauseTree->newVars();
			clauseTree->compile(CURR_REGION);
			clauseTree->toString(strbuf, symtable);
			puts(strbuf);
			assert (strcmp(strbuf,
					"<root>[1]\n"
					" ~p(X,Y) | p(102) ?0 ?1 [2]\n"
					"  q(Y,X) | q(202) ?1 ?0 *[0]\n"
					"  q(X,Y) | q(202) ?0 ?1 *[0]\n") == 0);
		} catch(const char *p) {
			puts(str[i]);
			while(p--!=str[i])
				putchar(' ');
			puts("^");
		}
		FREE_REGION;
	TEST_END(ClauseTreeNormalizeVariant3)

TEST_BEGIN(MatchClauseTree)
    MAKE_REGION;
	resetTermMatrix();
	resetSymbols();
// construct trie;
    symtable.insert(0, "<root>");
    char const *str1 = /*terms; //*/ "p(a,b) p(a,a) p(b,a) q(a,b) q(a,f(a,b)) q(a,f(b,a)) q(f(a,a),f(b,a)) q(f(b,b),a)";
    int counter = 0;
    Trie *trie = constructTrie(str1, counter, CURR_REGION);
    assert(counter == trie->items);
    assert(counter == 8);

    Clause *c;
    ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
    const char *str[] = {
        "(~p(X, Y), ~q(Z, W))",
        "(~p(X, Y), ~q(a, W))",
        "(~p(X, X), ~p(Z, X))",
        NULL
    };
    int i = 0;
    char strbuf[STRING_BUF_SIZE];
    try {
        while(str[i]!=NULL) {
                c = parseClause(str[i], CURR_REGION);
                c->toString(strbuf, symtable);
				printf("%s\n", strbuf);
                clauseTree->addClause(c, CURR_REGION);
                i++;
        }
    	clauseTree->newVars();
    	clauseTree->compile(CURR_REGION);
        clauseTree->root->generateTrieNodes(trie, CURR_REGION);
        clauseTree->toString(strbuf, symtable);
        puts(strbuf);

        puts("Matching clause tree");
        UserStack *st = new UserStack();
        runTest(3, NULL, clauseTree, trie, NULL, st, CURR_REGION);

    } catch(const char *p) {
        puts(str[i]);
        while(p--!=str[i])
            putchar(' ');
        puts("^");
    }
    assert(clauseTree->root->degree == 1);
    assert(clauseTree->root->subtrees[0]->degree == 3);

    FREE_REGION;

TEST_END(MatchClauseTree)

TEST_BEGIN(MatchClauseTreeFull)
    MAKE_REGION;
	resetTermMatrix();
	resetSymbols();
// construct trie;
    symtable.insert(0, "<root>");
    char const *str1 = /*terms; //*/ "p(a,b) p(a,a) p(b,a) q(a,b) q(a,f(a,b)) q(a,f(b,a)) q(f(a,a),f(b,a)) q(f(b,b),a)";
    int counter = 0;
    Trie *trie = constructTrie(str1, counter, CURR_REGION);
    parseTerm("c", CURR_REGION); // add c into symbol table
    assert(counter == trie->items);
    assert(counter == 8);

    Clause *c;
    ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
    const char *str[] = {
        "(~p(X, Y), q(Z, W))",
        "(~p(X, Y), q(a, W))",
        "(~p(X, X), p(Z, X))",
        NULL
    };
    int i = 0;
    char strbuf[STRING_BUF_SIZE];
    try {
        while(str[i]!=NULL) {
                c = parseClause(str[i], CURR_REGION);
                c->toString(strbuf, symtable);
				printf("%s\n", strbuf);
                clauseTree->addClause(c, CURR_REGION);
                i++;
        }
    	clauseTree->newVars();
    	clauseTree->compile(CURR_REGION);
        clauseTree->root->generateTrieNodes(trie, CURR_REGION);
        clauseTree->toString(strbuf, symtable);
        puts(strbuf);

        puts("Matching clause tree with both positive and negative literals");
        UserStack *st = new UserStack();
        runTest(4, NULL, clauseTree, trie, NULL, st, CURR_REGION);

    } catch(const char *p) {
        puts(str[i]);
        while(p--!=str[i])
            putchar(' ');
        puts("^");
    }

    FREE_REGION;

TEST_END(MatchClauseTreeFull)

TEST_BEGIN(Arithmetics)
	initArithmetics();
	MaxFunctionSymbolDimension = 3;
	NumberOfFunctionSymbols[0] = 0;
	NumberOfFunctionSymbols[1] = 1;
	NumberOfFunctionSymbols[2] = 1;
	NumberOfFunctionSymbols[3] = 1;

	assert(numberOfDistinctTerms(1) == 0);
	assert(numberOfDistinctTerms(2) == 0);
	assert(numberOfDistinctTerms(3) == 0);
	assert(numberOfDistinctTerms(4) == 0);

	initArithmetics();
	MaxFunctionSymbolDimension = 3;
	NumberOfFunctionSymbols[0] = 3; // a b c
	NumberOfFunctionSymbols[1] = 1; // f
	NumberOfFunctionSymbols[2] = 1; // g
	NumberOfFunctionSymbols[3] = 1; // h

	unsigned int n[] = {
			0,
			3, // a b c
			3, // f(T1) 3
			12, // f(T2) 3 g(T1, T1) 9
			57, // f(T3) 12 g(T1, T2) 9 g(T2, T1) 9 h(T1, T1, T1) 27
			219, // f(T4) 57 g(T1, T3) 36 g(T2, T2) 9 g(T3, T1) 36 h(T1, T1, T2) 27 * 3
			1038 // f(T5) 219 g(T1, T4) 171 g(T2, T3) 36 g(T3, T2) 36 g(T4, T1) 171
				 // h(T1, T1, T3) 108 * 3 h(T1, T2, T2) 27 * 3

	};
	for(unsigned int i=1;i<=6;i++) {
		printf("n=%d, %d\n", i, numberOfDistinctTerms(i));
		assert(numberOfDistinctTerms(i) == n[i]);
	}
TEST_END(Arithmetics)

TEST_BEGIN(Hashtable)
MAKE_REGION;
	printf("test hashtable var len string -> int\n");
	Hashtable<int, -1> strToInt(1000, CURR_REGION);
	strToInt.insert("abc", 1);
	strToInt.insert("wxyz", 2);
	assert(strToInt["abc"] == 1);
	assert(strToInt["wxyz"] == 2);
	printf("test hashtable fixed len key -> int\n");
	Hashtable<int, 4> fixedLenToInt(1000, CURR_REGION);
	fixedLenToInt.insert("abcd", 1);
	fixedLenToInt.insert("wxyz", 2);
	assert(fixedLenToInt["abcd"] == 1);
	assert(fixedLenToInt["wxyz"] == 2);
	printf("test hashtable explicit var len key -> int\n");
	Hashtable<int, 0> explicitVarLenToInt(1000, CURR_REGION);
	explicitVarLenToInt.insert("abc", 1, 3);
	explicitVarLenToInt.insert("wxyz", 2, 4);
	assert(explicitVarLenToInt.lookup("abc", 3) == 1);
	assert(explicitVarLenToInt.lookup("wxyz", 4) == 2);
	FREE_REGION;
TEST_END(Hashtable)

TEST_BEGIN(GetFromTermMatrix)
    resetSymbols();
	resetTermMatrix();
	Symbol a = (Symbol) 0x100;
	Symbol b = (Symbol) 0x200;
	Symbol h = (Symbol) 0x301;
	Symbol f = (Symbol) 0x402;
	symtable.insert(a, "a");
	symtable.insert(b, "b");
	symtable.insert(h, "h");
	symtable.insert(f, "f");
	Symbol ft1[] = {a};
	Symbol ft2[] = {f, a, b};
	Symbol ft3[] = {f, a, a};
	Symbol ft4[] = {h, a};
	Symbol ft5[] = {f, h, a, f, a, h, a};
	Symbol ft6[] = {h, f, a, f, a, h, a};
	Symbol ft7[] = {h, h, h, h, f, b, b};
	Symbol* ft;
	Symbol* fts[] = {ft1, ft2, ft3, ft4, ft5, ft6, ft7};
	const char *str[] = {"a", "f(a,b)", "f(a,a)", "h(a)", "f(h(a),f(a,h(a)))", "h(f(a,f(a,h(a))))", "h(h(h(h(f(b,b)))))"};
	Term *t;
    char strbuf[STRING_BUF_SIZE];
    for(int i=0;i<7;i++) {
		t = getFromTermMatrix(fts[i], ft);
		t->toString(strbuf, symtable);
		assert(strcmp(strbuf, str[i]) == 0);
		printf("%s\n", strbuf);
    }

TEST_END(GetFromTermMatrix)

char acyclicGraphNodeBuffer[1024];
char buf[STRING_BUF_SIZE], buf2[STRING_BUF_SIZE];

void printAcyclicGraphNode(AcyclicGraph<int> *node) {
	printf("visiting node %d\n", node->value);
	sprintf(acyclicGraphNodeBuffer + strlen(acyclicGraphNodeBuffer), "%d ", node->value);
	LinkedList<Pair<AcyclicGraph<int> *, void *> *>::Node *curr = node->out.head;
	while(curr!=NULL) {
		printf("outgoing edge (%d,%d)\n", node->value, curr->value->fst->value);
		sprintf(acyclicGraphNodeBuffer + strlen(acyclicGraphNodeBuffer), "->%d ", curr->value->fst->value);
		curr = curr->next;
	}
	curr = node->in.head;
	while(curr!=NULL) {
		printf("incoming edge (%d,%d)\n", node->value, curr->value->fst->value);
		sprintf(acyclicGraphNodeBuffer + strlen(acyclicGraphNodeBuffer), "<-%d ", curr->value->fst->value);
		curr = curr->next;
	}
}
void printClauseGraphNode(AcyclicGraph<Clause *> *node) {
	node->value->toString(buf, symtable, 0);
	printf("visiting node %s\n", buf);
	sprintf(acyclicGraphNodeBuffer + strlen(acyclicGraphNodeBuffer), "%s ", buf);
	LinkedList<Pair<AcyclicGraph<Clause *> *, void *> *>::Node *curr = node->out.head;
	while(curr!=NULL) {
		curr->value->fst->value->toString(buf2, symtable, 0);
		printf("outgoing edge (%s,%s)\n", buf, buf2);
		sprintf(acyclicGraphNodeBuffer + strlen(acyclicGraphNodeBuffer), "->%s ", buf2);
		curr = curr->next;
	}
	curr = node->in.head;
	while(curr!=NULL) {
		curr->value->fst->value->toString(buf2, symtable, 0);
		printf("incoming edge (%s,%s)\n", buf, buf2);
		sprintf(acyclicGraphNodeBuffer + strlen(acyclicGraphNodeBuffer), "<-%s ", buf2);
		curr = curr->next;
	}
}
TEST_BEGIN(AcyclicGraph)
	AcyclicGraph<int> node0(0), node1(1), node2(2), nodem1(-1), nodem2(-2);
	printf("construct acyclic graph\n");
	node2.addOutgoingEdgeTo(&node1);
	node1.addOutgoingEdgeTo(&node0);
	node0.addOutgoingEdgeTo(&nodem1);
	nodem1.addOutgoingEdgeTo(&nodem2);
	acyclicGraphNodeBuffer[0] = '\0';
	node2.traverse(printAcyclicGraphNode);
	assert(strcmp("2 ->1 1 ->0 <-2 0 ->-1 <-1 -1 ->-2 <-0 -2 <--1 ", acyclicGraphNodeBuffer) == 0);
	printf("remove a node\n");
	node1.remove();
	acyclicGraphNodeBuffer[0] = '\0';
	node2.traverse(printAcyclicGraphNode);
	node0.traverse(printAcyclicGraphNode);
	nodem1.remove();
	printf("construct acyclic graph 2\n");
	assert(strcmp("2 0 ->-1 -1 ->-2 <-0 -2 <--1 ", acyclicGraphNodeBuffer) == 0);
	node2.addOutgoingEdgeTo(&node0);
	node1.addOutgoingEdgeTo(&node0);
	node0.addOutgoingEdgeTo(&nodem1);
	node0.addOutgoingEdgeTo(&nodem2);
	acyclicGraphNodeBuffer[0] = '\0';
	node2.traverse(printAcyclicGraphNode);
	node0.remove();
	printf("construct acyclic graph 3\n");
	assert(strcmp("2 ->0 0 ->-1 ->-2 <-2 <-1 -1 <-0 -2 <-0 1 ->0 ", acyclicGraphNodeBuffer) == 0);
	node2.addOutgoingEdgeTo(&node0);
	node2.addOutgoingEdgeTo(&node1);
	node0.addOutgoingEdgeTo(&nodem1);
	node1.addOutgoingEdgeTo(&nodem2);
	acyclicGraphNodeBuffer[0] = '\0';
	node2.traverse(printAcyclicGraphNode);
	assert(strcmp("2 ->0 ->1 0 ->-1 <-2 -1 <-0 1 ->-2 <-2 -2 <-1 ", acyclicGraphNodeBuffer) == 0);
TEST_END(AcyclicGraph)

TEST_BEGIN(Model)
	MAKE_REGION;
	resetSymbols();
	resetTermMatrix();
	acyclicGraphNodeBuffer[0] = '\0';
	Clause *a, *b, *c;
	Clause *a1, *a2; // clauses that contain the eligible literal in a
	Clause *d1, *d2; // clauses that depend on the eligible literal in b
	Clause *d;
	Clause *e; // clause that generates the eligible literal q which is contained by b not as an eligible literal
	a = parseClause("(p, q, r)", CURR_REGION);
	b = parseClause("(n, ~p, q)", CURR_REGION);
	c = parseClause("(l, ~n, ~p)", CURR_REGION);
	d = parseClause("(n, p, ~s)", CURR_REGION);
	a1 = parseClause("(a, p, q)", CURR_REGION);
	a2 = parseClause("(b, p, r)", CURR_REGION);
	d1 = parseClause("(a, ~n, q)", CURR_REGION);
	d2 = parseClause("(b, ~n, r)", CURR_REGION);
	e = parseClause("(s)", CURR_REGION);
	insertClauseInstanceIntoModel(a);
	insertClauseInstanceIntoModel(b);
	insertClauseInstanceIntoModel(c);
	a->literals[0]->vertex->traverse(printClauseGraphNode);
	// printf("%s\n", acyclicGraphNodeBuffer);
	assert(strcmp(acyclicGraphNodeBuffer, "[p|q|r] -><null> <-[n|~p|q] <-[l|~n|~p] <null> <-[p|q|r] [n|~p|q] ->[p|q|r] <-[l|~n|~p] [l|~n|~p] ->[n|~p|q] ->[p|q|r] ") == 0);
	deleteClauseInstanceFromModel(a);
	printf("a deleted\n");
	b->literals[0]->vertex->traverse(printClauseGraphNode);
	deleteClauseInstanceFromModel(b);
	printf("b deleted\n");
	c->literals[0]->vertex->traverse(printClauseGraphNode);
	deleteClauseInstanceFromModel(c);

	printf("test model: eager delete (1)\n");
	insertClauseInstanceIntoModel(a);
	insertClauseInstanceIntoModel(b);
	insertClauseInstanceIntoModel(c);
	insertClauseInstanceIntoModel(a1);
	insertClauseInstanceIntoModel(a2);
	eagerCascadingDeleteClauseFromModel(a, a->literals[0]);
	acyclicGraphNodeBuffer[0] = '\0';
	a->literals[0]->vertex->traverse(printClauseGraphNode);
	// printf("%s\n", acyclicGraphNodeBuffer);
	assert(strcmp(acyclicGraphNodeBuffer, "[p|q|r] -><null> <-[n|~p|q] <-[l|~n|~p] <null> <-[p|q|r] [n|~p|q] ->[p|q|r] <-[l|~n|~p] [l|~n|~p] ->[n|~p|q] ->[p|q|r] ") == 0);

	printf("test model: eager delete (2)\n");
	deleteClauseInstanceFromModel(a);
	deleteClauseInstanceFromModel(b);
	deleteClauseInstanceFromModel(c);
	insertClauseInstanceIntoModel(a);
	insertClauseInstanceIntoModel(e);
	// The clause e must be inserted before the clause d,
	// as the insert procedure only links all negative literals in the clause being inserted
	// with the corresponding eligible literal, but does not do so retrospectively for eligible literals.
	// This is ok because when building a model, we also insert clauses in the desired order.
	insertClauseInstanceIntoModel(d);
	insertClauseInstanceIntoModel(c);
	insertClauseInstanceIntoModel(d1);
	insertClauseInstanceIntoModel(d2);
	acyclicGraphNodeBuffer[0] = '\0';
	a->literals[0]->vertex->traverse(printClauseGraphNode);
//	printf("%s\n", acyclicGraphNodeBuffer);
	// this should delete d, d1, and d2
	eagerCascadingDeleteClauseFromModel(e, e->literals[0]);
	acyclicGraphNodeBuffer[0] = '\0';
	a->literals[0]->vertex->traverse(printClauseGraphNode);
	printf("%s\n", acyclicGraphNodeBuffer);
	assert(strcmp(acyclicGraphNodeBuffer, "[p|q|r] -><null> <-[l|~n|~p] <null> <-[p|q|r] <-[s] [s] -><null> <-[n|p|~s] [n|p|~s] ->[s] <-[l|~n|~p] <-[a|~n|q] <-[b|~n|r] [l|~n|~p] ->[n|p|~s] ->[p|q|r] [a|~n|q] ->[n|p|~s] [b|~n|r] ->[n|p|~s] ") == 0);
	FREE_REGION;

TEST_END(Model)

TEST_BEGIN(Quicksort)
	MAKE_REGION;
	resetSymbols();
	resetTermMatrix();
	Term *stack[12] =
	{parseTerm("a", CURR_REGION),
	parseTerm("b", CURR_REGION),
	parseTerm("c", CURR_REGION),
	parseTerm("d", CURR_REGION),
	parseTerm("e", CURR_REGION),
	parseTerm("f", CURR_REGION),
	parseTerm("g", CURR_REGION),
	parseTerm("h", CURR_REGION),
	parseTerm("i", CURR_REGION),
	parseTerm("j", CURR_REGION),
	parseTerm("k", CURR_REGION),
	parseTerm("l", CURR_REGION)};
	const int n = 6;
	Term *t[n] = {stack[0], stack[3], stack[9], stack[2], stack[6], stack[8]};
	bool s[n] = {true, false, true, false, true, false};
	/*for(int i=0;i<n;i++) {
		t[i]->toString(buf, symtable);
		printf("%s%s ", s[i]?"":"~", buf);
	}
	printf("\n");*/
	quicksort(t, s, n);
	assert(t[5] == stack[0]);
	assert( t[4] == stack[2]);
	assert( t[3] == stack[3]);
	assert( t[2] == stack[6]);
	assert( t[1] == stack[8]);
	assert( t[0] == stack[9]);
	assert( s[5] == true);
	assert( s[4] == false);
	assert( s[3] == false);
	assert( s[2] == true);
	assert( s[1] == false);
	assert( s[0] == true);
	FREE_REGION;
TEST_END(Quicksort)

void testMainLoopAux(const char *str[]) {
	MAKE_REGION;
	UserStack *st = new UserStack();
	resetSymbols();
	resetTermMatrix();
	resetArithmetics();
	resetModel();
	resetTypes();
    Clause *c;
    ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
    int i = 0;
    char strbuf[STRING_BUF_SIZE];
    try {
        while(str[i]!=NULL) {
                c = parseClause(str[i], CURR_REGION);
                c->toString(strbuf, symtable);
				printf("input clause: %s\n", strbuf);
                clauseTree->addClause(c, CURR_REGION);
                i++;
        }
        clauseTree->newVars();
        clauseTree->compile(CURR_REGION);
        //clauseTree->root->generateTrieNodes(&modelTrieRoot, CURR_REGION);
        clauseTree->toString(strbuf, symtable);
        printf("clause tree:\n%s", strbuf);

    } catch(const char *p) {
        puts(str[i]);
        while(p--!=str[i])
            putchar(' ');
        puts("^");
    }
	runTest(100, NULL, clauseTree, NULL, NULL, st, CURR_REGION);
	FREE_REGION;
}
TEST_BEGIN(MainLoop)
	const char *str[] = {
		 "(~p, ~q)",
		 "(p)",
		 "(q)",
		 NULL
	};
	testMainLoopAux(str);
	assert(globalProverStatus == PROOF_FOUND);
TEST_END(MainLoop)

TEST_BEGIN(MainLoop2)
	const char *str[] = {
		"(~p, ~q)",
		"(p, q)",
		"(~p, q)",
		"(p, ~q)",
		NULL
	};
	testMainLoopAux(str);
	assert(globalProverStatus == PROOF_FOUND);
TEST_END(MainLoop2)

TEST_BEGIN(MainLoop3)
	const char *str[] = {
		"(~p(X), ~q(X))",
		"(p(a))",
		"(q(X))",
		NULL
	};
	testMainLoopAux(str);
	assert(globalProverStatus == PROOF_FOUND);
TEST_END(MainLoop3)

TEST_BEGIN(MainLoop4)
	const char *str[] = {
		"(~p(X), p(f(X)))",
		"(p(a))",
		"(~p(f(f(a))))",
		NULL
	};
	testMainLoopAux(str);
	assert(globalProverStatus == PROOF_FOUND);
TEST_END(MainLoop4)

TEST_BEGIN(MainLoop5)
	printf("ordered resolution that generates larger clauses\n");
	const char *str[] = {
		"(~p, q, r, s)",
		"(a, b, p, c)",
		"(~q)",
		"(~r)",
		"(~s)",
		"(~a)",
		"(~b)",
		"(~c)",
		NULL
	};
	testMainLoopAux(str);
	assert(globalProverStatus == PROOF_FOUND);
TEST_END(MainLoop5)

TEST_BEGIN(MainLoop6)
// this test case tests if nested inverse lookup calls, when found a matching term, are correctly routed to the outer call
// rather than trigger backtracking
// here when p(n) and l(n, h1) are in the model, l(P,h3) will trigger a nested lookup call for n, which will match with the n in l(n, h1)
	const char *str[] = {
		"(p(n))",
		"(l(n,h1))",
		"(~p(P), l(P,h3))",
		"(~l(n,h3))",
		NULL
	};
	testMainLoopAux(str);
	assert(globalProverStatus == PROOF_FOUND);
TEST_END(MainLoop6)

TEST_BEGIN(MainLoop7)
// this test case tests satuarting the mainloop by enabling the useBranchThreshold switch
// rather than trigger backtracking
// here when p(n) and l(n, h1) are in the model, l(P,h3) will trigger a nested lookup call for n, which will match with the n in l(n, h1)
	const char *str[] = {
		"(p(a, b))",
		"(~p(b, X))",
		"(~q(a, b, c, d))",
		"(q(b, X, Y, Z))",
		NULL
	};
	bool temp = globalUseBranchThreshold;
	globalUseBranchThreshold = true;
	testMainLoopAux(str);
	assert(globalProverStatus == SATURATED);
	globalUseBranchThreshold = temp;
TEST_END(MainLoop7)

TEST_BEGIN(MainLoop8)
	// this test case tests generating a very long instance
	const char *str[] = {
		"(p(west(X,Y),Z,east(m(s(W)),c(s(s(s(W))))),a))",
		NULL
	};
	bool temp = globalUseBranchThreshold;
	globalUseBranchThreshold = false;
	testMainLoopAux(str);
	assert(globalProverStatus == SATURATED);
	globalUseBranchThreshold = temp;
TEST_END(MainLoop8)

TEST_BEGIN(MainLoop9)
	// this test case generates super symbols
	bool guss = globalUseSuperSymbols;
	globalUseSuperSymbols = true;
	const char *str[] = {
		"(p(west(X,Y),Z,east(m(s(W)),c(s(s(s(W))))),a))",
		"(~p(west(X,Y),Z,east(m(s(W)),c(s(s(s(W))))),a))",
		NULL
	};
	bool temp = globalUseBranchThreshold;
	globalUseBranchThreshold = false;
	testMainLoopAux(str);
	assert(globalProverStatus == PROOF_FOUND);
	globalUseBranchThreshold = temp;
	globalUseSuperSymbols = guss;
TEST_END(MainLoop9)

void runProver(const char *filename, const char *options) {
	MAKE_REGION;
#ifdef USE_THREADS
	printf("USE_THREADS=1\n");
#else
	printf("USE_THREADS=0\n");
#endif
#ifdef USE_TYPES
	printf("USE_TYPES=1\n");
#else
	printf("USE_TYPES=0\n");
#endif
#ifdef USE_RELEVANCE
	printf("USE_RELEVANCE=1\n");
#else
	printf("USE_REVEVANCE=0\n");
#endif
#ifdef PROFILE_MODEL
	printf("PROFILE_MODEL=1\n");
#else
	printf("PROFILE_MODEL=0\n");
#endif
        // set option default
        strcpy(globalTPTPPath, ".");
	// parse options
	ParserContext *pcOptions = new ParserContext(CURR_REGION);
	Pointer eOptions(options);
	parseOptions(&eOptions, pcOptions);

	//check consistency
	if(globalSortSubtreesBySize && globalUpdateStartingIndex) {
		printf("error: incompatible switches\n");
		exit(0);
	}

	UserStack *st = new UserStack(globalCoroutineStackSize);
	/* resetSymbols();
	resetTermMatrix();
	resetArithmetics();
	resetModel(); */
	initSymbols();

	LinkedList<Clause *> clausesObj;
	parseInputFile(filename, &clausesObj, CURR_REGION);
//		if(pc.error==0) {
	char strbuf[STRING_BUF_SIZE];
	Clause **clausesArray = clausesObj.toArray();
	int clausesN = clausesObj.size();
	if(globalRandomClauseInsert) {
		randomizedArray(clausesArray, clausesN);
	}
	if(globalRandomClauseLiteral) {
		for(int i=0;i<clausesN;i++) {
			randomizedArray(clausesArray[i]->literals, clausesArray[i]->numberOfLiterals);
		}
	}

	Hashtable<bool, sizeof(Symbol)> flip(1024, CURR_REGION);

	if(globalFlipLiteral) {
		for(unsigned int i=0;i<numberOfFunctionSymbols;i++) {
			Symbol s = functionSymbols[i];
			bool f = true; // flip[HASHTABLE_KEY(s)]; // default is false
			flip.update(HASHTABLE_KEY(s), f);
		}
	}

	if(globalRandomFlipLiteral) {
		for(unsigned int i=0;i<numberOfFunctionSymbols;i++) {
			Symbol s = functionSymbols[i];
			bool f = flip[HASHTABLE_KEY(s)]; // default is false
			if(rand()%2 == 1) {
				f = !f;
			}
			flip.update(HASHTABLE_KEY(s), f);
		}
	}

	if(globalFlipLiteral || globalRandomFlipLiteral) {
		for(int i=0;i<clausesN;i++) {
			clausesArray[i]->flipLiterals(flip);
		}
	}

	ClauseTree *clauseTree = new (CURR_REGION) ClauseTree(CURR_REGION);
	for(int i=0;i<clausesN;i++) {
		clauseTree->addClause(clausesArray[i], CURR_REGION);
	}
	delete[] clausesArray;

	clauseTree->newVars();
	clauseTree->root->calculateBranchMinLiteralSize();
	if(globalSortSubtreesBySize) {
		clauseTree->root->sortSubtreesByLiteralSize();
	}

	// type inference
	clock_t start, end;
	start = clock();
	clauseTree->collectLiteralsAndVars();
	literalListToEquiv(&varTypeEquiv, &clauseTree->posLits, &clauseTree->negLits);
	strbuf[0] = '\0';
#ifdef USE_TYPES
	varTypeEquiv.toString(strbuf, symtable);
	printf("type equivalence:\n%s\n", strbuf);
	literalListToTypes(&clauseTree->posLits, &clauseTree->negLits, &clauseTree->vars);
	strbuf[0] = '\0';
	hashtableToString(&typeSymbolToType, strbuf, symtable);
	printf("types:\n%s\n", strbuf);
	printf("unfolding types\n");
	unfoldTypes();
	addConstToEmptyTypeAndReverseTermsInType();
	compileAllTypes(CURR_REGION);
	end = clock();
	printf("type inference %f seconds\n", ((double) end - start) / CLOCKS_PER_SEC);
		strbuf[0] = '\0';
		varTypeEquiv.toString(strbuf, symtable);
		printf("type equivalence:\n%s\n", strbuf);
	strbuf[0] = '\0';
	hashtableToString(&typeSymbolToType, strbuf, symtable);
	printf("simplified types:\n%s\n", strbuf);
	printf("number of types = %d\n", typeSymbolToType.len);
	printf("all types are LL1 = %d\n", checkAllTypesLL1());
#endif

	// compile depends on type inference
	clauseTree->compile(CURR_REGION);
	clauseTree->toString(strbuf, symtable);
	printf("clause tree:\n%s", strbuf);
	fflush(stdout);// due to a glitch in Eclipse/MinGW, you must fflush stdout and print a newline for the debugger to work
	printf("\n");
//	printf("generating static trie nodes\n");
	//clauseTree->root->generateTrieNodes(&modelTrieRoot, CURR_REGION);
	fflush(stdout);
	//getchar();
	//usleep(1000);
	runTest(100, NULL, clauseTree, NULL, NULL, st, CURR_REGION);

//		} else {
//			char buf[STRING_BUF_SIZE];
//			syncTokenQueue(&e, &pc);
//			generateErrMsg("syntax error", e.fpos, e.base, buf);
//			printf("%s", buf);
//		}
	printf("ar dealloc %d, alloc %d\n", ar_dealloc_counter, ar_alloc_counter);
	printf("state dealloc %d, alloc %d\n", state_dealloc_counter, state_alloc_counter);
	printf("dd dealloc %d, alloc %d\n", dd_dealloc_counter, dd_alloc_counter);
	printf("dealloc total %s, ", formatSize(dealloc_size_total));
	printf("alloc total %s, ", formatSize(alloc_size_total));
	printf("realloc total %s\n", formatSize(realloc_size_total));

	printf("===========\n");

	printf("LinkedStack<Term *> alloc %d\n", LinkedStack<Term *>::alloc_counter);
	printf("LinkedStack<TermCacheSeg *> alloc %d\n", LinkedStack<TermCacheSeg *>::alloc_counter);

	printMemoryUsage();

	fflush(stdout);
	region_free(subRegion);
	region_free(clauseRegion);
	region_free(clauseComponentRegion);
	region_free(termMatrixRegion);
//	region_free(termMatrixHashtableRegion);
	region_free(termCacheRegion);
	region_free(modelTrieRegion);
	region_free(stackRegion);
	region_free(coroutineStateRegion);
//	region_free(globalTrieHashtableRegion);
	delete pcOptions;
	FREE_REGION;
}

