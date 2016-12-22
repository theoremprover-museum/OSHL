/*
 *      Author: Hao Xu
 */
#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "linkedList.h"
#include "region.h"

#define HASHTABLE_KEY(x) (reinterpret_cast<const char *>(&(x)))
#define HASH_BASE 5381
#define hash(x, s) /*sdbm_hash*/B_hash((unsigned char*)(x), s)
#define MAX_HASH_TABLE_KEY_SIZE (sizeof(HashKey))
typedef union hashKey HashKey;
typedef char *HashKeyPtr;
#define GLOBAL_PTR_SYMBOL_HASHTABLE_KEY_SIZE sizeof(HashKeyPtrSymbol)

struct HashKeyPtrSymbol {
    void *node;
    Symbol key;
};

union hashKey {
    HashKeyPtrSymbol hkTrieNode;
    HashKeyPtr hkPtr;
};

template<typename T>
struct Bucket : RegionAlloc{
	const char *key;
	int keySize;
	T value;
	Bucket<T> *next;
        Bucket(const char* key, int keySize, T value);
        ~Bucket();
};

// variable size key if tKeySize = -1
// fixed size key if tKeySize >= 0
// this can be overridden by the keySize param in member functions
// when keySize = 0 it means use tKeySize
// when keySize >= 0 it means the key has fixed size keySize
// when keySize = -1 it means the key has variable size
template<typename T, int tKeySize>
struct Hashtable {
	Region *region;
	int capacity; // capacity
	int len;
	Bucket<T> **buckets;
        Hashtable(int capacity, Region *region);
        void insert(const char* keyNoInRegion, T value, int ks = 0);
        T update(const char* key, T value, int ks = 0);
        T deleteKey(const char* key, int ks = 0);
        T lookup(const char* key, int ks = 0);
        T operator[](const char *key);
        double balance();
        void clear();
        LinkedList<const char *> *keys();
        ~Hashtable();
private:
        int getKeySize(const char *key, int keySize);
};

unsigned long B_hash(const unsigned char* string, int size);
unsigned long sdbm_hash(const unsigned char* string, int size);
template<typename T>
void deleteBucketChain(Bucket<T> *h);

template<typename T>
Bucket<T>::Bucket(const char* key, int keySize, T value) :
	key(key),
	keySize(keySize),
    value(value),
    next(NULL) {
}

template<typename T>
Bucket<T>::~Bucket() {
	SET_DELETE(key);
	SET_DELETE(this);
}

template<typename T, int tKeySize>
Hashtable<T, tKeySize>::Hashtable(int capacity, Region *region) :
	region(region),
    capacity(capacity),
    len(0),
    buckets((Bucket<T> **) region_alloc(region, sizeof(Bucket<T> *)*capacity)) {
    memset(buckets, 0, sizeof(Bucket<T> *) * capacity);
}

template<typename T, int tKeySize>
int Hashtable<T, tKeySize>::getKeySize(const char *key, int keySize) {
	if(tKeySize == -1) {
		if(keySize == 0 || keySize == -1) {
			return strlen(key)+1;
		} else {
			return keySize;
		}
	} else {
		if(keySize == 0) {
			return tKeySize;
		} else if(keySize == -1) {
			return strlen(key)+1;
		} else {
			return keySize;
		}
	}
}
/**
 * the key is duplicated but the value is not duplicated.
 * MM: the key is managed by hashtable while the value is managed by the caller.
 */
template<typename T, int tKeySize>
void Hashtable<T, tKeySize>::insert(const char* keyNotInRegion, T value, int keySize) {
//            printf("insert ");
//    printKey((unsigned char *)key, h->keySize);

	keySize = getKeySize(keyNotInRegion, keySize);
	char *key = (char *) region_alloc(region, keySize);
	memcpy(key, keyNotInRegion, keySize);
	Bucket<T> *b = new (region) Bucket<T>(key, keySize, value);
	unsigned long hs = hash(key, keySize);
	unsigned long index = hs%capacity;
	if(buckets[index] == NULL) {
		buckets[index] = b;
	} else {
		Bucket<T> *b0 = buckets[index];
		while(b0->next!=NULL)
			b0=b0->next;
		b0->next = b;
	}
#ifdef DEBUG
                printf("key (%p, %d) inserted into bucket %ld\n", ((HashKeyPtrSymbol *)key)->node, ((HashKeyPtrSymbol *)key)->key, index);
#endif
	len ++;
}
/**
 * update hash table returns the pointer to the old value
 */
template<typename T, int tKeySize>
T Hashtable<T, tKeySize>::update(const char* key, T value, int keySize) {
	keySize = getKeySize(key, keySize);
	unsigned long hs = hash(key, keySize);
	unsigned long index = hs%capacity;
	if(buckets[index] != NULL) {
		Bucket<T> *b0 = buckets[index];
		while(b0!=NULL) {
			if(b0->keySize == keySize && memcmp(b0->key, key, keySize) == 0) {
				T tmp = b0->value;
				b0->value = value;
				return tmp;
				// free not the value
			}
			b0=b0->next;
		}
	}
	return (T) 0;
}
/**
 * delete from hash table
 */
template<typename T, int tKeySize>
T Hashtable<T, tKeySize>::deleteKey(const char* key, int keySize) {
	keySize = getKeySize(key, keySize);
	unsigned long hs = hash(key, keySize);
	unsigned long index = hs%capacity;
        Bucket<T> *b0;
#ifdef DEBUG
        printf("key (%p, %d) being deleted from bucket %ld\n", ((HashKeyPtrSymbol *)key)->node, ((HashKeyPtrSymbol *)key)->key, index);
#endif
	if((b0 = buckets[index]) != NULL) {
#ifdef DEBUG
            printf("found key (%p, %d)\n", ((HashKeyPtrSymbol *)b0->key)->node, ((HashKeyPtrSymbol *)b0->key)->key);
#endif
		if(b0->keySize == keySize && memcmp(b0->key, key, keySize) == 0) {
			buckets[index] = b0->next;
			T ret = b0->value;
			delete b0;
			len --;
			return ret;
		}
		while(b0->next!=NULL) {
#ifdef DEBUG
                    printf("found key (%p, %d)\n", ((HashKeyPtrSymbol *)b0->next->key)->node, ((HashKeyPtrSymbol *)b0->next->key)->key);
#endif
			if(b0->keySize == keySize && memcmp(b0->next->key, key, keySize) == 0) {
				Bucket<T> *temp = b0->next;
				T ret = b0->next->value;
				b0->next = b0->next->next;
				delete temp;
				len --;
				return ret;
			}
            b0 = b0->next;
		}
	}
	return NULL;
}
template<typename T, int tKeySize>
T Hashtable<T, tKeySize>::lookup(const char *key, int keySize) {
	//        printf("lookup ");
	//    printKey((unsigned char *)key, h->keySize);

	keySize = getKeySize(key, keySize);
	unsigned long hs = hash(key, keySize);
	unsigned long index = hs%capacity;
	Bucket<T> *b0 = buckets[index];
	while(b0!=NULL) {
		if(b0->keySize == keySize && memcmp(b0->key,key, keySize)==0) {
			return b0->value;
		}
		b0=b0->next;
	}
	return (T) 0;

}
template<typename T, int tKeySize>
T Hashtable<T, tKeySize>::operator[](const char* key) {
	return lookup(key, 0);
}

template<typename T, int tKeySize>
Hashtable<T, tKeySize>::~Hashtable() {
	int i;
	for(i =0;i<capacity;i++) {
		Bucket<T> *b0 = buckets[i];
		if(b0!=NULL)
		deleteBucketChain(b0);
	}
	SET_DELETE(buckets);
}
template<typename T>
void deleteBucketChain(Bucket<T> *b0) {
		if(b0->next!=NULL) {
			deleteBucketChain(b0->next);
		}
		delete b0;
}
template<typename T, int tKeySize>
double Hashtable<T, tKeySize>::balance() {
    int i;
    int nonempty = 0;
    for(i = 0;i<capacity;i++) {
        Bucket<T> *b=buckets[i];
        if(b!=NULL) {
            nonempty ++;
        }
    }
    return nonempty / (double)len;
}
template<typename T, int tKeySize>
void Hashtable<T, tKeySize>::clear() {
	for(int i = 0; i<capacity; i++) {
		if(buckets[i]!=NULL) {
			deleteBucketChain<T>(buckets[i]);
			buckets[i]=NULL;
		}
	}
	len = 0;
}

template<typename T, int tKeySize>
LinkedList<const char *> *Hashtable<T, tKeySize>::keys() {
	LinkedList<const char *> *keys = new LinkedList<const char *>();
	for(int i =0;i<capacity;i++) {
		for(Bucket<T> *b = buckets[i]; b!=NULL; b=b->next) {
			keys->append(b->key);
		}
	}
	return keys;
}
void nop(void *a);

struct SymTable {
	Region *region;
	Hashtable<Symbol, -1> nameToSymbol;
	Hashtable<const char *, (int) sizeof(Symbol)> symbolToName;
	SymTable() :
		region(make_region(0)),
		nameToSymbol(1024, region),
		symbolToName(1024, region) {}
	Symbol operator[] (const char *name) {
		return nameToSymbol[name];
	}
	const char *operator[] (Symbol symbol) {
		return symbolToName[(char *)&symbol];
	}
	void insert(Symbol symbol, const char *name) {
		nameToSymbol.insert(name, symbol);
		symbolToName.insert((char *)&symbol, name);
	}
	void clear() {
		nameToSymbol.clear();
		symbolToName.clear();
	}
};

#endif
