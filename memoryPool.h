/*
 * memoryPool.h
 *
 *      Author: Hao Xu
 */

#ifndef MEMORYPOOL_H_
#define MEMORYPOOL_H_
#include "stdlib.h"

// preconde capacity > 0
template<int blockSize>
struct MemoryPool {
	union PoolNode {
		PoolNode *nextFree;
	};
	// when sequentialAlloc == true, do not use nextFree pointer,
	// use freeIndex. every time a slot is allocated, increment the freeIndex
	// and test if freeIndex == capacity, if true, set sequenctialAlloc = false
	PoolNode *freeHead;
	size_t size;
	size_t inUse;
	MemoryPool() : freeHead(NULL), size(0), inUse(0) {}

	void *alloc() {
		inUse ++;
		if(freeHead == NULL) {
			size ++;
			return malloc(blockSize);
		} else {
			void *ret = freeHead;
			freeHead = freeHead->nextFree;
			return ret;
		}
	}

	void dealloc(void *elem) {
		inUse --;
		PoolNode *node = reinterpret_cast<PoolNode *>(elem);
		node->nextFree = freeHead;
		freeHead = node;
	}

	~MemoryPool() {}
};

#endif /* OBJECTPOOL_H_ */
