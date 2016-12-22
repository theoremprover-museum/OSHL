/*
 *      Author: Hao Xu
 */

// invariant: both n and the 3rd return value are <= universalTermSizeLimit
#define FUN lookupNS
	PARAMS(
		int, n,
		Trie *, curr,
		PMVMInstruction *, inp,
		Sub *, sub,
		Version, varVersionSource,
		Version, varVersionTarget)
	RETURNS(Trie *, Sub *, int)

	BEGIN
	DEFS(Term *, subterm,
		 PMVMInstruction *, varInp,
		 Trie *, varNode);

//		this is not necessary
//      the jump pointer of a clause tree node may point to the end of a literal if there is no variable in the literal
//      which is checked in lookupWrapper
//		if( VAR(inp)->subterm == NULL ) {
//			RETURN(VAR(curr), VAR(sub), VAR(n));
//		}
		if( IS_VAR_SYMBOL(VAR(inp)->symbol) ) {
			cinp = VAR(inp);
			cnode = VAR(curr);
		} else {
			if( IS_SUPER_SYMBOL(VAR(inp)->symbol) ) {
			rpNS_lookup_superSymbol:
				cinp = VAR(inp);
	#ifdef DEBUG_LOOKUP
				printf("lookup super symbol %s\n", symtable[VAR(inp)->symbol]);
	#endif
				cnode = VAR(curr)->lookUpChild(cinp->symbol);
				if(cnode == NULL) {
	#ifdef STAT_SUPER_SYMBOLS
					superSymbolMissCounter ++;
	#endif
					cnode = VAR(curr);
					// cnode is NULL. it means we haven't tried to add the super symbol edge yet,
					// we need to look up the sequence
					Symbol *seq = superSymbolToSequenceOfSymbols[cinp->symbol];
	#ifdef DEBUG_LOOKUP
					printf("null found, look up the expansion sequence: ");
					Symbol *s = seq;
					do {
						printf("%s ", symtable[*s]);
						s++;
					} while(*s != 0);
					printf("\n");
	#endif
					do { // seq must have length > 0
						// source function symbol
						cnode = cnode->lookUpChildAndGenerateDelNode(*seq, modelTrieRegion);
						if( cnode->del ) {
	#ifdef DEBUG_LOOKUP
							printf("%s not found! backtrack.\n", symtable[*seq]);
	#endif
							// failure condition 2.1 : super symbol lookup seq failure
							// if this node has been updated, then we need to redo the look up
							RESTORE_STATE;
						}
	#ifdef DEBUG_LOOKUP
						printf("%s found\n", symtable[*seq]);
	#endif
						seq++;
					} while(*seq != (Symbol) 0);
	#ifdef DEBUG_LOOKUP
					printf("sequence found, add forward edge to trie\n");
	#endif
					// add forward egde to the trie
					VAR(curr)->addForwardEdgeFunc(cinp->symbol, cnode);
				} else {
	#ifdef STAT_SUPER_SYMBOLS
					superSymbolHitCounter ++;
	#endif
					if(cnode->del) {
						// failure condition 2.2 : super symbol direct lookup failure
						// if this node has been updated, then we need to redo the look up
						RESTORE_STATE;
					}
				}
//				// if the bottommost node has been updated, then we need to redo the look up
//				SAVE_STATE_WITH_THRESHOLD_AND_THREAD(rpNS_lookup_superSymbol, &(cnode->version), cnode->version+1, cnode->delThread)

				cinp++;
				if( cinp->subterm == NULL ) {
					// it reached end of src term
					assert(cnode->subtermPtrs!=NULL);
//					LinkedStack<CoroutineState *> *restartFromNewNode = VAR(restartFrom);
					RETURN(cnode, VAR(sub), VAR(n));
				}
				// the next symbol must be a var symbol
			} else {
			rpNS_lookup_funcSymbol:
				cnode = VAR(curr);
				cinp = VAR(inp);
				do {
			// source function symbol
	#ifdef DEBUG_LOOKUP
					char triestrbuf[STRING_BUF_SIZE];
					printf("lookup symbol: %s ", symtable[VAR(inp)->symbol]);
					//VAR(curr)->toString(triestrbuf, symtable);
					//printf("from: \n%s", triestrbuf);
	#endif
					cnode = cnode->lookUpChildAndGenerateDelNode(cinp->symbol, modelTrieRegion); // this write is safe because when this happens there is no saved state from ***
	#ifdef DEBUG_LOOKUP
					if(VAR(curr) != NULL && !VAR(curr)->del) {
						printf("found!\n");
					} else {
						printf("not found!\n");
					}
	#endif                // the case that VAR(curr) == NULL will be handled in the recursive call
					if( cnode->del ) {
						// failure condition 3 : func symbol lookup failure
						// backtrack if the current node has been deleted
						RESTORE_STATE;
					}
					// iterative
					cinp++;
					if( cinp->subterm == NULL ) {
						// save the current branch_pointer so that when generating the next instance we can restart from here

						RETURN(cnode, VAR(sub), VAR(n));
					}
				} while(IS_FUNC_SYMBOL(cinp->symbol));
			}
		}
		// source is var
#ifdef DEBUG_LOOKUP
		char triestrbuf[STRING_BUF_SIZE], varbuf[STRING_BUF_SIZE];
		Term::varToString(VAR(inp)->symbol, varbuf, symtable);
		printf("lookup var symbol: %s ", varbuf);
		VAR(curr)->toString(triestrbuf, symtable);
		printf("from: \n%s", triestrbuf);
#endif
// uncomment the following code to verify that the newVar flag is set correctly
//                subterm = VAR(sub)->lookup(VAR(inp)->symbol, version);
//                printf("symbol = %lX, subterm is null = %d, newVar = %d\n", VAR(inp)->symbol, (subterm == NULL), VAR(inp)->newVar);
//                assert((subterm == NULL) == VAR(inp)->newVar);
			VAR(varInp) = cinp; // save cinp
			if(cinp->newVar) {
#ifdef DEBUG_LOOKUP
		char triestrbuf[STRING_BUF_SIZE];
		printf("it is a free var\n");
		//VAR(curr)->toString(triestrbuf, symtable);
		//printf("from: \n%s", triestrbuf);
#endif
				FUNC_CALL(branchNS,
					(
						VAR(n) - 1,
						cnode,
						cnode,
						(int) 1
					),
					retnode, retterm, nNew
				);
				if(VAR(varInp)->repeatedVar != 0) {
					subNew = VAR(sub)->push(VAR(varInp)->symbol, retterm, subRegion, VAR(varVersionTarget));
				} else {
					subNew = VAR(sub);
				}
			} else {
				VAR(varNode) = cnode; // save cnode
				VAR(subterm) = VAR(sub)->lookupBackLevel(cinp->repeatedVar, cinp->symbol);
#ifdef DEBUG_LOOKUP
		char triestrbuf[STRING_BUF_SIZE];
		printf("it is a bound var, back level = %d\n", cinp->repeatedVar);
		printByType(VAR(sub));
		VAR(subterm)->toString(triestrbuf, symtable);
		printf("subterm: %s\n", triestrbuf);
#endif
			rpNS_lookup_checkTermSize:
				nInput = VAR(n) + VAR(subterm)->termSize() - 1;
				CHECK_TERM_SIZE_NS(nInput, rpNS_lookup_checkTermSize);

				VAR(subterm)->compile(CURR_REGION);

				FUNC_CALL(lookupNS,
					(
						nInput,
						VAR(varNode),
						VAR(subterm)->inp,
						VAR(sub),
						(Version) 0,
						VAR(varVersionTarget)
					),
					retnode, subNew, nNew
				);

			}

			cinp = VAR(varInp) + 1;
			if( cinp->subterm == NULL ) {
				RETURN(retnode, subNew, nNew);
			}

			FUNC_TAIL_CALL(lookupNS,
				(
					nNew,
					retnode,
					cinp,
					subNew,
					VAR(varVersionSource),
					VAR(varVersionTarget)
				)
			);


	END
#undef FUN
#define FUN branchNS
    PARAMS(
    		int, n,
            Trie *, start,
            Trie *, curr,
            int, numberOfPlaceholders)
    RETURNS(Trie *, Term *, int)
BEGIN
//rpNS_branch:
	if( VAR(numberOfPlaceholders) == 0 ) {
		// There is exactly one term that matches the start term.
		// Therefore, there is no need to save state and the RETURN macro will be executed.
		subtermPtr = VAR(curr)->subtermPtrs;
		while(subtermPtr != NULL) {
			if(subtermPtr->startTrieNode == VAR(start)) {
				RETURN(VAR(curr), subtermPtr->subterm, VAR(n));
			}
			subtermPtr = subtermPtr->next;
		}
	} else {
		// function symbols in target trie
		FUNC_TAIL_CALL(branchTryTargetNodesNS, (VAR(curr)->funcChild));
	}
END
#undef FUN
#define FUN branchTryTargetNodesNS
    PARAMS(
            Trie *, curr)
    RETURNS(Trie *, Term *, int)
BEGIN

		if(VAR(curr)->del) {
			if(VAR(curr)->sibling != NULL) {
				FUNC_TAIL_CALL(branchTryTargetNodesNS, (VAR(curr)->sibling));
			} else {
				// failure condition 4
				RESTORE_STATE;
			}
		} else {
			if(VAR(curr)->sibling != NULL) {
				SAVE_STATE(rpNS_4);
			} else {
			}
		}
	rpNS_branchTryTargetNodes_checkTermSize:
        s = VAR(curr)->key;
        arity = IS_FUNC_SYMBOL(s) ? (int) (s & FUNC_ID_ARITY_MASK) : 0;
        nInput = VAR_F1(branchNS, n) + VAR_F1(branchNS, numberOfPlaceholders) + arity;
		CHECK_TERM_SIZE_NS(nInput, rpNS_branchTryTargetNodes_checkTermSize);

        FUNC_TAIL_CALL(branchNS,
        		(
        				VAR_F1(branchNS, n) + 1,
        				VAR_F1(branchNS, start),
        				VAR(curr),
        				VAR_F1(branchNS, numberOfPlaceholders) + arity - 1
        		)
        );
        // char buf[STRING_BUF_SIZE];
        // VAR(curr)->toString(buf, symtable);
        // printf("resume:\n%s\n", buf);
	rpNS_4:
		FUNC_TAIL_CALL(branchTryTargetNodesNS, (VAR(curr)->sibling));


END
#undef FUN
