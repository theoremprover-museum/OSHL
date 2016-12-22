/*
 * ordering.cpp
 *
 *      Author: Hao Xu
 */

#include "term.h"
#include "ordering.h"

int compareGroundTerms(Term *a, Term *b) {
	if(a==b) {
		return 0;
	}
	int cmp = a->termSize() - b->termSize();
	if(cmp != 0) {
		return cmp;
	}
	if(a->symbol > b->symbol) {
		return 1;
	} else if(a->symbol < b->symbol) {
		return -1;
	} else {
		for(int i = 0;i<a->symbolDimension;i++) {
			cmp = compareGroundTerms(a->subterms[i], b->subterms[i]);
			if(cmp!=0) {
				return cmp;
			}
		}
		return 0;
	}
}

void merge(Term **at, bool *as, int an, Term **bt, bool *bs, int bn, Term **mt, bool *ms, int *mn) {
#define PUSHA \
	mt[k] = at[i]; \
	ms[k++] = as[i++];
#define PUSHB \
	mt[k] = bt[j]; \
	ms[k++] = bs[j++];

	int i = 0;
	int j = 0;
	int k = 0;
	while(i < an && j < bn) {
		int cmp = compareGroundTerms(at[i], bt[j]);
		if(cmp > 0) {
			PUSHA;
		} else if(cmp < 0) {
			PUSHB;
		} else { // cmp == 0
			// as[i] == bs[j] must hold in the eager algorithm
			if(as[i] == bs[j]) {
				PUSHA;
				j++; // skip b
			} else { // choose negative literal
				if(as[i] == false) {
					PUSHA;
				} else {
					PUSHB;
				}
			}
		}
	}
	if(i < an) {
		while(i < an) {
			PUSHA;
		}
	} else { // j < bn
		while(j < bn) {
			PUSHB;
		}
	}

	*mn = k;
}

void quicksort(Term **t, bool *s, int n) {
	if(n <= 1) {
		return;
	}
	// pivotIndex = 0;
	Term *tempt;
	bool temps;

	int a = 1, b = n-1;
	// pivot [>] [<=]
	//         b a
	while(a <= b) {
		if(compareGroundTerms(t[a], t[0]) > 0) {
			a++;
		} else {
			SWAP2(a, b);
			b--;
		}
	}
	if(a == n) { // all > pivot
		SWAP2(0, b);
		quicksort(t, s, n-1);
	} else if (b == 0) { // all <= pivot
		quicksort(t+1, s+1, n-1);
	} else {
		SWAP2(0, b);
		quicksort(t, s, a-1);
		quicksort(t+a, s+a, n-a);
	}
}

void quicksort(LinkedList<Term *> *l) {
	int n = l->size();
	Term **terms = new Term*[n];
	bool *signs = new bool[n]; // dummy
	LinkedList<Term *>::Node *curr = l->head;
	for(int i =0;i<n;i++) {
		terms[i] = curr->value;
		signs[i] = false;
		curr = curr->next;
	}
	quicksort(terms, signs, n);
	curr = l->head;
	for(int i =0;i<n;i++) {
		curr->value = terms[i];
		curr = curr->next;
	}
	delete[] terms;
	delete[] signs;
}

