/*
 * ordering.h
 *
 *      Author: Hao Xu
 */

#ifndef ORDERING_H_
#define ORDERING_H_

#include "linkedList.h"
#include "stdlib.h"
#include "term.h"
#include "utils.h"

int compareGroundTerms(Term *a, Term *b);

void merge(Term **at, bool *as, int an, Term **bt, bool *bs, int bn, Term **mt, bool *ms, int *mn);

void quicksort(Term **t, bool *s, int n);

void quicksort(LinkedList<Term *> *l);

template <typename T>
void randomizedArray(T* array, int n) {
	int index;
	T temp;
	for(int i=0;i<n;i++) {
		index = rand() % (n-i);
		SWAP1(array, i, index);
	}
}
#endif /* ORDERING_H_ */
