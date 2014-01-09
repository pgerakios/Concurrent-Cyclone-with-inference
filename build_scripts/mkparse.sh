FILE=parse_tab
SRC=$1
BOOT=build/boot
DEST=build/i686-pc-linux-gnu
BIN=bin
FLAGS="-I./include -I./lib -I./src -I./include -B. -B./lib -save-c -B./bin/lib/cyc-lib -save-c"
$BIN/cycbison -d $SRC/parse.y -o $SRC/$FILE.cyc &&\
./mkfile.sh $SRC $FILE 

