/*
 * File:   stack.h
 *      Author: Hao Xu
 *
 */

/* Mini Manual
 * 
 * 1. How to use C local variables
 * C local variables = Stack global variables. 
 * They should only be used to pass values between two Stack function calls in a Stack function.
 * They should not be used to pass values across function calls in a Stack function.
 * 
 * 2. How to use Stack local variables
 * Stack local variables defined using PARAMS and DEFS should generally only be assigned once and 
 * should only be read after it is being assigned, as this may alter a previously saved state.
 * For example, suppose a Stack function looks like
 * VAR(c) = 1;
 * FUNC_CALL 1 depends on VAR(c);
 * code depending on VAR(c);
 * VAR(c) = 2;
 * If after FUNC_CALL 1, the stack looks like 
 * ... | ar1 | ar2 | saved state | 
 *     ^ar bottom
 * then it means that we may come back to the state where VAR(c) should be 1,
 * which will be altered if the second assignment is executed.
 * 
 * No write when there is a save_state at runtime, unless no read happens after restore_state before a new write happens
 *
 * 3. How to use SAVE_STATE and RESTORE_STATE
 * Suppose a Stack Function looks like
 * FUNC_CALL 1 return value x;
 * if(x != NULL) {
 *     RETURN(x);
 * }
 * FUNC_CALL 2;
 * where both FUNC_CALLs may have saved states 
 * and FUNC_CALL 1 produces a sequence of results which ends with NULL.
 * In this case the FUNC_CALL 2 should be executed only when x == NULL, 
 * i.e. FUNC_CALL 1 no longer produces and non NULL results.
 * It may be tempting to insert SAVE_STATE after FUNC_CALL 1,
 * FUNC_CALL 1 return value x;
 * if(x != NULL) {
 *     SAVE_STATE(rp);
 *     RETURN(x);
 * }
 * rp:
 * FUNC_CALL 2;
 * then this could be problematic because the SAVE_STATE will be restored before those saved in FUNC_CALL 1.
 * The correct solutions is
 * SAVE_STATE(rp);
 * FUNC_CALL 1 return value x;
 * if(x != NULL) {
 *     RETURN(x);
 * }
 * RESTORE_STATE;
 * rp:
 * FUNC_CALL 2;
 * The RESTORE_STATE is necessary to remove the saved state from the stack.
 * 
 * 4. How to print the current stack frame at the beginning of every function
 * #define STACK_DEBUG true
 *
 * 5. Level of nesting
 * We support multiple levels of nested functions
 * The semantics are as if each level has its own stack, and all stack tops are visible.
 * The levels of nesting supported is given by LEVEL_OF_NESTING
 * To access var x at level n, VAR_N(x, n).
 * To call a function f at level n, FUNC_CALL_N(n, f, (params), retvals)
 * For tail calls, it is necessary to pass in the level of nesting of the caller, FUNC_CALL_TAIL, N(n, f, (params), caller_nesting)
 * All operators defaults to level 1. For example, VAR(x) = VAR_N(x, 1).
 */
#ifndef STACK_H
#define	STACK_H

#include <string.h>
#include "defs.h"
#include "macros.h"
#include "linkedStack.h"

// print by type declarations
struct CoroutineState;
typedef CoroutineState CoroutineDataDependency;

void printByType(CoroutineState *);
void printByType(CoroutineDataDependency *);


#define DEBUG_STACK_SIZE_CODE \
	if (DEBUG_STACK_SIZE) { \
		if(DEBUG_STACK_SIZE_SHOW) printf("stack size = %fM\n", (STACK_TOP - STACK_BOTTOM)/(1024 * 1024.0)); \
		assert( STACK_TOP - STACK_BOTTOM <= globalCoroutineStackSize); \
	}

#define MAX_STACK_SIZE (16 * 1024 * 1024)
#define MAX_STATE_STACK_SIZE (1024 * 1024)

#define RETVAL_BUF_SIZE 1024

// adjustable parameters
#define LEVEL_OF_NESTING 2

/*#define BRANCH_THRESHOLD_TYPE int

#define BRANCH_THRESHOLD_DEFAULT 0

#define BRANCH_THRESHOLD universalTermSizeLimit

#define BRANCH_THRESHOLD_CMP(v, a) \
	( \
			(void) (DEBUG_BRANCH_THRESHOLD? \
				((a)<=*(v)? \
						printf("threshold passed expected = %d, current = %d\n", (int) a, (int) BRANCH_THRESHOLD) : \
						printf("threshold no passed expected = %d, current = %d\n", (int)a, (int) BRANCH_THRESHOLD) \
				) : \
				0), \
			((v)==NULL || (a)<=*(v)) \
	)*/

// meta data
#define LEVEL LEVELOF(FUN)
#define LEVELOF(fn) AEP_LEVELOF(fn)
#define AEP_LEVELOF(fn) DISP_LEVELOF(fn)
#define DISP_LEVELOF(l) LEVEL_##l

//#define LEVEL_test 1
//LEVELOF(test)
// types
#define BYTE unsigned char
#define DEL_FLAG_TYPE unsigned int

#define STACK_BOTTOM __stackBottom // void* __stack_bottom

#define AR_BOTTOM_N(nesting) AEP_AR_BOTTOM_N(nesting)
#define AEP_AR_BOTTOM_N(nesting) __arBottom##__##nesting
#define AR_BOTTOM AR_BOTTOM_N(1)
#define AR_BOTTOM2 AR_BOTTOM_N(2)
//AR_BOTTOM_N(LEVELOF(test))


#define RET_BOTTOM retVals
#define STACK_TOP __stackTop
#define STACK_STACK __stackStack
#define STATE_STACK_TOP __stateStackTop
#define STATE_STACK_STACK __stateStackStack
#define BRANCH_POINTER __branchPointer // points to the stack top of the previous saved state in the current function frame set
#define GLOBAL_BRANCH_POINTER __globalBranchPointer // points to the stack top of the previous saved state
#define DATA_DEPENDENCY __dataDependency
#define DATA_DEPENDENCY_ROOT __dataDependencyRoot

#define STACK_STACK_DEF StackStack** STACK_STACK

// naming conventions:
// suffix _N means it takes in a parameter indicating the nesting level
// suffix _F means it takes in a parameter indicating the function
// suffix 1, 2, ... mean it calls the _N version and pass 1, 2, ... in as the nesting level
#define TYPE_IMPL_F(fn, varname) \
	PASTE(__type_, PASTE(fn, PASTE(_, varname)))

#define OFFSET_IMPL_F(fn, varname) \
	PASTE(__, PASTE(fn, PASTE(_, varname)))

#define FUN_FUNC_LOCAL_OFFSET(fn) \
	PASTE(__local_offset_,fn)

#define FUNC_LOCAL_OFFSET \
	FUN_FUNC_LOCAL_OFFSET(FUN)

#define FUN_FUNC_AR_TOP_OFFSET(fn) \
	PASTE(__ar_top_offset_,fn)

#define FUNC_AR_TOP_OFFSET \
	FUN_FUNC_AR_TOP_OFFSET(FUN)

#define FUNC_LOCAL_DEFS_SIZE(fn) \
	PASTE(__local_defs_size_, fn)

#define RET_POINT(rp) \
	PASTE(ret_point_, PASTE(FUN, PASTE(_,rp)))

#define ENT_POINT(rp) \
	PASTE(ent_point_, rp)

#define TCALL_ENT_POINT(rp) \
	PASTE(tcall_ent_point_, rp)

#define EXT_POINT \
	ext_point

#define SEC_RET_POINT \
	sec_ret_point

#define TYPE_F(fn, vn) \
    TYPE_IMPL_F(fn, vn)

#define OFFSET(type, vbuf, size) (*(type *)((vbuf) + (size)))
#define SIZEOF(type) ALIGN(sizeof(type))

#define VAR_N(fn, varname,nesting) \
    (*(TYPE_F(fn, varname) *)(reinterpret_cast<BYTE *>(AR_BOTTOM_N(nesting)) + OFFSET_IMPL_F(fn, varname)))

#define VAR(varname) \
	VAR_N(FUN, varname, LEVEL) \

#define VAR_F1(fn, varname) \
	VAR_N(fn, varname, 1) \

#define POP(type, val) \
    STACK_DEALLOC_IMPLICIT(type); \
    (val) = *(type *)STACK_TOP

#define PUSH(type, val) \
    *(type *)STACK_TOP = (val); \
    STACK_ALLOC_IMPLICIT(type); \

    
/*** PUSH_LIST ***/
#define PUSH_LIST_NIL

#define PUSH_LIST_CONS(h, t) \
    PUSH(typeof(h),h); \
    t

#define PUSH_LIST(list) \
    FOLDR(PUSH_LIST_NIL, PUSH_LIST_CONS, list)

/*** COMP_SIZE ***/
#define COMP_SIZE_F(h) \
    SIZEOF(h)

#define COMP_SIZE(tlist) \
    MAP(COMP_SIZE_F, tlist)

#define SUM_SIZE(tlist) \
	SUM(COMP_SIZE(tlist)) \

/*** COMP_TYPE_SIZE ***/
#define COMP_TYPE_SIZE(list) \
	COMP_SIZE(EXP_TYPE(list))

#define EXP_TYPE_F(h) \
    typeof(h)

#define EXP_TYPE(list) \
    MAP(EXP_TYPE_F, list)

#define SUM_TYPE_SIZE(list) \
	SUM(COMP_TYPE_SIZE(list)) \

/*** GET_RET_VAL ***/
#define __GET_RET_VAL_F(lval, retvaloffset, retBase) \
    lval = *(typeof(lval) *)(retBase + retvaloffset);

#define GET_RET_VAL_F(rv) \
    __GET_RET_VAL_F(SND(FST(rv)), FST(FST(rv)), SND(rv));

#define GET_RET_VAL_EOL(retBase, retList) \
    CONCAT(MAP(GET_RET_VAL_F, \
        ZIP(ZIP(SHIFT(ACCU(COMP_TYPE_SIZE(retList)),0),retList),\
            REPEAT(retBase, COUNT(retList)))))

#define GET_RET_VAL(retBase, list) \
    GET_RET_VAL_EOL(retBase, APPEND_EOL(list))

/*** SET_RET_VAL ***/
#define SET_RET_VAL_F(x) \
    *(typeof(SND(x)) *)(RET_BOTTOM+FST(x)) = SND(x);

#define SET_RET_VAL_EOL(list) \
    MAP(SET_RET_VAL_F, ZIP(SHIFT(ACCU(COMP_TYPE_SIZE(list)), 0),list))

#define SET_RET_VAL(list) \
    CONCAT(SET_RET_VAL_EOL(APPEND_EOL(list)))

/*** DEF_VAR_ADDR ***/
#define __DEF_VAR_ADDR_F_N(type, addr, fn, vn) \
    static const int OFFSET_IMPL_F(fn, vn) = addr; \
    /*const*/ typedef type TYPE_IMPL_F(fn, vn);

#define DEF_VAR_ADDR_F_N(x, fn) \
    APP_EVAL_PARAMS(__DEF_VAR_ADDR_F_N, FST(SND(x)), FST(x), fn, SND(SND(x)))

/*** DEF_PARAM_ADDR ***/
#define CONVERT_PARAM_OFFSET(x) \
    (FUNC_VAR_OFFSET + (x))

#define DEF_PARAM_ADDR_N(fn, list) \
    MAP_ARG(DEF_VAR_ADDR_F_N, \
        fn, \
        ZIP(MAP( \
                CONVERT_PARAM_OFFSET, \
                SHIFT(ACCU(COMP_SIZE(UNZIPF(REVERSE(list)))),0)), \
            REVERSE(list)))

/*** PARAMS ***/
#define PARAMS_N(nesting, ...) \
    /*MARK(FUN, params);*/ \
    ENT_POINT(FUN): \
    CONCAT(DEF_PARAM_ADDR_N(FUN, PACK(APPEND_EOL((__VA_ARGS__))))) \
    static const int FUNC_LOCAL_OFFSET = SUM(COMP_SIZE(UNZIPF(PACK(APPEND_EOL((__VA_ARGS__)))))) + FUNC_VAR_OFFSET; \
    (void) FUNC_LOCAL_OFFSET; \
    if(STACK_DEBUG) { PRINT_STACK_N(nesting, ##__VA_ARGS__); } \

#define PARAMS(...) \
	PARAMS_N(LEVEL, ##__VA_ARGS__) \

/*** PROTO ***/
#define PROTO(fn, list_no_eol, ...) \
	PROTO_EOL(fn, APPEND_EOL(list_no_eol), LIST(__VA_ARGS__)) \

#define PROTO_EOL(fn, paramList, retList) \
	FOLDR(, DEF_PARAM_TYPE_CONS, ZIP(REPEAT(fn, COUNT(paramList)), ZIP_RANGE1(paramList))) \
	typedef Const<int, COUNT(paramList)> PARAM_TYPE_ARITY(fn); \
	FOLDR(, DEF_RETURN_TYPE_CONS, ZIP(REPEAT(fn, COUNT(retList)), ZIP_RANGE1(retList))) \
	typedef Const<int, COUNT(retList)> RETURN_TYPE_ARITY(fn); \
	typedef EXPAND(IS_ZERO(IS_EMPTY(retList),\
		IS_ZERO(IS_EMPTY(TAIL(retList)),\
				(PASTE(Tuple, COUNT(retList)) < \
					FOLDR( \
						HEAD(ROTATE_LEFT(retList, 1)), \
						RETURN_TYPE_TUPLE_CONS, \
						TAIL(ROTATE_LEFT(retList, 1)) \
					) > \
				), \
				(Tuple1<RETURN_TYPE(fn, 1)>)\
		),\
		(Tuple0)\
	)) \
		RETURN_TYPE_TUPLE(fn); \


#define DEF_PARAM_TYPE_CONS(x, t) \
	typedef SND(SND(x)) PARAM_TYPE(FST(x), FST(SND(x))); \
	t

#define PARAM_TYPE(f, n) \
	PASTE(__param_type_, PASTE(f, PASTE(_, n)))

#define PARAM_TYPE_ARITY(f) \
	PASTE(__param_type_arity_, f)

#define DEF_RETURN_TYPE_CONS(x, t) \
	typedef SND(SND(x)) RETURN_TYPE(FST(x), FST(SND(x))); \
	t

#define RETURN_TYPE(f, n) \
	PASTE(__return_type_, PASTE(f, PASTE(_, n)))

#define RETURN_TYPE_TUPLE(f) \
	PASTE(__return_type_tuple_, f)

#define RETURN_TYPE_ARITY(f) \
	PASTE(__return_type_arity_, f)

#define RETURN_TYPE_TUPLE_CONS(x, t) \
	x, t



//PROTO(fff, ())
//PROTO(fff, (int))
//PROTO(fff, (int, int))
//PROTO(fff, (int), int)
//PROTO(fff, (int), int, int)
//PROTO(fff, (int, int), int, int)



/*** RETVALS ***/
#define RETURNS_N(/*nesting, */...) \
    /*MARK(FUN, params);*/ \
    /*static const int FUNC_RETVAL_OFFSET = 0;*/ \

#define RETURNS(...) \
	RETURNS_N(__VA_ARGS__) \

/*** VARS ***/
#define CONVERT_VAR_OFFSET(x) \
    (FUNC_LOCAL_OFFSET + (x)) \

#define DEFS_EOL_N(list, nesting) \
    static const int FUNC_LOCAL_DEFS_SIZE(FUN) = SUM(COMP_SIZE(UNZIPF(list))); \
    static const int FUNC_AR_TOP_OFFSET = FUNC_LOCAL_DEFS_SIZE(FUN) + FUNC_LOCAL_OFFSET; \
    (void) FUNC_AR_TOP_OFFSET; \
	(void) FUNC_LOCAL_DEFS_SIZE(FUN); \
	STACK_PREREALLOC_IMPLICIT(AR_BOTTOM_N(nesting), ActivationRecordCommon, FUNC_LOCAL_OFFSET, FUNC_AR_TOP_OFFSET); \
	STACK_REALLOC_IMPLICIT(FUNC_LOCAL_DEFS_SIZE(FUN)); \
    DEBUG_STACK_SIZE_CODE \
    CONCAT(\
        MAP_ARG(DEF_VAR_ADDR_F_N, FUN, ZIP( \
            MAP(CONVERT_VAR_OFFSET, SHIFT(ACCU(COMP_SIZE(UNZIPF(list))),0)), list))) \

#define DEFS_N(nesting, ...) \
    /*CHECK_MARK(FUN, begin, defs);*/ \
    DEFS_EOL_N(PACK(APPEND_EOL((__VA_ARGS__))), nesting) \

#define DEFS(...) \
    DEFS_N(LEVEL, ##__VA_ARGS__) \

#define BEGIN_N(nesting) \
    /*CHECK_MARK(FUN, params, begin);*/
    /*MARK(FUN, begin);*/
    /*TCALL_ENT_POINT(FUN):*/

#define BEGIN \
	BEGIN_N(LEVEL)

#define END_N(nesting) \
    FUNC_EXIT(nesting);
    /*CHECK_MARK(FUN, begin, end)*/

#define END \
	END_N(LEVEL) \

/*** FUNC_CALL and FUNC_RET ***/
// Call function "fn", with parameters "list", with EOL, and return parameters "retList", without EOL, and return address "rp"
// "bp" is NULL and unused unless the function is called from a FUNC_CALL_RESUME macro. In that macro, BRANCH_POINTER is replaced by
// a new branch pointer generated by the FUNC_CALL_INIT macro, and the old BRANCH_POINTER values is stored in "bp".
// Stack layout:
// | current ar | saved ars |
// ^ar bottom               ^stack top
//             ||
//             \/
// | current ar | saved ars | rp(void *) prevArBottoms(BYTE *[]) bp param1 ... param n |
//                          ^ar bottom                                                       ^stack top
// jump to fn
// return point:
// | current av | saved ars | rp(void *) prevArBottoms(BYTE *[]) bp param1 ... param n locals |
//                          ^ar bottom                                                              ^stack top
//             ||
//             \/
// | current ar | saved ars |
// ^ar bottom               ^stack top
//
// We can't support varargs here because all new data have to be in the new ar in order to avoid overwriting saved ars.
//
#define FUNC_FLAGS_OFFSET \
    SIZEOF(void*) + SIZEOF(BYTE *) \

#define FLAG_TAIL_CALL ((DEL_FLAG_TYPE)1)

/***************************************************
 *              utilities                          *
 ***************************************************/
#define MAX(x,y) \
	(x)>=(y)?(x):(y)

#define MIN(x,y) \
	(x)<=(y)?(x):(y)

/***************************************************
 *              ACTIVATION_RECORD                  *
 ***************************************************/
// return_point (rp) ar_bottom_1 ... ar_bottom_n branch_pointer (bp) params defs
#define AR_BOTTOMS_SIZE \
	(SIZEOF(BYTE *) * LEVEL_OF_NESTING)

#define FUNC_VAR_OFFSET \
     (SIZEOF(ActivationRecordCommon))

#define CURR_AR_BOTTOMS_F(nesting) \
	AR_BOTTOM_N(nesting) \

#define CURR_AR_BOTTOMS \
	MAP(CURR_AR_BOTTOMS_F, RANGE1(LEVEL_OF_NESTING))

#define PREV_AR_BOTTOMS_F(nesting, curr_nesting) \
	(AR_BOTTOM_N(curr_nesting)->arBottoms[nesting-1]) \

#define PREV_AR_BOTTOMS(curr_nesting) \
	MAP_ARG(PREV_AR_BOTTOMS_F, curr_nesting, RANGE1(LEVEL_OF_NESTING))

#define NULL_AR_BOTTOMS \
	REPEAT(NULL, LEVEL_OF_NESTING)

#define FILL_AR_BOTTOMS_INTO_AR(currArBottom) \
	SAVE_AR_BOTTOMS(currArBottom, CURR_AR_BOTTOMS)

#define SAVE_AR_BOTTOMS_F(arb_curr_arb, record) \
    record->arBottoms[SND(arb_curr_arb)-1] = FST(arb_curr_arb); \

#define SAVE_AR_BOTTOMS(record, arbs) \
    CONCAT(MAP_ARG(SAVE_AR_BOTTOMS_F, record, ZIP(arbs, RANGE1(LEVEL_OF_NESTING)))) \

#define RESTORE_AR_BOTTOMS_F(arb_curr_arb, record) \
    FST(arb_curr_arb) = record->arBottoms[SND(arb_curr_arb)-1]; \

#define RESTORE_AR_BOTTOMS(record, arbs) \
    CONCAT(MAP_ARG(RESTORE_AR_BOTTOMS_F, record, ZIP(arbs, RANGE1(LEVEL_OF_NESTING)))) \

/* #define PUSH_AR_BOTTOMS_F(arb) \
    PUSH(BYTE *, arb); \

#define PUSH_AR_BOTTOMS(arbs) \
    CONCAT(MAP(PUSH_AR_BOTTOMS_F, arbs)) \

#define POP_AR_BOTTOMS_F(arb) \
    POP(BYTE *, arb); \

#define POP_AR_BOTTOMS(arbs) \
    CONCAT(MAP(POP_AR_BOTTOMS_F, arbs)) \ */

#define MAKE_AR(rpladdr, arbs, bp, list, nesting, caller_nesting, newArb) \
	STACK_PREALLOC_IMPLICIT(CONS(ActivationRecordCommon, EXP_TYPE(list))); \
	if(DEBUG_TRACE_DEALLOC) ar_alloc_counter++; \
	if(DEBUG_DEALLOC) printf("%d, ar %p alloced\n", ar_alloc_counter, STACK_TOP); \
	newArb = reinterpret_cast<ActivationRecordCommon *>(STACK_TOP); \
    reinterpret_cast<ActivationRecordCommon *>(STACK_TOP)->returnProgramPointer = rpladdr; \
    SAVE_AR_BOTTOMS(reinterpret_cast<ActivationRecordCommon *>(STACK_TOP), arbs); \
    reinterpret_cast<ActivationRecordCommon *>(STACK_TOP)->returnBranchPointer = bp; \
    reinterpret_cast<ActivationRecordCommon *>(STACK_TOP)->nestingLevel = nesting; \
    reinterpret_cast<ActivationRecordCommon *>(STACK_TOP)->callerNestingLevel = caller_nesting; \
    STACK_ALLOC_IMPLICIT(ActivationRecordCommon); \
    DEBUG_STACK_SIZE_CODE \
    PUSH_LIST(list); \

/*** REF_COUNT macros ***/
#ifdef HEAP_STACK
#define INIT_REF_COUNT(arb) \
	(arb)->refCount = 0; \

#define INC_REF_COUNT(arbs) \
	CONCAT(MAP(INC_REF_COUNT_F, arbs)) \

#define INC_REF_COUNT_F(arb) \
	(arb)->refCount ++; \

#define DEC_REF_COUNT(arbs) \
	CONCAT(MAP(DEC_REF_COUNT_F, arbs)) \

#define DEC_REF_COUNT_F(arb) \
	(arb)->refCount --;

#define REF_COUNT_IS_ZERO(arb) \
	((arb)->refCount == 0)

#else
#define INIT_REF_COUNT(arb) \

#define INC_REF_COUNT_F(arb) \

#define INC_REF_COUNT(arb) \

#define DEC_REF_COUNT_F(arb) \

#define DEC_REF_COUNT(arb) \

#define REF_COUNT_IS_ZERO(arb) false

#endif

#define MAKE_AR_AND_SET_AR_BOTTOM_N(rpladdr, arbs, bp, list, nesting, caller_nesting) \
	{ \
    	ActivationRecordCommon *temp; \
		MAKE_AR(rpladdr, arbs, bp, list, nesting, caller_nesting, temp); \
		if(DEBUG_AR_BOTTOM_SET) { printf("setting ar_bottom_"#nesting" to %p\n", temp); } \
		AR_BOTTOM_N(nesting) = temp; \
    }


/***************************************************
 *              FUNC_CALL_EOL_N                    *
 ***************************************************/
#define CHECK_FUNC_CALL_PARAM_TYPE(fn, paramList) \
		{ \
			typedef ConstEq<PARAM_TYPE_ARITY(fn), Const<int, COUNT(paramList)> > __checkType_paramList; \
			CHECK(func_call_param_type_arity_mismatch, __checkType_paramList); \
			FOLDR(,CHECK_PARAM_TYPE_CONS, ZIP(REPEAT(fn, COUNT(paramList)), ZIP_RANGE1(paramList))) \
		} \

#define CHECK_FUNC_CALL_RETURN_TYPE(fn, retList) \
		{ \
			typedef ConstEq<RETURN_TYPE_ARITY(fn), Const<int, COUNT(retList)> > __checkType_retList; \
			CHECK(func_call_return_type_arity_mismatch, __checkType_retList); \
			FOLDR(,CHECK_FUNC_CALL_RETURN_TYPE_CONS, ZIP(REPEAT(fn, COUNT(retList)), ZIP_RANGE1(retList))) \
		} \

#define CHECK_FUNC_TAIL_CALL_RETURN_TYPE(fn) \
		{ \
			typedef ConstEq<RETURN_TYPE_TUPLE(fn), RETURN_TYPE_TUPLE(FUN) > __checkType_retList; \
			CHECK(__tail_call_return_type_mismatch, __checkType_retList); \
		} \

#define CHECK_PARAM_TYPE_CONS(x, t) \
	{ \
    	typedef ConstEq<PARAM_TYPE(FST(x), FST(SND(x))), typeof(SND(SND(x))) > __checkType; \
    	CHECK(PASTE(__param_type_mismatch_, FST(SND(x))), __checkType); \
	} \
	t \

#define CHECK_FUNC_CALL_RETURN_TYPE_CONS(x, t) \
	{ \
    	typedef ConstEq<RETURN_TYPE(FST(x), FST(SND(x))), typeof(SND(SND(x))) > __checkType; \
    	CHECK(PASTE(return_type_mismatch_, FST(SND(x))), __checkType); \
	} \
	t \

#define RESET_CALLER_AR_BOTTOMS_F(nestingLevel, callee_nesting) \
	AR_BOTTOM_N(nestingLevel) = AR_BOTTOM_N(callee_nesting)->arBottoms[nestingLevel-1]; \

/* reset ar_bottom so that local variables of the caller are visible
 * We need to rotate the range so that the ar_bottom at level callee_nesting gets overwritten last and remains valid throughout the update,
 * after this macro the ar_bottoms will be restored to those of the caller */
#define RESET_CALLER_AR_BOTTOMS(callee_nesting) \
	CONCAT(MAP_ARG(RESET_CALLER_AR_BOTTOMS_F, callee_nesting, ROTATE_LEFT(RANGE1(LEVEL_OF_NESTING), callee_nesting)));




#define FUNC_CALL_EOL_N(rpl, fn, list, retList, nesting, caller_nesting) \
	CHECK_FUNC_CALL_PARAM_TYPE(fn, list); \
	CHECK_FUNC_CALL_RETURN_TYPE(fn, retList); \
    MAKE_AR_AND_SET_AR_BOTTOM_N(&& rpl, CURR_AR_BOTTOMS, NULL, list, nesting, caller_nesting); \
    goto ENT_POINT(fn); \
    rpl: \
    /* if GLOBAL_BRANCH_POINTER > current ar_bottom, then we cannot move stack_top the current ar_bottom because that would allow overwriting saved states on the stack.
     * instead we move stack_top to global_branch_pointer */ \
    STACK_ADJUST(MAX(reinterpret_cast<BYTE *>(GLOBAL_BRANCH_POINTER) + SIZEOF(CoroutineState), reinterpret_cast<BYTE *>(AR_BOTTOM_N(nesting)))); \
    { \
    	 ActivationRecordCommon *temp = AR_BOTTOM_N(nesting); (void) temp; \
    	 RESET_CALLER_AR_BOTTOMS(nesting); \
    } \
    GET_RET_VAL_EOL(RET_BOTTOM, retList); \

/***************************************************
 *              FUNC_TAIL_CALL_EOL_N               *
 ***************************************************/
#define FUNC_TAIL_CALL_EOL_N(fn, list, nesting, caller_nesting) \
	CHECK_FUNC_CALL_PARAM_TYPE(fn, list); \
	CHECK_FUNC_TAIL_CALL_RETURN_TYPE(fn); \
	/* push previous rp, ar_bottom, and bp */ \
    MAKE_AR_AND_SET_AR_BOTTOM_N( \
    		AR_BOTTOM_N(caller_nesting)->returnProgramPointer, /* rp */ \
    		PREV_AR_BOTTOMS(caller_nesting), \
    		AR_BOTTOM_N(caller_nesting)->returnBranchPointer, /* bp */ \
    		list, nesting, caller_nesting); \
    goto ENT_POINT(fn); \

/***************************************************
 *              FUNC_CALL_INIT_EOL                 *
 ***************************************************/
#define FUNC_CALL_INIT_EOL(rpl, startFrameArBottom, fsbp, fn, list) \
    /* create a new ar and a new state */ \
    startFrameArBottom = reinterpret_cast<ActivationRecordCommon *>(STACK_TOP); \
    ActivationRecordCommon *newArb; (void) newArb; \
    MAKE_AR((void *) NULL, NULL_AR_BOTTOMS, (CoroutineState *) NULL, list, 1, 1, newArb); \
    STACK_TOP+=FUNC_LOCAL_DEFS_SIZE(fn); \
    DEBUG_STACK_SIZE_CODE \
    /* create a branch pointer record, we need to store global branch pointer before updating it */ \
	reinterpret_cast<CoroutineState *>(STACK_TOP)->globalBranchPointer = GLOBAL_BRANCH_POINTER; \
    GLOBAL_BRANCH_POINTER = reinterpret_cast<CoroutineState *>(STACK_TOP); \
    STACK_ALLOC_IMPLICIT(CoroutineState); \
    DEBUG_STACK_SIZE_CODE \
    reinterpret_cast<CoroutineState *>(STACK_TOP)->programPointer = && ENT_POINT(fn); \
	/* the nested level 1 ar_bottom is startFrameArBottom */ \
	reinterpret_cast<CoroutineState *>(STACK_TOP)->arBottoms[0] = startFrameArBottom; \
    /* the nested level 2 ar_bottom is null */ \
	reinterpret_cast<CoroutineState *>(STACK_TOP)->arBottoms[1] = NULL; \
	reinterpret_cast<CoroutineState *>(STACK_TOP)->branchPointer = NULL; \
    fsbp = reinterpret_cast<CoroutineState *>(STACK_TOP); \

/***************************************************
 *              FUNC_CALL_RESUME_EOL_RET_HANDLE_N  *
 ***************************************************/
#define FUNC_CALL_RESUME_EOL_RET_HANDLE_N(startFrameArBottom, fsbp, retList, nesting) \
    /* update startFrameArBottom and fsbp */ \
	{ \
	ActivationRecordCommon *calleeArBottom = AR_BOTTOM_N(nesting); /* temporarily store callee AR_BOTTOM in STACK_TOP */ \
    RESET_CALLER_AR_BOTTOMS(nesting); /* reset ar_bottom so that local variables (startFrameArBottom, fsbp) of the caller are visible */ \
	startFrameArBottom = calleeArBottom; \
	fsbp = BRANCH_POINTER; \
	BRANCH_POINTER = calleeArBottom->returnBranchPointer; \
	RESTORE_BRANCH_POINTER_STATE(BRANCH_POINTER); \
    /* move stack_top to global_branch_pointer if necessary */ \
    STACK_ADJUST(MAX(reinterpret_cast<BYTE *>(GLOBAL_BRANCH_POINTER) + SIZEOF(CoroutineState), STACK_TOP)); \
    GET_RET_VAL_EOL(RET_BOTTOM, retList); \
    } \

/***************************************************
 *              FUNC_CALL_RESUME_EOL_N             *
 ***************************************************/
#define FUNC_CALL_RESUME_EOL_N(rpl, startFrameArBottom, fsbp, startFrameArBottom2, fsbp2, retList) \
    /* save branch pointer */ \
    SAVE_BRANCH_POINTER_STATE(BRANCH_POINTER, 1); \
    startFrameArBottom->returnProgramPointer =  && rpl; \
    FILL_AR_BOTTOMS_INTO_AR(startFrameArBottom); \
    startFrameArBottom->returnBranchPointer = BRANCH_POINTER; \
    BRANCH_POINTER = fsbp; /* set branch pointer to fsbp */ \
    AR_BOTTOM_N(1) = startFrameArBottom; \
    RESTORE_STATE; \
    rpl: \
    FUNC_CALL_RESUME_EOL_RET_HANDLE_N(startFrameArBottom2, fsbp2, retList, 1); \

/***************************************************
 *              FUNC_CALL_N                        *
 ***************************************************/
// Call function "fn", with parameters "list", without EOL, and return parameters "..."
#define FUNC_CALL_N(nesting, caller_nesting, fn, list, ...) \
	if(STACK_DEBUG) { PRINT_FUNC(CALL, nontail, __LINE__, fn); } \
	FUNC_CALL_EOL_N(RET_POINT(__LINE__), fn, APPEND_EOL(list), APPEND_EOL((__VA_ARGS__)), nesting, caller_nesting) \

#define FUNC_CALL(fn, list, ...) \
	FUNC_CALL_N(LEVELOF(fn), LEVELOF(FUN), fn, list, ##__VA_ARGS__) \

/***************************************************
 *              FUNC_CALL_TAIL_N                   *
 ***************************************************/
// Call function "fn" with tail call optimization, with parameters "list", without EOL, and return parameters "..."
#define FUNC_TAIL_CALL_N(nesting, fn, list, caller_nesting) \
	if(STACK_DEBUG) { PRINT_FUNC(CALL, tail, __LINE__, fn); } \
	FUNC_TAIL_CALL_EOL_N(fn, APPEND_EOL(list), nesting, caller_nesting)

#define FUNC_TAIL_CALL(fn, list) \
	FUNC_TAIL_CALL_N(LEVELOF(fn), fn, list, LEVEL) \

#define FUNC_TAIL_CALL2(fn, list) \
	FUNC_TAIL_CALL_N(2, fn, list, LEVEL) \

/***************************************************
 *              FUNC_CALL_INIT                     *
 ***************************************************/
// Call function "fn", with parameters "list", without EOL, and return pararmeters "..."
// the arBottom of the starting frame and branch pointer of new thread is returned in startFrameArBottom and fsbp
// Start a new stack of branching pointers
#define FUNC_CALL_INIT(startFrameArBottom, fsbp, fn, list) \
    FUNC_CALL_INIT_EOL(RET_POINT(__LINE__), startFrameArBottom, fsbp, fn, APPEND_EOL(list)) \
	if(STACK_STATE_DEBUG) { \
		ADD_RET_POINT_AND_DESC(ENT_POINT(fn), #fn); \
		PRINT_FUNC(INIT, nontail, __LINE__, fn); \
		PRINT_BRANCH_RECORD(fsbp); \
	} \

/***************************************************
 *              FUNC_CALL_RESUME_N                 *
 ***************************************************/
// Call function from frame set startFrameArBottom, resume from fsbp
#define FUNC_CALL_RESUME_N(nesting, startFrameArBottom, fsbp, startFrameArBottom2, fsbp2, ...) \
	if(STACK_STATE_DEBUG) { PRINT_FUNC(RESUME, nontail, __LINE__, <func>); PRINT_BRANCH_RECORD(fsbp); } \
    FUNC_CALL_RESUME_EOL_N(RET_POINT(__LINE__), startFrameArBottom, fsbp, startFrameArBottom2, fsbp2, APPEND_EOL((__VA_ARGS__)))

#define FUNC_CALL_RESUME(startFrameArBottom, fsbp, startFrameArBottom2, fsbp2, ...) \
	FUNC_CALL_RESUME_N(1, startFrameArBottom, fsbp, startFrameArBottom2, fsbp2, ##__VA_ARGS__) \

#define FUNC_CALL_RESUME2(startFrameArBottom, fsbp, startFrameArBottom2, fsbp2, ...) \
	FUNC_CALL_RESUME_N(2, startFrameArBottom, fsbp, startFrameArBottom2, fsbp2, ##__VA_ARGS__) \

/***************************************************
 *              RETURN_N                           *
 ***************************************************/
// return from a function
#define RETURN_N(nesting, ...) \
	{ \
		typedef ConstEq<RETURN_TYPE_ARITY(FUN), Const<int, COUNT(LIST(__VA_ARGS__))> > __checkType; \
		CHECK(return_type_arity_mismatch, __checkType); \
		FOLDR(,CHECK_RETURN_TYPE_CONS, ZIP_RANGE1(LIST(__VA_ARGS__))) \
	} \
	FUNC_EXIT(nesting, __VA_ARGS__); \

#define CHECK_RETURN_TYPE_CONS(x, t) \
	{ \
    	typedef ConstEq<RETURN_TYPE(FUN, FST(x)), typeof(SND(x)) > __checkType; \
    	CHECK(PASTE(return_type_mismatch_, FST(x)), __checkType); \
	} \
	t \

#define FUNC_EXIT(nesting, ...) \
    SET_RET_VAL((__VA_ARGS__)); \
    goto *AR_BOTTOM_N(nesting)->returnProgramPointer; /* return to callee */ \

#define RETURN(...) \
		RETURN_N(1, ##__VA_ARGS__)
#define RETURN2(...) \
		RETURN_N(2, ##__VA_ARGS__)

#define DEFINE_AR_BOTTOM_POINTERS_F(nesting, arg) \
	arg ActivationRecordCommon *AR_BOTTOM_N(nesting); \

/* define all nested levels */
#define DEFINE_AR_BOTTOM_POINTERS \
	CONCAT(MAP_ARG(DEFINE_AR_BOTTOM_POINTERS_F, register, RANGE1(LEVEL_OF_NESTING)))

#ifdef HEAP_STACK
#define USE_STACK \
    BYTE *STACK_BOTTOM; \
    register BYTE *STACK_TOP; \
    DEFINE_AR_BOTTOM_POINTERS; \
    CoroutineState *BRANCH_POINTER; \
    CoroutineDataDependency *GLOBAL_BRANCH_POINTER; \
    CoroutineDataDependency *DATA_DEPENDENCY; \
    CoroutineDataDependency *DATA_DEPENDENCY_ROOT = &__userStack->dataDependencyRoot; \
    struct { \
    	void *retPoint; \
    	const char *desc; \
    } retPointAndDesc[1024 * 16]; \
    int retPointAndDescTop = 0; \
    (void) retPointAndDesc; (void) retPointAndDescTop; \
    if(__userStack->empty()) { \
        STACK_BOTTOM = __userStack->buf; \
        STACK_ADJUST(STACK_BOTTOM); \
        /* define all nested levels */ \
        AR_BOTTOM_N(1) = NULL; \
        AR_BOTTOM_N(2) = NULL; \
        BRANCH_POINTER = NULL; \
        GLOBAL_BRANCH_POINTER = NULL; \
        DATA_DEPENDENCY = &__userStack->dataDependencyRoot; \
    } else { \
        STACK_BOTTOM = __userStack->buf; \
        BRANCH_POINTER = __userStack->branchPointer; \
        GLOBAL_BRANCH_POINTER = __userStack->globalBranchPointer; \
        DATA_DEPENDENCY = __userStack->dataDependency; \
        RESTORE_STATE; \
    } \

#else
#define USE_STACK \
    BYTE *STACK_BOTTOM; \
    register BYTE *STACK_TOP; \
    DEFINE_AR_BOTTOM_POINTERS; \
    CoroutineState *BRANCH_POINTER; \
    CoroutineDataDependency *GLOBAL_BRANCH_POINTER; \
    CoroutineDataDependency *DATA_DEPENDENCY; \
    CoroutineDataDependency *DATA_DEPENDENCY_ROOT = &__userStack->dataDependencyRoot; \
    struct { \
    	void *retPoint; \
    	const char *desc; \
    } retPointAndDesc[1024 * 16]; \
    int retPointAndDescTop = 0; \
    (void) retPointAndDesc; (void) retPointAndDescTop; \
    if(__userStack->empty()) { \
        STACK_BOTTOM = __userStack->buf; \
        STACK_ADJUST(STACK_BOTTOM); \
        /* define all nested levels */ \
        AR_BOTTOM_N(1) = reinterpret_cast<ActivationRecordCommon *>(NULL); \
        AR_BOTTOM_N(2) = reinterpret_cast<ActivationRecordCommon *>(NULL); \
        BRANCH_POINTER = NULL; \
        GLOBAL_BRANCH_POINTER = NULL; \
        DATA_DEPENDENCY = &__userStack->dataDependencyRoot; \
    } else { \
        STACK_BOTTOM = __userStack->buf; \
        BRANCH_POINTER = __userStack->branchPointer; \
        GLOBAL_BRANCH_POINTER = __userStack->globalBranchPointer; \
        DATA_DEPENDENCY = __userStack->dataDependency; \
        RESTORE_STATE; \
    } \

#endif
/* Saves the return point (rp), ar_bottom, ar_bottom_2, branch_pointer (bp), del_flag, and global_branch_pointer (gbp). Set pb and gbp to the new stack top.
 * Invariant: stack_top always points to rp.
 * ars and saved states | previous saved state | ars | ars |
 *                                             ^bp   ^gbp  ^stack top
 * ||
 * \/
 * ars and saved states | previous saved state | ars | rp ar_bottoms del_flag(0) bp gbp |
 *                                                                                      ^bp, gbp, and stack top
 * This macro does not actually save the current ar (local var values), so there should absolutely be no more assignment to local vars between this macro and RETURN
 */  

#define SAVE_STATE(rp) \
	SAVE_STATE_WITH_THREAD(rp, BRANCH_POINTER, -currThreadId, NULL, del) \

#define SAVE_STATE_ANCHOR(rp) \
	SAVE_STATE_WITH_THREAD_ANCHOR(rp, BRANCH_POINTER, -currThreadId, NULL, del) \

#define SAVE_STATE_WITH_THREAD(rp, thread, thid, dep, depType) \
		SAVE_STATE_WITH_THREAD_ANCHOR_PARAM(rp, thread, thid, dep, depType, false) \

#define SAVE_STATE_WITH_THREAD_ANCHOR(rp, thread, thid, dep, depType) \
		SAVE_STATE_WITH_THREAD_ANCHOR_PARAM(rp, thread, thid, dep, depType, true) \

#define SAVE_STATE_WITH_THREAD_ANCHOR_PARAM(rp, thread, thid, dep, depType, anc) \
	STACK_TOP = (BYTE *) region_alloc_type<CoroutineState>(coroutineStateRegion); \
	if(DEBUG_TRACE_DEALLOC) state_alloc_counter++; \
	if(DEBUG_DEALLOC) printf("%d: state %p alloced\n", state_alloc_counter, STACK_TOP); \
	reinterpret_cast<CoroutineState *>(STACK_TOP)->programPointer = && rp; \
	reinterpret_cast<CoroutineState *>(STACK_TOP)->threadId = thid; \
	reinterpret_cast<CoroutineState *>(STACK_TOP)->anchor = anc; \
	/* save all nest levels */ \
	SAVE_AR_BOTTOMS(reinterpret_cast<CoroutineState *>(STACK_TOP), CURR_AR_BOTTOMS); \
	SAVE_BRANCH_POINTER_STATE(thread, LEVELOF(FUN)); \
    if(dep != NULL) { \
    	appendDep(((Trie *)dep)->depType##DependencyNodes, ((Trie *)dep)->depType##DependencyNodesTail, thread); \
    } \
	if(STACK_STATE_DEBUG) { \
		printf("%d:SAVE ", save_state_counter++); \
		printByType(thread); \
		printf("\n"); \
		ADD_RET_POINT_AND_LOC(rp, "saved state at "__FILE__, __LINE__); \
		PRINT_STATE(SAVE, __FILE__, __LINE__);\
		/*PRINT_BRANCH_RECORD(BRANCH_POINTER);*/\
	} \

// precond: preallocation of CoroutineState at STACK_TOP
#define SAVE_BRANCH_POINTER_STATE(thread, nesting) \
	SAVE_DATA_DEPENDENCY_COMMON(nesting); \
	reinterpret_cast<CoroutineState *>(STACK_TOP) -> branchPointer = thread; \
	thread = reinterpret_cast<CoroutineState *>(STACK_TOP); \
    DEBUG_STACK_SIZE_CODE \

#define SAVE_DATA_DEPENDENCY(dep, depType) \
		STACK_TOP = (BYTE *) region_alloc_type<CoroutineState>(coroutineStateRegion); \
		if(DEBUG_TRACE_DEALLOC) dd_alloc_counter++; \
		if(DEBUG_DEALLOC) printf("%d: dd %p alloced\n", dd_alloc_counter, STACK_TOP); \
		SAVE_DATA_DEPENDENCY_COMMON(LEVELOF(FUN)); \
	    if(dep != NULL) { \
	    	appendDep(((Trie *)dep)->depType##DependencyNodes, ((Trie *)dep)->depType##DependencyNodesTail, GLOBAL_BRANCH_POINTER); \
	    } \
		DATA_DEPENDENCY = GLOBAL_BRANCH_POINTER; \
		DATA_DEPENDENCY->threadId = 0; \
		if(STACK_STATE_DEBUG) { \
			printf("%d:SAVE DATA DEPENDENCY ", save_state_counter++); \
			printByType(DATA_DEPENDENCY); \
			printf("\n"); \
			PRINT_STATE(SAVE, __FILE__, __LINE__);\
			/*PRINT_BRANCH_RECORD(BRANCH_POINTER);*/\
		} \

// precond: preallocation of CoroutineDataDependency at STACK_TOP
#define SAVE_DATA_DEPENDENCY_COMMON(nesting) \
	reinterpret_cast<CoroutineDataDependency *>(STACK_TOP)->super = DATA_DEPENDENCY; \
	reinterpret_cast<CoroutineDataDependency *>(STACK_TOP)->next = DATA_DEPENDENCY->child; \
	reinterpret_cast<CoroutineDataDependency *>(STACK_TOP)->prev = NULL; \
	reinterpret_cast<CoroutineDataDependency *>(STACK_TOP)->child = NULL; \
	reinterpret_cast<CoroutineDataDependency *>(STACK_TOP)->directDataDependencyNext = NULL; \
    reinterpret_cast<CoroutineDataDependency *>(STACK_TOP)->globalBranchPointer = GLOBAL_BRANCH_POINTER; \
    reinterpret_cast<CoroutineDataDependency *>(STACK_TOP)->delFlag = 0; \
    reinterpret_cast<CoroutineDataDependency *>(STACK_TOP)->nestingLevel = nesting; \
    if(DATA_DEPENDENCY->child != NULL) { \
    	DATA_DEPENDENCY->child->prev = reinterpret_cast<CoroutineDataDependency *>(STACK_TOP); \
    } \
	DATA_DEPENDENCY->child = reinterpret_cast<CoroutineDataDependency *>(STACK_TOP); \
	GLOBAL_BRANCH_POINTER = reinterpret_cast<CoroutineDataDependency *>(STACK_TOP); \

#ifdef HEAP_STACK
#define RESTORE_BRANCH_POINTER_STATE(thread) \
    /* printf("delete:\n\tglobal = %p, local = %p\n", GLOBAL_BRANCH_POINTER, BRANCH_POINTER); */\
	if(!thread->anchor) { \
		thread->delFlag |= DEL_FLAG_PERM_DEL; /* permanently delete */ \
		thread = thread->branchPointer; \
	}

#else
// need to modify to support anchor states
#define RESTORE_BRANCH_POINTER_STATE(thread) \
    if(reinterpret_cast<CoroutineDataDependency *>(thread) == GLOBAL_BRANCH_POINTER) { \
    	RESTORE_BRANCH_POINTER_STATE1(thread); \
    } else { /* delete current branch pointer from the global branch pointer linked list */ \
    	RESTORE_BRANCH_POINTER_STATE2(thread); \
    } \

#define PREV_IN_THREAD(thread) \
		do { \
			/*printf("discard %p\n", thread); */\
			thread->threadId = -thread->threadId; \
			thread = thread->branchPointer; \
		} while(thread != NULL && thread->delFlag != 0); \
		/*printf("next in thread %p\n", thread); */\

/* set del_flag of the current branch pointer to 1 to mark deleted */
#define RESTORE_BRANCH_POINTER_STATE2(thread) \
		if(STACK_STATE_DEBUG) { \
			printf("%p is not the global_branch_pointer\n", thread); \
			printf("mark branch record %p as deleted\n", thread); \
		} \
    /* printf("delete:\n\tglobal = %p, local = %p\n", GLOBAL_BRANCH_POINTER, BRANCH_POINTER); */\
		thread->delFlag |= DEL_FLAG_PERM_DEL; /* permanently delete */ \
		PREV_IN_THREAD(thread); \

#define RESTORE_BRANCH_POINTER_STATE1(thread) \
		if(STACK_STATE_DEBUG) { \
			printf("%p is the global_branch_pointer\n", GLOBAL_BRANCH_POINTER); \
		} \
        STACK_ADJUST(thread); \
    /* printf("delete:\n\tglobal = %p, local = %p\n", GLOBAL_BRANCH_POINTER, BRANCH_POINTER); */\
		while(GLOBAL_BRANCH_POINTER !=NULL && (GLOBAL_BRANCH_POINTER->delFlag & DEL_FLAG_GARBAGE_COLLECT) != 0) { \
			if(STACK_STATE_DEBUG) { \
				printf("discard deleted branch record %p\n", GLOBAL_BRANCH_POINTER); \
			} \
			CoroutineState *temp = GLOBAL_BRANCH_POINTER; \
			GLOBAL_BRANCH_POINTER = GLOBAL_BRANCH_POINTER->globalBranchPointer; \
			STACK_DEALLOC(temp); \
			printf("%d: state %p dealloced\n", state_dealloc_counter++, temp); \
		} \
        /* the tail of global branch pointer must also be the tail of any thread */ \
		thread->delFlag |= 0x4; /* permanently delete */ \
        PREV_IN_THREAD(thread); \
	/* printf("recycle:\n\tglobal = %p, bottom = %p\n", GLOBAL_BRANCH_POINTER, STACK_BOTTOM); */\

#endif
#define RESTORE_STATE \
	RESTORE_STATE_WITH_THREAD(BRANCH_POINTER) \

#define RESTORE_STATE_WITH_THREAD(thread) \
	RESTORE_STATE_WITH_THREAD_FALL_THROUGH(thread, false) \

// remove the anchor state at BRANCH_POINTER
// this can only be called if there will be no save_state/save_data_dependency before restore_state, as this will recusively delete all dd nodes
// which is correct only if the subtree is fully constructed, i.e., no new leaves without resuming a leaf

#define REMOVE_ANCHOR \
	BRANCH_POINTER->anchor = false; \
	REMOVE_NODE_FROM_TREE(BRANCH_POINTER); \
	CoroutineState *node = BRANCH_POINTER->super; \
	while(node->child == NULL) { \
		REMOVE_NODE_FROM_TREE(node); \
		node = node->super; \
	} \
	BRANCH_POINTER->delFlag |= DEL_FLAG_PERM_DEL; \
	BRANCH_POINTER = BRANCH_POINTER->branchPointer; \



#define RESTORE_STATE_WITH_THREAD_FALL_THROUGH(thread, fallThrough) \
	while(thread != NULL && thread->delFlag != 0) { \
		/* if the delflag is not zero, it means that
		 * this node has been added to this thread by a flip,
		 * and then marked as being deleted by another flip (there may be multiple rounds),
		 * hence its threadId must be 0.
		 * therefore, when we delete it from the current thread, we need to set it threadId back to universalTermSizeLimit,
		 * so that the next time when a flip is performed this one can be added back to this thread.
		 */ \
		if(STACK_STATE_DEBUG) { \
			printf("discard deleted branch record %p\n", thread); \
		} \
		thread->threadId = -thread->threadId; \
		thread = thread->branchPointer; \
	} \
	if(STACK_STATE_DEBUG) printf("starting to restore %p\n", thread); \
	if(thread != NULL) { \
		if(STACK_STATE_DEBUG) { \
				printf("%d:RESTORE ", restore_state_counter++); \
				printByType(thread); \
				printf("\n"); \
				PRINT_STATE(RESTORE, __FILE__, __LINE__); \
				/*PRINT_BRANCH_RECORD(BRANCH_POINTER); */\
				/*breakHere(); */\
		} \
		/* remove current node from data dependency tree */ \
		if(!thread->anchor) {\
			REMOVE_NODE_FROM_TREE(thread); \
		} \
		/* restore data dependency */ \
		DATA_DEPENDENCY = thread->super; \
		/* restore all nested levels */ \
		RESTORE_AR_BOTTOMS(thread, CURR_AR_BOTTOMS); \
		void *temp = thread->programPointer; \
		/* restore branch pointer state */ \
		RESTORE_BRANCH_POINTER_STATE(thread); \
		goto *temp; \
	} else { \
		if(fallThrough) { \
			/* fall through if no state to restore */ \
		} else { \
			throw "RESTORE_STATE: no state to restore and cannot fall through"; \
		} \
	} \

#define REMOVE_NODE_FROM_TREE(thread) \
		if(thread->prev == NULL) { \
			if(thread->next == NULL) { \
				thread->super->child = NULL; \
			} else { \
				thread->super->child = thread->next; \
				thread->next->prev = NULL; \
			} \
		} else { \
			if(thread->next == NULL) { \
				thread->prev->next = NULL; \
			} else { \
				thread->prev->next = thread->next; \
				thread->next->prev = thread->prev; \
			} \
		} \

/* truncate the stack so that all saved state becomes inaccessible 
 * and the only coroutine running is the current one, with no saved state.
 * The stack top is reset to the current activation record top
 * This macro can only be used at a point with no previous SAVE_STATE call */
#define TRUNCATE_STACK \
        GLOBAL_BRANCH_POINTER = NULL; \
        BRANCH_POINTER = NULL; \
        DATA_DEPENDENCY = DATA_DEPENDENCY_ROOT; \
        DATA_DEPENDENCY->child = NULL; \
        STACK_ADJUST(reinterpret_cast<BYTE *>(AR_BOTTOM_N(LEVEL)) + FUN_FUNC_LOCAL_OFFSET(FUN) + FUNC_LOCAL_DEFS_SIZE(FUN)); \




#define GLOBAL_HAS_SAVED_STATES \
    (GLOBAL_BRANCH_POINTER != NULL)


#define HAS_SAVED_STATES \
    (BRANCH_POINTER != NULL)
/*** SEC_BEGIN and SEC_END ***/

#define SEC_BEGIN(fn, list, ...) \
    USE_STACK; \
    /* always go into the first level of nesting */ \
    FUNC_CALL_EOL_N(SEC_RET_POINT, fn, APPEND_EOL(list), APPEND_EOL((__VA_ARGS__)), 1, 1); \
    goto EXT_POINT; \

#define SEC_END \
    EXT_POINT: \
    __userStack->branchPointer = BRANCH_POINTER; \
    __userStack->globalBranchPointer = GLOBAL_BRANCH_POINTER; \

/*******************************************
 *             BRANCH_RECORD               *
 *******************************************/
/* A branch record marks that some of the ars below it may still be in use by some coroutine,
 * so that they are not overwritten by new ars or branch records.
 * There are two usages of branch records:
 * 1. Mark a saved state of a coroutine in the current thread
 * 2. Mark a saved thread, when resuming another thread. When the latter returns, 
 *    the saved thread is restored. In this usage, only the branchPointer field is used.
 */
struct ActivationRecordCommon;

/*template <typename T>
struct DoublyLinkedListPtr {
	T *next;
	T *prev;
	DoublyLinkedListPtr() {
		next = NULL;
		prev = NULL;
	}
};

#define DOUBLY_LINKED_LIST_APPEND(tail, newTail, ptr) \
	if(tail == NULL) { \
	    newTail->ptr.prev = tail; \
	    newTail->ptr.next = NULL; \
	} else { \
	    newTail->ptr.prev = tail; \
	    newTail->ptr.next = NULL; \
	    tail->ptr.next = newTail; \
	} \

#define DOUBLY_LINKED_LIST_DELETE_TAIL(newTail, tail, ptr) \
	    newTail = tail->ptr.prev; \
	    if(newTail != NULL) { \
	    	newTail->ptr.next = NULL; \
	    } \

#define DOUBLY_LINKED_LIST_DELETE_NON_TAIL(elem, ptr) \
	elem->ptr.next->ptr.prev = elem->ptr.prev; \
	if(elem->ptr.prev != NULL) { \
		elem->ptr.prev->ptr.next = elem->ptr.next; \
	} \

#define DOUBLY_LINKED_LIST_DELETE(elem, ptr) \
	if(elem->ptr.next != NULL) { \
		elem->ptr.next->ptr.prev = elem->ptr.prev; \
	} \
	if(elem->ptr.prev != NULL) { \
		elem->ptr.prev->ptr.next = elem->ptr.next; \
	} \
*/

void printIndent(int n);

#define DEL_FLAG_DISABLED_BY_DIRECT_DATA_DEPENDENCY 0x1
#define DEL_FLAG_DISABLED_BY_SUPER_DATA_DEPENDENCY 0x2
#define DEL_FLAG_PERM_DEL 0x4 // this struct is permanently deleted
#define DEL_FLAG_GARBAGE_COLLECT 0x8 // this struct can be garbage collected, 0x4 must also be set if this bit is set

struct CoroutineState {
	CoroutineState();
	CoroutineState(bool isCS);
    DEL_FLAG_TYPE delFlag;
    CoroutineState *super; // super dd tree
    CoroutineState *child;
    CoroutineState *prev;
    CoroutineState *next;
    CoroutineState *directDataDependencyNext; // direct dd thread
    CoroutineState *globalBranchPointer; // thread
    int nestingLevel;
    void *programPointer;
    ActivationRecordCommon *arBottoms[LEVEL_OF_NESTING];
    CoroutineState* branchPointer;
    int threadId;
    bool anchor;


	void printDependencyTree() {
		printDependencyTree(0);
	}
	void printDependencyTree(int indent) {
		printIndent(indent);
		printf("%p", this);
		for(CoroutineDataDependency *c = child; c!=NULL; c=c->next) {
			c->printDependencyTree(indent + 1);
		}
	}
};

struct ActivationRecordCommon {
    void *returnProgramPointer;
    ActivationRecordCommon *arBottoms[LEVEL_OF_NESTING];
    CoroutineState *returnBranchPointer; // use when calling from a different thread by FUNC_CALL_RESUME
    int nestingLevel;
    int callerNestingLevel;
    ActivationRecordCommon();
};

struct UserStack {
    BYTE *buf;
    CoroutineState *stateStack;
    /* branchPointer == NULL if stack is newly created or empty */
    CoroutineState *branchPointer;
    /* globalBranchPointer == NULL iff stack is newly created */
    CoroutineDataDependency *globalBranchPointer;
    CoroutineDataDependency *dataDependency;
    CoroutineDataDependency dataDependencyRoot;
    UserStack() : buf(new BYTE[MAX_STACK_SIZE]), stateStack(new CoroutineState[MAX_STATE_STACK_SIZE]), branchPointer(NULL), globalBranchPointer(NULL), dataDependency(&dataDependencyRoot), dataDependencyRoot() { }
    UserStack(long stackSize, long stateStackSize) : buf(new BYTE[stackSize]), stateStack(new CoroutineState[stateStackSize]), branchPointer(NULL), globalBranchPointer(NULL), dataDependency(&dataDependencyRoot), dataDependencyRoot() { }
    UserStack(long stackSize) :
#ifdef HEAP_STACK
    	buf(NULL),
#else
    	buf(new BYTE[stackSize]),
#endif
    	stateStack(NULL), branchPointer(NULL), globalBranchPointer(NULL), dataDependency(&dataDependencyRoot), dataDependencyRoot() { }
    ~UserStack() {
        delete[] buf;
    }
    bool empty() {
        return this->branchPointer==NULL;
    }
    void printBranchStack() {
        CoroutineState *bp = this->branchPointer;
        int n = 0;
        while(bp!=NULL) {
                printf("%d:%p\n", n,bp);
                bp = bp->branchPointer;
                n++;
        }
    }
};

void flip(CoroutineState *node);
void deleteAndDisableDependencySubtree(CoroutineState *node);
void undeleteAndEnableDependencySubtree(CoroutineState *node);
void markDataDependencySubtreeAsDisabled(CoroutineDataDependency *node);
void markDataDependencySubtreeAsEnabled(CoroutineDataDependency *node);
void printThread(CoroutineState *thread);
// true if it is circular
void checkThread(CoroutineState *thread);


#define STACK_DEF UserStack *__userStack

/* The return value buffer that is used to transiently store return values */
extern BYTE retVals[RETVAL_BUF_SIZE];

/* The ThreadState represent a portion of the stack starting from the arBottomStartFrame to branchPointer.
 * When restoring a ThreadState, the return point and caller arBottom in the start frame should be set to those of the current caller.
 * Any function called in the start frame should not be a tail call, as a tail call duplicates the return point and caller arBottom to the frame of the callee.
 * This will cause inconsistency when the return point and caller arBottom in the start frame change. */
struct ThreadState {
	ActivationRecordCommon *arBottomStartFrame; /* this is the arBottom of the starting frame */
	CoroutineState *branchPointer;
};

#ifdef HEAP_STACK
#define STACK_ALLOC(type, var, dataThread) \
	STACK_PREALLOC_IMPLICIT(LIST(type)); \
	type *var = (type *) STACK_TOP; \
	if(DEBUG_TRACE_DEALLOC) { \
		type::alloc_counter++; \
		type::alloc_size_total += SIZEOF(type); \
	} \


#define STACK_PREALLOC_IMPLICIT(list) \
	STACK_TOP = (BYTE *) region_alloc(stackRegion, SUM_SIZE(list)); \
	if(DEBUG_TRACE_DEALLOC) alloc_size_total += SUM_SIZE(list); \

#define STACK_PREREALLOC_IMPLICIT(ptr, type, oldSize, newSize) \
{ \
	void *oldPtr = ptr; \
	ptr = (type *) region_realloc(stackRegion, ptr, newSize); \
	if(memcmp(oldPtr, ptr, oldSize) != 0){ \
	printf("error\noldPtr = %p, newPtr = %p\n", oldPtr, ptr); \
	for(int i=0;i<oldSize;i++) { \
			printf("%02x ", ((unsigned char *)oldPtr)[i]); \
	} \
	printf("\n"); \
	for(int i=0;i<oldSize;i++) { \
			printf("%02x ", ((unsigned char *)ptr)[i]); \
	} \
	printf("\n"); \
	exit(-1); \
	} \
	STACK_TOP = ((BYTE *) ptr) + oldSize; \
	if(DEBUG_TRACE_DEALLOC) { \
		alloc_size_total += newSize - oldSize; \
		realloc_size_total += newSize - oldSize; \
	} \
}

#define STACK_REALLOC_IMPLICIT(inc) \
	STACK_TOP += inc;\


#define STACK_ALLOC_IMPLICIT(type) \
	STACK_TOP += SIZEOF(type); \

#define STACK_DEALLOC_IMPLICIT(type) \
	STACK_TOP -= SIZEOF(type); \

#define STACK_DEALLOC(ptr) \
	free(ptr); \

#define STACK_ADJUST(ptr) \

#else
//#define STACK_ALLOC(type, var, dataThread) \
//	type *var = reinterpret_cast<type *>(STACK_TOP); \
//	STACK_ALLOC_IMPLICIT(type); \
//    /* save the current state to protect stack allocated data, should not be restored */ \
//    SAVE_STATE_WITH_THREAD(PASTE(rp_dummy_, __LINE__), dataThread, universalTermSizeLimit, NULL); \
//    PASTE(rp_dummy_, __LINE__): \
//	if(DEBUG_STACK_ALLOC_SWITCH) { \
//		DEBUG_RECORD_STACK_ALLOC(var, SIZEOF(type)); \
//	} \

#define STACK_ALLOC(type, var, dataThread) \
	type *var = (type *) malloc(sizeof(type));
//	type *var = reinterpret_cast<type *>(STACK_TOP); \
//	STACK_ALLOC_IMPLICIT(type); \
//	if(DEBUG_STACK_ALLOC_SWITCH) { \
//		DEBUG_RECORD_STACK_ALLOC(var, SIZEOF(type)); \
//	} \

#define STACK_PREALLOC_IMPLICIT(type) \

#define STACK_ALLOC_IMPLICIT(type) \
	STACK_TOP += SIZEOF(type); \
	DEBUG_STACK_SIZE_CODE \
	if(DEBUG_STACK_ALLOC_SWITCH) { \
		DEBUG_OVERWRITE_STACK_ALLOCS(STACK_TOP - SIZEOF(type), SIZEOF(type)); \
	} \

#define STACK_PREREALLOC_IMPLICIT(ptr, type, oldSize, newSize) \

#define STACK_REALLOC_IMPLICIT(inc) \
	STACK_TOP += inc;\


#define STACK_DEALLOC_IMPLICIT(type) \
	STACK_TOP -= SIZEOF(type); \
	if(DEBUG_STACK_ALLOC_SWITCH) { \
		DEBUG_INVALIDATE_STACK_ALLOCS; \
	} \

#define STACK_DEALLOC(ptr) \


#define STACK_ADJUST(ptr) \
	STACK_TOP = reinterpret_cast<BYTE *>(ptr); \
	if(DEBUG_STACK_ALLOC_SWITCH) { \
		DEBUG_INVALIDATE_STACK_ALLOCS; \
	} \

#endif

#define CONSTRUCTOR(T) T##_new

#define NEW(T, param, vn) FUNC_CALL(CONSTRUCTOR(T), param, vn)

#define AEP_TO_STRING(x) #x

#define TO_STRING(x) AEP_TO_STRING(x)

// DEBUG STACK_ALLOC
#ifdef DEBUG_STACK_ALLOC
extern LinkedStack<int> *stackAllocatedBlock;
extern LinkedStack<int> *stackInvalidatedBlock;
extern LinkedStack<int> *stackOverwrittenBlock;

#define DEBUG_RECORD_STACK_ALLOC(ptr, size) \
	printf("stack block %p allocated on line %d\n", ptr, __LINE__); \
	DEBUG_OVERWRITE_STACK_ALLOCS(ptr, size); \
	stackAllocatedBlock = stackAllocatedBlock->push((Symbol) ptr, size); \
	REMOVE_FROM_STACK_LIST(stackInvalidatedBlock, (Symbol) ptr); \
	REMOVE_FROM_STACK_LIST(stackOverwrittenBlock, (Symbol) ptr); \

#define REMOVE_FROM_STACK_LIST(list, k) \
	{\
		LinkedStack<int> *temp = list; \
		LinkedStack<int> *tempNext = NULL; \
		while(temp != NULL) { \
			if(temp->key == k) { \
				if(tempNext == NULL) { \
					list = temp->prev; \
				} else { \
					tempNext->prev = temp->prev; \
				} \
				delete temp; \
				break; \
			} \
			tempNext = temp; \
			temp = temp -> prev; \
		} \
	}\


#define DEBUG_INVALIDATE_STACK_ALLOCS \
	while(stackAllocatedBlock != NULL && stackAllocatedBlock->key >= (Symbol) STACK_TOP) { \
		printf("stack block %p invalidated on line %d\n", (void *) stackAllocatedBlock->key, __LINE__); \
		LinkedStack<int> *temp = stackAllocatedBlock; \
		stackAllocatedBlock = stackAllocatedBlock->prev; \
		temp->prev = stackInvalidatedBlock; \
		stackInvalidatedBlock = temp; \
	} \
	/* printf("stack alloc'ed address %p") */ \
	assert(stackAllocatedBlock == NULL || stackAllocatedBlock->key + stackAllocatedBlock->val <= (u_int64_t) STACK_TOP); \

#define DEBUG_OVERWRITE_STACK_ALLOCS(ptr, size) \
	{ \
		LinkedStack<int> *temp = stackInvalidatedBlock; \
		LinkedStack<int> *tempNext = NULL; \
		while(temp != NULL) { \
			if((temp->key >= (Symbol) ptr && temp->key < ((Symbol) ptr) + size) || \
			   (temp->key + temp->val >= (Symbol) ptr && temp->key + temp->val < ((Symbol) ptr) + size)) { \
				if(tempNext == NULL) { \
					stackInvalidatedBlock = temp->prev; \
				} else { \
					tempNext->prev = temp->prev; \
				} \
				printf("stack block %p overwritten on line %d\n", (void *) temp->key, __LINE__); \
				temp->prev = stackOverwrittenBlock; \
				stackOverwrittenBlock = temp; \
				break; \
			} \
			tempNext = temp; \
			temp = temp -> prev; \
		} \
	} \

#else
#define DEBUG_INVALIDATE_STACK_ALLOCS
#define DEBUG_RECORD_STACK_ALLOC(ptr, size)
#define REMOVE_FROM_STACK_LIST(list, k)
#define DEBUG_OVERWRITE_STACK_ALLOCS(ptr, size)
#endif

/*** DEBUG: VAR_FN_TYPE ***/
#define VAR_TYPE_FN_N(type, fn, vn, nesting) \
    (*(type *)(reinterpret_cast<BYTE *>(AR_BOTTOM_N(nesting))+OFFSET_IMPL_F(fn, vn)))

#define PRINT_STACK_CONS(h, t) \
	h\
	t

#define PRINT_STACK_ONE(nil, cons, x) \
	x

#define PRINT_STACK_F_N(h, nesting) \
    printf("%s(%ld) : %s = ", TO_STRING(SND(h)), (long) SIZEOF(FST(h)), TO_STRING(FST(h))); \
    if((strchr(TO_STRING(FST(h)), '*') != NULL) && (VAR_TYPE_FN_N(FST(h), FUN, SND(h),nesting) == 0)) { \
		printf("<<null>>"); \
    } else { \
		printByType(VAR_TYPE_FN_N(FST(h), FUN, SND(h),nesting)); \
	} \
    printf("\n"); \

#define PRINT_STACK_N(nesting, ...) \
    printf("stack %s:\n", TO_STRING(FUN)); \
    printf("----------\n"); \
    printf("nesting = %s, stack_bottom = %p, stack_top = %p, global_branch_pointer = %p, thread_branch_pointer = %p,\nret_pointer = %p, return_thread_branch_pointer = %p,\n", \
        #nesting, STACK_BOTTOM, STACK_TOP, GLOBAL_BRANCH_POINTER, BRANCH_POINTER, \
        AR_BOTTOM_N(nesting)->returnProgramPointer, \
        AR_BOTTOM_N(nesting)->returnBranchPointer); \
	PRINT_PREV_AR_BOTTOMS(nesting); \
    PRINT_AR_BOTTOMS; \
    printf("----------\n"); \
    MAP_FOLDR(NOTHING,PRINT_STACK_ONE,PRINT_STACK_CONS,ID,PRINT_STACK_F_N,nesting, PACK(APPEND_EOL((__VA_ARGS__)))) \
    printf("==========\n");

#define PRINT_FUNC(act, type, line, fn) \
	printf("%s function %s, line %d, type %s\n", #act, #fn, line, #type);

#define PRINT_STATE(act, file, line) \
	printf("%s STATE, at %s:%d\n", #act, file, line);

#define PRINT_AR_BOTTOMS_F(nesting) \
	printf("ar_bottom_"#nesting" = %p, ", AR_BOTTOM_N(nesting)); \

#define PRINT_AR_BOTTOMS \
	CONCAT(MAP(PRINT_AR_BOTTOMS_F, RANGE1(LEVEL_OF_NESTING))) \
	printf("\n");

#define PRINT_PREV_AR_BOTTOMS_F_ARG(arb_nesting, nesting) \
	printf("prev_ar_bottom_"STRINGIFY(arb_nesting)" = %p, ", AR_BOTTOM_N(nesting)->arBottoms[nesting-1]); \

#define PRINT_PREV_AR_BOTTOMS(nesting) \
	CONCAT(MAP_ARG(PRINT_PREV_AR_BOTTOMS_F_ARG, nesting, RANGE1(LEVEL_OF_NESTING))) \
	printf("\n");

#define PRINT_BRANCH_RECORD_AR_BOTTOMS_F(nesting, coroutineState) \
	printf("ar_bottom_"#nesting" = %p, ", coroutineState->arBottoms[nesting-1]); \

#define PRINT_BRANCH_RECORD_AR_BOTTOMS(branchPoint) \
	CONCAT(MAP_ARG(PRINT_BRANCH_RECORD_AR_BOTTOMS_F, branchPoint, RANGE1(LEVEL_OF_NESTING))) \
	printf("\n");

#define PRINT_BRANCH_RECORD(branchPoint) \
	{ \
		CoroutineState *temp = branchPoint; \
		int counter = 0; \
		while(temp != NULL) { \
			const char *desc; \
			LOOKUP_RET_POINT_AND_DESC(temp->programPointer, desc); \
			printf("%d: branch point %p:\n", counter ++, temp); \
			printf("----------\n"); \
			printf("ret_pointer = %p (%s), thread_prev_branch_pointer = %p, global_prev_branch_pointer = %p\n", \
				temp->programPointer, desc, \
				temp->branchPointer, \
				temp->globalBranchPointer); \
			PRINT_BRANCH_RECORD_AR_BOTTOMS(temp); \
			printf("----------\n"); \
			temp = temp->branchPointer; \
		} \
	} \
    printf("==========\n");

#define EAP_ADD_RET_POINT_AND_LOC(rp, f, l) \
	ADD_RET_POINT_AND_DESC(rp, f ":" #l)

#define ADD_RET_POINT_AND_LOC(rp, f, l) \
	EAP_ADD_RET_POINT_AND_LOC(rp, f, l)

#define ADD_RET_POINT_AND_DESC(rp, d) \
	{ \
    	/*printf("added desc %s for rp %s (%p)\n", d,#rp, && rp);*/ \
		bool found = false; \
		for(int i = 0; i < retPointAndDescTop; i++) { \
			if(retPointAndDesc[i].retPoint == && rp) { \
				found = true; \
				break; \
			} \
		} \
		if(!found) { \
			retPointAndDesc[retPointAndDescTop].retPoint = && rp; \
			retPointAndDesc[retPointAndDescTop++].desc = d; \
		} \
	}

#define LOOKUP_RET_POINT_AND_DESC(rpptr, d) \
	{\
    	/*printf("lookup desc for rp %p\n", rpptr);*/ \
		bool found = false; \
		for(int i = 0; i < retPointAndDescTop; i++) { \
			if(retPointAndDesc[i].retPoint == rpptr) { \
				d = retPointAndDesc[i].desc; \
				found = true; \
				break; \
			} \
		} \
		if(!found) { \
			d = "<not found>"; \
		}\
	}
extern int save_state_counter;
extern int restore_state_counter;
extern int ar_dealloc_counter;
extern int ar_alloc_counter;
extern int state_dealloc_counter;
extern int state_alloc_counter;
extern int dd_dealloc_counter;
extern int dd_alloc_counter;
extern size_t dealloc_size_total;
extern size_t alloc_size_total;
extern size_t realloc_size_total;
extern Region *firstGenStackRegion;
extern Region *secondGenStackRegion;
extern Region *stackRegion;
extern Region *coroutineStateRegion;

void breakHere();
#endif	/* STACK_H */

