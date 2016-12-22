/*
 *      Author: Hao Xu
 */



// This coroutine wraps the recursive lookup function, in which some coroutines are call by a tail call.
    // This wrapper coroutine calls the lookup function with a non tail call so that the stack can be saved starting from this coroutine and resumed later.
    // This wrapper coroutine also provides a return point "rp0" which the inner coroutines can backtrack to when there no (more) instances can be found.
    #define FUN lookupWrapperNS
        PARAMS(
        		bool, inverse,
        		int, n, // base term size, where var size is 1
                Trie *, curr,
                PMVMInstruction *, inp,
                Sub *, sub,
                Version, varVersionSource,
                Version, varVersionTarget)
        RETURNS(Trie *, Sub *)
    BEGIN
        DEFS()
    assert(VAR(sub)!=NULL);
    //    SAVE_STATE(rp0);
    if(VAR(inverse)) {
    	FUNC_CALL(lookupInverseTypeNS,
			(
        		VAR(n),
        		VAR(curr),
        		VAR(inp),
        		VAR(sub),
        		VAR(varVersionSource),
        		VAR(varVersionTarget),
        		true
			),
			retnode, subNew, nNew
		);

    } else {
        // this is necessary because the jump pointer of a clause tree node may point to the end of a literal if there is no variable in the literal
		if( VAR(inp)->subterm == NULL ) {
			retnode = VAR(curr);
			subNew = VAR(sub);
		} else {
			FUNC_CALL(lookupNS,
				(
					VAR(n),
					VAR(curr),
					VAR(inp),
					VAR(sub),
					VAR(varVersionSource),
					VAR(varVersionTarget)
				),
				retnode, subNew, nNew
			);
		}
    }
#ifdef DEBUG_LOOKUP_CLAUSE_TREE
    if(universalTermSizeLimit == 10) {
		printIndent(VAR(varVersionTarget));
    	printByType(subNew);
	}
#endif
    RETURN(retnode, subNew );
//    rp0:
//        RETURN((Trie *)NULL, (Sub *)NULL);
    END
#undef FUN

#define FUN ClauseTree_Node_traverse_subtreesAdapterNS
		PARAMS(
				ClauseTree::Node *, node, // accessed by nested functions
				Trie *, triePos, // accessed by nested functions
				Version, varVersionTrie,
				Sub *, subInit/*,
				LinkedStack<CoroutineState *> *, restartFrom*/
		)
		RETURNS(ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
	BEGIN
		CHECK_TERM_SIZE_NS(VAR(node)->branchMinLiteralSize, rpNS_ClauseTree_Node_traverse_subtreesAdapter_start);
    rpNS_ClauseTree_Node_traverse_subtreesAdapter_start:

    	assert(VAR(subInit) != NULL);
    	// VAR(node) must be root, there for cannot be a leaf
		FUNC_TAIL_CALL(ClauseTree_Node_traverse_dispatchNS, (VAR(node), VAR(triePos), VAR(subInit), VAR(varVersionTrie), (LinkedStack<Term *> *) NULL));
	END
#undef FUN

// This coroutine traverses the clause tree rooted at "node", matching postive literals with terms in "triePos", and negative literals with terms in "trieNeg"
// When matching the literal at the root node of the clause tree, variables in the tries will have version "varVersionTrie".
// When matching its subtrees, variables in the tries will have version > varVersionTrie.
// The initial substituion is "subInit".
// The coroutine yields, returning the substitution for the next instance of some clause in the clause tree.
// The coroutine backtracks if no (more) instances can be found.
#define FUN ClauseTree_Node_traverseNS
    PARAMS(
        ClauseTree::Node *, node,
        Trie *, triePos,
        Version, varVersionTrie,
        Sub *, subInit,
        LinkedStack<Term *> *, instStack
    )
    RETURNS(ClauseTree::Node *, Sub *, LinkedStack<Term *> *, LinkedStack<CoroutineState *> *)
    BEGIN
    DEFS(
            Sub *, subNew,
            LinkedStack<Term *> *, instStackNew,
            Trie *, nodeNew
    )
		if(!globalSortSubtreesBySize) {
			nInput = VAR(node)->branchMinLiteralSize;
			CHECK_TERM_SIZE_NS(nInput, rpNS_ClauseTree_Node_traverse);
		}
	rpNS_ClauseTree_Node_traverse:
#ifdef DEBUG_LOOKUP_CLAUSE_TREE
    // if(universalTermSizeLimit == 10) {
    	VAR(node)->literal->toString(buf, symtable);
		printIndent(VAR(varVersionTrie));
		printf("look up term %ld:%s%s, ", VAR(varVersionTrie), VAR(node)->sign?"":"~", buf);
		printf("subinit = ");
		printByType(VAR(subInit));
		fflush(stdout);
    // }
#endif

        cnode = VAR(node)->jump;
        if(cnode->del && !VAR(node)->sign) {
#ifdef DEBUG_LOOKUP_CLAUSE_TREE
		//if(universalTermSizeLimit == 10) {
			VAR(node)->literal->toString(buf, symtable);
			printIndent(VAR(varVersionTrie));
			printf("look up term %ld:%s%s failed\n", VAR(varVersionTrie), VAR(node)->sign?"":"~", buf);
		//}
#endif
        	// failure condition 1: try to unify but jump pointer points to a deleted node
			RESTORE_STATE;
        }

        FUNC_CALL(lookupWrapperNS, (
            VAR(node)->sign,
            (int) VAR(node)->literal->termSize(),
            /*VAR(triePos) */ cnode,
            VAR(node)->inpJump,
            VAR(subInit),
            (Version) 0,
            VAR(varVersionTrie)
        ), VAR(nodeNew), VAR(subNew) );
//		if(retsub != NULL) {
#ifdef DEBUG_LOOKUP_CLAUSE_TREE
			VAR(node)->literal->toString(buf, symtable);
			printIndent(VAR(varVersionTrie));
			printf("look up term %ld:%s%s succeeded\n", VAR(varVersionTrie), VAR(node)->sign?"":"~", buf);
#endif
			//idleThreads->delFlag = 1;
		rpNS_restart: // restart here when the saved state is invalidate and validated again due to data dependency changes
			if(!VAR(node)->sign) { // if literal is negative, then save the instance
				VAR(instStackNew) = VAR(instStack)->push(0, VAR(nodeNew)->subtermPtrs->subterm, instStackRegion); // key is not used
			} else {
				VAR(instStackNew) = VAR(instStack);
			}
			if(VAR(node)->leaf) {
				// success condition 1
				if(VAR(node)->degree > 0) {
					SAVE_STATE(rpNS_ClauseTree_Node_traverse_dispatch);
				}
				// because when this state is restored it is permanently deleted, we need to resave it
	/*            // link node with corresponding trie node
	            VAR(node)->linkToTrie = retnode;*/
				// save current branch pointer into the current thread
				//threads[universalTermSizeLimit] = BRANCH_POINTER;
				RETURN(VAR(node), VAR(subNew), VAR(instStackNew));
			}
		rpNS_ClauseTree_Node_traverse_dispatch:

			FUNC_TAIL_CALL(ClauseTree_Node_traverse_dispatchNS, (
					VAR(node),
					VAR(triePos),
					VAR(subNew),
					(Version)(VAR(varVersionTrie)+1),
					VAR(instStackNew)
            ));

//        } else {
//#ifdef DEBUG_LOOKUP_CLAUSE_TREE
//		//if(universalTermSizeLimit == 10) {
//			VAR(node)->literal->toString(buf, symtable);
//			printIndent(VAR(varVersionTrie));
//			printf("look up term %ld:%s%s failed2\n", VAR(varVersionTrie), VAR(node)->sign?"":"~", buf);
//		//}
//#endif
//
//        	RESTORE_STATE;
//        }
    END
#undef FUN

#define FUN ClauseTree_Node_traverse_dispatchNS
    PARAMS(
        ClauseTree::Node *, node,        // used in nested calls
        Trie *, triePos,                 // used in nested calls
        Sub *, subNew,                   // used in nested calls
        Version, varVersionTrieSubtrees, // used in nested calls
        LinkedStack<Term *> *, instStack // used in nested calls
    )
    RETURNS(ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
    BEGIN
		assert(VAR(subNew) != NULL);
		FUNC_TAIL_CALL(ClauseTree_Node_traverse_subtreesNS, (VAR(node)->startingIndex));
    END
#undef FUN
// This coroutine traverse the subtree of a clause tree node "node" at index >= "subtreeIndex"
// It backtracks if no (more) instances can be found
// It does not directly yield and return anything. It always tail calls the "Clause_Node_traverse" function or itself.
// Since the subtrees are sorted in ascending ordering of the size of their branch min, we try subtrees starting from the leftmost subtree and stop when the branch min is oversized
#define FUN ClauseTree_Node_traverse_subtreesNS
    PARAMS(
        unsigned int, subtreeIndex
    )
    RETURNS(ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
    BEGIN
    DEFS(unsigned int, indexNext)
		if(globalSortSubtreesBySize) {
			nInput = VAR_F1(ClauseTree_Node_traverse_dispatchNS, node)->subtrees[VAR(subtreeIndex)]->branchMinLiteralSize;
			CHECK_TERM_SIZE_NS(nInput, rpNS_ClauseTree_Node_traverse_subtrees_checkTermSize);
		}
	rpNS_ClauseTree_Node_traverse_subtrees_checkTermSize:

		VAR(indexNext) = (VAR(subtreeIndex) + 1) % VAR_F1(ClauseTree_Node_traverse_dispatchNS, node)->degree;
		if(VAR(indexNext) != VAR_F1(ClauseTree_Node_traverse_dispatchNS, node)->startingIndex) {

			SAVE_STATE(rpNS_ClauseTree_Node_traverse_subtrees);
		}
        FUNC_TAIL_CALL(ClauseTree_Node_traverseNS, (
        		VAR_F1(ClauseTree_Node_traverse_dispatchNS, node)->subtrees[VAR(subtreeIndex)],
        		VAR_F1(ClauseTree_Node_traverse_dispatchNS, triePos),
        		VAR_F1(ClauseTree_Node_traverse_dispatchNS, varVersionTrieSubtrees),
        		VAR_F1(ClauseTree_Node_traverse_dispatchNS, subNew),
        		VAR_F1(ClauseTree_Node_traverse_dispatchNS, instStack)))
    rpNS_ClauseTree_Node_traverse_subtrees:
		FUNC_TAIL_CALL(ClauseTree_Node_traverse_subtreesNS, (VAR(indexNext)));
    END
#undef FUN

