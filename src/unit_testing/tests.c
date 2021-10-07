
#include "munit.h"
#include "main.h"
#include "bumper.h"

#define DES 1
#define AES 2

// Made this no longer static in main.c so we
// can access and test it without modifying.
// Initially used ifndef TESTING but this would give
// different sort of important variable for performers to evaluate.
// Tough decision. I may revist to see if there is any .o hackage 
// to be done later.
extern struct conf_t conf;

// Munit logging is not working easily
// Fixed. Issue was requires cmdline --show-stderr as well as the --log-visible <level>
// However, this will go to stdout and thus not get buffered in case required
void dlog(const char * fmt,...) {
  va_list valist;
  puts("\n--------------------------------------------");
  va_start(valist,fmt);
  vprintf(fmt,valist);
  va_end(valist);
  puts("\n--------------------------------------------");
}

inline void zeroit(uint8_t * buf, int size) __attribute__ ((always_inline)); 
void zeroit(uint8_t * buf, int size) {
  for (;size--;)
    buf[size] = 0;
}

/*********************************************************************/

static MunitResult
test_check(const MunitParameter params[], void* user_data) {
  (void) params;
  (void) user_data;
  // Simple test to see if the framework is working
  return MUNIT_OK;
}

//void rx_signal_routine( unsigned char buff[], struct Bumper *bumper );
/* The tested routine here does not do too much, but therefore is a good
   candidate for testing framework idiosyncrasies */
static MunitResult
test_rx_signal_routine(const MunitParameter params[], void* user_data) {
  (void) params;
  (void) user_data;

  unsigned char buff[20] = {0};
  struct Bumper * bumper;
  bumper = (struct Bumper*) malloc(sizeof(struct Bumper));

  init_bumper(bumper);
  munit_assert_not_null(bumper);

  // Just test a bunch of randoms
  for (int i=0; i<0x64; i++) {
    buff[1] = (char)munit_rand_int_range(0, 255);
    rx_signal_routine(buff,bumper);
    munit_assert_not_null(buff);
    munit_assert_uchar((buff[1]>>4), ==, bumper->signal);
  }

  return MUNIT_OK;
}

//void rx_brake_routine( unsigned char buff[], struct Bumper *bumper )
static MunitResult
test_rx_brake_routine(const MunitParameter params[], void* user_data) {
  (void) params;
  (void) user_data;
  
#define BRAKE_ON 12
#define BRAKE_OFF 0
  unsigned char buff[20] = {0};
  struct Bumper * bumper;
  bumper = (struct Bumper*) malloc(sizeof(struct Bumper));
  init_bumper(bumper);
  munit_assert_not_null(bumper);

  // Lets do these manually to check better for different states
  
  bumper->has_flashed = 0;
  buff[3] = 0 ;// Speed
  buff[4] = BRAKE_OFF; // Brake switch
  rx_brake_routine(buff,bumper);
  munit_assert_uchar((buff[3]<<8), ==, 0);
  munit_assert_uchar(bumper->flash_lock,==,0);

  bumper->has_flashed = 0;
  buff[3] = 15 ;// Speed
  buff[4] = BRAKE_OFF;
  rx_brake_routine(buff,bumper);
  munit_assert_int((buff[3]<<8), >, 0);
  munit_assert_uchar(bumper->flash_lock,==,0);

  bumper->has_flashed = 0;
  buff[3] = 15 ;// Speed
  buff[4] = BRAKE_ON;
  rx_brake_routine(buff,bumper);
  munit_assert_int((buff[3]<<8), >, 0);
  munit_assert_uchar(bumper->flash_lock,==,1);

  bumper->has_flashed = 0;
  buff[3] = 127 ;// Speed
  buff[4] = BRAKE_ON; 
  rx_brake_routine(buff,bumper);
  munit_assert_int((buff[3]<<8), >, 0);
  munit_assert_uchar(bumper->flash_lock,==,1);

  bumper->has_flashed = 1;
  buff[3] = 127 ;// Speed
  buff[4] = BRAKE_OFF; 
  rx_brake_routine(buff,bumper);
  munit_assert_int((buff[3]<<8), >, 0);
  munit_assert_uchar(bumper->flash_lock,==,0);

  bumper->has_flashed = 0;
  buff[2] = 40; // Decimal speed
  buff[3] = 128 ;// Speed
  buff[4] = BRAKE_ON;
  // Be sure systems acts as we'd expect
  munit_assert_short((buff[3]<<8), <, 0);
  rx_brake_routine(buff,bumper);
  munit_assert_uchar(bumper->flash_lock,==,1);

  bumper->has_flashed = 0;
  buff[3] = 253 ;// Speed
  buff[4] = BRAKE_ON;
  rx_brake_routine(buff,bumper);
  munit_assert_uchar(bumper->flash_lock,==,1);

  /* These last two may need to be undefined */

  bumper->has_flashed = 0;
  buff[3] = 254 ;// Speed
  buff[4] = BRAKE_ON;
  rx_brake_routine(buff,bumper);
  munit_assert_uchar(bumper->flash_lock,==,1);

  bumper->has_flashed = 0;
  buff[3] = 255 ;// Speed
  buff[4] = BRAKE_OFF;
  rx_brake_routine(buff,bumper);
  munit_assert_uchar(bumper->flash_lock,==,0);

  return MUNIT_OK;
}

//void choose_new_sa(int current_index, int iterations, int *avail_addrs)
static MunitResult
test_choose_new_sa(const MunitParameter params[], void* user_data) {
  (void) params;
  (void) user_data;

  int curr_idx, iters, *addr_pool;
  uint8_t prev;

  // Initialize our junk
  addr_pool = calloc(J1939_IDLE_ADDR, sizeof(int));
  munit_assert_ptr_not_null(addr_pool);
  for (int i = DYNAMIC_MIN; i < DYNAMIC_MAX; i++){ addr_pool[i] = F_USE; }
  addr_pool[conf.current_sa] &= !F_USE;
  munit_assert_uchar(conf.current_sa,==,DYNAMIC_MIN);

  munit_logf(MUNIT_LOG_DEBUG,"munit: conf.current_sa %d\n",conf.current_sa);

  // Be sure we get back a new address
  munit_logf(MUNIT_LOG_DEBUG,"munit: conf.current_sa %d\n",conf.current_sa);
  iters = 0;
  conf.current_sa = DYNAMIC_MIN;
  curr_idx = conf.current_sa;
  prev = conf.current_sa;
  choose_new_sa(curr_idx, iters, addr_pool);
  munit_assert_uchar(conf.current_sa,!=,prev);

  // Be sure claimed address is updated in pool
  munit_logf(MUNIT_LOG_DEBUG,"munit: conf.current_sa %d\n",conf.current_sa);
  conf.current_sa = 253;
  curr_idx = 19;
  addr_pool[curr_idx] = F_USE;
  choose_new_sa(curr_idx, iters, addr_pool);
  munit_assert_uchar(addr_pool[curr_idx]&F_USE,==,!F_USE);

  // Anything ouside of dynamic range
  munit_logf(MUNIT_LOG_DEBUG,"munit: conf.current_sa %d\n",conf.current_sa);
  iters = 0xFF+15 ;
  conf.current_sa = 12; 
  choose_new_sa(curr_idx, iters, addr_pool);
  // Patch should make it so that we don't hit this spot
  munit_assert_uchar(conf.current_sa,!=,J1939_IDLE_ADDR);
  // Not going to leave in the individual patch dependencies
  //munit_assert_uchar(conf.current_sa,<=,72);
  //munit_assert_uchar(conf.current_sa,>=,70);

  // Idx too great
  munit_logf(MUNIT_LOG_DEBUG,"munit: conf.current_sa %d\n",conf.current_sa);
  iters = 15;
  conf.current_sa = 19;
  curr_idx = 255 * 2;
  choose_new_sa(curr_idx, iters, addr_pool);
  munit_assert_uchar(conf.current_sa,<=,J1939_IDLE_ADDR);

  munit_logf(MUNIT_LOG_DEBUG,"munit: conf.current_sa %d\n",conf.current_sa);

  return MUNIT_OK;
}

//void rx_claim_routine(uint8_t source_address, unsigned char NAME[], int sock, int *avail_addrs) {
static MunitResult
test_rx_claim_routine(const MunitParameter params[], void* user_data) {
  (void) params;
  (void) user_data;

  uint64_t can_id, contending_name;
  unsigned char buff[20] = {0}; 
  uint8_t sa, prev;
  int sock, *addr_pool;
  FILE * fp;
  uint8_t tbuf[sizeof(conf.claim_msg)+1] = {0};

  // Initialization
  addr_pool = calloc(J1939_IDLE_ADDR, sizeof(int));
  munit_assert_ptr_not_null(addr_pool);
  for (int i = DYNAMIC_MIN; i < DYNAMIC_MAX; i++){ addr_pool[i] = F_USE; }
  addr_pool[conf.current_sa] &= !F_USE;

  // sa != conf.current_sa and contending name has higher priority, but not in dynamic range
  fp = fopen("/tmp/munit_testing","w+");
  sock = fileno(fp);
  if (sock == -1){ puts("\nNO CAN DO");return MUNIT_FAIL;}
  conf.name = 0xDEADBEEFCAFEBABE;
  contending_name = 0x8008580085800858;
  for (int j=0; j<8;j++)
    buff[8-1-j] = (unsigned char)(contending_name >> (j*8));
  prev = conf.current_sa;
  sa = 199;
  rx_claim_routine(sa, buff, sock, addr_pool);
  fseek(fp,0,SEEK_SET);
  munit_assert_uchar(prev,==,conf.current_sa);
  munit_assert_uchar(addr_pool[sa],==,!F_USE);
  fclose(fp);

  // sa != conf.current_sa , contending name has higher priority
  //  but sa is not in dynamic range
  fp = fopen("/tmp/munit_testing","w+");
  sock = fileno(fp);
  if (sock == -1){ puts("\nNO CAN DO");return MUNIT_FAIL;}
  conf.name = 0xDEADBEEFCAFEBABE;
  contending_name = 0x8008580085800858;
  for (int j=0; j<8;j++)
    buff[8-1-j] = (unsigned char)(contending_name >> (j*8));
  prev = conf.current_sa;
  sa = 67;
  rx_claim_routine(sa, buff, sock, addr_pool);
  fseek(fp,0,SEEK_SET);
  munit_assert_uchar(prev,==,conf.current_sa);
  munit_assert_uchar(addr_pool[sa],==,0);
  fclose(fp);

  // sa == conf.current_sa , contending name has higher priority
  fp = fopen("/tmp/munit_testing","w+");
  zeroit(tbuf,sizeof(conf.claim_msg)+1);
  sock = fileno(fp);
  if (sock == -1){ puts("\nNO CAN DO");return MUNIT_FAIL;}
  conf.name = 0xDEADBEEFCAFEBABE;
  contending_name = 0x8008580085800858;
  for (int j=0; j<8;j++)
    buff[8-1-j] = (unsigned char)(contending_name >> (j*8));
  prev = conf.current_sa;
  sa = conf.current_sa;
  rx_claim_routine(sa, buff, sock, addr_pool);
  fseek(fp,0,SEEK_SET);
  fread(tbuf,sizeof(conf.claim_msg),1,fp);
  munit_assert_uchar(prev,!=,conf.current_sa);
  munit_assert_uchar(addr_pool[sa],==,!F_USE);
  munit_assert_memory_equal(sizeof(conf.claim_msg),&conf.claim_msg,tbuf);
  fclose(fp);

  // Push to defunct state if not patched
  fp = fopen("/tmp/munit_testing","w+");
  sock = fileno(fp);
  if (sock == -1){ puts("\nNO CAN DO");return MUNIT_FAIL;}
  // All dyn addresses have been claimed
  for (int i = DYNAMIC_MIN; i < DYNAMIC_MAX; i++){ addr_pool[i] &= !F_USE; }
  conf.current_sa = DYNAMIC_MAX;
  conf.name = 0xDEADBEEFCAFEBABE;
  contending_name = 0x8008580085800858;
  for (int j=0; j<8;j++)
    buff[8-1-j] = (unsigned char)(contending_name >> (j*8));
  prev = conf.current_sa;
  sa = conf.current_sa;
  rx_claim_routine(sa, buff, sock, addr_pool);
  fseek(fp,0,SEEK_SET);
  munit_assert_uchar(conf.state,!=,STATE_DEFUNCT);
  munit_assert_uchar(conf.current_sa,!=,J1939_IDLE_ADDR);
  // Re-init claimed pool
  for (int i = DYNAMIC_MIN; i < DYNAMIC_MAX; i++){ addr_pool[i] &= F_USE; }
  fclose(fp);


  return MUNIT_OK;
}

//void rx_request_routine (int sock, int destination) {
static MunitResult
test_rx_request_routine(const MunitParameter params[], void* user_data) {
  (void) params;
  (void) user_data;
 
  int sock, dest;
  FILE * fp;
  uint8_t tbuf[sizeof(conf.claim_msg)+1] = {0};

  // Dest is us
  fp = fopen("/tmp/munit_testing","w+");
  sock = fileno(fp);
  if (sock == -1){ puts("\nNO CAN DO");return MUNIT_FAIL;}
  conf.current_sa = DYNAMIC_MIN;
  dest = conf.current_sa;
  rx_request_routine (sock, dest);
  fseek(fp,0,SEEK_SET);
  fread(tbuf,sizeof(conf.claim_msg),1,fp);
  munit_assert_memory_equal(sizeof(conf.claim_msg),&conf.claim_msg,tbuf);
  fclose(fp);
  zeroit(tbuf,sizeof(conf.claim_msg)+1);

  // dest is bcast
  fp = fopen("/tmp/munit_testing","w+");
  sock = fileno(fp);
  if (sock == -1){ puts("\nNO CAN DO");return MUNIT_FAIL;}
  conf.current_sa = DYNAMIC_MIN;
  dest = 0xFF;
  rx_request_routine (sock, dest);
  fseek(fp,0,SEEK_SET);
  fread(tbuf,sizeof(conf.claim_msg),1,fp);
  munit_assert_memory_equal(sizeof(conf.claim_msg),&conf.claim_msg,tbuf);
  fclose(fp);
  zeroit(tbuf,sizeof(conf.claim_msg)+1);

  // neither of the upper two 
  fp = fopen("/tmp/munit_testing","w+");
  sock = fileno(fp);
  ftruncate(sock,0);
  if (sock == -1){ puts("\nNO CAN DO");return MUNIT_FAIL;}
  conf.current_sa = DYNAMIC_MIN;
  dest = 0xF0;
  rx_request_routine (sock, dest);
  fseek(fp,0,SEEK_SET);
  fread(tbuf,sizeof(conf.claim_msg),1,fp);
  munit_assert_memory_not_equal(sizeof(conf.claim_msg),&conf.claim_msg,tbuf);
  fclose(fp);
  zeroit(tbuf,sizeof(conf.claim_msg)+1);

  return MUNIT_OK;
}

static MunitResult
test_des_routine(const MunitParameter params[], void* user_data) {
  (void) params;
  (void) user_data;
 
  // This will get run when tests compiled with -DENC=1
   
  unsigned char key[24] = {0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3};
  unsigned char iv[8] = {0,1,2,3,4,5,6,7};
  unsigned char *plaintext =
      (unsigned char *)"The quick brown fox jumps over the lazy dog";
  size_t plaintext_len = strlen(plaintext);
  unsigned char * decryptedtext = malloc(plaintext_len+4); 
  unsigned char *ciphertext = malloc(plaintext_len + 16);
  int ciphertext_len = encrypt(plaintext,plaintext_len,
                               key,
                               iv,
                               ciphertext);
  int decryptedtext_len = decrypt(ciphertext, ciphertext_len,
                               key, iv,
                               decryptedtext);
  if (strncmp(plaintext,decryptedtext,decryptedtext_len) == 0)
    return MUNIT_OK;
  else
    return MUNIT_FAIL;
}

static MunitResult
test_aes_routine(const MunitParameter params[], void* user_data) {
  (void) params;
  (void) user_data;
  // This will get run when tests compiled with -DENC=2

  /* A 256 bit key */
  unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
  /* A 128 bit IV */
  unsigned char *iv = (unsigned char *)"0123456789012345";

  unsigned char *plaintext =
       (unsigned char *)"The quick brown fox jumps over the lazy dog";
  size_t plaintext_len = strlen(plaintext);
  unsigned char * decryptedtext = malloc(plaintext_len+4); 
  unsigned char *ciphertext = malloc(plaintext_len + 16);
  int ciphertext_len = encrypt(plaintext,plaintext_len,
                               key,
                               iv,
                               ciphertext);
  int decryptedtext_len = decrypt(ciphertext, ciphertext_len,
                               key, iv,
                               decryptedtext);
  if (strncmp(plaintext,decryptedtext,decryptedtext_len) == 0)
    return MUNIT_OK;
  else
    return MUNIT_FAIL;
}

static MunitTest test_suite_tests[] = {
  {
    (char*) "check", // Test name
    test_check, // Function for the test name
    NULL,//test_compare_setup, // Setup function
    NULL,//test_compare_tear_down, // Teardown function
    MUNIT_TEST_OPTION_NONE, // Which option
    NULL // Params
  },
  {
    (char*) "rx_signal_routine",
    test_rx_signal_routine,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  {
    (char*) "rx_brake_routine",
    test_rx_brake_routine,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  {
    (char*) "choose_new_sa",
    test_choose_new_sa,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  {
    (char*) "rx_request_routine",
    test_rx_request_routine,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  {
    (char*) "rx_claim_routine",
    test_rx_claim_routine,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
#if ENC == AES
  {
    (char*) "aes_routine",
    test_aes_routine,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
#elif ENC == DES
  {
    (char*) "des_routine",
    test_des_routine,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
#endif
// Nulls mean the end of suite
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};


/*********************************************************************/
static const MunitSuite test_suite = {
  (char*) "Challenge_05/",
  /* The first parameter is the array of test suites. */
  test_suite_tests,
  NULL, // Other suites to include
  1,  //  Runs
  MUNIT_SUITE_OPTION_NONE
};


#include <stdlib.h>
int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}
