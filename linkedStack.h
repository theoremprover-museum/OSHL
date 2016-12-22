/* 
 * File:   linkedStack.h
 * Author: Hao Xu
 *
 */

#ifndef LINKEDSTACK_H
#define	LINKEDSTACK_H

#include <stdio.h>
#include "defs.h"
#include "assert.h"

template<typename T>
struct LinkedStack;

extern LinkedStack<int> *stackOverwrittenBlock;

template <typename T>
struct LinkedStack : RegionAlloc {
    Symbol key;
    T val;
	// this is the version number of the variable in the term
	// for every var in the term, its varid = symbol | varVersion
    Version version;
    LinkedStack<T> *prev;
    static int alloc_counter;
    LinkedStack() : key(0), val((T)(0)), version(0), prev(NULL)
    {
    	alloc_counter ++;
    }

    LinkedStack(Symbol key, T val, Version version, LinkedStack<T> *prev)
    :key(key), val(val), version(version), prev(prev)
    {
    	alloc_counter ++;
    }

    void checkOverwrite() {
        LinkedStack<int> *invBlock = stackOverwrittenBlock;
        while(invBlock != NULL) {
    		if(invBlock->key == (Symbol) this) {
    			printf("trying to access overwritten stack block %p on line %d\n", this, __LINE__);
    			assert(invBlock->key != (Symbol) this);
    		}
    		invBlock = invBlock->prev;
    	}
    }

    T operator ()(Symbol key, Version & version) {
#ifdef DEBUG_STACK_ALLOC
    	checkOverwrite();
#endif
        if(this->val == (T)(0)){
            return (T)(0);
        }else
            if(this->key == key){
                version = this->version;
                return this->val;
            }else{
                return (*prev)(key, version);
            }

    }

    inline T lookup(Symbol key, Version & version)
    {
        return (*this)(key, version);
    }

    T operator [](Symbol key) {
#ifdef DEBUG_STACK_ALLOC
    	checkOverwrite();
#endif
        if(this->val == (T)(0)){
            return (T)(0);
        } else if(this->key == key) {
            return this->val;
        } else {
            return (*prev)[key];
        }
    }
    T lookupBackLevel(int level, Symbol key) {
/*    	T v = lookup(key);
    	int n = 0;
    	LinkedStack<T> *pp = this;
    	while(pp->val != (T) 0 && pp->key != key) {
    		pp = pp->prev;
    		n++;
    	}
    	printf("v = %p, n = %d, level = %d\n", v, n, level);
    	assert(v != NULL && n == level);
    	return v;*/

    	LinkedStack<T> *p = this;
    	int i = level;
    	while(i!=0) {
    		p =p->prev;
    		i--;
    	}
    	return p->val;
    }
    inline T lookup(Symbol key) {
    	return (*this)[key];
    }
    LinkedStack<T> *push(Symbol key, T val, Region *r, Version version = 0) {
        return new(r) LinkedStack<T>(key, val, version, this);
    }
    LinkedStack<T> *pushNode(LinkedStack<T> *newNode, Symbol key, T val, Version version = 0) {
    	newNode->key = key;
    	newNode->prev = this;
    	newNode->val = val;
    	newNode->version = version;
        return newNode;
    }
    LinkedStack<T> *copy() {
        if(this->val == NULL) {
            return new LinkedStack<T>();
        } else {
            return new LinkedStack<T>(key, val, version, prev->copy());
        }
    }
    LinkedStack<T> *mark() {
        return this;
    }
    LinkedStack<T> *restore(LinkedStack<T> *node) {
        deleteToNodeExclusive(this, node);
        return node;
    }
};

template<typename T>
void deleteToNodeExclusive(LinkedStack<T> *from, LinkedStack<T> *to) {
    LinkedStack<T> *p = from, *pp;
    while(p != to) {
        pp = p->prev;
        delete p;
        p = pp;
    }
}

template<typename T>
void deleteAllButFirstNode(LinkedStack<T> *from) {
	LinkedStack<T> *p = from, *pp;
	while(p->prev!=NULL) {
		pp = p->prev;
		delete p;
		p = pp;
	}
}

template<typename T>
int LinkedStack<T>::alloc_counter = 0;

#endif	/* LINKEDSTACK_H */

