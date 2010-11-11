#CROSS_COMPILE=mipsel-linux
CC     = $(CROSS_COMPILE)-gcc
CPP    = $(CROSS_COMPILE)-g++
LD     = $(CROSS_COMPILE)-gcc
AR     = $(CROSS_COMPILE)-ar
RANLIB = $(CROSS_COMPILE)-ranlib

INCS +=-Wundef -Wall -pipe -O0 -g
CFLAGS += $(INCS) $(CPPFLAGS) 

SRC += btree.c memstore.c user_info.c

OBJS=$(addprefix objects/, $(addsuffix .o, $(basename $(notdir $(SRC)))))


LIB=libbtree.a

##############################################################################
ENABLE_MEMWATCH=yes

ifeq ($(ENABLE_MEMWATCH), yes)
    CPPFLAGS += -DMEMWATCH
    SRC += memwatch.c
endif

CFLAGS += -D_GNU_SOURCE=1 -D_REENTRANT -g -I./include -I$(THIRDPARTY)/include


##############################################################################

all: deps objects $(OBJS) $(LIB)

# automatic generation of all the rules written by vincent by hand.
deps: $(SRC) Makefile
	@echo "Generating new dependency file...";
	@-rm -f deps;
	@for f in $(SRC); do \
		OBJ=objects/`basename $$f|sed -e 's/\.cpp/\.o/' -e 's/\.cxx/\.o/' -e 's/\.c/\.o/'`; \
		echo $$OBJ: $$f>> deps; \
		echo '	$(CC) $$(CFLAGS) -c -o $$@ $$^'>> deps; \
	done

-include ./deps

objects:
	@mkdir objects
.PHONY: madlib

$(LIB): objects $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@
	gcc $(LIB) -o btree -g -O0

install: $(LIB)
	install -d $(THIRDPARTY)/lib
	install -d $(THIRDPARTY)/include/guicore
	cp $(LIB) $(THIRDPARTY)/lib/ -arf
	cp include/*.h $(THIRDPARTY)/include/guicore -arf

clean: 
	rm -rf $(OBJS) *.o *~ .*swp objects deps $(CLEANFILE) $(BIN) *.log $(LIB) cscope.* tags btree
