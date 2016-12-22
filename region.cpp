/*
 *      Author: Hao Xu
 */
#include "region.h"
#include "memory.h"
#include "memoryPool.h"

MemoryPool<DEFAULT_BLOCK_SIZE> regionPagePool;

// utility function
struct region_node *make_region_node(size_t is) {
	unsigned char *ptr =
			(is == DEFAULT_BLOCK_SIZE)?
					(unsigned char *)regionPagePool.alloc():(unsigned char *)malloc(is);
	if(ptr == NULL) {
		return NULL;
	}
	total_memory_usage += is;
	//printf("total memory usage = %ld", total_memory_usage);
	struct region_node *node = (struct region_node *)ptr;

	node->block = ptr;
	
	node->size = is;
	node->used = ALIGN(sizeof(struct region_node));
	node->next = NULL;
	
	return node;
}
Region *make_region_init_node(size_t is) {
	unsigned char *ptr =
			(is == DEFAULT_BLOCK_SIZE)?
					(unsigned char *)regionPagePool.alloc():(unsigned char *)malloc(is);
	if(ptr == NULL) {
		return NULL;
	}
	total_memory_usage += is;
	//printf("total memory usage = %ld", total_memory_usage);

	Region *r = (Region *) ptr;
	struct region_node *node = (struct region_node *)(ptr+ALIGN(sizeof(Region)));

	node->block = ptr;

	node->size = is;
	node->used = ALIGN(sizeof(struct region_node)) + ALIGN(sizeof(Region));
	node->next = NULL;

	r->alloc_size = is;

	r->head = r->active = node;
	r->free = NULL;
//	r->error.code = 0; // set no error


	return r;
}
Region *make_region(size_t is) {
	if(is == 0)
		is = DEFAULT_BLOCK_SIZE;
	if(is < ALIGN(sizeof(struct region_node)) + ALIGN(sizeof(Region))) {
		is = ALIGN(sizeof(struct region_node)) + ALIGN(sizeof(Region));
	}
	Region *r = make_region_init_node(is);
	if(r == NULL)
		return NULL;

	return r;
}
unsigned char *region_alloc_nodesc(Region *r, size_t s) {
	size_t alloc_size;
	if(s + r->active->used > r->active->size ) {
            int blocksize;
            if(s > DEFAULT_BLOCK_SIZE - ALIGN(sizeof(struct region_node))) {
            	alloc_size = blocksize=s + ALIGN(sizeof(struct region_node));

            } else {
                blocksize= DEFAULT_BLOCK_SIZE;
                alloc_size = ALIGN(s);
            }
            struct region_node *next = make_region_node(blocksize);
            if(next == NULL) {
				return NULL;
            }
            r->alloc_size += blocksize;
            r->active->next = next;
            r->active = next;
		
	} else
          alloc_size = ALIGN(s);

	unsigned char *pointer = r->active->block + r->active->used;
	r->active->used+=alloc_size;
	return pointer;
		
}
void *region_alloc(Region *r, size_t size) {
    unsigned char *mem = region_alloc_nodesc(r, size + ALIGN(sizeof(RegionDesc)));
    ((RegionDesc *)mem)->r = r;
    ((RegionDesc *)mem)->size = size;
    ((RegionDesc *)mem)->del = 0;
    return mem + ALIGN(sizeof(RegionDesc));
}

void *region_realloc(Region *r, void *ptr, size_t size) {
	assert(r == REGIONDESCOF(ptr)->r);
	assert(r->active->block + r->active->used == ((unsigned char *) ptr) + SIZE(ptr));
	size_t newUsed;
	if((newUsed = r->active->used + size - SIZE(ptr)) <= r->active->size) {
		SIZE(ptr) = size;
		r->active->used = newUsed;
		return ptr;
	} else {
		void *newPtr = region_alloc(r, size);
		memcpy(newPtr, ptr, min(SIZE(ptr), size));
		SET_DELETE(ptr);
		return newPtr;
	}
}


void region_free(Region *r) {
	total_memory_usage -= r->alloc_size;
	struct region_node *ptr = r->head;
	while(ptr!=NULL) {
		struct region_node *node = ptr;
		ptr = node->next;

		if(node->size == DEFAULT_BLOCK_SIZE) {
			regionPagePool.dealloc(node->block);
		} else {
			free(node->block);
		}
	}
}

long region_data_size(Region *r) {
	long size = 0;
	struct region_node *node = r->head;
	while(node != NULL) {
		size += node->used;
		node = node->next;
	}
	return size;
}
// tests
//void assert(int res) {
//	if(!res) {
//		printf("error");
//	}
//}
//void test() {
//	Region *r = make_region(0, NULL);
//	assert(region_alloc(r, 16)!=NULL);
//	assert(region_alloc(r, 20)!=NULL);
//	assert(region_alloc(r,1000)!=NULL);
//	assert(region_alloc(r,10000)!=NULL);
//	assert(region_alloc(r,16)!=NULL);
//	assert(region_alloc(r,10)!=NULL);
//	assert(region_alloc(r,1024-16-10)!=NULL);
//	region_free(r);
//}
