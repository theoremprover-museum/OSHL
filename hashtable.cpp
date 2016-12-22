/*
 *      Author: Hao Xu
 */
#include "hashtable.h"
void printKey(const unsigned char *key, int keySize) {
    for(int i=0;i<keySize;i++) {
        printf("%d ", key[i]);
    }
        printf("\n");
}

void nop(void *) {
}

const char undefinedChar = '\0';



unsigned long B_hash(const unsigned char* string, int size) { // Bernstein hash
	unsigned long hash = HASH_BASE;
        if(size==-1) {
            while(*string!='\0') {
                hash = ((hash << 5) + hash) + (int)*string;
            string++;
            }
        } else {
            int i;
            for(i=0;i<size;i++) {
                hash = ((hash << 5) + hash) + (int)string[i];
            }
        }
//#define DEBUG
#ifdef DEBUG
        printf("hash(%p, %d) = %lu\n", ((HashKeyTrieNode *)string)->node, ((HashKeyTrieNode *)string)->key, hash);
#endif
	return hash;

}
unsigned long sdbm_hash(const unsigned char* str, int size) { // sdbm
        unsigned long hash = 0;
        if(size==-1) {
        while (*str!='\0') {
            hash = ((int)*str) + (hash << 6) + (hash << 16) - hash;
			str++;
		}
        }else {
            int i;
            for(i=0;i<size;i++) {
            hash = ((int)str[i]) + (hash << 6) + (hash << 16) - hash;
            }
        }
#ifdef DEBUG
        printf("hash(%p, %d) = %lu\n", ((HashKeyTrieNode *)str)->node, ((HashKeyTrieNode *)str)->key, hash);
#endif
        return hash;
}

/*
#include "utils.h"

void dumpHashtableKeys(Hashtable *t) {
    int i;
    for(i = 0;i<t->size;i++) {
        struct bucket *b=t->buckets[i];
        while(b!=NULL) {
            writeToTmp("htdump", b->key);
            writeToTmp("htdump", "\n");
            b= b->next;
        }
    }
}
*/
