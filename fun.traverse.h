/*
 * fun.traverse.h
 *
 *      Author: Hao Xu
 */

    // This coroutine wraps the recursive lookup function, in which some coroutines are call by a tail call.
    // This wrapper coroutine calls the lookup function with a non tail call so that the stack can be saved starting from this coroutine and resumed later.
    // This wrapper coroutine also provides a return point "rp0" which the inner coroutines can backtrack to when there no (more) instances can be found.
    #define FUN lookupWrapper
        PARAMS(
        		bool, inverse,
        		int, n, // base term size, where var size is 1
                PMVMInstruction *, inp,
                Sub *, sub,
                int, relevance)
        RETURNS(Trie *, Sub *)
    BEGIN
        DEFS()
    assert(VAR(sub)!=NULL);
    //    SAVE_STATE(rp0);
    if(VAR(inverse)) {
    	FUNC_CALL(lookupInverseType,
			(
        		VAR(n),
#ifdef USE_RELEVANCE
        		modelTrieRoot[0],
#else
        		&modelTrieRoot,
#endif
        		VAR(inp),
        		VAR(sub),
        		true
			),
			retnode, subNew, nNew
		);
    } else {
#ifdef USE_RELEVANCE
            if(VAR(relevance) > universalRelevanceLimit) {
            	// failure condition 1: relevance not found
    			// max relevance must grow one at a time
            	int tid = universalTermSizeLimit * (MAX_RELEVANCE + 1) + VAR(relevance);
    			SAVE_STATE_WITH_THREAD(rp_ClauseTree_Node_traverse_relevance, threads[tid], -tid, NULL, del);
    			RESTORE_STATE;
            }
		rp_ClauseTree_Node_traverse_relevance:

		SAVE_STATE(rp_Clause_Node_traverse_next_relevance);
#endif
		FUNC_CALL(lookup,
			(
				VAR(n),
#ifdef USE_RELEVANCE
				modelTrieRoot[VAR(relevance)],
#else
				&modelTrieRoot,
#endif
				VAR(inp),
				VAR(sub)
			),
			retnode, subNew, nNew
		);

    }
#ifdef DEBUG_LOOKUP_CLAUSE_TREE
    if(universalTermSizeLimit == 10) {
		printIndent(VAR(varVersionTarget));
    	printByType(subNew);
	}
#endif
    RETURN(retnode, subNew );
#ifdef USE_RELEVANCE
    rp_Clause_Node_traverse_next_relevance:
	FUNC_TAIL_CALL(lookupWrapper, (false, VAR(n), VAR(inp), VAR(sub), VAR(relevance)+1));
#endif
    END
#undef FUN

// This coroutine traverses the clause tree rooted at "node", matching postive literals with terms in "triePos", and negative literals with terms in "trieNeg"
// When matching the literal at the root node of the clause tree, variables in the tries will have version "varVersionTrie".
// When matching its subtrees, variables in the tries will have version > varVersionTrie.
// The initial substituion is "subInit".
// The coroutine yields, returning the substitution for the next instance of some clause in the clause tree.
// The coroutine backtracks if no (more) instances can be found.
#define FUN ClauseTree_Node_traverse
    PARAMS(
        ClauseTree::Node *, node,
        Sub *, subInit,
        LinkedStack<Term *> *, instStack
    )
    RETURNS(ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
    BEGIN
		if(!globalSortSubtreesBySize) {
			nInput = VAR(node)->branchMinLiteralSize;
			CHECK_TERM_SIZE(nInput, rp_ClauseTree_Node_traverse);
		}
	rp_ClauseTree_Node_traverse:
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

        FUNC_CALL(lookupWrapper, (
            VAR(node)->sign,
            (int) VAR(node)->literal->termSize(),
            VAR(node)->inp,
            VAR(subInit),
            1
        ), retnode, retsub );
        FUNC_TAIL_CALL(ClauseTree_Node_handle_return, (retnode, retsub));
	END
#undef FUN

#define FUN ClauseTree_Node_handle_return
	PARAMS(
			Trie *, nodeNew,
			Sub *, subNew
	)
	BEGIN
    DEFS(
            LinkedStack<Term *> *, instStackNew
    )
#ifdef DEBUG_LOOKUP_CLAUSE_TREE
			VAR_F1(ClauseTree_Node_traverse, node)->literal->toString(buf, symtable);
			printIndent(VAR_F1(ClauseTree_Node_traverse, varVersionTrie));
			printf("look up term %ld:%s%s succeeded\n", VAR_F1(ClauseTree_Node_traverse, varVersionTrie), VAR_F1(ClauseTree_Node_traverse, node)->sign?"":"~", buf);
#endif
			if(!VAR_F1(ClauseTree_Node_traverse, node)->sign) { // if literal is negative, then save the instance
				VAR(instStackNew) = VAR_F1(ClauseTree_Node_traverse, instStack)->push(0, VAR(nodeNew)->subtermPtrs->subterm, instStackRegion); // key is not used
				assert(VAR(nodeNew)->subtermPtrs->subterm->vertex != NULL);
			} else {
				VAR(instStackNew) = VAR_F1(ClauseTree_Node_traverse, instStack);
			}

			SAVE_DATA_DEPENDENCY(retnode, del);

			if(VAR_F1(ClauseTree_Node_traverse, node)->leaf) {
				// this only depends on the previous saved control point,
			    // which has already encapsulated all necessary data dependencies
				// this is an anchor state it will not be deleted even after restore_state
				if(VAR_F1(ClauseTree_Node_traverse, node)->degree > 0) {
					SAVE_STATE(rp_ClauseTree_Node_traverse_dispatch);
				}
				SAVE_STATE_ANCHOR(rp_success);
			rp_success:
				// save current branch pointer into the current thread
				threads[currThreadId] = BRANCH_POINTER;
				RETURN(VAR_F1(ClauseTree_Node_traverse, node), VAR(subNew), VAR(instStackNew));
			}

		rp_ClauseTree_Node_traverse_dispatch:

			FUNC_TAIL_CALL(ClauseTree_Node_traverse_subtrees, (
					VAR_F1(ClauseTree_Node_traverse, node),
					VAR(subNew),
					VAR(instStackNew)
            ));

	END
#undef FUN

// This coroutine traverse the subtree of a clause tree node "node" at index >= "subtreeIndex"
// It backtracks if no (more) instances can be found
// It does not directly yield and return anything. It always tail calls the "Clause_Node_traverse" function or itself.
// Since the subtrees are sorted in ascending ordering of the size of their branch min, we try subtrees starting from the leftmost subtree and stop when the branch min is oversized
#define FUN ClauseTree_Node_traverse_subtrees
    PARAMS(
            ClauseTree::Node *, node,        // used in nested calls
            Sub *, subNew,                   // used in nested calls
            LinkedStack<Term *> *, instStack // used in nested calls
    )
    RETURNS(ClauseTree::Node *, Sub *, LinkedStack<Term *> *)
    BEGIN
    DEFS(
    	int, indexNext
    )

		VAR(indexNext) = -1;
	rp_ClauseTree_Node_traverse_subtrees:
		VAR(indexNext)++;
		if(globalSortSubtreesBySize) {
			nInput = VAR(node)->subtrees[VAR(indexNext)]->branchMinLiteralSize;
			CHECK_TERM_SIZE(nInput, rp_ClauseTree_Node_traverse_subtrees_checkTermSize);
		}
	rp_ClauseTree_Node_traverse_subtrees_checkTermSize:
		if(VAR(indexNext) != VAR(node)->degree - 1) {
			SAVE_STATE(rp_ClauseTree_Node_traverse_subtrees);
		}
        FUNC_TAIL_CALL(ClauseTree_Node_traverse, (
        		VAR(node)->subtrees[VAR(indexNext)],
        		VAR(subNew),
        		VAR(instStack)))
    END
#undef FUN

