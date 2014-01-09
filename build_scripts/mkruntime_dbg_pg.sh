DEST=`grep "\prefix=" Makefile.inc | head -n 1 | cut -d'=' -f2`
/bin/cp bin/lib/cyc-lib/cyc_include.h lib/ && make lib_src_runtime_dbg_pg && ./update_cyc_include.sh && ./mklink.sh
/bin/cp inst/lib/cyclone/libcyc.a $DEST/lib/cyclone/libcyc_pg.a
/bin/cp build/boot/pg/pthread/runtime_cyc.a $DEST/lib/cyclone/cyc-lib/i686-pc-linux-gnu/runtime_cyc_pg.a
