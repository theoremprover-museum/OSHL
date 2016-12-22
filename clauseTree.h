/*
 * File:   clauseTree.h
 * Author: Hao Xu
 *
 */

#ifndef CLAUSE_TREE_H
#define	CLAUSE_TREE_H

//#define DEBUG_ADD_CLAUSE
//#define DEBUG_COMPILE

#include "utils.h"
#include "trie.h"
#include "term.h"
#include "defs.h"
#include "hashtable.h"
#include "clause.h"
#include "symbols.h"
#include "superSymbol.h"
#include <string.h>

struct ClauseTree : RegionAlloc {
    
    struct Node : RegionAlloc {
        static const int ALLOC_BLOCK = 16;
        unsigned int size;
        bool sign;
        Term *literal; // root if literal == NULL
        unsigned int degree;
        Node **subtrees;
        bool leaf;
        bool unit; // whether this is a unit clause
        Symbol tightUpperBoundOfFixedVarIdPlusOne; // minimum free var id so far in this path,
        										   // this takes into account of vars in current node as fixed vars
        Node *super;
        Trie *linkToTrie; // this is set by matchers to mark which trie node this literal is currently being unified with
                          // using this link is not thread-safe
        int branchMinLiteralSize; // the "branch min" is the minimum of the maximums of sizes of literals on each path.
        PMVMInstruction *inp; // the specific inp for this instance
        					  // because terms are shared, there is only one compiled inp stored in each term
        					  // the newVar of inp maybe different for different occurrences of the same term
        					  // for example, P(X,Y), P(X,Y). In the first literal X is a new var, in the second clause X is not a new var.
        					  // as a result, we need to store the inp separately
        PMVMInstruction *inpJump; // this jumps past the longest non var prefix
        Trie *jump; // this is set to a trie node that matches the longest non var prefx
        unsigned int index; // this is the index of the current node in the super node's subtrees array
        unsigned int startingIndex; // this is the starting subtree index to try when generating the next index
                           // we use this to avoid having to repeat failed attempts of subtrees that happens to have lower indices every time the prover tries to generate a new instance

        Node(bool sign, Term *literal, Node *super, REGION) :
        		size(ALLOC_BLOCK),
                sign(sign),
                literal(literal),
                degree(0),
                subtrees(HEAP_ALLOC(Node *, ALLOC_BLOCK)),
                leaf(false),
                unit(false),
                super(super),
                branchMinLiteralSize(0),
                inp(NULL),
                inpJump(NULL),
                jump(NULL),
                index(super->degree),
                startingIndex(0) {
        	tightUpperBoundOfFixedVarIdPlusOne = max(maxVarId(literal)+1, super->tightUpperBoundOfFixedVarIdPlusOne);
        	//printf("max(%lX, %lX) = %lX\n", maxVarId(literal), super->tightUpperBoundOfFixedVarIdPlusOne, tightUpperBoundOfFixedVarIdPlusOne);
        }
        
        Node(REGION) : 
        		size(ALLOC_BLOCK),
                sign(false),
                literal(NULL), 
                degree(0),
                subtrees(HEAP_ALLOC(Node *, ALLOC_BLOCK)),
                leaf(false),
                unit(false),
                tightUpperBoundOfFixedVarIdPlusOne(VAR_ID_OFFSET),
                super(NULL),
                branchMinLiteralSize(0),
                inp(NULL),
                inpJump(NULL),
                jump(NULL),
                index(0),
                startingIndex(0) {}

        Node *addSubtree(bool sign, Term *literal, REGION) {
            Node *subtree = new (CURR_REGION) Node(sign, literal, this, CURR_REGION);
            if(degree == size) {
                Node **subtreesNew = HEAP_ALLOC(Node *, size + ALLOC_BLOCK);
                memcpy(subtreesNew, subtrees, sizeof(Node *) * size);
                size += ALLOC_BLOCK; // add test case that size is properly updated
                subtrees = subtreesNew;
            }
            
            subtrees[degree++] = subtree;
            return subtree;
        }
        
        int calculateBranchMinLiteralSize() {
        	int thisSize = this->literal == NULL?
        			0 : // root
        			this->literal->termSize();
        	if(degree > 0) {
				int min = MAX_TERM_SIZE;
				for(unsigned int i = 0; i<degree; i++) {
					int subSize = subtrees[i]->calculateBranchMinLiteralSize();
					min = MIN(min, subSize);
				}
				return branchMinLiteralSize = MAX(thisSize, min);
        	} else {
        		return branchMinLiteralSize = thisSize;
        	}

        }

        int toString(char buf[STRING_BUF_SIZE], SymTable &symtable, int offset = 0, int indent = 0) {
            for(int i=0;i<indent;i++) {
            	SPRINT(buf, offset, " ");
            }
            if(literal!=NULL) {
                SPRINT(buf, offset, "%s", sign?"":"~");
                offset = literal->toString(buf, symtable, offset);
                SPRINT(buf, offset, "%s", " | ");
                if(inp != NULL) {
                	offset = inp->toString(buf, symtable, offset);
                }
                if(leaf) {
                    SPRINT(buf, offset, "*");
                }
            } else {
				SPRINT(buf, offset, "<root>");
            }
//			SPRINT(buf, offset, "{minNewVarID = %lX}", tightUpperBoundOfFixedVarIdPlusOne);
//			SPRINT(buf, offset, "{branchMinLitearlSize = %d}", branchMinLiteralSize);
            SPRINT(buf, offset, "[%d]", degree);
            SPRINT(buf, offset, "\n");
            for(unsigned int i=0;i<degree;i++) {
                offset = subtrees[i]->toString(buf, symtable, offset, indent+1);
            }
            return offset;
        }
        void generateTrieNodes(Trie *root, REGION) {
        	Symbol func = getNullaryFunctionSymbol();
        	if(globalUseSuperSymbols && this->sign == false) {
        		if(this->literal!=NULL) {
        			Term *t = literal->ground(func);
        			// todo replace with code that generate del nodes for nonvar prefix only
        			root->insert(t, CURR_REGION);
        			root->remove(t);
					if(IS_SUPER_SYMBOL(inp->symbol)) {
						Symbol *seq = superSymbolToSequenceOfSymbols[inp->symbol];
						Trie *cnode = root;
						do {
							cnode = cnode->lookUpChild(*seq);
							seq++;
						} while (*seq!=(Symbol) 0);
						jump = cnode; // the first symbol must be pred or super symbol
						inpJump = inp + 1;

        			} else {
            			Trie *cnode = root;
        				cnode = cnode->lookUpChild(inp->symbol);
            			jump = cnode; // the first symbol must be pred or super symbol
            			inpJump = inp + 1;

        			}
        		}
        	} else {
        		jump = root;
        		inpJump = inp;
        	}
    		for(unsigned int i =0;i<degree;i++) {
    			subtrees[i]->generateTrieNodes(root, CURR_REGION);
    		}

        }
        void compile(OffsetList<Symbol, PMVMInstruction *> *firstOccurVar,  REGION) {
        	if(literal != NULL) {
#ifdef DEBUG_COMPILE
				char buf[STRING_BUF_SIZE];
				literal->toString(buf, symtable);
				printf("compiling literal %s\n", buf);
#endif
				inp = literal->convertToPMVMInstructions(firstOccurVar, CURR_REGION);
				if(globalUseSuperSymbols) {// convert to super symbols, if the sign is negative,
					// if the sign is positive, we need disunification which does not support super symbol yet
					if(sign == false) {
						inp = generateSuperSymbols(inp, firstOccurVar, CURR_REGION);
					}
				}
#ifdef DEBUG_COMPILE
				inp->toString(buf, symtable);
				printf("compiled: %s\n", buf);
#endif
        	}
        	for(unsigned int i =0;i<degree;i++) {
        		// if different subtrees share variables, then this won't work and we have to restore the original value of firstOccurVar after each iteration.
        		// however if they don't share variable, this works
        		subtrees[i]->compile(firstOccurVar, CURR_REGION);
        	}

        }
        void computeBackLevels(OffsetList<Symbol, PMVMInstruction *> *firstOccurVar, int nVarsCounted) {
            if(literal != NULL) {
            	inp->computeBackLevels(firstOccurVar, nVarsCounted, sign);  // count repeated var if sign is positive
            }
        	for(unsigned int i =0;i<degree;i++) {
        		// if different subtrees share variables, then this won't work and we have to restore the original value of firstOccurVar after each iteration.
        		// however if they don't share variable, this works
        		subtrees[i]->computeBackLevels(firstOccurVar, nVarsCounted);
        	}
        }
        void newVars(Renaming *ren, Symbol freeVarOffset, Symbol &nextNewVarId) {
        	Renaming *renNew = ren;
        	if(literal!=NULL) {
        		Symbol oldFreeVarOffset = freeVarOffset;
				renNew = literal->newVars(ren, freeVarOffset, nextNewVarId);
				literal = (*literal)(renNew);
	        	tightUpperBoundOfFixedVarIdPlusOne = oldFreeVarOffset == freeVarOffset? super->tightUpperBoundOfFixedVarIdPlusOne : nextNewVarId;
        	}
        	for(unsigned int i =0;i<degree;i++) {
        		subtrees[i]->newVars(renNew, freeVarOffset, nextNewVarId);
        	}
        	deleteToNodeExclusive(renNew, ren);
        }
        void collectLiterals(LinkedList<Term *> *posLits, LinkedList<Term *> *negLits) {
            if(literal!=NULL) {if(sign) {
            	posLits->append(literal);
            } else {
            	negLits->append(literal);
            }
            }
        	for(unsigned int i =0;i<degree;i++) {
        		subtrees[i]->collectLiterals(posLits, negLits);
        	}

        }
        void collectVars(LinkedList<Symbol> *vars) {
            if(literal!=NULL) {
            	literal->fv(vars);
            }
        	for(unsigned int i =0;i<degree;i++) {
        		subtrees[i]->collectVars(vars);
        	}
        }

        void sortSubtreesByLiteralSize() {
        	quicksort_gen<Node *, cmpNode>(subtrees, degree);
        	for(unsigned int i =0;i<degree;i++) {
        		subtrees[i]->sortSubtreesByLiteralSize();
        	}
        }
    private:
        static int cmpNode(Node *a, Node *b) {
        	return b->branchMinLiteralSize - a->branchMinLiteralSize;
        }

    };
    
    void newVars() {
    	Renaming ren;
    	Symbol nextNewVarId = VAR_ID_OFFSET;
    	root->newVars(&ren, 0, nextNewVarId);
    }
    void collectLiteralsAndVars() {
    	root->collectLiterals(&posLits, &negLits);
    	root->collectVars(&vars);
    }
    Node *root;
    
    LinkedList<Term *> posLits;

    LinkedList<Term *> negLits;

    LinkedList<Symbol> vars;

    ClauseTree(REGION) : root(new (CURR_REGION) Node(CURR_REGION)), posLits(), negLits() {}
    
    void addClause(Clause *c, REGION) {
#ifdef DEBUG_ADD_CLAUSE
    	char buf[STRING_BUF_SIZE];
    	c->toString(buf, symtable);
    	printf("original clause %s\n", buf);
#endif
    	c->sortBySign();
    	c->normalize(VAR_ID_OFFSET);

#ifdef DEBUG_ADD_CLAUSE
    	c->toString(buf, symtable);
    	printf("normalized clause %s\n", buf);
#endif

        Node *curr = root;
        Renaming subObj, *sub = &subObj;
        curr = addNodes(curr, c->numberOfNegativeLiterals, c->signs, c->literals, sub, CURR_REGION);
        curr = addNodes(curr, c->numberOfLiterals - c->numberOfNegativeLiterals, c->signs + c->numberOfNegativeLiterals, c->literals + c->numberOfNegativeLiterals, sub, CURR_REGION);
        curr->leaf = true;
        curr->unit = c->numberOfLiterals == 1;
#ifdef DEBUG_ADD_CLAUSE
        this->toString(buf, symtable);
        printf("new clause tree %s\n", buf);
#endif

    }

	void compile(REGION) {
		OffsetList<Symbol, PMVMInstruction *> firstOccurVar;
		root->compile(&firstOccurVar, CURR_REGION);
		root->computeBackLevels(&firstOccurVar, 0);
	}

	void toString(char buf[STRING_BUF_SIZE], SymTable &symtable) {
        root->toString(buf, symtable);
    }

private:
    // literals should be sorted by sign and normalized and should include all literals in a clause
    // as this function may swap literals and renormalize literals
	Node *addNodes(Node *curr, int n, bool *signs, Term **literals,Renaming *&sub, REGION) {
        Renaming *sub2;
		for(int i=0;i<n;i++) { // number of negative literals to add
			bool match = false;
			int ilit;
			unsigned int isubtree;
			for(ilit = i;ilit<n;ilit++) { // number of literals to try
					for(isubtree = 0;isubtree < curr->degree;isubtree++) {
						sub2 = sub;
						// here we can not use the following test
						// signs[ilit] == curr->subtrees[isubtree]->sign && literals[ilit] == curr->subtrees[isubtree]->literal)
						// because we need to make sure that for all fixed var x, sub2(x) != 0. such a sub2 must be constructed by calling variant
#ifdef DEBUG_ADD_CLAUSE
						char buf1[STRING_BUF_SIZE];
						char buf2[STRING_BUF_SIZE];
						literals[ilit]->toString(buf1, symtable);
						curr->subtrees[isubtree]->literal->toString(buf2, symtable);
						printf("variant\n==%s==\n==%s==\n? %lX: ", buf1, buf2, curr->tightUpperBoundOfFixedVarIdPlusOne);
						Renaming *sub3 = sub2;
						while(sub3->val!=0) {
							Term::varToString(sub->val, buf1, symtable);
							printf("?%lX(%s) = %s:%ld, ", (long) sub3->key - VAR_ID_OFFSET, symtable[sub3->key], buf1, (long) sub3->version);
							sub3 = sub3->prev;
						}
#endif
						Symbol minFreeVarId = curr->tightUpperBoundOfFixedVarIdPlusOne;
						if(
							variant(signs[ilit], literals[ilit],
									curr->subtrees[isubtree]->sign, curr->subtrees[isubtree]->literal,
									sub2, minFreeVarId, CURR_REGION)
						) {
#ifdef DEBUG_ADD_CLAUSE
							printf("found: ");
							Renaming *sub3 = sub2;
							while(sub3->val!=0) {
								Term::varToString(sub->val, buf1, symtable);
								printf("?%lX(%s) = %s:%ld, ", (long) sub3->key - VAR_ID_OFFSET, symtable[sub3->key], buf1, (long) sub3->version);
								sub3 = sub3->prev;
							}
							printf("\n");
#endif
							sub = sub2;
							// swap literal at index i and literal at index ilit
							Term *tt = literals[i];
							literals[i] = literals[ilit];
							literals[ilit] = tt;
							bool st = signs[i];
							signs[i] = signs[ilit];
							signs[ilit] = st;
							// no need to renormalize
							// update ubFixedVarIdPlusOne
							match = true;
							break;
						} else {
#ifdef DEBUG_ADD_CLAUSE
							printf("not found\n");
#endif
							deleteToNodeExclusive(sub2, sub);
						}
					}
					if(match) {
						break;
					}
			}
			if(match) {
				curr = curr->subtrees[isubtree];
			} else { // no match, just append
#ifdef DEBUG_ADD_CLAUSE
				sub2 = sub;
				printf("adding new branch: ");
				char buf[STRING_BUF_SIZE];
				while(sub2->val!=0) {
				    Term::varToString(sub2->val, buf, symtable);
					printf("?%lX(%s) = %s:%ld, ", (long) sub2->key - VAR_ID_OFFSET, symtable[sub2->key], buf, (long) sub2->version);
				    sub2 = sub2->prev;
				}
			    printf("\n");
#endif
				ilit = i;
				while(ilit < n) {
					literals[ilit]->normalize(sub, 0, curr->tightUpperBoundOfFixedVarIdPlusOne);
					curr = curr->addSubtree(signs[ilit], (*literals[ilit])(sub), CURR_REGION);
					ilit++;
				}
				break;
			}
		}
		// free memory
		return curr;
	}

};


#endif	/* CLAUSE_TREE_H */

