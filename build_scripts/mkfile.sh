FILE=$2
SRC=$1
BOOT=build/boot
DEST=build/i686-pc-linux-gnu
BIN=bin
FLAGS="-I./include -I./lib -I./src -I./include -B. -B./lib -save-c -B./bin/lib/cyc-lib -save-c"
$BIN/cyclone -g -c -o $SRC/$FILE.c -compile-for-boot $FLAGS -D__FILE2__=\"$FILE.cyc\" -stopafter-toc $SRC/$FILE.cyc  &&\
$BIN/cyclone -M -MG  $FLAGS  $SRC/$FILE.cyc > $SRC/$FILE.dd &&\
sed -e 's/\.o:/\.c:/' $SRC/$FILE.dd > $BOOT/$FILE.d &&\
rm -f $SRC/$FILE.dd &&\
mv $SRC/$FILE.c $BOOT  &&\
gcc -march=i686 -g -c   -O $BOOT/$FILE.c -o $DEST/$FILE.o
