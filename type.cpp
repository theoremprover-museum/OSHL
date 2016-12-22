/*
 * type.cpp
 *
 *      Author: Hao Xu
 */

#include "type.h"
#include "symbols.h"
#include "utils.h"
#include "ordering.h"
#include "linkedStack.h"
#include "termCache.h"

Region *typeRegion = make_region(0);

Hashtable<Type *, sizeof(Symbol)> typeSymbolToType(TYPE_SYMBOL_TO_TYPE_MAP_CAPACITY, typeRegion);
EquivalenceRelation<Symbol> varTypeEquiv;
LinkedList<Symbol> varTypeReps; // this is generated only after types are generated, not immediately after equivalence classes are generated

void resetTypes() {
	typeSymbolToType.clear();
	varTypeEquiv.clear();
	varTypeReps.clear();
}
ReverseTerm::ReverseTerm(Symbol var, Term *term, Symbol typeVar) : var(var), term(term), typeVar(typeVar) {}

int ReverseTerm::toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset, const char *pre, const char *post) {
	SPRINT(buf, offset, "PROJ_");
	offset = Term::varToString(var, buf, symtable, offset, pre, post);
	SPRINT(buf, offset, " INV[");
	offset = term->toString(buf, symtable, offset);
	SPRINT(buf, offset, "](");
	offset = Term::varToString(typeVar, buf, symtable, offset, pre, post);
	SPRINT(buf, offset, ")");
	return offset;
}

void mapReverseTerm(Term *term, Symbol var, Term *s, Term *&resTerm, ReverseTerm *&resReverseTerm) {
	resTerm = NULL;
	resReverseTerm = NULL;
	Region *r = make_region(0);
	Sub *subInit = new (r) Sub();
	Sub *sub = unify(term, s, subInit, r, 0, VAR_VERSION_OFFSET);
	if(sub == NULL) { // not unifiable, return
		region_free(r);
		return;
	} else {
		Term *ret = (*sub)[var];
		// there are two possibilities
		// 1. var is still free under sub
		// 2. var is bound to a term
		if(ret == NULL || ret->symbol == var) { // free
			// we choose the first var in sub the term bound to which contains var
			while(sub != subInit) {
				Term *boundTerm = sub->val;
				Symbol boundVar = sub->key & MAX_VAR_ID_MASK;
				if(boundTerm->containsVar(var)) { // contains var
					resReverseTerm = new ReverseTerm(var, boundTerm, boundVar);
					region_free(r);
					return;
				}
				sub = sub->prev;
			}
			region_free(r);
			return;
		} else { // bound to a term
			// just return the term
			deleteAllButFirstNode(sub);
			resTerm = ret;
			region_free(r);

			return;
		}
	}
}

bool cmpReverseTerm(ReverseTerm *a, ReverseTerm *b) {
	Region *r = make_region(0);
	Sub *subInit = new (r) Sub();
	Sub* sub = unify(a->term, b->term, subInit, r);
	if(varTypeEquiv.equiv(a->typeVar, b->typeVar) && sub!=NULL && isRenaming(sub)) {
		Term *s = (*sub)[a->var];
		Symbol ss = s == NULL? a->var : s->symbol;
		Term *t = (*sub)[b->var];
		Symbol ts = t == NULL? b->var : t->symbol;
		region_free(r);
		return ss == ts;
	} else {
		region_free(r);
		return false;
	}
}


// two terms s and t are equivalent iff
// mgu(s, t) is a renaming, and it renames variables to their equivalent
bool cmpTerm(Term *a, Term *b) {
	Region *r = make_region(0);
	Sub *subInit = new (r) Sub();
	Sub *sub = unify(a, b, subInit, r);
	if(sub!=NULL && isRenaming(sub)) {
		for(;sub->prev!=NULL;sub = sub->prev) {
			if(! varTypeEquiv.equiv(sub->key, sub->val->symbol)) {
				region_free(r);
				return false;
			}
		}
		region_free(r);
		return true;
	} else {
		region_free(r);
		return false;
	}

}

bool cmpType(Type *a, Type *b) {
//	char buf1[STRING_BUF_SIZE], buf2[STRING_BUF_SIZE];
//	a->toString(buf1, symtable);
//	b->toString(buf2, symtable);
//	printf("compare %s and %s = ", buf1, buf2);
	int n1, n2;
	if((n1 = a->terms.size()) != b->terms.size() || (n2 = a->reverseTerms.size()) != b->reverseTerms.size()) {
//		printf("false 1\n");
		return false;
	}
	for(LinkedList<Term *>::Node * currA = a->terms.head, *currB = b->terms.head; currA != NULL; currA = currA->next, currB = currB->next) {
		if(!cmpTerm(currA->value, currB->value)) {
//			printf("false 2\n");
			return false;
		}
	}
	for(LinkedList<ReverseTerm *>::Node * currA = a->reverseTerms.head, *currB = b->reverseTerms.head; currA != NULL; currA = currA->next, currB = currB->next) {
		if(!cmpReverseTerm(currA->value, currB->value)) {
//			printf("false 3\n");
			return false;
		}

	}
//	printf("true\n");
	return true;

}

bool cmpPairSymbolSymbol(Pair<Symbol, Symbol> a, Pair<Symbol, Symbol> b) {
	return a.fst == b.fst && a.snd == b.snd;
}
bool typeSubsumesType(Type *a, Type *b, Type *excludeTermType, Term *excludeTerm, LinkedList<Pair<Symbol, Symbol> > *candidates) {
	for(LinkedList<Term *>::Node *curr1 = b->terms.head; curr1 != NULL; curr1 = curr1->next) { // iterate over all components of the union
		if(!typeSubsumesTerm(a, curr1->value, excludeTermType, excludeTerm, candidates)) {
			return false;
		}
	}
	return true;
}
bool typeSubsumesTerm(Type *a, Term *b, Type *excludeTermType, Term *excludeTerm, LinkedList<Pair<Symbol, Symbol> > *candidates) {
#ifdef DEBUG_TYPE
	char buf1[STRING_BUF_SIZE], buf2[STRING_BUF_SIZE], buf3[STRING_BUF_SIZE], buf4[STRING_BUF_SIZE];
	a->toString(buf1, symtable);
	b->toString(buf2, symtable);
	excludeTerm->toString(buf3, symtable);
	excludeTermType->toString(buf4, symtable);
#endif
	Region *r = make_region(0);
	Sub *subInit = new (r) Sub();
	LinkedList<Term *>::Node *curr1;
	int nCandidates = candidates->size();
	for(curr1 = a->terms.head; curr1 != NULL; curr1 = curr1->next) { // iterate over all components of the union
		if(a != excludeTermType || curr1->value != excludeTerm) {
			Sub *sub = unify(curr1->value, b, subInit, r, VAR_VERSION_OFFSET, 0);
			if(sub!=NULL && outOfDom(sub, b)) {
				// this does not always work but should cover most case
				// even if this does not work if the function returns to true, then it is true, i.e., there is no false positive
				// where this does not work: if mgu(p(X, Y), p(Z, W)) = [X->Z, W->Y], then neither term is out of dom, even though they are obviously instances of each other
				bool match = true;
				for(Sub *subCurr = sub; subCurr->prev != NULL; subCurr = subCurr->prev) {
					Symbol key = sub->key & MAX_VAR_ID_MASK;
					if(IS_VAR_SYMBOL(sub->val->symbol)) {
						// if target term is var, then check equivalence
						Symbol repA = varTypeEquiv.findEquivRep(key);
						Symbol repB = varTypeEquiv.findEquivRep(sub->val->symbol);
						Pair<Symbol, Symbol> candidate(repA, repB);
						if(!candidates->contains(candidate, cmpPairSymbolSymbol)) {
							candidates->append(candidate);
							if(!typeSubsumesType(typeSymbolToType[HASHTABLE_KEY(repA)], typeSymbolToType[HASHTABLE_KEY(repB)], excludeTermType, excludeTerm, candidates)) {
	//							printf("type %s subsumes term %s = ", buf1, buf2);
	//							printf("false 1\n");
								candidates->reduceTo(nCandidates);
								match = false;
								break;
							}
						}
					} else {
						// if target term is not a var, then check recursively
						// does this always work?
						Symbol rep = varTypeEquiv.findEquivRep(key);
						Type *t = typeSymbolToType[HASHTABLE_KEY(rep)];
						if(!typeSubsumesTerm(t, sub->val, excludeTermType, excludeTerm, candidates)) {
//							printf("type %s subsumes term %s = ", buf1, buf2);
//							printf("false 2\n");
							candidates->reduceTo(nCandidates);
							match = false;
							break;
						}
					}
				}
				if(match) {
#ifdef DEBUG_TYPE
					printf("type %s subsumes term %s exclude term %s in type %s = ", buf1, buf2, buf3, buf4);
					printf("true\n");
#endif
					region_free(r);
					return true;
				}
			}
		}
	}
#ifdef DEBUG_TYPE
	printf("type %s subsumes term %s exclude term %s in type %s = ", buf1, buf2, buf3, buf4);
	printf("false 3\n");
#endif
	region_free(r);
	return false;

}

// -1 no inst; 0 undecided; 1 inst
int Type::hasGroundInstance(Symbol rep, LinkedList<Symbol> *visitedInst, LinkedList<Symbol> *visitedUndecided, LinkedList<Symbol> *visitedNoInst) {
	if(visitedInst->contains(rep)) {
		return 1;
	} else if(visitedNoInst->contains(rep)) {
		return -1;
	} else if(visitedUndecided->contains(rep)) {
		return 0;
	}

	visitedUndecided->append(rep);

	int ret = -1;
	for(LinkedList<Term *>::Node *curr = terms.head; curr != NULL; curr = curr->next) {
		Term *t = curr->value;
		LinkedList<Symbol> *fvs = t->fv();
		int inst = 1;
		for(LinkedList<Symbol>::Node *currfv = fvs->head; currfv != NULL; currfv = currfv->next) {
			Symbol fvrep = varTypeEquiv.findEquivRep(currfv->value);
			if(visitedInst->contains(fvrep)) {
				continue;
			} else if(visitedNoInst->contains(fvrep)) {
				inst = -1;
				break;
			} else if(visitedUndecided->contains(fvrep)) {
				inst = 0;
			}

			Type *fvtype = typeSymbolToType.lookup(HASHTABLE_KEY(fvrep));
			int instcurrfv = fvtype->hasGroundInstance(fvrep, visitedInst, visitedUndecided, visitedNoInst);
			switch(instcurrfv) {
			case 1:
				visitedInst->append(fvrep);
				break;
			case 0:
				visitedUndecided->append(fvrep);
				inst = 0;
				break;
			case -1:
				visitedNoInst->append(fvrep);
				inst = -1;
				break;
			}
		}
		delete fvs;
		ret = max(ret, inst);
		if(ret == 1) {
			return ret;
		}
	}
	return ret;
}

bool Type::simplify() {
	// remove redundant types
	bool changed;
	bool simplified = false;
	do{
		changed = false;

		LinkedList<Term *>::Node *curr1;
		for(curr1 = terms.head; curr1 != NULL; curr1 = curr1->next) { // iterate over all terms
			Term *t1 = curr1->value;
			LinkedList<Pair<Symbol, Symbol> > candidates;
			if(typeSubsumesTerm(this, t1, this, t1, &candidates)) {
				terms.remove(curr1);
				changed = true;
				simplified = true;
				break;
			}
		}
	}while(changed);
	return simplified;
}
bool Type::unfold(Symbol typeSymbol) {
	LinkedList<Term *> newTerms;
	LinkedList<ReverseTerm *> newReverseTerms;
	LinkedList<ReverseTerm *>::Node *rtnode = reverseTerms.head;
	Term *newTerm;
	ReverseTerm *newReverseTerm;
	bool newEquiv = false;
	while(rtnode != NULL) {
		ReverseTerm *rt = rtnode->value;
		Symbol rep = varTypeEquiv.findEquivRep(rt->typeVar);
		Type *type = typeSymbolToType[HASHTABLE_KEY(rep)];
		// unfold rt->typeVar to type
		LinkedList<Term *>::Node *tnode = type->terms.head;
		while(tnode != NULL) { // iterate over all terms in type
			mapReverseTerm(rt->term, rt->var, tnode->value, newTerm, newReverseTerm);
			if(newTerm != NULL && !terms.contains(newTerm, cmpTerm)) {
				if(IS_VAR_SYMBOL(newTerm->symbol)) {
					if(!varTypeEquiv.equiv(typeSymbol, newTerm->symbol)) {
						// may be new equivalence, for example p(f(X)), p(f(Z)), ~p(Y)
						Symbol rep = varTypeEquiv.findEquivRep(newTerm->symbol);
						varTypeEquiv.setEquiv(rep, typeSymbol);
						Type *merge = typeSymbolToType.deleteKey(HASHTABLE_KEY(rep));
						varTypeReps.remove(rep);
						newTerms.appendAllNew(&merge->terms, cmpTerm);
						newReverseTerms.appendAllNew(&merge->reverseTerms, cmpReverseTerm);
						// delete merge; memory leak
						newEquiv = true;
					}
				} else {
					// if new term is not NULL,
					// not a repeat,
					// and not a var in the same equiv class (which should already been modeled by equivalence), then add it
					newTerms.appendNew(newTerm, cmpTerm);
				}
			}
			if(newReverseTerm != NULL && !reverseTerms.contains(newReverseTerm, cmpReverseTerm)) {  // if new reverse term is not NULL and not a repeat, add it
				newReverseTerms.appendNew(newReverseTerm, cmpReverseTerm);
			}
			tnode = tnode->next;
		}
		rtnode = rtnode->next;
	}
	terms.appendAllNew(&newTerms, cmpTerm);
	reverseTerms.appendAllNew(&newReverseTerms, cmpReverseTerm);
	return newEquiv || !newTerms.empty() || !newReverseTerms.empty();

}

bool Type::LL1() {
	LinkedList<Symbol> symbols;
	for(LinkedList<Term *>::Node *curr = terms.head; curr != NULL; curr = curr->next) {
		if(symbols.contains(curr->value->symbol)) {
			return false;
		} else {
			symbols.append(curr->value->symbol);
		}
	}
	return true;
}

int Type::toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset) {
	LinkedList<Term *>::Node *termNode;
	LinkedList<ReverseTerm *>::Node *reverseTermNode;
	for(termNode = terms.head; termNode != NULL; termNode = termNode->next) {
		if(termNode != terms.head) {
			SPRINT(buf, offset, "%s", " | ");
		}
		offset = termNode->value->toString(buf, symtable, offset);
	}
	if(terms.head != NULL && reverseTerms.head != NULL) {
		SPRINT(buf, offset, "%s", " | ");
	}
	for(reverseTermNode = reverseTerms.head; reverseTermNode != NULL; reverseTermNode = reverseTermNode->next) {
		if(reverseTermNode != reverseTerms.head) {
			SPRINT(buf, offset, "%s", " | ");
		}
		offset = reverseTermNode->value->toString(buf, symtable, offset);
	}
	return offset;
}

void addEmptyType(LinkedList<Symbol> *vars) {
	for(LinkedList<Symbol>::Node *curr = vars->head; curr!=NULL; curr=curr->next) {
		Symbol var = curr->value;
		Type *type = typeSymbolToType[HASHTABLE_KEY(var)];
		if(type == NULL) {
			//printf("add empty type to %lX\n", var);
			typeSymbolToType.insert(HASHTABLE_KEY(var), new Type());
		} else {
			//printf("do not all empty type to %lX\n", var);
		}
	}
}

void subToEquiv(EquivalenceRelation<Symbol> *equiv, Sub *sub) {
	while(sub->prev!=NULL)  {
		if(IS_VAR_SYMBOL(sub->val->symbol)) {
			equiv->setEquiv(sub->key, sub->val->symbol);
		}
		sub=sub->prev;
	}
}

void literalListToEquiv(EquivalenceRelation<Symbol> *equiv, LinkedList<Term *> *pos, LinkedList<Term *> *neg) {
	Region *r = make_region(0);
	LinkedList<Term *>::Node *posNode, *negNode;
	Sub *subInit = new (r) Sub();
	for(posNode = pos->head; posNode != NULL; posNode = posNode->next) {
		for(negNode = neg->head; negNode != NULL; negNode = negNode->next) {
			Sub *sub = unify(posNode->value, negNode->value, subInit, r);
			if(sub != NULL) {
				subToEquiv(equiv, sub);
				deleteAllButFirstNode(sub);
			}
		}
	}
	region_free(r);
}

void literalListToTypes(LinkedList<Term *> *pos, LinkedList<Term *> *neg, LinkedList<Symbol> *vars) {
	Region *r = make_region(0);
	LinkedList<Term *>::Node *posNode, *negNode;
	Sub *subInit = new (r) Sub();
	for(posNode = pos->head; posNode != NULL; posNode = posNode->next) { // iterate over all positive literals
		for(negNode = neg->head; negNode != NULL; negNode = negNode->next) { // iterate over all negative literals
			Sub *sub = unify(posNode->value, negNode->value, subInit, r, 0, VAR_VERSION_OFFSET);
			if(sub != NULL) { // if unifiable
				for(LinkedStack<Term *> *s = sub; s->prev!=NULL; s=s->prev) { // iterate over all variables in the mgu
					Term *term = s->val;
					if(!IS_VAR_SYMBOL(term->symbol)) { // not an equivalence
						Symbol var = s->key & MAX_VAR_ID_MASK;
						Type *type = typeSymbolToType[HASHTABLE_KEY(var)];
						if(type == NULL) {
							type = new Type();
							typeSymbolToType.insert(HASHTABLE_KEY(var), type);
						}
						type->terms.appendNew(term, cmpTerm);
						LinkedList<Symbol> *fvlist = term->fv();
						for(LinkedList<Symbol>::Node *fvnode = fvlist->head; fvnode != NULL; fvnode = fvnode -> next) {
							ReverseTerm *rt = new ReverseTerm(fvnode->value, term, var);
							type = typeSymbolToType[HASHTABLE_KEY(fvnode->value)];
							if(type == NULL) {
								type = new Type();
								typeSymbolToType.insert(HASHTABLE_KEY(fvnode->value), type);
							}
							type->reverseTerms.appendNew(rt, cmpReverseTerm);
						}
					}
				}
				deleteAllButFirstNode(sub);
			}
		}
	}
//	Symbol key = 0x800300, key2 = 0x80005B;
//	Type *type1 = typeSymbolToType[HASHTABLE_KEY(key)];
//	Type *type2 = typeSymbolToType[HASHTABLE_KEY(key2)];
	for(LinkedList<Symbol>::Node *curr = vars->head; curr != NULL; curr = curr->next) {
		Symbol key = curr->value;
		Symbol rep = varTypeEquiv.findEquivRep(key);
		varTypeReps.appendNew(rep);
	}
	addEmptyType(vars);
	mergeTypesUnderEquivalenceRelation(vars);
	region_free(r);
}

void mergeTypesUnderEquivalenceRelation(LinkedList<Symbol> *vars) {
	// printf("merge types under equivalence relation");
	// merge types under equivalence relation
//	for(LinkedList<Symbol>::Node *curr = vars->head; curr != NULL; curr = curr->next) {
//		Symbol key = curr->value;
//		char buf[STRING_BUF_SIZE];
//		Term::varToString(key, buf, symtable);
//		printf("%s %ld precheck\n", buf, key);
//		Type *currType = typeSymbolToType[HASHTABLE_KEY(key)];
//		currType->toString(buf, symtable);
//		printf("%s precheck type\n", buf);
//
//		// check type
//		for(LinkedList<Term *>::Node *curr2 = currType->terms.head; curr2 != NULL; curr2=curr2->next) {
//			if(curr2->value == NULL) {
//
//				printf("%s has null components, precheck\n", buf);
//				exit(-1);
//			}
//		}
//
//	}
//	printf("completed precheck\n");
//	Symbol key2 = 8388699;
//	Type *currType2 = typeSymbolToType[HASHTABLE_KEY(key2)];

	for(LinkedList<Symbol>::Node *curr = vars->head; curr != NULL; curr = curr->next) {
//		Symbol key1 = 0x800300;
//		Type *currType1 = typeSymbolToType[HASHTABLE_KEY(key1)];
		Symbol key = curr->value;
		Symbol rep = varTypeEquiv.findEquivRep(key);
//		if(currType1!=(void *)0x17a0eb0) {
//			printf("break here");
//		}
		if(rep != key) {

//			char buf[STRING_BUF_SIZE];
//			Term::varToString(key, buf, symtable);
//			char buf2[STRING_BUF_SIZE];
//			Term::varToString(rep, buf2, symtable);
//			printf("merge %s into %s\n", buf, buf2);
//
//			if(strcmp(buf, "?300") == 0) {
//				printf("break here");
//			}

			Type *repType = typeSymbolToType[HASHTABLE_KEY(rep)];
			Type *currType = typeSymbolToType.deleteKey(HASHTABLE_KEY(key));

//			currType->toString(buf2, symtable);
//			printf("%s type\n", buf2);
//
//			// check type
//			for(LinkedList<Term *>::Node *curr2 = currType->terms.head; curr2 != NULL; curr2=curr2->next) {
//				if(curr2->value == NULL) {
//					char buf[STRING_BUF_SIZE];
//					Term::varToString(key, buf, symtable);
//					printf("%s has null components\n", buf);
//					exit(-1);
//				}
//			}
//
//			currType2->toString(buf2, symtable);
//			printf("?5b %s type\n", buf2);
//
//			// check type
//			for(LinkedList<Term *>::Node *curr2 = currType2->terms.head; curr2 != NULL; curr2=curr2->next) {
//				if(curr2->value == NULL) {
//					char buf[STRING_BUF_SIZE];
//					Term::varToString(key, buf, symtable);
//					printf("%s has null components\n", buf);
//					exit(-1);
//				}
//			}


			if(repType == NULL) {
				repType = new Type();
				typeSymbolToType.insert(HASHTABLE_KEY(rep), repType);
			}
			if(currType != NULL) {
				repType->terms.appendAllNew(&currType->terms, cmpTerm);
				repType->reverseTerms.appendAllNew(&currType->reverseTerms, cmpReverseTerm);
			}
			delete currType;
		}
		// varTypeReps.appendNew(rep);
		// rep may not be in keys, for example, if we have p(X), ~p(Y), p(f(Z)), then the hashtable will be
		// Y->f(Z)
		// Z->proj_Z inv[f(Z)](Y)
//		char buf[STRING_BUF_SIZE];
//		Term::varToString(key, buf, symtable);
//		printf("processed key %s\n", buf);
	}
	// delete keys;
	// sort union
	for(LinkedList<Symbol>::Node *curr = varTypeReps.head; curr != NULL; curr = curr->next) {
		Type *type = typeSymbolToType[HASHTABLE_KEY(curr->value)];
		quicksort(&type->terms);
	}

}

bool simplifyTypes() {
	bool changed = false;
	LinkedList<Symbol>::Node *curr;
	for(curr = varTypeReps.head; curr != NULL; curr = curr->next) { // iterate over all reps

		Type *t = typeSymbolToType[HASHTABLE_KEY(curr->value)];
		if(t!=NULL) {
			changed |= t->simplify();
		}
	}
	return changed;
}
void setEquivAndDeleteType(Symbol s, Symbol t) {
#ifdef DEBUG_TYPE
	char buf1[STRING_BUF_SIZE], buf2[STRING_BUF_SIZE];
	Term::varToString(s, buf1, symtable);
	Term::varToString(t, buf2, symtable);
	printf("set term equivalence %s == %s\n", buf1, buf2);
#endif

	s = varTypeEquiv.findEquivRep(s);
	t = varTypeEquiv.findEquivRep(t);
	if(s!=t) {
		varTypeEquiv.setEquiv(s, t);
		delete typeSymbolToType.deleteKey(HASHTABLE_KEY(s));
		varTypeReps.remove(s);
	}

}
bool deleteRepeatedTypes() {
	// remove repeated types
	LinkedList<Symbol>::Node *curr1, *curr2;
	LinkedList<Symbol> toRemove;
	LinkedList<Symbol>::Node *curr3;
	for(curr1 = varTypeReps.head; curr1 != NULL; curr1 = curr1->next) { // iterate over all reps
		Type *t1 = typeSymbolToType[HASHTABLE_KEY(curr1->value)];
		for(curr2 = curr1->next; curr2 != NULL; curr2 = curr2->next) { // iterate over all reps to the right of curr1
			Type *t2 = typeSymbolToType[HASHTABLE_KEY(curr2->value)];
			if(cmpType(t1, t2)) {
				Symbol rep1 = varTypeEquiv.findEquivRep(curr1->value);
				Symbol rep2 = varTypeEquiv.findEquivRep(curr2->value);
				typeSymbolToType.deleteKey(HASHTABLE_KEY(rep1)); // delete key from hashtable
				varTypeEquiv.setEquiv(rep1, rep2); // set rep1 -> rep2
				toRemove.append(rep1);
				break;
			}
		}
	}
	// delete all nodes from toRemove
	for(curr3 = toRemove.head; curr3 != NULL; curr3 = curr3->next) {
		varTypeReps.remove(curr3->value);
	}
	return !toRemove.empty();
}
void deleteReversedTermsFromTypes() {
#ifdef DEBUG_TYPE
	char buf[STRING_BUF_SIZE];
	printf("deleting reverse terms from types:\n");
#endif
	// remove repeated types
	LinkedList<Symbol>::Node *curr1;
	for(curr1 = varTypeReps.head; curr1 != NULL; curr1 = curr1->next) { // iterate over all reps
		Type *t1 = typeSymbolToType[HASHTABLE_KEY(curr1->value)];
		if(t1!=NULL) {
			t1->reverseTerms.clear();
		}
#ifdef DEBUG_TYPE
		Term::varToString(curr1->value, buf, symtable);
		printf("processed rep %s\n", buf);
#endif
	}
}

bool deleteEquivalentTypes() {
	bool changed;
	bool simplified = false;
#ifdef DEBUG_TYPE
	int it = 0;
#endif
	do {
		restart:
#ifdef DEBUG_TYPE
		printf("iteration %d, number of reps = %d\n", it++, varTypeReps.size());
#endif
		changed = false;

		for(LinkedList<Symbol>::Node *curr1 = varTypeReps.head; curr1 != NULL; curr1 = curr1->next) {
			for(LinkedList<Symbol>::Node *curr2 = curr1->next; curr2 != NULL; curr2 = curr2->next) {
				LinkedList<Pair<Symbol, Symbol> > candidatesObj;
				if(typeEquivalenceByCoinduction(curr1->value, curr2->value, &candidatesObj)) {
#ifdef DEBUG_TYPE
					printf("type equivalence by coinduction, %d candidates found\n", candidatesObj.size());
#endif
					if(candidatesObj.size() != 0) {
					for(LinkedList<Pair<Symbol, Symbol> >::Node *curr3 = candidatesObj.head; curr3!=NULL; curr3 = curr3->next) {
						Symbol s = curr3->value.fst;
						Symbol t = curr3->value.snd;
						setEquivAndDeleteType(s, t);
					}
					simplified = true;
					changed = true;
					goto restart;
					}
				}
			}

		}
	} while(changed);
	return simplified;
}
bool termEquivalenceByCoinduction(Term *a, Term *b, LinkedList<Pair<Symbol, Symbol> > *candidates) {
#ifdef DEBUG_TYPE
	char buf1[STRING_BUF_SIZE], buf2[STRING_BUF_SIZE];
	a->toString(buf1, symtable);
	b->toString(buf2, symtable);
	printf("add term equivalence candidate %s == %s\n", buf1, buf2);
#endif
	Region *r = make_region(0);
	Sub* subInit = new (r) Sub();
	Sub* sub = unify(a, b, subInit, r);
	int n = candidates->size();
	if(sub!=NULL && isRenaming(sub)) {
		for(;sub->prev!=NULL;sub = sub->prev) {
			Symbol s = varTypeEquiv.findEquivRep(sub->key);
			Symbol t = varTypeEquiv.findEquivRep(sub->val->symbol);
			if(s != t && ! varTypeEquiv.equiv(s, t)) {
				Pair<Symbol, Symbol> pair(s, t);
				if(!candidates->contains(pair, cmpPairSymbolSymbol)) { // if already in candidates, do not compare
					candidates->append(pair);
#ifdef DEBUG_TYPE
					Term::varToString(s, buf1, symtable);
					Term::varToString(t, buf2, symtable);
					printf("add coinduction candidate %s == %s\n", buf1, buf2);
#endif

					if(!typeEquivalenceByCoinduction(s, t, candidates)) {
						candidates->reduceTo(n);
						region_free(r);
						return false;
					}
				}
			}
		}
		region_free(r);
		return true;
	} else {
		region_free(r);
		return false;
	}
}

bool termListEquivalenceByCoinduction(LinkedList<Term *> *a, LinkedList<Term *> *b, LinkedList<Term *> *aVisited, LinkedList<Term *> *bVisited, LinkedList<Pair<Symbol, Symbol> > *candidates) {
	/*char buf[STRING_BUF_SIZE];
	printf("equiv? ");
	for(LinkedList<Term *>::Node *currb = a->head; currb!=NULL; currb=currb->next) {
		currb->value->toString(buf, symtable);
		printf("%s ", buf);
	}
	printf(" and ");
	for(LinkedList<Term *>::Node *currb = b->head; currb!=NULL; currb=currb->next) {
		currb->value->toString(buf, symtable);
		printf("%s ", buf);
	}
	printf(" visited ");
	if(bVisited == NULL) {
		printf("all ");
	} else {
		for(LinkedList<Term *>::Node *currb = bVisited->head; currb!=NULL; currb=currb->next) {
			currb->value->toString(buf, symtable);
			printf("%s ", buf);
		}
	}
	printf("\n");*/
	if(a->empty()) {
		if(bVisited == NULL) {
			return true;
		}
		LinkedList<Term *> *bPrime = b->duplicate();
		for(LinkedList<Term *>::Node *currb = bVisited->head; currb!=NULL; currb=currb->next) {
			bPrime->remove(currb->value);
		}
		return termListEquivalenceByCoinduction(bPrime, aVisited, NULL, NULL, candidates);
	}
	int nCandidates = candidates->size();
	LinkedList<Term *>::Node *curra = a->tail;
	for(LinkedList<Term *>::Node *currb = b->head; currb!=NULL; currb=currb->next) {
		if(termEquivalenceByCoinduction(curra->value, currb->value, candidates)) {
			LinkedList<Term *> *aPrime = a->duplicate();
			aPrime->removeLast();
			bool removeFromBVisit = false;
			if(bVisited != NULL) {
				if(!bVisited->contains(currb->value)) {
					removeFromBVisit = true;
					bVisited->appendNew(currb->value);
				}
				aVisited->append(curra->value);
			}
			if(termListEquivalenceByCoinduction(aPrime, b, aVisited, bVisited, candidates)) {
				return true;
			}
			if(bVisited != NULL) {
				if(removeFromBVisit)
					bVisited->removeLast();
				aVisited->removeLast();
			}
			candidates->reduceTo(nCandidates);

		}
	}
	return false;
}
bool typeEquivalenceByCoinduction(Symbol a, Symbol b, LinkedList<Pair<Symbol, Symbol> > *candidates) {
	// e.g. PUZ008-1, PUZ008-3
	Symbol repa = varTypeEquiv.findEquivRep(a);
	Symbol repb = varTypeEquiv.findEquivRep(b);
	Type *ta = typeSymbolToType[HASHTABLE_KEY(repa)];
	Type *tb = typeSymbolToType[HASHTABLE_KEY(repb)];
	LinkedList<Term *> bVisited, aVisited;
	return termListEquivalenceByCoinduction(&ta->terms, &tb->terms, &bVisited, &aVisited, candidates);
}

void unfoldTypes() {
#ifdef DEBUG_TYPE
	char buf[STRING_BUF_SIZE];
#endif
	bool changed;
	do {
		LinkedList<const char *> *allTypeSymbols = typeSymbolToType.keys();
		changed = false;
		for(LinkedList<const char *>::Node *curr = allTypeSymbols->head; curr != NULL; curr = curr->next) {
			const Symbol typeSymbol = *reinterpret_cast<const Symbol *>(curr->value);
			Type *type = typeSymbolToType[HASHTABLE_KEY(typeSymbol)];
			if(type != NULL) { // unfold may have merged and deleted types
				changed = type->unfold(typeSymbol);
				if(changed) {
					// some keys in the allTypeSymbols list may have deleted, need to restart
					break;
				}
			}
		}


		for(LinkedList<Symbol>::Node *curr = varTypeReps.head; curr != NULL; curr = curr->next) {
			Type *type = typeSymbolToType[HASHTABLE_KEY(curr->value)];
			quicksort(&type->terms);
		}
#ifdef DEBUG_TYPE
		buf[0] = '\0';
		hashtableToString(&typeSymbolToType, buf, symtable);
		printf("unfold:\n%s\n", buf);
#endif
	} while(changed);
	deleteReversedTermsFromTypes();
	bool simplified;
	do {
		simplified = false;
		simplified |= simplifyTypes();
#ifdef DEBUG_TYPE
		buf[0] = '\0';
		hashtableToString(&typeSymbolToType, buf, symtable);
		printf("simplify types:\n%s\n", buf);
#endif
		simplified |= deleteRepeatedTypes();
#ifdef DEBUG_TYPE
		buf[0] = '\0';
		hashtableToString(&typeSymbolToType, buf, symtable);
		printf("delete repeated types:\n%s\n", buf);
#endif
		simplified |= deleteEquivalentTypes();
#ifdef DEBUG_TYPE
		buf[0] = '\0';
		hashtableToString(&typeSymbolToType, buf, symtable);
		printf("delete equivalent types:\n%s\n", buf);
#endif
	} while(simplified);
}

void addConstToEmptyTypeAndReverseTermsInType() {
	// add empty types, e.g. PUZ012-1
	Symbol f = getNullaryFunctionSymbol();
	// general a constant if there is no constants
	if(f == 0) { // there is no nullary function symbol
		f = updateSymTable("%%", 0, funcid, true);
	}
	// add to all types with empty instance sets, which include empty types and types with only nonground terms in which all variables have empty instance sets
	for(LinkedList<Symbol>::Node *curr = varTypeReps.head; curr!=NULL; curr=curr->next) {
		Symbol rep = curr->value;
		Type *type = typeSymbolToType[HASHTABLE_KEY(rep)];
		LinkedList<Symbol> visitedInst;
		LinkedList<Symbol> visitedNoInst;
		LinkedList<Symbol> visitedUndecided;
		if(type->hasGroundInstance(rep, &visitedInst, &visitedUndecided, &visitedNoInst) != 1) {
			type->terms.append(getFromTermMatrix(f));
			quicksort(&type->terms);
		}
	}
	for(LinkedList<Symbol>::Node *curr = varTypeReps.head; curr!=NULL; curr=curr->next) {
		Type *type = typeSymbolToType[HASHTABLE_KEY(curr->value)];
		type->terms.reverse();
	}

}


bool checkAllTypesLL1() {
	bool all = true;
	char buf[STRING_BUF_SIZE];
	for(LinkedList<Symbol>::Node *curr = varTypeReps.head; curr != NULL; curr = curr->next) {
		Type *t = typeSymbolToType[HASHTABLE_KEY(curr->value)];
		t->toString(buf, symtable);
		bool b = t->LL1();
		if(!b) {
			printf("%s is not LL1\n", buf);
		}
		all &= b;
	}
	return all;
}

void compileAllTypes(REGION) {
	// bool all = true;
	// char buf[STRING_BUF_SIZE];
	for(LinkedList<Symbol>::Node *curr = varTypeReps.head; curr != NULL; curr = curr->next) {
		Type *t = typeSymbolToType[HASHTABLE_KEY(curr->value)];
		for(LinkedList<Term *>::Node *termsCurr = t->terms.head; termsCurr != NULL; termsCurr = termsCurr->next) {
			termsCurr->value->compile(CURR_REGION);
		}
	}
}
