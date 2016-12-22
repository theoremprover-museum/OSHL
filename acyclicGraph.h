/*
 * AcyclicGraph.h
 *
 *      Author: Hao Xu
 */

#ifndef ACYCLICGRAPH_H_
#define ACYCLICGRAPH_H_

#include "linkedList.h"


template <typename T>
struct AcyclicGraph {
	T value;
	bool del; // this is used to mark if a node is to be deleted
	// If C++ supported type fixpoints, this type could be defined as
	// mu x. LinkedList<Pair<AcyclicGraph<T> *, x::Node *> *>
	LinkedList<Pair<AcyclicGraph<T> *, void *> *> in;
	LinkedList<Pair<AcyclicGraph<T> *, void *> *> out;

	AcyclicGraph(T value) : value(value), del(false), in(), out() {

	}

	void addOutgoingEdgeTo(AcyclicGraph<T> *c) {
		Pair<AcyclicGraph<T> *, void *> * p = new Pair<AcyclicGraph<T> *, void *>(c, NULL);
		Pair<AcyclicGraph<T> *, void *> * q = new Pair<AcyclicGraph<T> *, void *>(this, NULL);
		typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *np = out.append(p);
		typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *nq = c->in.append(q);
		p->snd = nq;
		q->snd = np;
	}

	void remove() {
		typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *childNode = out.head;
		while(childNode!=NULL) {
			delete ((typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *) childNode->value->snd)->value;
			childNode->value->fst->in.remove((typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *) childNode->value->snd);
			childNode = childNode ->next;
		}
		typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *superNode = in.head;
		while(superNode!=NULL) {
			delete ((typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *) superNode->value->snd)->value;
			superNode->value->fst->out.remove((typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *) superNode->value->snd);
			superNode = superNode ->next;
		}
		out.clear();
		in.clear();

	}

	void traverse(void(*func)(AcyclicGraph<T> *)) {
		LinkedList<AcyclicGraph<T> *> visitedObj;
		__traverse(func, &visitedObj);

	}

	void __traverse(void(*func)(AcyclicGraph<T> *), LinkedList<AcyclicGraph<T> *> *visited) {
		if(visited->contains(this)) {
			return;
		} else {
			func(this);
			visited->append(this);
			typename LinkedList<Pair<AcyclicGraph<T> *, void *> *>::Node *curr = out.head;
			while(curr!=NULL) {
				curr->value->fst->__traverse(func, visited);
				curr = curr->next;
			}

			curr = in.head;
			while(curr!=NULL) {
				curr->value->fst->__traverse(func, visited);
				curr = curr->next;
			}
		}

	}

};

#endif /* ACYCLICGRAPH_H_ */
