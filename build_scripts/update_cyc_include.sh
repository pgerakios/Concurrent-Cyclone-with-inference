DEST=`grep "\prefix=" Makefile.inc | head -n 1 | cut -d'=' -f2`
cp lib/cyc_include.h bin/cyc-lib/
cp lib/cyc_include.h $DEST/lib/cyclone/cyc-lib/
cp ./build/boot/runtime_cyc.a  $DEST/lib/cyclone/cyc-lib/i686-pc-linux-gnu/
cp ./build/boot/pthread/runtime_cyc.a  $DEST/lib/cyclone/cyc-lib/i686-pc-linux-gnu/runtime_cyc_pthread.a
cp ./build/boot/nogc.a  $DEST/lib/cyclone/cyc-lib/i686-pc-linux-gnu/
cp ./build/boot/pthread/nogc.a  $DEST/lib/cyclone/cyc-lib/i686-pc-linux-gnu/nogc_pthread.a
cp build/boot/runtime_cyc.a build/i686-pc-linux-gnu
#the last command is for cyclone1 compiler
cp include/core.h  $DEST/include/cyclone/
