#CFLAGS=-static -O2
CFLAGS=-emit-llvm -g
CLINK=-s
#COMPILER=gcc
COMPILER=clang

all: vuln.bc patched.bc

vuln.bc: main.bc gpio.bc bumper.bc logging.bc glue.bc EVP_des_ede3_cbc.bc
	llvm-link -o vuln.bc main.bc gpio.bc bumper.bc logging.bc glue.bc EVP_des_ede3_cbc.bc

patched.bc: main.bc gpio.bc bumper.bc logging.bc glue.bc EVP_aes_256_cbc.bc
	llvm-link -o patched.bc main.bc gpio.bc bumper.bc logging.bc glue.bc EVP_aes_256_cbc.bc

util.bc: src/util.c
	$(COMPILER) -c -Iinc $(CFLAGS) src/util.c

logging.bc: src/logging.c
	$(COMPILER) -Wall -c -Iinc $(CFLAGS) src/logging.c

main.bc: src/main.c
	$(COMPILER) -c -Iinc $(CFLAGS) src/main.c

gpio.bc: src/gpio.c
	$(COMPILER) -c -Iinc $(CFLAGS) src/gpio.c

bumper.bc: src/bumper.c
	$(COMPILER) -c -Iinc $(CFLAGS) src/bumper.c

glue.bc: glue.c
	$(COMPILER) -c -Iinc $(CFLAGS) glue.c -I/usr/include/openssl-1.0/ -I/usr/include/klee

EVP_des_ede3_cbc.bc: src/EVP_des_ede3_cbc.c
	$(COMPILER) -Wall -c -Iinc $(CFLAGS) src/EVP_des_ede3_cbc.c

EVP_aes_256_cbc.bc: src/EVP_aes_256_cbc.c
	$(COMPILER) -Wall -c -Iinc $(CFLAGS) src/EVP_aes_256_cbc.c

libs:
	$(COMPILER) -fPIC -Iinc -shared -lcrypto  -o ../build/libdes.so src/EVP_des_ede3_cbc.c
	$(COMPILER) -fPIC -Iinc -shared -lcrypto  -o ../build/libaes.so src/EVP_aes_256_cbc.c

clean:
	rm -f *.bc
	rm -rf klee-*

.PHONY: all vuln patch clean
