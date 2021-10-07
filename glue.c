#include <stdio.h>
#include <openssl/ossl_typ.h>
#include <openssl/evp.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <string.h>

#include <assert.h>
#include "klee.h"
#include "glue.h"


int symbolic_ready[NR_ITERATIONS] = {1};
struct can_frame symboic_can_frame[NR_ITERATIONS] = {0};
int g_iteration_number = 0;
unsigned char seed = 42;

//////////////////// OPENSSL ////////////////////////////
int RAND_bytes(unsigned char *buf, int num) {
	// This is Lehmer random number generator
	for(int i = 0 ; i < num ; i ++) {
		buf[i] = seed;
		seed *= 167;
	}
	return 1;
}

int BIO_dump_fp(FILE *fp, const char *s, int len) {
	return 0;
}

void ERR_print_errors_fp(FILE *fp) {
}

void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx) {
	free(ctx);
}

void fake_aes_encrypt(
		const unsigned char* plaintext, unsigned long plaintext_len,
		const unsigned char* key, unsigned long key_len,
		unsigned char* ciphertext) {
	unsigned long key_idx = 0;

	for(int i = 0 ; i < plaintext_len ; i ++) {
		unsigned int p = plaintext[i];
		// rotare the byte one bit to the left

		p <<= 1;
		p |= ((p >> 8) & 1);
		p &= (1 << 8) - 1;

		//xor with key byte
		p ^= key[key_idx];
		key_idx ++;
		key_idx %= key_len;

		ciphertext[i] = p;
	}
}

void fake_aes_decrypt(
		const unsigned char* ciphertext, unsigned long ciphertext_len,
		const unsigned char* key, unsigned long key_len,
		unsigned char* plaintext) {
	unsigned long key_idx = 0;

	for(int i = 0 ; i < ciphertext_len ; i ++) {
		unsigned int p = ciphertext[i];

		//xor with key byte
		p ^= key[key_idx];
		key_idx ++;
		key_idx %= key_len;

		// rotate the byte one bit to the right
		int lsb = p & 1;
		p >>= 1;
		p |= (lsb << 7);

		plaintext[i] = p;
	}
}

void fake_des_encrypt(
		const unsigned char* plaintext, unsigned long plaintext_len,
		const unsigned char* key, unsigned long key_len,
		unsigned char* ciphertext) {
	unsigned long key_idx = 0;

	for(int i = 0 ; i < plaintext_len ; i ++) {
		unsigned int p = plaintext[i];

		// rotate the byte two bits to the left
		p <<= 2;
		p |= ((p >> 8) & 3);
		p &= (1 << 8) - 1;

		//xor with key byte
		p ^= key[key_idx];
		key_idx ++;
		key_idx %= key_len;

		ciphertext[i] = p;
	}
}

void fake_des_decrypt(
		const unsigned char* ciphertext, unsigned long ciphertext_len,
		const unsigned char* key, unsigned long key_len,
		unsigned char* plaintext) {
	unsigned long key_idx = 0;

	for(int i = 0 ; i < ciphertext_len ; i ++) {
		unsigned int p = ciphertext[i];

		//xor with key byte
		p ^= key[key_idx];
		key_idx ++;
		key_idx %= key_len;

		// rotare the byte two bits to the right
		int lsb = p & 3;
		p >>= 2;
		p |= (lsb << 6);

		plaintext[i] = p;
	}
}

struct my_ctx {
	// this is the struct I will use as context
	// instead of EVP_CIPHER_CTX
	const unsigned char* key;
	unsigned long key_len;
	const unsigned char* iv;
	const EVP_CIPHER * func; // I am going to put a pinter to the encrypt function to use for encryption here
};

EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void) {
	return (EVP_CIPHER_CTX *) malloc(sizeof(struct my_ctx));
}

int EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm,
		int *outl) {
	return 1;
}

int EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
		ENGINE *impl, const unsigned char *key, const unsigned char *iv) {
	return 1;
}

int EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
		int *outl, const unsigned char *in, int inl) {
	return 1;
}

int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
		int *outl) {
	 for(int i = 0 ; i < 8 ; i ++) {
		out[i] = 0xaa;
	 }
	 *outl = 8; // Make this update the output and not just do nothing
	 return 1;
}

int EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
		ENGINE *impl, const unsigned char *key, const unsigned char *iv) {
	struct my_ctx* c = (struct my_ctx*)ctx;
	c->key = key; // I hope that's OK I don't memcpy the key
	c->key_len = KEY_SIZE;
	c->iv = iv; // I hope it's OK I don't memcpy
	c->func = type;
	return 1;
}


typedef void (*func_fake_encrypt)(const unsigned char*, unsigned long, const unsigned char*, unsigned long, unsigned char*);

// this function is in superglue now because it needs to have 2 versions XXX
int EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
		int *outl, const unsigned char *in, int inl) {
	struct my_ctx* c = (struct my_ctx*)ctx;
	func_fake_encrypt encrypt_func = (func_fake_encrypt)c->func;
	encrypt_func(in, inl, c->key, c->key_len, out);
	*outl = inl;
	return 1;
}

const EVP_CIPHER * EVP_aes_256_cbc (void) {
	return (EVP_CIPHER*)fake_aes_encrypt;
}

const EVP_CIPHER * EVP_des_ede3_cbc (void) {
	return (EVP_CIPHER*)fake_des_encrypt;
}

//////////////////// LIBC ///////////////////////////////

//off_t lseek(int fd, off_t offset, int whence);
unsigned long lseek(int fd, unsigned long offset, int whence) {
	return offset;
}

//ssize_t write(int fd, const void *buf, size_t count);
unsigned long write(int fd, const void *buf, unsigned long count) {
	return count;
}

clock_t clock(void) {
	return 5;
}

int close(int fd) {
	return 0;
}

void err (int __status, const char *__format, ...) {
}

int fclose(FILE *stream) {
	return 0;
}

int fflush(FILE *stream) {
	return 0;
}

FILE log_file;
FILE *fopen(const char *restrict pathname, const char *restrict mode) {
	if(strcmp(pathname, "/tmp/challenge05.log") == 0) {
		return &log_file;
	}
	return NULL; // Fix this, because this will trigger a bug in code
}

int fprintf (FILE *__restrict __stream, const char *__restrict __fmt, ...) {
	return 0;
}

unsigned char output[OUTPUT_SIZE];
unsigned long output_idx = 0;

size_t fwrite(const void *restrict ptr, size_t size, size_t nitems,
		FILE *restrict stream) {
	if(stream == &log_file) {
		//printf("logging\n");
		for(int j = 0 ; j < nitems; j ++) {
			for(int i = 0 ; i < size; i ++) {
				unsigned char val = ((char*)ptr)[j*size + i];
				output[output_idx++] = val;
				assert(output_idx < OUTPUT_SIZE);
			}
		}
		return nitems;
	}
	return 0;
}

char *optarg = NULL;
int optind = 1;
int getopt(int argc, char *const argv[],
		const char *optstring) {
	return -1;
}

void perror(const char *s) {
}

ssize_t read(int fd, void *buf, size_t count) {
	assert(fd == 5); // 5 is our emulated socket for input
	assert(count == sizeof(struct can_frame));
	memcpy(buf, &symboic_can_frame[g_iteration_number - 1], sizeof(struct can_frame));
	// select() advances g_iteration_number because it
	// occurs on each iteration, so I need the previous iteration here
	return 8;
}

int select(int nfds, fd_set *restrict readfds,
		fd_set *restrict writefds, fd_set *restrict exceptfds,
		struct timeval *restrict timeout) {
	assert(g_iteration_number < NR_ITERATIONS);
	return symbolic_ready[g_iteration_number++];
	// needs to return 1 if there is data from CAN ready for reading
}

int setitimer(int which, const struct itimerval *new_value,
		struct itimerval *old_value) {
	return 0;
}

int sigaction(int signum, const struct sigaction *restrict act,
		struct sigaction *restrict oldact) {
	return 0;
}

int sigfillset(sigset_t *set) {
	return 0;
}

unsigned int sleep(unsigned int seconds) {
	return 0;
}

int socket(int domain, int type, int protocol) {
	if(domain == PF_CAN && type == SOCK_RAW && protocol == CAN_RAW) {
		return 5;
	}
	klee_assert(0);
	return 0;
}

FILE *stdin;		/* Standard input stream.  */
FILE *stdout;		/* Standard output stream.  */
FILE *stderr;		/* Standard error output stream.  */

int open(const char *pathname, int flags) {
	return 0;
	// the only function that calls `open' is `initialize_pins'
}

int bind(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen) {
	return 0;
}

int gettimeofday(struct timeval *restrict tv,
		struct timezone *restrict tz)
{
	return 0;
}
