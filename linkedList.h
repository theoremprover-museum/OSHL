/*
 *      Author: Hao Xu
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include <stdlib.h>

template <typename T>
struct LinkedList {
	struct Node {
		T value;
		Node *next;
		Node *prev;
		LinkedList<T> *list;
		Node(T value, LinkedList<T> *list) : value(value), next(NULL), prev(NULL), list(list) {
		}
		~Node() {

		}
		void remove() {
			list->remove(this);
		}
	};
	Node *head;
	Node *tail;
	LinkedList() : head(NULL), tail(NULL) { }
	Node *append(T nvalue) {
		Node *l = new Node(nvalue, this);
		if(head == NULL) {
			head = tail = l;
		} else {
			l->prev = tail;
			tail->next = l;
			tail = l;
		}
		return l;
	}

	Node *appendNew(T nvalue) {
		if(!contains(nvalue)) {
			return append(nvalue);
		}
		return NULL;
	}

	Node *appendNew(T nvalue, bool (*cmp)(T, T)) {
		if(!contains(nvalue, cmp)) {
			return append(nvalue);
		}
		return NULL;
	}

	void remove(T e) {
		for(typename LinkedList<T>::Node *curr = head; curr != NULL; curr = curr->next) {
			if(curr->value == e) {
				remove(curr);
				return;
			}
		}
		throw "no such element";

	}
	void remove(T e, bool (*cmp)(T, T)) {
		for(typename LinkedList<T>::Node *curr = head; curr != NULL; curr = curr->next) {
			if(cmp(curr->value, e)) {
				remove(curr);
				return;
			}
		}
		throw "no such element";
	}
	void remove(Node *n) {
		if(n->prev!=NULL) {
			if(n->next!=NULL) {
				n->prev->next = n->next;
				n->next->prev = n->prev;
			} else {
				// tail
				n->prev->next = NULL;
				tail = n->prev;
			}
		} else {
			// head
			if(n->next!=NULL) {
				n->next->prev = NULL;
				head = n->next;
			} else {
				head = tail = NULL;
			}
		}
		delete n;
	}

	void removeLast(int n = 1) {
		for(int i=0;i<n;i++) {
			remove(tail);
		}
	}
	void reduceTo(int n) {
		int s = this->size();
		for(int i=s;i>n;i--) {
			remove(tail);
		}
	}
	void clear() {
		Node *curr = head, *next;
		while(curr!=NULL) {
			next = curr->next;
			delete curr;
			curr = next;
		}
		head = tail = NULL;
	}
	bool contains(T e) {
		Node *curr = head;
		while(curr!=NULL) {
			if(curr->value == e) {
				return true;
			}
			curr = curr->next;
		}
		return false;
	}
	bool contains(T e, bool (*cmp)(T, T)) {
		Node *curr = head;
		while(curr!=NULL) {
			if(cmp(curr->value, e)) {
				return true;
			}
			curr = curr->next;
		}
		return false;
	}
	void appendAll(LinkedList<T> *list) {
		Node *curr = list->head;
		while(curr!=NULL) {
			append(curr->value);
			curr = curr->next;
		}
	}
	void appendAllNew(LinkedList<T> *list) {
		Node *curr = list->head;
		while(curr!=NULL) {
			appendNew(curr->value);
			curr = curr->next;
		}
	}
	void appendAllNew(LinkedList<T> *list, bool (*cmp)(T, T)) {
		Node *curr = list->head;
		while(curr!=NULL) {
			appendNew(curr->value, cmp);
			curr = curr->next;
		}
	}
	bool empty() {
		return head == NULL;
	}
	int size() {
		int s = 0;
		Node *curr = head;
				while(curr!=NULL) {
					s ++;
					curr = curr->next;
				}
				return s;
	}
	LinkedList<T> *duplicate() {
		LinkedList<T> *dup = new LinkedList<T>();
		dup->appendAll(this);
		return dup;

	}
	void reverse() {
		int n = size();
		T* array = new T[n];
		for(int i = 0; i<n;i++) {
			array[i] = tail->value;
			removeLast();
		}
		for(int i = 0;i<n;i++) {
			append(array[i]);
		}
		delete[] array;
	}

	T* toArray() {
		T* arr = new T[size()];
		int s = 0;
		Node *curr = head;
		while(curr!=NULL) {
			arr[s] = curr->value;
			s ++;
			curr = curr->next;
		}
		return arr;

	}
	~LinkedList() {
		clear();
	}

};

#endif /* LINKEDLIST_H_ */
