# incOSHL
incOSHL is an implementation of an incremental version of the OSHL algorithm. 

## Compiling and Linking the Source Code
All source code of incOSHL is located in one directory. To compile to the source code, go to the directory contain the source code and run the following command:

    g++ -O3 -o incOSHL *. cpp

The g++ version used for development is 4.6.3. And the g++ version used for testing is 4.1.2. Other version of g++ may also work, but they are not tested.

## Running the Prover

To run incOSHL, use the following command:

    incOSHL <file_path> <options>

`<file_path>` is a relative path to the tptp path, which can be specified in `<options>`. `<options>` is a comma separated list of key value pairs of the form `<key>=<value>`. Most supported keys include both a long form and a short form. The supported keys are given in the following table.

<TABLE cellpadding=0 cellspacing=0 class="t1">
<TR>
	<TD class="tr0 td4"><P class="p11 ft13">Key Long Form</P></TD>
	<TD class="tr0 td5"><P class="p12 ft13">SF</P></TD>
	<TD class="tr0 td6"><P class="p13 ft13">Value Type</P></TD>
	<TD class="tr0 td7"><P class="p14 ft13">Usage</P></TD>
</TR>
<TR>
	<TD class="tr2 td12"><P class="p17 ft15">timeout</P></TD>
	<TD class="tr2 td13"><P class="p18 ft15">to</P></TD>
	<TD class="tr2 td14"><P class="p18 ft15">integer</P></TD>
	<TD class="tr2 td15"><P class="p18 ft16">This key sets the cpu clock time limit for the main loop. The prover periodically checks if the cpu time has exceeded this limit and stops the proof search. In some cases, the periodic check may not occur around the time limit, resulting a longer running time. The "timeout" command from coreutils can be used to solve this problem.</P></TD>
</TR>
<TR>
	<TD class="tr2 td12"><P class="p21 ft20">useSuperSymbol</P></TD>
	<TD class="tr2 td13"><P class="p18 ft21">uss</P></TD>
	<TD class="tr2 td14"><P class="p18 ft12">boolean</P></TD>
	<TD class="tr2 td15"><P class="p19 ft19">This key sets whether the prover creates supersymbols during the preprocessing phase. When enabled, it replaces multiple consecutive symbols in a flatterm with a supersymbol, which provides minor efficiency improvement on some problems.</P></TD>
</TR>
<TR>
	<TD class="tr2 td12"><P class="p21 ft16">sortSubtreesBySize</P></TD>
	<TD class="tr2 td13"><P class="p18 ft21">ssbs</P></TD>
	<TD class="tr2 td14"><P class="p18 ft15">boolean</P></TD>
	<TD class="tr2 td15"><P class="p19 ft16">This key sets whether the prover sorts the subtrees of every clause tree node by the size of the minimum clause instance that can be generated from those subtrees, in ascending order of the minimum clause instance size of a subtree, a value computed during preprocessing. When enabled, it allows the prover to stop looking further at other subtrees when the minimum clause instance size of a subtree is larger than the current term size limit.</P></TD>
</TR>
<TR>
	<TD class="tr5 td12"><P class="p21 ft18">stacksize</P></TD>
	<TD class="tr5 td13"><P class="p18 ft21">ss</P></TD>
	<TD class="tr5 td14"><P class="p18 ft15">integer (megabytes)</P></TD>
	<TD class="tr5 td15"><P class="p18 ft16">This key sets the memory size limit for the prover. The prover generates a "max memory" error if the memory usage is greater than this size plus a small constant (non-growing) memory footprint. In some case, memory leak causes the prover go over this memory usage limit.</P></TD>
</TR>
<TR>
	<TD class="tr2 td12"><P class="p21 ft15">flipLiteral</P></TD>
	<TD class="tr2 td13"><P class="p18 ft11">fl</P></TD>
	<TD class="tr2 td14"><P class="p18 ft15">boolean</P></TD>
	<TD class="tr2 td15"><P class="p19 ft16">This key sets whether the sign of the literals should be flipped.</P></TD>
</TR>
<TR>
	<TD class="tr5 td12"><P class="p21 ft15">randomFlipLiteral</P></TD>
	<TD class="tr5 td13"><P class="p18 ft15">rfl</P></TD>
	<TD class="tr5 td14"><P class="p18 ft15">boolean</P></TD>
	<TD class="tr5 td15"><P class="p19 ft16">This key sets whether the prover randomly chooses a subset of predicates and randomly flips the signs of all the literals with predicates in that set.</P></TD>
</TR>
<TR>
	<TD class="tr2 td12"><P class="p22 ft15">randomLexicalOrder</P></TD>
	<TD class="tr2 td13"><P class="p18 ft15">rlo</P></TD>
	<TD class="tr2 td14"><P class="p18 ft15">boolean</P></TD>
	<TD class="tr2 td15"><P class="p19 ft16">This key sets whether the prover randomly assigns the lexical order of symbols, as opposed to the order they appear in the input file.</P></TD>
</TR>
<TR>
	<TD class="tr2 td12"><P class="p22 ft15">randomClauseInsert</P></TD>
	<TD class="tr2 td13"><P class="p18 ft15">rci</P></TD>
	<TD class="tr2 td14"><P class="p18 ft15">boolean</P></TD>
	<TD class="tr2 td15"><P class="p19 ft16">This key sets whether the prover randomly assigns the order in which the input clauses are inserted into the clause tree, as opposed to the order they appear in the input file.</P></TD>
</TR>
<TR>
	<TD class="tr2 td12"><P class="p22 ft15">randomClauseLiteral</P></TD>
	<TD class="tr2 td13"><P class="p18 ft15">rcl</P></TD>
	<TD class="tr2 td14"><P class="p18 ft15">boolean</P></TD>
	<TD class="tr2 td15"><P class="p19 ft16">This key sets whether the prover randomly assigns the order in which literals appear in an input clause when the clause is inserted into the clause tree, as opposed to the order they appear in that clause in the input file.</P></TD>
</TR>
<TR>
	<TD class="tr2 td12"><P class="p17 ft15">tptppath</P></TD>
	<TD class="tr2 td13"><P class="p18 ft15">tp</P></TD>
	<TD class="tr2 td14"><P class="p19 ft15">string (with quotes)</P></TD>
	<TD class="tr2 td15"><P class="p19 ft15">This key sets the tptp path.</P></TD>
</TR>
</TABLE>

##Prover Output

The prover generates on of the following outputs:
<TABLE cellpadding=0 cellspacing=0 class="t3">
<TR>
	<TD class="tr0 td16"><P class="p25 ft13">output</P></TD>
	<TD class="tr0 td17"><P class="p16 ft17">&nbsp;</P></TD>
</TR>
<TR>
	<TD class="tr2 td20"><P class="p21 ft15">time out</P></TD>
	<TD class="tr2 td21"><P class="p26 ft15">The prover timed out without finding a proof.</P></TD>
</TR>
<TR>
	<TD class="tr2 td20"><P class="p21 ft16">max memory</P></TD>
	<TD class="tr2 td21"><P class="p26 ft15">The prover reached the memory limit without finding a proof.</P></TD>
</TR>
<TR>
	<TD class="tr2 td20"><P class="p21 ft16">max term size</P></TD>
	<TD class="tr2 td21"><P class="p26 ft15">The prover reached the maximum term size without finding a proof.</P></TD>
</TR>
<TR>
	<TD class="tr3 td18"><P class="p21 ft22">proof found</P></TD>
	<TD class="tr3 td19"><P class="p19 ft22">The prover found a proof.</P></TD>
</TR>
<TR>
	<TD class="tr0 td18"><P class="p17 ft22">saturated</P></TD>
	<TD class="tr0 td19"><P class="p18 ft22">The prover found a model.</P></TD>
</TR>
</TABLE>

## User Manual
For more information see the [user manual](usermanual.pdf).
