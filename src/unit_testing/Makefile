
challengename=AMP-Challenge-05-CAN-Log-Encryption
# Make relative path from our root challenge dir
startloc=$(shell echo $(PWD) | sed -e 's@.*$(challengename)/@@' -e 's@[^/]\+/@../@g' -e 's@/[^/]\+$$@/..@')
munit_dir=$(startloc)/munit
munit_file=$(munit_dir)/munit
srcfiles=main.c bumper.c gpio.c logging.c

# Which testing to be done
ENC?=des
ifeq ($(ENC),aes) 
srcfiles += EVP_aes_256_cbc.c
desaes=2
else
srcfiles +=  EVP_des_ede3_cbc.c
desaes=1
endif

srcdir=$(shell find $(startloc) -type f -name $(word 2,$(srcfiles)) -exec dirname "{}" \;)
includedir=$(subst src,inc,$(srcdir))
list=$(subst /, ,$(shell echo $(PWD) | sed 's@.*$(challengename)/@@'))

CC=gcc
TESTENVIRONMENT:=
CFLAGS:=-I $(munit_dir) -I $(includedir) -DENC=$(desaes) -g -lcrypto 
SRC:=$(addprefix $(srcdir)/,$(srcfiles))
OBJS:=$(patsubst %.c,%.o,$(SRC))
PATCH?=p_one_line

all: run runpatch

tests: $(munit_file).c $(OBJS) tests.c
	$(shell objcopy -N main $(srcdir)/main.o $(srcdir)/main.o)
	$(CC) $(CFLAGS) $^ -o $@

testspatch: $(patsubst %main.o,,$(OBJS))
	patch $(includedir)/logging.h $(includedir)/logging.patch
	touch $(srcdir)/main.c
	$(MAKE) -e ENC=aes tests
	patch -R $(includedir)/logging.h $(includedir)/logging.patch

# Testing for this script
me:
	@echo PWD: $(PWD)
	@echo startloc: $(startloc)
	@echo SRC: $(SRC)
	@echo includedir: $(includedir)

clean:
	rm $(OBJS) tests
	
run: tests
	# $(TESTENVIRONMENT) ./tests # --show-stderr --log-visible debug
	$(TESTENVIRONMENT) ./tests

runpatch: testspatch
	$(TESTENVIRONMENT) ./tests

.PHONY: all run me clean testspatch runpatch
