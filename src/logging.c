#include "logging.h"

/* Global variables */
static uint32_t current_frame_count = 0;
static FILE * wfd = NULL;
static struct MBlock mblock = {
        . generation = "CAN2",
        . version = "TU2",
        . logger_number = "12",
        . file_number = "AMP",
        . micro_of_sdcard = "N/A"
};

/* Key used for the encryption routines */
static unsigned char KEY[KEY_SIZE];
/* IV used for encryption routines */
static unsigned char IV[IV_SIZE];


/* The CRC functions are obtained from http://home.thep.lu.se/~bjorn/crc/ */
uint32_t crc32_for_byte(uint32_t r) {
    for(int j = 0; j < 8; ++j)
        r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
    return r ^ (uint32_t)0xFF000000L;
}


/**
  * @brief  CRC32 function (used on Mblock)
  *
  * @param  data:  Pointer to data to be CRC'd
  * @param  n_bytes:  Amount of bytes in the buffer to iterate over
  * @param  crc:  CRC to store final value
  */
void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
    static uint32_t table[0x100];
    if(!*table) {
        for(size_t i = 0; i < 0x100; ++i) {
            table[i] = crc32_for_byte(i);
        }
    }
    for(size_t i = 0; i < n_bytes; ++i) {
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
    }
}


/**
  * @brief  Resets iterative values in the Mblock structure
  *
  */
void reset_mblock(){
    /* Zero our Mblock fields */
    memset(mblock.frames, 0, BUFFER_SIZE*sizeof(struct can_frame));
    memset(mblock.time_stamps, 0, BUFFER_SIZE*sizeof(struct timeval));
    mblock.crc32 = 0;
}


/**
  * @brief  Setup function for the logger, opens a file and initializes the encryption parameters
  *
  * @param  log_name:  Name of the log file that Mblocks should be logged to
  */
void logging_setup(char * log_name){
    /* Open File Descriptor */
     if ((wfd = fopen(log_name, "wb")) == NULL)
            perror("Could not create initial descriptor for %s");
    /* Initialize the tally counts */
    memset(mblock.rx_counts, 0, sizeof (mblock.rx_counts));
    memset(mblock.can_rx_err_counts, 0, sizeof (mblock.can_rx_err_counts));
    memset(mblock.can_tx_err_counts, 0, sizeof (mblock.can_tx_err_counts));
    /* Setup and write Key and IV */
    initialize_write(KEY, KEY_SIZE);
    initialize_write(IV, IV_SIZE);
}


/**
  * @brief  Fills the crypto_param with RAND bytes and writes the param out to logging file
  *
  * @param  crypto_param: This is a crypto related value that needs to be randomized and written to a file
  * @param  size: Size of the crypto_param
  */
void initialize_write(unsigned char* crypto_param, int size) {
    if (!RAND_bytes(crypto_param, size)) {
        handleErrors();
    }
    /* Write out key length & key */
    if (fwrite(&size, 1, sizeof(int), wfd) < 1) {
        perror("Unable to write crypto param size to file");
    }
    if (fwrite(crypto_param, 1, size, wfd) < 1) {
        perror("Unable to write crypto param to file");
    }
}


/**
  * @brief  Encrypts the Mblock and writes it to a file.
  *
  */
void write_encrypted(void) {
    int bytes_written = 0;
    /* Allocate plaintext buffer, and fill with Mblock data */
    unsigned char *plaintext = malloc(sizeof(mblock));
    memcpy(plaintext, &mblock, sizeof(mblock));
    /* Allocate buffer for returning ciphertext, plus some extra bytes */
    unsigned char *ciphertext = malloc(sizeof(mblock) + EXTRA_ENC_BUFF);
    /* Pass plaintext to encryption algorithm */
    bytes_written = encrypt(plaintext, sizeof(mblock), KEY, IV, ciphertext);
    /* Write Mblock Length:Mblock */
    if (fwrite(&bytes_written, sizeof(int), 1, wfd) < 1) {
        perror("Could not write tag length");
    }
    if (fwrite(ciphertext, 1, bytes_written, wfd) < 1) {
        perror("Could not write ciphertext of length");
    }
    free(plaintext);
    free(ciphertext);
    initialize_write(IV, IV_SIZE);
}


/**
  * @brief  Logging Handler, takes a CAN frame, and proceeds to store the frame in the Mblock structure
  *         and writing to file if full
  *
  * @param  read_frame: The incoming can frame to be logged
  */
void logging_handler(struct can_frame read_frame) {
    memcpy(&mblock.frames[current_frame_count], &read_frame, sizeof(struct can_frame));
    gettimeofday(&mblock.time_stamps[current_frame_count], NULL);

    current_frame_count++;
    mblock.rx_counts[0]++;
    if (current_frame_count >= BUFFER_SIZE){
        /* Update CRC */
        crc32(&mblock, sizeof (mblock), &mblock.crc32);
        /* Dump into log */
        write_encrypted();

        if (FLUSH_AT_RUNTIME == 1) {
            if (fflush(wfd)) {
                perror("Could not flush CAN data log");
            }
        }
        /* Refresh */
        reset_mblock();
        current_frame_count = 0;
    }
}


/**
  * @brief  Call on program exit, will close file properly, ensuring all data is logged out
  *
  */
void terminate_logging_gracefully() {
    /* Check if we have frames to write out */
    if (current_frame_count > 0) {
        write_encrypted();
    }
    /* fclose will also flush */
    fclose(wfd);
}
