/*
 * hybridStack.h
 *
 *      Author: Hao Xu
 */

#include "defs.h"
#include "assert.h"

#ifndef HYBRID_STACK_H
#define HYBRID_STACK_H

#define HYBRID_STACK_STATIC_CAPACITY 1024

template<typename T>
struct HybridStack {
	int top;
	int extraCapacity;
	T elements[HYBRID_STACK_STATIC_CAPACITY];
    T *extra;

    bool isEmpty() {
    	return top == 0;
    }
    void push(T value) {
    	if(top >= HYBRID_STACK_STATIC_CAPACITY) {
    		if(extraCapacity + HYBRID_STACK_STATIC_CAPACITY <= top) {
    			if(extraCapacity == 0) {
    				extra = new T[HYBRID_STACK_STATIC_CAPACITY];
    				extraCapacity = HYBRID_STACK_STATIC_CAPACITY;
    			} else {
					T *extraNew = new T[extraCapacity * 2];
					memcpy(extraNew, extra, extraCapacity * sizeof(T));
					delete[] extra;
					extra = extraNew;
					extraCapacity *= 2;
    			}

    		}
			extra[(top++) - HYBRID_STACK_STATIC_CAPACITY] = value;
    	} else {
    		elements[top++] = value;
    	}
    }

    T pop() {
    	if(top > HYBRID_STACK_STATIC_CAPACITY) {
    		return extra[(--top) - HYBRID_STACK_STATIC_CAPACITY];
    	} else {
    		return elements[--top];
    	}
    }
/*    void pop(bool free) {
        pop();
        if(free) {
        	assert(indexStack[isTop]>=0 && indexStack[isTop]<capacity);

            delete elements[indexStack[isTop]];
        }
    }*/

    HybridStack() :
        top(0),
        extraCapacity(0),
        extra(NULL) {
    }
    ~HybridStack() {
    	if(this->extra == NULL) {
    		delete[] this->extra;
    	}
    }
};

#endif
