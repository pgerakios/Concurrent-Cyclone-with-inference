include Makefile.inc
MAKEFILE_ORIG=Makefile_orig
DEST_BASE=build/reglock
DEST_C=$(DEST_BASE)/compiler
SRC=src
BIN=bin
INC=include
LIB=lib
OTHER_LIB=bin/lib/cyc-lib
BISON=bin/cycbison
LEXER=$(BIN)/cyclex
CYCFLAGS=$(TARGET_CFLAGS) -I$(INC) -I$(LIB) -I$(SRC)  -B. -B$(LIB) -save-c -B$(OTHER_LIB) -save-c
CYCC=bin/cyclone runtime

# The new compiler uses old libraries ... fixme 
COMP_FILES=$(addprefix $(DEST_C)/, $(addsuffix .o, $(CYCLONE_SRCS))) $(DEST_C)/install_path.c
COMP_DEBUG_FLAG= -g
COMP_LIB_PATH=build/i686-pc-linux-gnu
COMP_LIBS=$(COMP_LIB_PATH)/libcycboot.a \
			 $(COMP_LIB_PATH)/runtime_cyc.a \
			 libsigsegv/src/.libs/libsigsegv.a \
			 gc/.libs/libgc.a
COMP_NAME=cyclone1
#
.SECONDARY: 
	#runtime


all: make_dir $(BIN)/cyclone $(BIN)/$(COMP_NAME) 
#make_runtime 

lib_src_runtime:
	$(MAKE) -f $(MAKEFILE_ORIG) lib_src_runtime

lib_src_runtime_dbg:
	$(MAKE) -f $(MAKEFILE_ORIG) lib_src_runtime_dbg

$(BIN)/cyclone:
		$(MAKE) -f $(MAKEFILE_ORIG) all

make_dir:
	mkdir -p $(DEST_C)

$(SRC)/lex.cyc: $(SRC)/lex.cyl  
	$(LEXER)  $(SRC)/lex.cyl  $(SRC)/lex.cyc 

$(SRC)/parse_tab.cyc: $(SRC)/parse.y
	$(BIN)/cycbison -d $(SRC)/parse.y -o $(SRC)/parse_tab.cyc 
	
$(DEST_C)/%.c: $(SRC)/%.cyc
	$(CYCC) -c -o $@ -compile-for-boot -stopafter-toc $(CYCFLAGS) -D__FILE2__=\"$(notdir $<)\" $<

$(DEST_C)/install_path.c: Makefile.inc
	 (echo "char *Carch = \"$(build)\";"; \
	  echo "char *Cdef_lib_path = \"$(LIB_INSTALL)\";"; \
	  echo "char *Cversion = \"$(VERSION)\";"; \
	  echo "int Wchar_t_unsigned = $(WCHAR_T_UNSIGNED);"; \
	  echo "int Sizeof_wchar_t = $(SIZEOF_WCHAR_T);") > $@

$(DEST_C)/%.o: $(DEST_C)/%.c
	$(CC) -march=i686 $(COMP_DEBUG_FLAG) -c -O $^ -o $@

setup_runtime:
	mkdir -p build/boot
#	/bin/cp bin/lib/cyc-lib/cyc_include.h lib/

make_runtime: setup_runtime 
	$(MAKE)  -f $(MAKEFILE_ORIG) lib_src_runtime
#	$(MAKE)  -f $(MAKEFILE_ORIG) lib_src_runtime_dbg

install_runtime: make_runtime
	cp build/boot/runtime_cyc.a  $(prefix)/lib/cyclone/cyc-lib/i686-pc-linux-gnu/
	cp build/boot/pthread/runtime_cyc.a  $(prefix)/lib/cyclone/cyc-lib/i686-pc-linux-gnu/runtime_cyc_pthread.a
	cp build/boot/nogc.a  $(prefix)/lib/cyclone/cyc-lib/i686-pc-linux-gnu/
	cp build/boot/pthread/nogc.a  $(prefix)/lib/cyclone/cyc-lib/i686-pc-linux-gnu/nogc_pthread.a
#	cp build/boot/runtime_cyc.a $(prefix)/build/i686-pc-linux-gnu

clean:
	$(MAKE) -f $(MAKEFILE_ORIG) clean
	$(MAKE) -C gc clean
	$(MAKE) -C libsigsegv clean
	$(RM) $(BIN)/$(COMP_NAME)

distclean: clean
	$(MAKE) -C gc distclean
	$(MAKE) -C libsigsegv distclean
	$(RM) $(LIB)/runtime_cyc.o

install_orig:
	$(MAKE)  -f $(MAKEFILE_ORIG) install 

install: install_orig install_runtime
  #	install_runtime
	#mkdir -p $(prefix)/bin
#	cp $(BIN)/$(COMP_NAME) $(prefix)/bin

$(BIN)/$(COMP_NAME): $(COMP_FILES) $(BIN)/cyclone
	$(CC) $(COMP_DEBUG_FLAG) -o $@ $(COMP_FILES) $(COMP_LIBS)
