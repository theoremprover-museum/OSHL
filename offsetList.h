/*
 *      Author: Hao Xu
 */
#include "defs.h"
#include "utils.h"
#include <string.h>
#include "assert.h"

#ifndef OFFSET_LIST_H
#define OFFSET_LIST_H

// default value 0
template<typename I, typename T>
struct OffsetList {
	u_int64_t capacity;
    I offset; // index offset
    I top;
    T *elements;

    void increaseTop(I inc) {
    	if(top + inc > capacity) {
    		capacity = max(top + inc, capacity * 2);
    		T *newArray = new T[capacity];
    		memcpy(newArray, elements, sizeof(T) * top);
    		delete[] elements;
    		elements = newArray;
    	}
		top += inc;
    }

    void decreaseBottom(I inc) {
    	if(top + inc > capacity) {
    		capacity = max(top + inc, capacity * 2);
    		T *newArray = new T[capacity];
    		memcpy(newArray + inc, elements, sizeof(T) * top);
    		delete[] elements;
    		elements = newArray;
    	} else {
			memmove(elements + inc, elements, sizeof(T) * top);
    	}
    	offset -= inc;
		top += inc;
    }
    void increaseTopAndZero(I inc) {
    	I oldTop = top;
    	increaseTop(inc);
		memset(elements + oldTop, 0, sizeof(T) * inc);
		/*for(u_int64_t i = 0; i<top;i++) {
			printf("%p\n",elements[i]);
		}*/
    }

    void decreaseBottomAndZero(I inc) {
    	decreaseBottom(inc);
		memset(elements, 0, sizeof(T) * inc);
		/*for(u_int64_t i = 0; i<top;i++) {
			printf("%p\n",elements[i]);
		}*/
    }

    void setAndZero(I index, T value) {
    	//printf("set %d, %d", index, (int) value);
    	if(top == 0) {
    		offset = index;
    		increaseTopAndZero(1);
    		elements[0] = value;
    	} else {
    		if(offset > index) {
    			decreaseBottomAndZero(offset - index);
    			elements[0] = value;
    		} else {
    			I realIndex = index - offset;
    			increaseTopAndZero(max(realIndex + 1, top) - top);
    			elements[realIndex] = value;
    		}
    	}

    }

    void set(I index, T value) {
    	//printf("set %d, %d", index, (int) value);
    	if(top == 0) {
    		offset = index;
    		increaseTop(1);
    		elements[0] = value;
    	} else {
    		if(offset > index) {
//    			printf("offset = %ld, index = %ld, top = %ld, sizeof(T) = %d\n", offset, index, top, sizeof(T));
    			decreaseBottom(offset - index);
//    			printf("new capacity = %ld\n", capacity);
    			elements[0] = value;
    		} else {
    			I realIndex = index - offset;
    			increaseTop(max(realIndex + 1, top) - top);
    			elements[realIndex] = value;
    		}
    	}
    }
    
    T operator[](const I index) {
        return elements[index - offset];
    }

    T lookupCheckBound(const I index) {
    	if(index < offset || index >= offset + top) {
    		return (T) 0;
    	} else {
    		return (*this)[index];
    	}
    }

    OffsetList() :
        capacity(16),
        offset(0),
        top(0),
        elements(new T[capacity])
        {
    }

    ~OffsetList() {
        delete[] this->elements;
    }
};

#endif
