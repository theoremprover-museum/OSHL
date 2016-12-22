/*
 *      Author: Hao Xu
 */
#ifndef _REGION_H
#define _REGION_H

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#define DEFAULT_BLOCK_SIZE ((size_t)1024 * 1024)
// the alignment in the region in bytes
#define ALIGNMENT 8
#define ALIGN(x) ((x)%ALIGNMENT == 0?(x):((x)/ALIGNMENT+1)*ALIGNMENT)


struct error {
    int code;
    char msg[1024];
    void *obj; // a generic point to an object which is the source of the error
};

struct region_node {
	unsigned char *block; // pointer to memory block
	size_t size; // size of the memory block in bytes
	size_t used; // used bytes of the memory block
	struct region_node *next; // pointer to the next region
        
};


typedef struct region {
	size_t alloc_size;
	struct region_node *head, *active;
	struct region_desc *free;
//        struct error error;
} Region;

typedef struct region_desc {
    Region *r;
    struct region_desc *nextFree; // this is set only if del is set
    size_t size;
    int del;
} RegionDesc;

#define REGIONDESCOF(x) ((RegionDesc *)(((unsigned char *)(x))-ALIGN(sizeof(RegionDesc))))
#define IN_REGION(x,r) (REGIONDESCOF(x)->r == (r))
#define SET_DELETE_AND_ADD_TO_FREE_LIST(x) \
{ \
	RegionDesc *rd = REGIONDESCOF(x); \
	rd->del=1; \
	rd->nextFree=rd->r->free; \
	rd->r->free = rd; \
} \

#define SET_DELETE(x) \
	REGIONDESCOF(x)->del=1; \

#define DELETED(x) (REGIONDESCOF(x)->del != 0)
#define SIZE(x) (REGIONDESCOF(x)->size)

#define FIRST_OBJ_INIT_BLOCK(x) ((x)->block+ALIGN(sizeof(Region))+ALIGN(sizeof(struct region_node))+ALIGN(sizeof(RegionDesc)))
#define FIRST_OBJ_NONINIT_BLOCK(x) ((x)->block+ALIGN(sizeof(struct region_node))+ALIGN(sizeof(RegionDesc)))
#define NEXT_OBJ_BLOCK(o) ((o)+SIZE(o)+ALIGN(sizeof(RegionDesc)))
#define OUT_OF_BOUND_BLOCK(x, o) ((o) >= (x)->used+(x)->block)

#define FIRST_OBJ_REGION(r, x, o) \
	struct region_node *x = r->head; \
	unsigned char *o = FIRST_OBJ_INIT_BLOCK(x); \
	if(OUT_OF_BOUND_BLOCK(x, o)) { \
		NEXT_OBJ_REGION(x, o); \
	} \

#define NEXT_OBJ_REGION(x, o) \
	o = NEXT_OBJ_BLOCK(o); \
	if(OUT_OF_BOUND_BLOCK(x, o)) { \
		if(x->next == NULL) { \
			o = NULL; \
		} else {\
			x = x->next; \
			o = FIRST_OBJ_NONINIT_BLOCK(x); \
		} \
	} \

// create a region with initial size is
// if s == 0 then the initial size is DEFAULT_BLOCK_SIZE
// returns NULL if it runs out of memory
Region *make_region(size_t is = 0);
// allocation s bytes in region r
// return NULL if it runs out of memory
void *region_alloc(Region *r, size_t s);
void *region_realloc(Region *r, void *ptr, size_t s);

// this one only works if this region only contains allocation of type T
// it looks through the free list
template<typename T>
void *region_alloc_type(Region *r);
template<typename T>
void *region_alloc_type(Region *r) {
	if(r->free != NULL) {
		// assume that every allocation has ALIGN(sizeof(T))
		RegionDesc *temp = r->free;
		r->free = r->free->nextFree;
		temp->del = 0;
		return ((unsigned char *) temp) + ALIGN(sizeof(RegionDesc));
	} else {
		return region_alloc(r, sizeof(T));
	}
}

// free region r
void region_free(Region *r);
long region_data_size(Region *r);

struct RegionAlloc {
    void *operator new(size_t size, Region *r) {
        return region_alloc(r, size);
    }

    void operator delete(void *ptr) {
    	SET_DELETE(ptr);
    }
};

template<typename T>
struct RegionAllocType {
    void *operator new(size_t size, Region *r) {
        return region_alloc_type<T>(r);
    }

    void operator delete(void *ptr) {
    	SET_DELETE_AND_ADD_TO_FREE_LIST(ptr);
    }
};
#endif
