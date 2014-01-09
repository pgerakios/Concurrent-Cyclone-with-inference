FILE=$2
SRC=$1
BOOT=build/boot
DEST=build/i686-pc-linux-gnu
BIN=bin
FLAGS="-I./include -I./lib -I./src -I./include -B. -B./lib -save-c -B./bin/lib/cyc-lib -save-c"
$BIN/cyclex  $SRC/$FILE.cyl  $SRC/$FILE.cyc &&\
./mkfile.sh $SRC $FILE 

