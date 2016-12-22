/*
 * model.cpp
 *
 *      Author: Hao Xu
 */
//#define DEBUG_TRIE
//#define DEBUG_MODEL

#include "clause.h"
#include "hashtable.h"
#include "acyclicGraph.h"
#include "trie.h"
#include "hybridStack.h"
#include "term.h"
#include "ordering.h"
#include "symbols.h"
// Assume that the trivial model is all negative.
// The model data structure consists of a trie of all eligible literals.
#ifdef USE_RELEVANCE
Trie *modelTrieRoot[MAX_RELEVANCE+1];
#else
Trie modelTrieRoot;
#endif
Region *modelTrieRegion = make_region(0);
// and an acyclic graph of clause instances that generates the eligible literals.
AcyclicGraph<Clause *> modelGraphStartVertex(NULL); // all positive clauses should have an outgoing edge to this vertex, so that the graph is always connected
// The directed edges in the graph denotes relevance;
// if there is an edge going from instance X to instance Y,
// it means that instance X is generated from instance Y.
//
// When a new instance is added to the graph, some instances will be deleted.
// There are two strategies, eager and lazy.
//
// The eager strategy never discards the new instance
// and deletes existing instances according to the following criteria:
// 1. If an existing instance X contains the new eligible literal L then X will be deleted.
// 2. If there is an edge from an instance X to a deleted instance, then X will be deleted.
// The first criteria is needed to prevent the following nontermination:
// Suppose that there is one instance {p, q}
// with eligible literal q
// If the newly generated instance is {p}, and we don't delete {p, q}, then the instances will be
// {p, q}, {p}
// If we can generate another instance {~q}, then we will have to unify it with {p, q}
// which results in adding {p} again, where the algorithm enters an infinite loop.
//
// The lazy strategy only retains criteria 2, and change criteria one to
// 1'. If the newly added instance generates an eligible literal that is already in the model,
//     then the newly added instance will be discarded and all instances with the eligible literal
//     which it depends on will be deleted.
//
// To implement the eager strategy efficiently, we need to find a way to efficiently look up all instances
// that contains a given literal.
// This can be done by a global map from literals to a linked list of clauses.
// As all literals comes from the term matrix, we can use the pointer to the literal as the key.
// Hashtable<LinkedList<Clause *> *, sizeof(Term *)> globalPosLiteralToClauseInstanceMap(GLOBAL_POS_LITERAL_TO_CLAUSE_INSTANCE_MAP_CAPACITY);
// Term::instances

// We also need a way to efficiently remove a clause instance from from the map.
// Hashtable<LinkedList<typename LinkedList<Clause *>::Node *> *, sizeof(Clause *)> globalClauseInstanceToListOfLinkedListNodeMap(GLOBAL_CLAUSE_INSTANCE_TO_LIST_OF_LINKED_LIST_NODE_MAP_CAPACITY);
// Clause::listOfNodes

// We also need a way to efficiently go from a literal to the vertex that contains the clause that generated it
// Term::vertex


// Inserting a clause instance into the maps
// precond:
// (a) inst is nonempty and sorted in the descending order,
// (b) the max literal is positive, and
// (c) there is no repeated literals
void insertClauseInstanceIntoModel(Clause *inst) {
	// allocate memory
	LinkedList<LinkedList<Clause *>::Node *> *listOfNodes = new LinkedList<LinkedList<Clause *>::Node *>();
	AcyclicGraph<Clause *> *gNode = new AcyclicGraph<Clause *>(inst);

	// set vertex and listOfNodes
	inst->literals[0]->vertex = gNode;
	inst->listOfNodes = listOfNodes;

	// linked with other nodes
	bool positiveClause = true;
	for(int i=0;i<inst->numberOfLiterals;i++) {
		if(inst->signs[i]) { // positive literals
			if(inst->literals[i]->instances == NULL) {
				inst->literals[i]->instances = new LinkedList<Clause *>();
			}
			LinkedList<Clause *>::Node *n = inst->literals[i]->instances->append(inst);
			listOfNodes->append(n);
		} else { // negative literals
			AcyclicGraph<Clause *> *hNode = inst->literals[i]->vertex;
			// add test case for linking / delete by linking
#ifdef DEBUG_MODEL
			char buf1[STRING_BUF_SIZE], buf2[STRING_BUF_SIZE];
			inst->literals[i]->toString(buf1, symtable);
			printf("link literal %s\n", buf1);
			fflush(stdout);

			inst->toString(buf1, symtable);
			hNode->value->toString(buf2, symtable);
			printf("link %s to %s\n", buf1, buf2);
#endif
			gNode->addOutgoingEdgeTo(hNode);
			positiveClause = false;
		}
	}
	if(positiveClause) {
		gNode->addOutgoingEdgeTo(&modelGraphStartVertex);
	}

	// insert into trie
#ifdef USE_RELEVANCE
	if(modelTrieRoot[inst->relevance] == NULL) {
		modelTrieRoot[inst->relevance] = new (modelTrieRegion) Trie();
	}
	modelTrieRoot[inst->relevance]->insert(inst->literals[0], modelTrieRegion);
	modelTrieRoot[0]->insert(inst->literals[0], modelTrieRegion);
#else
	modelTrieRoot.insert(inst->literals[0], modelTrieRegion);
#endif
#ifdef DEBUG_TRIE
	char buf3[STRING_BUF_SIZE];
	modelTrieRoot.toString(buf3, symtable);
	printf("trie:\n%s", buf3);
	printf("\n");
	fflush(stdout);
#endif

}

void __checkTrie(Trie *node, bool mustBeDel) {
	if(mustBeDel && !node->del) {
		printf("trie inconsistent, undeleted node under del node\n");
		exit(-1);
	}
	if(node->funcChild != NULL) {
		// internal node
		for(Trie *curr = node->funcChild; curr!= NULL; curr=curr->sibling) {
			__checkTrie(curr, mustBeDel | node->del);
		}
	} else {
		if(!node->del) {
		char buf[STRING_BUF_SIZE];
		Term *elit = node->subtermPtrs->subterm;
		if(elit->vertex == NULL) {
			elit->toString(buf, symtable);
			printf("checkTrie: %s\n", buf);
			printf("trie inconsistent, vertex null\n");
			exit(-1);
		}
		}
	}
}
// check that the trie for integrity
void checkTrie() {
#ifndef USE_RELEVANCE
	__checkTrie(&modelTrieRoot, false);
#endif
}
// Deleting a clause instance from the maps
void deleteClauseInstanceFromModel(Clause *inst) {
#ifdef DEBUG_MODEL
	char buf[1024];
	inst->toString(buf, symtable, 0);
	printf("deleting clause %s from model\n", buf);
	// modelTrieRoot.toString(buf, symtable);
	// printf("model trie: \n%s\n", buf);
	// modelTrieRoot.remove(inst->literals[0]);
#endif

	LinkedList<LinkedList<Clause *>::Node *> *list = inst->listOfNodes;
	if(list != NULL) {
		LinkedList<LinkedList<Clause *>::Node *>::Node *curr = list->head;
		while(curr!=NULL) {
			curr->value->remove();
			curr = curr->next;
		}
	}
	inst->literals[0]->vertex->remove();

	delete list;
	delete inst->literals[0]->vertex;

	// reset listOfNodes and vertex
	inst->listOfNodes = NULL;
	inst->literals[0]->vertex = NULL;
}


// return the minimum of the size of literals that gets inserted or deleted
int eagerCascadingDeleteClauseFromModel(Clause *inst, Term *elilit) {
	HybridStack<Clause *> toDelete;
	HybridStack<AcyclicGraph<Clause *> *> toVisit;
#ifdef DEBUG_MODEL
	char buf[STRING_BUF_SIZE];
#endif
	// 1. If an existing instance X contains the new eligible literal L then X will be deleted.
	LinkedList<Clause *>::Node *curr = elilit->instances->head;
	while(curr!=NULL) {
		// do not delete in the input clause
		if(curr->value != inst) {
#ifdef DEBUG_MODEL
			curr->value->toString(buf, symtable);
			printf("delete %s because it contains the newly added eligible literal\n", buf);
#endif
			toVisit.push(curr->value->literals[0]->vertex);
#ifdef DEBUG_MODEL
				printf("to visit %s\n", buf);
#endif
		}
		curr = curr->next;
	}
	//int counter = 0;
	// 2. If there is an edge from an instance X to a deleted instance, then X will be deleted.
	while(!toVisit.isEmpty()) {
		AcyclicGraph<Clause *> *v1 = toVisit.pop();
#ifdef DEBUG_MODEL
				v1->value->toString(buf, symtable);
				printf("visiting %s\n", buf);
#endif
		if(!v1->del) {
			v1->del = true;
			LinkedList<Pair<AcyclicGraph<Clause *> *, void *> *>::Node *inNode = v1->in.head;
			while(inNode != NULL) {
#ifdef DEBUG_MODEL
				inNode->value->fst->value->toString(buf, symtable);
				printf("delete %s because it depends on a clause that is deleted\n", buf);
#endif
				toVisit.push(inNode->value->fst);
#ifdef DEBUG_MODEL
				printf("to visit %s\n", buf);
#endif
				inNode = inNode->next;
			}
#ifdef DEBUG_MODEL
			v1->value->toString(buf, symtable);
			counter ++;
				printf("to delete %s\n", buf);
#endif
			toDelete.push(v1->value);
		} else {
#ifdef DEBUG_MODEL
			v1->value->toString(buf, symtable);
			printf("do not delete %s again, because it has already been deleted\n", buf);
#endif

		}
	}
	//assert(toDelete.top == counter);
	int restartSize = elilit->termSize();
	// delete all marked nodes
	while(!toDelete.isEmpty()) {
		//counter --;
		Clause *c = toDelete.pop();
		int litSize = c->literals[0]->termSize();
		restartSize = min(litSize, restartSize);
#ifdef USE_RELEVANCE
		modelTrieRoot[c->relevance]->remove(c->literals[0]);
		modelTrieRoot[0]->remove(c->literals[0]);
#else
		//Trie *removeNode = 
		modelTrieRoot.remove(c->literals[0]);
		//char buf2[STRING_BUF_SIZE];
		//c->literals[0]->toString(buf2, symtable);
		//if(strcmp(buf2, "c_in(c_Message_Omsg_OKey(c_Message_OinvKey(v_K)),v_G,tc_Message_Omsg)") == 0) {
		//	removeNode->toString(buf2, symtable);
		//	printf("remove node %s\n", buf2);
		//}
#endif
		deleteClauseInstanceFromModel(c);
		delete c;
	}
	//assert(counter == 0);
//	printf("checking trie... ");
//	checkTrie();
//	printf("ok\n");
	return restartSize;
}


Term *literals1[MAX_NUM_CLAUSE_LITERALS_RESOLVENT];
bool signs1[MAX_NUM_CLAUSE_LITERALS_RESOLVENT];
Term *literals2[MAX_NUM_CLAUSE_LITERALS_RESOLVENT];
bool signs2[MAX_NUM_CLAUSE_LITERALS_RESOLVENT];
// given a clause instance, perform ordered resolution with clauses in the model,
// in-place replace the instance with the resolvent
void orderedResolve(Clause **instPtr) {
	Clause *inst = *instPtr;

	Term **literalsA = literals1;
	bool *signsA = signs1;
	int nLiteralsA;

	Term **literalsMerged = literals2;
	bool *signsMerged = signs2;
	int nLiteralsMerged;

	memcpy(literalsA, inst->literals, inst->numberOfLiterals * sizeof(Term *));
	memcpy(signsA, inst->signs, inst->numberOfLiterals * sizeof(bool));
	nLiteralsA = inst->numberOfLiterals;

	while(nLiteralsA != 0 && signsA[0] == false) { // max literal is negative, resolve with eligible literals
		// find the clause
		Clause *clauseB = literalsA[0]->vertex->value;
#ifdef DEBUG_ORDERED_RESOLVE
		char buf[STRING_BUF_SIZE];
		clauseB->toString(buf, symtable);
		printf("resolve with %s\n", buf);
#endif
		// merge old and new clause
		merge(literalsA + 1, signsA + 1, nLiteralsA - 1, clauseB->literals + 1, clauseB->signs + 1, clauseB->numberOfLiterals - 1, literalsMerged, signsMerged, &nLiteralsMerged);
		// swap old and new
		Term **templ = literalsA;
		literalsA = literalsMerged;
		literalsMerged = templ;
		bool *temps = signsA;
		signsA = signsMerged;
		signsMerged = temps;
		nLiteralsA = nLiteralsMerged;
	}

	// put new clause in the same region as the model trie
	Clause *c;
	if(nLiteralsA > inst->numberOfLiterals) {
		c = new (clauseRegion) Clause(nLiteralsA,
#ifdef USE_RELEVANCE
				(*instPtr)->relevance,
#else
				0,
#endif
				0);
		delete inst;
	} else {
		c = inst;
		inst->numberOfLiterals = nLiteralsA;
	}
	memcpy(c->literals, literalsA, nLiteralsA * sizeof(Term *));
	memcpy(c->signs, signsA, nLiteralsA * sizeof(bool));
	*instPtr = c;
}

void resetModel() {
	// need to free memories
	memset(&modelTrieRoot, 0, sizeof(Trie));
	modelGraphStartVertex.in.clear();
	modelGraphStartVertex.out.clear();
	Trie::globalTrieNodeHashtable->clear();
#ifdef USE_RELEVANCE
	universalRelevanceMax = 0;
#endif
}

void initModel() {
	// need to free memories
#ifdef USE_RELEVANCE
	memset(modelTrieRoot, 0, sizeof(Trie *) * MAX_RELEVANCE);
	modelTrieRoot[0] = new (modelTrieRegion) Trie();
	universalRelevanceMax = 0;
#endif
}

void checkInstance(Clause *inst, bool elit) {
#ifndef USE_RELEVANCE
// verify that the instance is correctly generated
	char buf[STRING_BUF_SIZE];
	char buf2[STRING_BUF_SIZE];
for(int k = 0; k < inst->numberOfLiterals; k++) {
	Trie *cnode = modelTrieRoot.lookUp(inst->literals[k]);
	if(((k==0) && elit) ^ ((inst->signs[k] && cnode != NULL && !cnode->del) ||
			(!inst->signs[k] && (cnode == NULL || cnode->del)))) {
		inst->toString(buf2, symtable);
		inst->literals[k]->toString(buf, symtable);
		printf("error: literal %s in instance %s is not consistent with trie\n", buf, buf2);
		modelTrieRoot.toString(buf, symtable);
		//printf("trie:\n%s", buf);
		exit(-1);
	}
	if(cnode != NULL && !cnode->del && inst->literals[k]->vertex == NULL) {
		inst->literals[k]->toString(buf, symtable);
		printf("error: instance %s, vertex is null\n", buf);
		exit(-1);
	}
	printf("checking linked instances\n");
	if(!inst->signs[k]) {
		checkInstance(inst->literals[k]->vertex->value, true);
	}
}
#endif
}

void funcCheckEligibleLiteral(AcyclicGraph<Clause *> *v) {
	//checkTrie();
	if(v != &modelGraphStartVertex) {
		assert(v->value->literals[0]->vertex == v);
		checkInstance(v->value, true);
	}
}
void checkEligibleLiterals() {
//	char buf[STRING_BUF_SIZE];
	printf("check eligible literals\n");
//	modelTrieRoot.toString(buf, symtable);
//	printf("%s", buf);
	modelGraphStartVertex.traverse(funcCheckEligibleLiteral);
}
void funcPrintEligibleLiteral(AcyclicGraph<Clause *> *v) {
	char buf[STRING_BUF_SIZE];
	char buf2[STRING_BUF_SIZE];
	if(v != &modelGraphStartVertex) {
		v->value->literals[0]->toString(buf, symtable);
		v->value->toString(buf2, symtable);
		if(strlen(buf2) > CLAUSE_STRING_TRUNCATE_THRESHOLD) {
			truncate(buf2, CLAUSE_STRING_TRUNCATE_THRESHOLD);
		}
		printf("%-100s", buf);
		printf("%s\n", buf2);
	}
}
void printEligibleLiterals() {
//	char buf[STRING_BUF_SIZE];
	printf("eligible literals\n");
	printf("%-100sclause\n", "literal");
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
//	modelTrieRoot.toString(buf, symtable);
//	printf("%s", buf);
	modelGraphStartVertex.traverse(funcPrintEligibleLiteral);
}
