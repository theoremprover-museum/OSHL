/*
 * type.h
 *
 *      Author: Hao Xu
 */

#ifndef TYPE_H_
#define TYPE_H_

#include "linkedList.h"
#include "hashtable.h"
#include "term.h"
#include "symbols.h"

#define TYPE_SYMBOL_TO_TYPE_MAP_CAPACITY 1000
#define EQUIV_REL_CAPACITY 1000
/*
 * A term function is a function that maps vectors of ground terms to ground terms.
 * Given a term t with free variables X1, X2, ..., Xn (ordered by their occurrence in t), the corresponding term function f_t is defined as follows:
 * f_t [t1, t2, ..., tn] = t [X1->t1, X2->t2, ..., Xn->tn].
 *
 * It can be proved that f_t is an injection.
 *
 * A reverse term function is a partial function that maps ground terms to vectors of ground terms.
 * Given a term t with free variables X1, X2, ..., Xn (ordered by their occurrence in t), the corresponding reverse term function g_t is defined as follows:
 * g_t s = [s1, s2, ..., sn] s.t. t [X1->s1, X2->s2, ..., Xn->sn] = s
 *
 * A reverse term is a projection composed with a reverse term function.
 */
struct ReverseTerm {
	Symbol var; // the var to be projected in the projection
	Term *term; // the term t
	Symbol typeVar; // the type var

	ReverseTerm(Symbol var, Term *term, Symbol typeVar);

	int toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0, const char *pre= "?", const char *post = "");

};

void mapReverseTerm(Term *term, Symbol var, Term *s, Term *&resTerm, ReverseTerm *&resReverseTerm);

struct Type;

// each type is denoted by a unique variable, this hashtable maps variable symbols to types.
extern Hashtable<Type *, sizeof(Symbol)> typeSymbolToType;

struct Type {
	LinkedList<Term *> terms;
	LinkedList<ReverseTerm *> reverseTerms;

	// this function unfolds the definitions of types into the reverse terms once
	bool unfold(Symbol rep);
	bool simplify();
	bool LL1();
	int hasGroundInstance(Symbol rep, LinkedList<Symbol> *visitedInst, LinkedList<Symbol> *visitedUndecided, LinkedList<Symbol> *visitedNoInst);
	int toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0);
};

template<typename T>
struct EquivalenceRelation {
	Region *region;

	Hashtable<T, sizeof(T)> equivMap;

	LinkedList<T> elems;

	EquivalenceRelation() : region(make_region(0)), equivMap(EQUIV_REL_CAPACITY, region) {}

	void setEquiv(T a, T b) {
		elems.appendNew(a);
		elems.appendNew(b);
		T ar = findEquivRep(a);
		T br = findEquivRep(b);
		if(ar != br) {
			equivMap.insert(reinterpret_cast<char *>(&ar), br);
		}
	}

	T findEquivRep(T a) {
		char *key = reinterpret_cast<char *>(&a);
		T tmp2 = equivMap[key];
		T tmp;
		if(tmp2 != (T) 0 && tmp2 != a) {
			tmp = findEquivRep(tmp2);
			if(tmp!=tmp2) {
				equivMap.update(key, tmp2);
			}
			return tmp;
		} else {
			return a;
		}
	}

	bool equiv(T a, T b) {
		return findEquivRep(a) == findEquivRep(b);

	}

	int toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0, const char *pre = "?", const char *post = "") {
		Region *r = make_region(0);
		Hashtable<LinkedList<Symbol> *, sizeof(Symbol)> *classes = new Hashtable<LinkedList<Symbol> *, sizeof(Symbol)>(100, r);
		LinkedList<Symbol> reps;
		for(LinkedList<Symbol>::Node *curr = elems.head; curr!=NULL; curr = curr->next) {
			Symbol rep = findEquivRep(curr->value);
			LinkedList<Symbol> *cls;
			if((cls = (*classes)[HASHTABLE_KEY(rep)]) == NULL) {
				cls = new LinkedList<Symbol>();
				classes->insert(HASHTABLE_KEY(rep), cls);
			}
			cls->append(curr->value);
			if(rep == curr->value) {
				reps.append(curr->value);
			}
		}
		for(LinkedList<Symbol>::Node *curr = reps.head; curr!=NULL; curr = curr->next) {
			SPRINT(buf, offset, "class ");
			offset = Term::varToString(curr->value, buf, symtable, offset, pre, post);
			SPRINT(buf, offset, ": ");
			LinkedList<Symbol> *cls = (*classes)[HASHTABLE_KEY(curr->value)];
			for(LinkedList<Symbol>::Node *curr2 = cls->head; curr2!=NULL; curr2 = curr2->next) {
				offset = Term::varToString(curr2->value, buf, symtable, offset, pre, post);
				SPRINT(buf, offset, " ");
			}
			delete cls;
			SPRINT(buf, offset, "\n");
		}
		delete classes;
		region_free(r);
		return offset;
	}

	void clear() {
		equivMap.clear();
		elems.clear();
	}


};

extern EquivalenceRelation<Symbol> varTypeEquiv;

template<typename T, int tKeySize>
int hashtableToString(Hashtable<T, tKeySize> *ht, char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0) {
	for(int i = 0;i<ht->capacity; i++) {
		for(Bucket<Type *> *b = ht->buckets[i]; b!=NULL; b=b->next) {
			Symbol symbol = *(Symbol *)b->key;
//			if(varTypeEquiv.findEquivRep(symbol) != symbol) {
//				continue;
//			}
			offset = Term::varToString(symbol, buf, symtable, offset);
			SPRINT(buf, offset, " : ");
			offset = b->value->toString(buf, symtable, offset);
			SPRINT(buf, offset, "\n");
		}
	}
	return offset;

}

void subToEquiv(EquivalenceRelation<Symbol> *equiv, Sub *sub);
void literalListToEquiv(EquivalenceRelation<Symbol> *equiv, LinkedList<Term *> *pos, LinkedList<Term *> *neg);
void literalListToTypes(LinkedList<Term *> *pos, LinkedList<Term *> *neg, LinkedList<Symbol> *vars);
void resetTypes();
void unfoldTypes();
bool typeEquivalenceByCoinduction(Symbol a, Symbol b, LinkedList<Pair<Symbol, Symbol> > *candidates);
bool termEquivalenceByCoinduction(Term *a, Term *b, LinkedList<Pair<Symbol, Symbol> > *candidates);
bool termListEquivalenceByCoinduction(LinkedList<Term *> *a, LinkedList<Term *> *b, LinkedList<Term *> *aVisited, LinkedList<Term *> *bVisited, LinkedList<Pair<Symbol, Symbol> > *candidates);
void addConstToEmptyTypeAndReverseTermsInType();
bool checkAllTypesLL1();
void compileAllTypes(REGION);
bool typeSubsumesTerm(Type *a, Term *b, Type *excludeTermType, Term *excludeTerm, LinkedList<Pair<Symbol, Symbol> > *candidates);
bool typeSubsumesType(Type *a, Type *b, Type *excludeTermType, Term *excludeTerm, LinkedList<Pair<Symbol, Symbol> > *candidates);
void mergeTypesUnderEquivalenceRelation(LinkedList<Symbol> *candidates);

#endif /* TYPE_H_ */
