FILES="absyn warn parse_errors absynpp	absyndump rgnorder 	tcenv unify tcutil tctyp relations-ap tcstmt tcpat tcexp formatstr evexp callgraph tc binding pratt-ap jump_analysis cf_flowinfo 			  	new_control_flow insert_checks toc remove_aggregates toseqc tovc interface cifc tcdecl port specsfile cyclone ioeffect"
 ./mkparse.sh src
 ./mklex.sh src lex
 ./mklex.sh src buildlib
 for i in $FILES; do
	 	 ./mkfile.sh src $i
 done
								
