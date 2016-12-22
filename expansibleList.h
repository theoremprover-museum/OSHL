/*
 *      Author: Hao Xu
 */
#include "defs.h"
#include "assert.h"

#ifndef EXPASIBLE_LIST_H
#define EXPASIBLE_LIST_H

template<typename T, T undefinedValue>
struct ExpansibleList {
	u_int64_t capacity;
    u_int64_t tail; // index of the last non NULL term + 1, or 0, does not retract
    u_int64_t head; // index of the first non NULL term, or 0, does not retract
    T *elements;
    u_int64_t *indexStack;
    u_int64_t isTop;

    void set(u_int64_t index, T value) {
    	//printf("set %d, %d", index, (int) value);
        if(this->tail == this->head) { // empty
            this->head = index;
            this->tail = index + 1;
        } else
        if(this->tail <= index) {
            // increase size
        	u_int64_t p;
            for(p=this->tail;p<index;p++) {
            	assert(p>=0 && p<capacity);
                this->elements[p] = undefinedValue;
            }
            this->tail = index + 1;
        } else
        if(this->head > index) {
            // increase size
        	u_int64_t p;
            for(p=index+1;p<this->head;p++) {
            	assert(p>=0 && p<capacity);
                this->elements[p] = undefinedValue;
            }
            this->head = index;
        }
    	assert(index>=0 && index<capacity);
        this->elements[index] = value;
        indexStack[isTop++] = index;
    }
    
    T operator[](const u_int64_t index) {
        if(this->head > index || this->tail <= index) {
            return undefinedValue;
        } else {
        	assert(index>=0 && index<capacity);

            return this->elements[index];
        }
    }
    int mark() {
        return isTop;
    }
    
    void restore(u_int64_t isTop0) {
        for(u_int64_t i=isTop;i>isTop0;i--) {
            pop();
        }
    }

    void restore(u_int64_t isTop0, bool free) {
        for(u_int64_t i=isTop;i>isTop0;i--) {
            pop(free);
        }
    }
    
    void push(T value) {
        // printf("%p, %d, %p\n", this, ((Pair<int, Term *> *)value)->fst, ((Pair<int, Term *> *)value)->snd);
        indexStack[isTop++] = tail;
    	assert(tail>=0 && tail<capacity);

        this->elements[this->tail++] = value;
    }

    void pop() {
    	//printf("isTop %d pop %d", isTop, indexStack[isTop-1]);
    	assert(indexStack[isTop-1]>=0 && indexStack[isTop-1]<capacity);
        elements[indexStack[--isTop]] = undefinedValue;
    }
    void pop(bool free) {
        pop();
        if(free) {
        	assert(indexStack[isTop]>=0 && indexStack[isTop]<capacity);

            delete elements[indexStack[isTop]];
        }
    }

    ExpansibleList(int capacity) :
        capacity(capacity),
        tail(0),
        head(0),
        elements(new T[capacity]),
        indexStack(new u_int64_t[capacity]),
        isTop(0) {
    }
    ExpansibleList(ExpansibleList *list) {
        this->tail = list->tail;
        this->capacity = list->capacity;
        this->head = list->head;
        this->elements = new T[this->capacity];
        this->indexStack = new int[capacity];
        this->isTop = list->isTop;
        memcpy(this->elements + this->head, list->elements + list->head, (list->tail - list->head) * sizeof(T));
        memcpy(this->indexStack, list->indexStack, list->isTop * sizeof(int));
    }
    ~ExpansibleList() {
        delete[] this->elements;
        delete[] this->indexStack;
    }
};

#endif
