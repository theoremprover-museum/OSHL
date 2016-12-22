/*
 * File:   macros.h
 * Author: Hao Xu
 *
 */

#ifndef MACROS_H
#define	MACROS_H

// var arg functions
/*** EXPAND LIST ***/
#define __EXPAND(...) \
    __VA_ARGS__

#define EXPAND(list) \
    __EXPAND list

/*** END_OF_LIST ***/
#define EOL __EOL__

/*** PREC ***/
#define __PREC1 \
    0
#define __PREC2 \
    1
#define __PREC3 \
    2
#define __PREC4 \
    3
#define __PREC5 \
    4
#define __PREC6 \
    5
#define __PREC7 \
    6
#define __PREC8 \
    7
#define __PREC9 \
    8
#define __PREC10 \
    9
#define __PREC11 \
    10
#define __PREC12 \
    11
#define __PREC13 \
    12
#define __PREC14 \
    13
#define __PREC15 \
    14
#define __PREC16 \
    15
#define __PREC17 \
    16
#define __PREC18 \
    17
#define __PREC19 \
    18
#define __PREC20 \
    19
#define PREC_DISP(N) \
    __PREC##N

#define PREC(N) \
    APP_EVAL_PARAMS(PREC_DISP, N)

/*** SUCC ***/
#define __SUCC0 \
    1
#define __SUCC1 \
    2
#define __SUCC2 \
    3
#define __SUCC3 \
    4
#define __SUCC4 \
    5
#define __SUCC5 \
    6
#define __SUCC6 \
    7
#define __SUCC7 \
    8
#define __SUCC8 \
    9
#define __SUCC9 \
    10
#define __SUCC10 \
    11
#define __SUCC11 \
    12
#define __SUCC12 \
    13
#define __SUCC13 \
    14
#define __SUCC14 \
    15
#define __SUCC15 \
    16
#define __SUCC16 \
    17
#define __SUCC17 \
    18
#define __SUCC18 \
    19
#define __SUCC19 \
    20
#define SUCC_DISP(N) \
    __SUCC##N

#define SUCC(N) \
    APP_EVAL_PARAMS(SUCC_DISP, N)

/*** APP_EVAL_PARAMS ***/
#define APP_EVAL_PARAMS(f, ...) \
    f(__VA_ARGS__)

/*** NO EOL THE TRICKY MACROS ***/

/*** IS_EMPTY_NO_EOL */
#define __IS_EMPTY_NO_EOL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) \
    N

/* the syntax ", ##__VA_ARGS__" is a gnu extension that deletes the comma if __VA_ARGS__ is empty */
#define IS_EMPTY_NO_EOL_EXPAND_LIST(...) \
    __IS_EMPTY_NO_EOL(0 , ##__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1) // works for size <= 7

#define IS_EMPTY_NO_EOL(list) \
    APP_EVAL_PARAMS(IS_EMPTY_NO_EOL_EXPAND_LIST, __EXPAND list)

/*** APPEND_EOL */
#define __APPEND_EOL0(list) \
    (__EXPAND list, EOL)
#define __APPEND_EOL1(list) \
    (EOL)
#define APPEND_EOL_DISP(list, N) \
    __APPEND_EOL##N(list)

#define APPEND_EOL(list) \
    APP_EVAL_PARAMS(APPEND_EOL_DISP, list, IS_EMPTY_NO_EOL(list))

#define LIST(...)\
	APPEND_EOL((__VA_ARGS__))

/*** HAS EOL THE LESS TRICKY MACROS ***/

/*** GET ***/
#define __GET0(_, x1,...) \
    x1
#define __GET1(_, x1, x2,...) \
    x2
#define EVAL_PARAMS_GET1(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET2(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define EVAL_PARAMS_GET2(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET3(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define EVAL_PARAMS_GET3(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET4(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define EVAL_PARAMS_GET4(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET5(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define EVAL_PARAMS_GET5(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET6(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define EVAL_PARAMS_GET6(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET7(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define EVAL_PARAMS_GET7(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET8(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define EVAL_PARAMS_GET8(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET9(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define EVAL_PARAMS_GET9(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET10(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET10(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET11(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET11(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET12(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET12(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET13(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET13(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET14(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET14(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET15(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET15(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET16(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET16(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET17(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET17(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET18(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET18(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET19(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET19(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)
#define __GET20(PREC_REC,x1,...) \
    EVAL_PARAMS_GET##PREC_REC(PREC_REC, PREC(PREC_REC) , ##__VA_ARGS__)
#define __EVAL_PARAMS_GET20(PREC_REC, PREC_PREC_REC, ...) \
    __GET##PREC_REC(PREC_PREC_REC , ##__VA_ARGS__)

#define GET_DISP(N, PREC_N, ...) \
    __GET##N(PREC_N , ##__VA_ARGS__)

#define GET_EVAL_PARAMS(f, ...) \
    f(__VA_ARGS__)

#define GET(N, list) \
    GET_EVAL_PARAMS(GET_DISP, N, PREC(N), __EXPAND list)

/*** COUNT ***/
#define __COUNT(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) \
    N

#define COUNT(list) \
    APP_EVAL_PARAMS(__COUNT, __EXPAND list, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) // works for size <= 7

/*** HEAD ***/
#define HEAD(list) \
    GET(0, list)

/*** TAIL ***/
#define __TAIL(x, ...) \
    (__VA_ARGS__)
#define TAIL(list) \
    APP_EVAL_PARAMS(__TAIL, __EXPAND list)

/*** IS_EMPTY ***/
#define IS_EMPTY(list) \
    IS_EMPTY_NO_EOL(TAIL(list))

/*** MAP_FOLDR ***/
#define __MAP_FOLDR0(_, nil, one, cons, hd, tl, farg, f, arg, list) \
    nil

#define __MAP_FOLDR1(_, nil, one, cons, hd, tl, farg, f, arg, list) \
    one(nil, cons, f(HEAD(list), arg))

#define APP_EVAL_PARAM_MAP_FOLDR1(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR2(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR2(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR3(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR3(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR4(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR4(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR5(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR5(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR6(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR6(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR7(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR7(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR8(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR8(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR9(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR9(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR10(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR10(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR11(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR11(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR12(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR12(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR13(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR13(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR14(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR14(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR15(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR15(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR16(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR16(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR17(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR17(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR18(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR18(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR19(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR19(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)
#define __MAP_FOLDR20(PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    cons(f(hd(list), arg), APP_EVAL_PARAM_MAP_FOLDR##PREC_N(PREC_N, PREC(PREC_N), nil, one, cons, hd, tl, farg, f, farg(arg), tl(list)))
#define APP_EVAL_PARAM_MAP_FOLDR20(PREC_N, PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list) \
    __MAP_FOLDR##PREC_N(PREC_PREC_N, nil, one, cons, hd, tl, farg, f, arg, list)

#define APP_EVAL_PARAMS_MAP_FOLDR_DISP(f, ...) \
    f(__VA_ARGS__)

#define MAP_FOLDR_DISP(nil, one, cons, hd, tl, N, farg, f, arg, list) \
    APP_EVAL_PARAMS_MAP_FOLDR_DISP(__MAP_FOLDR##N, PREC(N), nil, one, cons, hd, tl, farg, f, arg, list)

#define APP_EVAL_PARAMS_MAP_FOLDR(f, ...) \
    f(__VA_ARGS__)
/*
#define MAP_FOLDR(nil, one, cons, farg, f, arg, list) \
    APP_EVAL_PARAMS_MAP_FOLDR(MAP_FOLDR_DISP, nil, one, cons, farg, f, arg, list)
*/
/* map_foldr_disp(nil, one, cons, hd, tl, n, farg, f, arg, list) = 
 *     match n with
 *         0 => nil
 *         1 => one(nil, cons, f(hd(list), arg))
 *         n => cons(f(hd(list), arg), map_folder(nil, one, cons, farg, f, farg(arg), tl(list))
 *     
 */
/* map_foldr(nil, one, cons, farg, f, arg, list) = 
 *     match list with
 *         () => nil
 *         (x) => one(nil, cons, f(x, arg))
 *         x::y => cons(f(x, arg), map_folder(nil, one, cons, farg, f, farg(arg), y)
 *     
 */
#define MAP_FOLDR(nil, one, cons, farg, f, arg, list) \
    APP_EVAL_PARAMS_MAP_FOLDR(MAP_FOLDR_DISP, nil, one, cons, HEAD, TAIL, COUNT(list), farg, f, arg, list)

/*** FOLDR ***/
#define NOTHING

#define ID(x, ...) \
    x

#define FOLDR_ONE(nil, cons, x) \
    cons(x, nil)

#define FOLDR(nil, cons, list) \
    MAP_FOLDR(nil, FOLDR_ONE, cons, ID, ID, NOTHING, list)

/*** MAP ***/
#define MAP_CONS(h, t) \
    h, t

#define MAP_F(x, arg) \
    arg(x)

#define MAP(f, list) \
    (MAP_FOLDR(EOL, FOLDR_ONE, MAP_CONS, ID, MAP_F, f, list))

/*** MAP_ARG ***/
#define MAP_ARG_CONS(h, t) \
    h, t

#define AEP_MAP_ARG_F(x, arg1, arg2) \
    arg1(x, arg2)

#define MAP_ARG_F(x, arg) \
    AEP_MAP_ARG_F(x, FST(arg), SND(arg))
    
#define MAP_ARG(f, arg, list) \
    (MAP_FOLDR(EOL, FOLDR_ONE, MAP_CONS, ID, MAP_ARG_F, (f, arg), list))

/*** ZIP ***/
/*
 * ZIP((a1, a2, ..., an, #), (b1, b2, ..., bn, #)) = ((a1, b1), (a2, b2), ..., (an, bn), #)
 */
#define ZIP_F(x, arg) \
    (x, HEAD(arg))

#define ZIP_FARG(arg) \
    TAIL(arg)

#define ZIP(list1, list2) \
    (MAP_FOLDR(EOL, FOLDR_ONE, MAP_CONS,ZIP_FARG, ZIP_F, list2, list1))

/*** UNZIPF AND UNZIPS ***/
#define UNZIPF_F(x) \
    FST(x)

#define UNZIPS_F(x) \
    SND(x)

#define UNZIPF(list) \
    MAP(UNZIPF_F, list)

#define UNZIPS(list) \
    MAP(UNZIPS_F, list)

/*** REVERSE ***/
/*
 * REVERSE((a1, a2, ..., an, #)) = (an, a(n-1), a(n-2), ..., a1, #)
 */
#define REVERSE_CONS(h, t) \
    t, h
#define REVERSE(list) \
    TAIL((FOLDR(EOL, REVERSE_CONS, list), EOL))

/*** DISTRI ***/
/*
 * DISTRI((a1, a2, ..., a(2n), #)) = ((a1, a3, ..., a(2n-1), #), (a2, a4, ..., a(2n), #))
 */
#define DISTRI_NIL \
    ((EOL), (EOL))

#define DISTRI_CONS(h, t) \
    (SND(t), CONS(h, FST(t)))

#define DISTRI(list) \
    FOLDR(DISTRI_NIL, DISTRI_CONS, list)

/*** PACK ***/
/* PACK((a1, a2, ..., a(2n), #)) = ((a1,a2), (a3,a4), ..., (a(2n-1), a(2n-2)), #) */
#define PACK_NIL \
    (PACK_CONS1, EOL)

#define PACK_CONS1(h, t) \
    CONS(PACK_CONS2, CONS(h, t))

#define PACK_CONS2(h, t) \
    CONS(PACK_CONS1, CONS((h, HEAD(t)), TAIL(t)))

#define AEP_PACK_CONS(f, x, y) \
    f(x, y)

#define PACK_CONS(h, t) \
    AEP_PACK_CONS(HEAD(t), h, TAIL(t))

#define PACK(list) \
    TAIL(FOLDR(PACK_NIL, PACK_CONS, list))

/*** DELETE TAIL ***/
/* DELETE_TAIL((a1, a2, ..., an, #)) = (a1, a2, ..., a(n-1), #) */
#define DELETE_TAIL_ONE(nil, cons, x) \
    EOL

#define DELETE_TAIL(list) \
    (MAP_FOLDR(NOTHING, DELETE_TAIL_ONE, MAP_CONS, ID, ID, NOTHING, list))

/*** APPEND ***/
/* APPEND((a1, a2, ..., an, #), b) = (a1, a2, ..., an, b, #) */
#define APPEND(list, x) \
    (FOLDR(x, MAP_CONS, list),EOL)

#define APPEND_LIST(l1, l2) \
	FOLDR(l2, CONS, l1)

/*** CONS ***/
/* CONS(b, (a1, a2, ..., an, #)) = (b, a1, a2, ..., an, #) */
#define CONS(x, list) \
    (x, __EXPAND list)

/*** DELETE_EOL ***/
#define DELETE_EOL_NIL

#define DELETE_EOL_ONE(nil, cons, x) \
    x

#define DELETE_EOL_CONS(h, t) \
    h, t

#define DELETE_EOL(list) \
    (MAP_FOLDR(DELETE_EOL_NIL, DELETE_EOL_ONE, DELETE_EOL_CONS, ID, ID, NOTHING, list))

/*** FST ***/
#define FST(list) \
    GET(0, list)

/*** SND ***/
#define SND(list) \
    GET(1, list)

/*** CONCAT ***/
#define CONCAT_CONS(h, t) \
    h t
#define CONCAT(list) \
    FOLDR(NOTHING, CONCAT_CONS, list)
/*** SUM ***/
#define ADD(x, y) \
    ((x) + (y))

#define SUM(list) \
    FOLDR(0, ADD, list)

/*** ACCU ***/
/* ACCU((a1, a2, ..., an, #)) = (FOLDR(0, +, (a1, a2, ..., an, #)), FOLDR(0, +, (a2, ..., an, #)), ..., FOLDR(0, +, (an, #)),  #)
 */
#define ACCU_CONS(h, t) \
    CONS(ADD(h, HEAD(t)), CONS(ADD(h, HEAD(t)), TAIL(t)))

#define ACCU(list) \
    TAIL( FOLDR((0,EOL), ACCU_CONS, list) )

/*** ACCU_REV ***/
/* ACCU_REV is the same as ACCU except that it reverses the operands of ADD
 */
#define ACCU_REV_CONS(h, t) \
    CONS(ADD(HEAD(t), h), CONS(ADD(HEAD(t), h), TAIL(t)))

#define ACCU_REV(list) \
    TAIL( FOLDR((0,EOL), ACCU_REV_CONS, list) )

/*ACCU_REV((1,2,EOL))
ACCU((1,2,EOL))*/

/*** IS_ZERO ***/
/* IS_ZERO(0, z, o) = z
 * IS_ZERO(1, z, o) = o */
#define __IS_ZERO1(zero, one) \
    one
#define __IS_ZERO0(zero, one) \
    zero
#define AEP_IS_ZERO_DISP(N, zero, one) \
    __IS_ZERO##N(zero, one)
#define IS_ZERO(N, zero, one) \
    AEP_IS_ZERO_DISP(N, zero, one)

/*** SHIFT ***/
/* SHIFT((EOL), e) = (EOL)
 * SHIFT((x,y,...,z,EOL), e) = SHIFT((y,...,z,e,EOL)) */
#define __SHIFT1(list, e) \
    list
#define __SHIFT0(list, e) \
    APPEND(TAIL(list), e)
#define AEP_SHIFT_DISP(N, list, e) \
    __SHIFT##N(list, e)
#define SHIFT_DISP(N, list, e) \
    AEP_SHIFT_DISP(N, list, e)
#define SHIFT(list, e) \
    SHIFT_DISP(IS_EMPTY(list), list, e)

/* map_foldr_disp(nil, one, cons, hd, tl, n, farg, f, arg, list) = 
 *     match n with
 *         0 => nil
 *         1 => one(nil, cons, f(hd(list), arg))
 *         n => cons(f(hd(list), arg), map_folder(nil, one, cons, farg, f, farg(arg), tl(list))
 *     
 * nil = ()
 * one(_, _, x) = (x)
 * cons(x, y) = CONS(x, y)
 * hd = ID
 * tl = ID
 * farg = ID
 * f(_, arg) = arg
 * arg = x
 * list = __UNUSED__
 */
#define REPEAT_ONE(_1, _2, x) (x, EOL)
#define REPEAT_F(_, x) x

#define REPEAT(x, n) \
    APP_EVAL_PARAMS_MAP_FOLDR(MAP_FOLDR_DISP, (EOL), REPEAT_ONE, CONS, ID, ID, n, ID, REPEAT_F, x, __UNUSED__)

/* range1(n) = (1, ..., n, #) */
#define RANGE1_CONS(x, y) \
	CONS(PREC(HEAD(y)), y)

// need to do it this way to support n = 0
#define RANGE1(n) \
	TAIL(ROTATE_LEFT(FOLDR(LIST(SUCC(n)), RANGE1_CONS, REPEAT(TOKEN, n)), n))

/* apply_n(f,x,n) = f(f...f(x)...) for n times, for n >= 0 */
#define APPLY_N_CONS(f, x) f(x)

#define APPLY_N(f, x, n) \
    FOLDR(x, APPLY_N_CONS, REPEAT(f, n))

#define DIFF(n, m) APPLY_N(PREC, n, m)

/* rotate_left(n, m) = (m+1, ..., n, 1, ..., m) */
/* y is of the form (tail, head elems) */
#define ROTATE_LEFT_SPLIT_CONS(x, y) (TAIL(FST(y)), CONS(HEAD(FST(y)), SND(y)))

#define ROTATE_LEFT_SPLIT(l, n) FOLDR((l, LIST()), ROTATE_LEFT_SPLIT_CONS, REPEAT(TOKEN, n))

#define ROTATE_LEFT(l, n) \
	APPEND_LIST(FST(ROTATE_LEFT_SPLIT(l, n)), REVERSE(SND(ROTATE_LEFT_SPLIT(l,n))))

// ROTATE_LEFT(LIST(1,2,3,4),2)
// RANGE1(0)
// RANGE1(3)
/*** MARK AND CHECK_MARK ***/

#define PASTE(x, y) AEP_PASTE(x,y)
#define AEP_PASTE(x, y) x##y

#define AEP_STRINGIFY(s) \
		#s

#define STRINGIFY(s) \
	AEP_STRINGIFY(s)


#define __MARK(a, b) \
    __mark_##a##_##b
#define MARK(a, b) \
    const int __MARK(a, b) = 0

#define __CHECK_MARK(a, b, c) \
    __check_##a##_##b##_##c
#define CHECK_MARK(a, b, c) \
    const int __CHECK_MARK(a, b, c) = __MARK(a, b)

#define ZIP_RANGE1(l) \
	ZIP(RANGE1(COUNT(l)), l)

template <typename T1, typename T2>
struct IsSame {
	static const int value = -1;
};

template <typename T>
struct IsSame<T,T> {
	static const int value = 1;
};

template <typename T, T val>
struct Const { };

template <typename S, typename T>
struct ConstEq {
	static const int value = IsSame<S, T>::value;
};

#define CHECK(name, x) \
	char PASTE(__check_failed_, name)[x::value] __attribute__((unused));

struct Tuple0 {};

template <typename T1>
struct Tuple1 {};

template <typename T1, typename T2>
struct Tuple2 {};

template <typename T1, typename T2, typename T3>
struct Tuple3 {};

template <typename T1, typename T2, typename T3, typename T4>
struct Tuple4 {};

template <typename T1, typename T2, typename T3, typename T4, typename T5>
struct Tuple5 {};

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct Tuple6 {};

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
struct Tuple7 {};

//template <typename T>
//struct __Wrap {};

#endif	/* STACK_H */

