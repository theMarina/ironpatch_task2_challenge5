# University of Michigan Cahallenge 5 Subtask II
## Running the code
```
cd symcmp
make
klee linked.bc 
```
Here is a [video of me executing the code](https://drive.google.com/file/d/1psbU6Ns0bt4Jwxffq1HEIq9T2Aq2GE4A/view?usp=sharing).

To run the code in a VM that has all the enviroment installed, you can use
this [Virtual Box image](https://drive.google.com/file/d/1i26PqOXh4j9cKXcOpFdKoRYp8FE50kKT/view?usp=sharing). (Username and password are both `mini`)
Here is [a video of booting this VM](https://drive.google.com/file/d/1PbCelmncdE7HDVt8gKhy7haxfVVdz0Oi/view?usp=sharing)

## Symbolic coparison of patched and vulnerable version
In our symbolic comparison approach, we pack both patched and unpatched versions into the same executable (after changing global symbol names to avoid duplicates).
Both versions receive the same symbolic inputs. Each version encrypts its output using the corresponding openssl function, and writes the output into a file using the function `fwrite`.

### Stubbing out library functions
The challenge code performs calls to C library functions as well as to openssl functions. We modelled these functions to allow correct execution of the challenge code and model IO to receive symbolic input from the CAN bus as well as saving the output into global variables. All of these stubs are in the file [glue.c](glue.c).
Additionally, to avoid bound symbolic execution, we limited the loop in the main function to a hard-coded amount of iterations. The limit is defined in the file [glue.h](glue.h)

### Symbolically executing encryption
In this challane, we want to symbolically compare the result of symbolically enrypting and decrypting data in two version of the code. This computation is not feasible with AES and DES computation. This is why we implemented our symplified "encryption" functions: `fake_aes_encrypt`, `fake_des_encrypt`, `fake_aes_decrypt`, and `fake_des_decrypt`.
These functions provide similar functionality to actual encryption and decription, while a symbolic execution engine is able to compute them symbolically: our stub for AES encryption shifts each input byte 1 bit to the left, and xors the result with the key. Our sub for DES operates similarly, but shifts each input bit twice to the left. We notice that for the challenge problem, encryption results of these functions differ, which allows up to verify that the patch replaced the function.

### Verifying the output
The challenge code encrypts the output, but it never decrypts it. While the challange provides us with a python script to verify output, it does not work with our stubs for cryptographic functions. Additionally, the KLEE symbolic enviroment does not support python code. This is why we implemented a similar checker in C and located it in the file [superglue.c](symcmp/superglue.c). The code calls this chacker after symbolically executing both version of the code, and compares output from both versions.

## Bugs we discovered
While working on this challenge, we discovered addiotional bugs that are present in both patched and unpatched version of the challenge code:

### [Bug 1](https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/main.c#L66)
```
int *addr_pool = calloc(J1939_IDLE_ADDR, sizeof(int));
for (int i = DYNAMIC_MIN; i <= DYNAMIC_MAX; i++){ addr_pool[i] = F_USE; }
addr_pool[conf.current_sa] &= !F_USE;
```
If the `calloc` on the first line return `NULL`, the 3rd line is a null pointer dereference..

### [Bug 2](https://github.com/Assured-Micropatching/Challenge-Problems/blame/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/unit_testing/tests.c#L55)
```
struct Bumper * bumper;
bumper = (struct Bumper*) malloc(sizeof(struct Bumper));

init_bumper(bumper);
munit_assert_not_null(bumper);
```
malloc data into bumber, init bumper (maybe null pointer dereference), check if bumper was null

### [Bug 3](https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/main.c#L59)
```
bumper = (struct Bumper*) malloc(sizeof(struct Bumper));
init_bumper(bumper);
```
[init_bumper](https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/bumper.c#L3) can cause null pointer dereference if malloc fails


### [Bug 4](https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/logging.c#L165)
```
fclose(wfd);
```
This created a bug if wfd is NULL, because `fopen` does not exit on error (it does print an error message) <https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/logging.c#L67>. Also, `fwrite` might fail before the `fclose` <https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/logging.c#L90>

### [Bug 5](https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/logging.c#L86), [same bug again](https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/EVP_des_ede3_cbc.c#L9)
```
if (!RAND_bytes(key, sizeof key)) {
	handleErrors();
}
```
From the [documentation](https://www.openssl.org/docs/man1.0.2/man3/RAND_bytes.html) of this function:
"Both functions return -1 if they are not supported by the current RAND method."

### [Bug 6](https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/logging.c#L106)
```
unsigned char *plaintext = malloc(sizeof(mblock));
memcpy(plaintext, &mblock, sizeof(mblock));
```
no check if `malloc` failed. There is another one [a few lines below that](https://github.com/Assured-Micropatching/Challenge-Problems/blob/f5baea56373fcdb55e659aa785d213eb55d6d030/Challenge_05/program_c/src/logging.c#L109)

### [Bug 7](https://github.com/Assured-Micropatching/Challenge-Problems/blob/master/Challenge_05/program_c/src/main.c#L102)
```
nbytes = read(s, &cf, sizeof(struct can_frame));

if (nbytes < 0 ) {
	perror("CAN read");
	return 1;
}
```
CAN bus is allowed to send messages of 8 bytes or less- if `read` returns a values between 1 and 7, the code might use uninitialized memory or data a from previous CAN message.

### [Bug 8](https://github.com/Assured-Micropatching/Challenge-Problems/blob/master/Challenge_05/program_c/src/logging.c#L86)
```
if (!RAND_bytes(crypto_param, size)) {
	handleErrors();
}
```
According to the [specification](https://www.openssl.org/docs/man1.0.2/man3/RAND_bytes.html), `RAND_bytes` might return -1 in some error case. This will make the software use a non random key or IV.
