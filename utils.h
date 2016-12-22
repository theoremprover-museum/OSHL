/* 
 * File:   utils.h
 * Author: Hao Xu
 *
 */

#ifndef UTILS_H
#define	UTILS_H
#include "defs.h"
#include "region.h"
#include "linkedStack.h"
#include "expansibleList.h"

#define SWAP2(i, j) \
		tempt = t[i];\
		temps = s[i];\
		t[i] = t[j];\
		s[i] = s[j];\
		t[j] = tempt;\
		s[j] = temps;

#define SWAP1(t, i, j) \
		temp = t[i];\
		t[i] = t[j];\
		t[j] = temp;\


struct Term;

template<typename S, typename T>
struct Pair {
    S fst;
    T snd;
	Pair(S fst, T snd) : fst(fst), snd(snd) {
	}
};

template<typename S, typename T, typename U>
struct Triple {
    S fst;
    T snd;
    U third;
	Triple(S fst, T snd, U third) : fst(fst), snd(snd), third(third) {
	}
};

typedef LinkedStack<Term *> Sub;

typedef ExpansibleList<Symbol, 0> FastRenaming;

typedef LinkedStack<Symbol> Renaming;

typedef ExpansibleList<bool, false> FastMask;

typedef LinkedStack<bool> Mask;

// precond: the space of sub > max var id in b and sub
// precond: for all var id i in b and sub, if the var is free, then sub[i] == NULL
bool occur(Symbol a, Term *b, Sub *sub, Version versionB = 0);

// precond: the space of sub > max var id in a, b, and sub
// precond: for all var id i in a, b, and sub, if the var is free, then sub[i] == NULL
Sub* unify(Term *a, Term *b, Sub *sub, Region *r, Version versionA = 0, Version versionB = 0);

Sub *unify(bool signa, Term *a, bool signb, Term *b, Sub *sub, Region *r);

bool outOfDom(Sub *sub, Term *t);

bool isRenaming(Sub *sub);

// extract sub-independent term mapped from var with id a
Term *extractTermFromSub(int a, Sub *sub);

bool variant(Term *a, Term *b, Sub *&sub, Symbol &minFreeVarIdB, Region *r);

bool variant(bool signa, Term *a, bool signb, Term *b, Renaming *&sub, Symbol &minFreeVarIdB, Region *r);

Symbol minVarId(Term **terms, int n);

Symbol minVarId(Term *term);

Symbol maxVarId(Term *term);

void truncate(char *buf, int len);

char *formatSize(size_t size);

template<typename T>
T min(T a, T b) {
	return a<=b?a:b;
}
template<typename T>
T max(T a, T b) {
	return a>=b?a:b;
}


template<typename T, int (*cmp)(T, T)>
void quicksort_gen(T *t, unsigned int n) {
	if(n <= 1) {
		return;
	}
	// pivotIndex = 0;
	T temp;

	unsigned int a = 1, b = n-1;
	// pivot [>] [<=]
	//         b a
	while(a <= b) {
		if(cmp(t[a], t[0]) > 0) {
			a++;
		} else {
			SWAP1(t, a, b);
			b--;
		}
	}
	if(a == n) { // all > pivot
		SWAP1(t, 0, b);
		quicksort_gen<T, cmp>(t, n-1);
	} else if (b == 0) { // all <= pivot
		quicksort_gen<T, cmp>(t+1, n-1);
	} else {
		SWAP1(t, 0, b);
		quicksort_gen<T, cmp>(t, a-1);
		quicksort_gen<T, cmp>(t+a, n-a);
	}
}
#endif	/* UTILS_H */

