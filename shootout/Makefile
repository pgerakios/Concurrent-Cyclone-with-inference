.PHONY: all clean distclean specs table bencher

PROGRAMS = binary-trees thread-ring mandelbrot fannkuch \
           chameneos-redux spectral-norm regex-dna 

JAVA_SRC = binarytrees.java chameneosredux.java fannkuch.java \
           mandelbrot.java threadring.java

CPROGS= $(PROGRAMS)
CYC_PROGS= $(addsuffix _cyc.cyco, $(PROGRAMS))

CC=gcc
CFLAGS=-Wall -O3 -fomit-frame-pointer -march=i686
INCLUDE=
LIB=

CYCC=../bin/cyclone1
CYCFLAGS=$(CYCDEBUG) $(CFLAGS) -nogc
CYCLIB=-lpthread

%: %.c
	$(CC) $(CFLAGS) -o $@ $^

%.cyco: %.cyc
	$(CYCC) $(CYCFLAGS) $(CYCLIB) -o $@ $^

all: $(CPROGS) $(CYC_PROGS)

bencher: 
	@./configure 
	env PATH=$(PATH):. $(MAKE) -C bin/bencher all

bin/csv: bin/csv.c
	$(MAKE) -C bin

table: bin/csv
	@bin/csv bin/bencher/tmp/data.csv  > eval_table.tex 
	@sed -i 's/fannkuch/fannkuch-redux/g' eval_table.tex 
	@cat eval_table.tex

%.class: %.java
	javac $^

makejava: $(addsuffix .class, $(basename $(JAVA_SRC))) 

#At least 2 ITERATIONS otherwise bin/hist will fail
ITERATIONS=2
%.res: %.spec 
	bin/spec2res.sh $(basename $^) $(ITERATIONS) 

build_spec_tools:
	$(MAKE) -C bin all

specs: build_spec_tools $(addsuffix .res, $(PROGRAMS))

clean:
	$(RM) *~ *.bak *.o *.cyco 

distclean: clean
	$(RM) $(PROGRAMS) $(CYC_PROGS) *.res \
		chameneos-redux-fair chameneos-redux-gcc2 mandelbrot.exe
	$(MAKE) -C bin clean

binary-trees: binary-trees.c
	$(CC) $(CFLAGS) \
	   -fopenmp -D_FILE_OFFSET_BITS=64 -I/usr/include/apr-1.0 \
	   -o $@ $^ -lapr-1 -lgomp
binary-trees_cyc.cyco: binary-trees_cyc.cyc

chameneos-redux: chameneos-redux.c -lpthread
chameneos-redux_cyc.cyco: chameneos-redux_cyc.cyc

FANNKUCH_OPT=-O0 
fannkuch: fannkuch.c -lpthread
	$(CC) $(CFLAGS) $(FANNKUCH_OPT) -o $@ $^
fannkuch_cyc.cyco: fannkuch_cyc.cyc
	$(CYCC) $(CYCFLAGS) $(FANNKUCH_OPT) $(CYCLIB) -o $@ $^

mandelbrot: mandelbrot.c -lpthread
mandelbrot_cyc.cyco: mandelbrot_cyc.cyc

mandelbrot.exe: mandelbrot.cs
	gmcs -optimize+ mandelbrot.cs

regex-dna: regex-dna.c
	$(CC) $(CFLAGS) \
	   -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include/ \
	   -I/usr/include/tcl8.4 \
	   -o $@ $^ -lpthread -lglib-2.0 -ltcl8.4
regex-dna_cyc.cyco: regex-dna_cyc.cyc regex-dna_stub.o
	$(CYCC) $(CYCFLAGS) $(CYCLIB) -o $@ $^ -lglib-2.0 -ltcl8.4
regex-dna_stub.o: regex-dna_cyc.cyc
	cp regex-dna_cyc.cyc tmp000.c
	$(CC) $(CFLAGS) -DC_STUB \
	   -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include/ \
	   -I/usr/include/tcl8.4 \
	   -o $@ -c tmp000.c
	rm -f tmp000.c

spectral-norm: spectral-norm.c -lm -lpthread
spectral-norm_cyc.cyco: spectral-norm_cyc.cyc spectral-norm_stub.o -lm
spectral-norm_stub.o: spectral-norm_cyc.cyc
	cp spectral-norm_cyc.cyc tmp000.c
	$(CC) $(CFLAGS) -DC_STUB -o $@ -c tmp000.c
	rm -f tmp000.c

thread-ring: thread-ring.c -lpthread
thread-ring_cyc.cyco: thread-ring_cyc.cyc

# not used anymore

fasta: fasta.c -lpthread
k-nucleotide: k-nucleotide.c -lpthread
meteor-contest: meteor-contest.c -lpthread
n-body: n-body.c -lpthread
pidigits: pidigits.c -lpthread
reverse-complement: reverse-complement.c -lpthread
