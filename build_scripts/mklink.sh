DEST=build/i686-pc-linux-gnu
BIN=bin
gcc -g -o $BIN/cyclone1 $DEST/absyn.o $DEST/warn.o $DEST/parse_tab.o $DEST/parse_errors.o $DEST/lex.o $DEST/absynpp.o \
	  			$DEST/absyndump.o $DEST/rgnorder.o 	$DEST/tcenv.o $DEST/unify.o $DEST/tcutil.o $DEST/tctyp.o $DEST/relations-ap.o \
			  	$DEST/tcstmt.o $DEST/tcpat.o $DEST/tcexp.o $DEST/formatstr.o \
			  	$DEST/evexp.o $DEST/callgraph.o $DEST/tc.o $DEST/binding.o $DEST/pratt-ap.o $DEST/jump_analysis.o $DEST/cf_flowinfo.o \
            $DEST/new_control_flow.o $DEST/insert_checks.o $DEST/toc.o $DEST/remove_aggregates.o $DEST/toseqc.o $DEST/tovc.o \
            $DEST/interface.o $DEST/cifc.o $DEST/tcdecl.o $DEST/port.o $DEST/specsfile.o $DEST/cyclone.o $DEST/install_path.o \
			  	$DEST/ioeffect.o $DEST/libcycboot.a $DEST/runtime_cyc.a 	bin/lib/cyc-lib/i686-pc-linux-gnu/libsigsegv.a \
			   bin/lib/cyc-lib/i686-pc-linux-gnu/gc.a  -lpthread
