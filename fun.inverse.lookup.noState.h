/*
 * fun.inverse.lookup.noState.h
 *
 *      Author: Hao Xu
 */



    // This function performs inverse lookup: it looks up a term not in the trie
    // If such as term is found, it returns the new substitution. If not, it returns null.
    // The variable number of placeholder stores the number of subtree that haven't been matched.
    // For example, if was have a term g(g(a,b), f(b)), and we have matched the prefix g(g(a,
    // then the number of placeholders is 2: one for b and one for f(b). Number of placeholders represent the shape of
    // the part of the term that hasn't been visited yet. It is one-one with the prefix. This information can be used
    // to find out whether a node in a trie is saturated for a given term size, thereby determining whether the inverse trie node is empty.
    // For example, if we have a term g(g(a,X), Y), and we have matched the prefix g(g(a, and the term size is 6,
    // then we know that the size of X and Y has to be 1 2 or 2 1, hence the number of terms.
    // If the node already have that many number of terms with size 6, then the inverse trie node is empty.
    // In a typed setting, we need to know both the number of placeholders and the type of each place holder.

#define FUN lookupInverseTypeNS
	PARAMS(
		int, n, // current term size
		Trie *, curr,
		PMVMInstruction *, inp,
		Sub *, sub,
		Version, varVersionSource,
		Version, varVersionTarget,
		bool, backtrackingForMatchedTerms)
	RETURNS(Trie *, Sub *, int)

	BEGIN
		DEFS(Term *, subterm);

    	if( VAR(inp)->subterm == NULL ) {
			// i.e., it reached end of src term and is still matching
			if( VAR(backtrackingForMatchedTerms) && !VAR(curr)->del) {
				// failure condition 5
				RESTORE_STATE;
			} else {
			rpNS_no_match:
				RETURN(VAR(curr), VAR(sub), VAR(n));
			}
		} else {
#ifdef DEBUG_INVERSE_LOOKUP
			printf("inverse lookup symbol %s\n", symtable[VAR(inp)->symbol]);
#endif
			if( IS_FUNC_SYMBOL(VAR(inp)->symbol) ) { // source function symbol
				cnode = VAR(curr)->lookUpChildAndGenerateDelNode(VAR(inp)->symbol, modelTrieRegion);
				assert(VAR(sub) != NULL);
				FUNC_TAIL_CALL(lookupInverseTypeNS,
					(
						VAR(n),
						cnode,
						VAR(inp)+1,
						VAR(sub),
						VAR(varVersionSource),
						VAR(varVersionTarget),
						VAR(backtrackingForMatchedTerms)
					)
				);
			} else { // source is var
				if(VAR(inp)->newVar) {
					FUNC_CALL(branchInverseTypeNS,
						(
							VAR(n),
							&globalTermCacheType,
							VAR(curr),
							VAR(inp)
						),
						tctnode, retnode, nNew
					);

					if(tctnode->term == NULL) {
						tctnode->fillInTerm();
					}
					retterm = tctnode->term;

					subNew = VAR(sub)->push(VAR(inp)->symbol, retterm, subRegion, VAR(varVersionTarget));
				} else {
					VAR(subterm) = VAR(sub)->lookupBackLevel(VAR(inp)->repeatedVar, VAR(inp)->symbol);
				rpNS_lookupInverseType_checkTermSize:
					nInput = VAR(n) + VAR(subterm)->termSize() - 1;
					CHECK_TERM_SIZE_NS(nInput, rpNS_lookupInverseType_checkTermSize);

					VAR(subterm)->compile(CURR_REGION);
					assert(VAR(sub)!=NULL);
					FUNC_CALL(lookupInverseTypeNS,
						(
							nInput,
							VAR(curr),
							VAR(subterm)->inp,
							VAR(sub),
							(Version) 0,
							VAR(varVersionTarget),
							false
						),
						retnode, subNew, nNew
					);

				}

				FUNC_TAIL_CALL(lookupInverseTypeNS,
					(
						nNew,
						retnode,
						VAR(inp)+1,
						subNew,
						VAR(varVersionSource),
						VAR(varVersionTarget),
						VAR(backtrackingForMatchedTerms)
					)
				);

			}
		}
	END
#undef FUN
#define FUN generateInverseTypeNS
	PARAMS(
		int, n, // current prefix size + context
		TermCacheType *, currTermCacheNode,
		Trie *, curr,
		PMVMInstruction *, inp,
		LinkedStack<TermCacheSeg *> *, subSeg) // the sub is used when the patterm is something like p(X, X)
	RETURNS(TermCacheType*, Trie *, int)

	BEGIN
	DEFS(TermCacheSeg *, seg)

    	if( VAR(inp)->subterm == NULL ) {
#ifdef DEBUG_GENERATE_TERM
    		printf("term generated return: ");
    		Symbol flatterm[MAX_TERM_SIZE];
    		Symbol *p = flatterm+MAX_TERM_SIZE;
    		TermCacheType *curr = VAR(currTermCacheNode);
    		while (curr->super != NULL) {
    			*(--p) = curr->key;
    			curr = curr->super;
    		}
    		for(int i=0;i<flatterm+MAX_TERM_SIZE-p;i++) {
    			printf("%lX(%s) ", p[i], symtable[p[i]]);
    		}
    		printf("\n");

#endif
			RETURN(VAR(currTermCacheNode), VAR(curr), VAR(n));
		} else {
			if( IS_FUNC_SYMBOL(VAR(inp)->symbol) ) { // source function symbol
				cnode = VAR(curr)->lookUpChildAndGenerateDelNode(VAR(inp)->symbol, modelTrieRegion);
				tctnode = VAR(currTermCacheNode)->lookUpChild(VAR(inp)->symbol);
#ifdef DEBUG_GENERATE_TERM
				printf("generating term from func symbol %s, super = %p, curr = %p, progress:", symtable[VAR(inp)->symbol], VAR(currTermCacheNode), tctnode);
	    		TermCacheType *curr = tctnode;
	    		while (curr->super != NULL) {
	    			printf("%lX(%s) %p ", curr->key, symtable[curr->key], curr->super);
	    			curr = curr->super;
	    		}
	    		printf("\n");
				assert(tctnode != NULL);
#endif
				FUNC_TAIL_CALL(generateInverseTypeNS,
					(
						VAR(n),
						tctnode,
						cnode,
						VAR(inp)+1,
						VAR(subSeg)
					)
				);
			} else { // source is var
#ifdef DEBUG_GENERATE_TERM
				printf("generate term from var symbol %lX\n", VAR(inp)->symbol);
#endif
				if(VAR(inp)->newVar) {
//					int nNew;
					FUNC_CALL(branchInverseTypeNS,
						(
							VAR(n),
							VAR(currTermCacheNode),
							VAR(curr),
							VAR(inp)
						),
						tctnode, retnode, nNew
					);
					if(VAR(inp)->repeatedVar != 0) {
						subSegNew = VAR(subSeg)->push(VAR(inp)->symbol, new TermCacheSeg(VAR(currTermCacheNode), tctnode, nNew - VAR(n) + 1), subRegion);
					} else {
						subSegNew = VAR(subSeg);
					}
	#ifdef DEBUG_GENERATE_TERM
					printf("generate term from var symbol successful\n");
	#endif
					FUNC_TAIL_CALL(generateInverseTypeNS,
						(
							nNew,
							tctnode,
							retnode,
							VAR(inp)+1,
							subSegNew
						)
					);
				} else {
					VAR(seg) = VAR(subSeg)->lookup(VAR(inp)->symbol);
				rpNS_generateInverseType_checkTermSize:
					nInput = VAR(n) - 1 + VAR(seg)->third;

					CHECK_TERM_SIZE(nInput, rpNS_generateInverseType_checkTermSize);

					Symbol symbols[MAX_TERM_SIZE];
					Symbol *p = symbols + MAX_TERM_SIZE;
					for(TermCacheType *curr = VAR(seg)->snd, *to = VAR(seg)->fst; curr != to; curr = curr->super) {
						*(--p) = curr->key;
					}
					cnode = VAR(curr);
					tctnode = VAR(currTermCacheNode);
					for(;p != symbols + MAX_TERM_SIZE; p++) {
						cnode = cnode->lookUpChildAndGenerateDelNode(VAR(inp)->symbol, modelTrieRegion);
						tctnode = tctnode->lookUpChild(VAR(inp)->symbol);
					}
					FUNC_TAIL_CALL(generateInverseTypeNS,
						(
							nInput,
							tctnode,
							cnode,
							VAR(inp)+1,
							VAR(subSeg)
						)
					);
				}

			}
		}
	END
#undef FUN
#define FUN branchInverseTypeNS
	PARAMS(
			int, n, // length of the current prefix + context
			TermCacheType *, currTermCacheNode,
			Trie *, curr,
			PMVMInstruction *, var)
	RETURNS(TermCacheType *, Trie *, int)
BEGIN
#ifdef DEBUG_GENERATE_TERM
	printf("branching term from var type %lX\n", VAR(var)->symbol);
	rep = varTypeEquiv.findEquivRep(VAR(var)->symbol);
	typeSymbolToType[HASHTABLE_KEY(rep)]->toString(buf, symtable);
	printf("equiv class %lX ::= %s\n", rep, buf);
#endif
	termPatternHead = VAR(var)->varType->terms.head;
	// termPatternHead should never be null, unless this is an empty type
	// assert(termPatternHead != NULL);
#ifdef DEBUG_VAR_TYPE
	VAR(var)->varType->toString(buf, symtable);
	printf("var = %lX  %s \n", VAR(var)->symbol, buf);
#endif
	assert(termPatternHead != NULL);
	FUNC_TAIL_CALL(branchInverseTypeTryTermPatternsNS, (termPatternHead));

END
#undef FUN

// This coroutine takes in a currTermCacheNode which, unlike the currSuper, is the child of the current node.
#define FUN branchInverseTypeTryTermPatternsNS
	PARAMS(
			LinkedList<Term *>::Node *, termPattern)
	RETURNS(TermCacheType *, Trie *, int)
BEGIN
#ifdef DEBUG_GENERATE_TERM
	VAR(termPattern)->value->toString(buf, symtable);
	printf("try term pattern %s, from trie node %p\n", buf, VAR_F1(branchInverseType, curr));
#endif
rpNS_branchInverseTypeTryTermPatterns_checkTermSize:
	nInput = VAR_F1(branchInverseTypeNS, n) + VAR(termPattern)->value->termSize() - 1;
	CHECK_TERM_SIZE_NS(nInput, rpNS_branchInverseTypeTryTermPatterns_checkTermSize);
	if(VAR(termPattern)->next != NULL) {
		SAVE_STATE(rpNS_branchInverseTryTermPatterns);
	}
	assert(VAR(termPattern)->value->inp!=NULL);
	FUNC_TAIL_CALL(generateInverseTypeNS, (
			nInput,
			VAR_F1(branchInverseTypeNS, currTermCacheNode),
			VAR_F1(branchInverseTypeNS, curr),
			VAR(termPattern)->value->inp,
			new (subRegion) LinkedStack<TermCacheSeg *>() // new sub // (LinkedStack<TermCacheSeg *> *) NULL // here we use NULL as C++ allows invoking methods on NULL pointer
		) // assume that the term pattern has already been compiled
	);
rpNS_branchInverseTryTermPatterns:
	FUNC_TAIL_CALL(branchInverseTypeTryTermPatternsNS, (VAR(termPattern)->next));
END
#undef FUN
