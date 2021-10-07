#ifndef AMP_CHALLENGE_05_CAN_LOG_ENCRYPTION_LOGGING_H
#define AMP_CHALLENGE_05_CAN_LOG_ENCRYPTION_LOGGING_H

#include "conf.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <linux/can.h>
#include <sys/time.h>
#include <stdint.h>
#include "EVP_des_ede3_cbc.h"

struct MBlock{
    char generation[4];
    struct can_frame frames[BUFFER_SIZE];
    struct timeval time_stamps[BUFFER_SIZE];
    uint32_t rx_counts[3];
    uint8_t can_rx_err_counts[3];
    uint8_t can_tx_err_counts[3];
    char version[3];
    char logger_number[2];
    char file_number[3];
    char micro_of_sdcard[3];
    uint32_t crc32;
};

#define KEY_SIZE 24
#define IV_SIZE 8
#define EXTRA_ENC_BUFF 8

/* Functions */
void logging_setup(char * log_name);
void logging_handler(struct can_frame read_frame);
void terminate_logging_gracefully();
void log_to_file();
void reset_mblock();
uint32_t crc32_for_byte(uint32_t r);
void crc32(const void *data, size_t n_bytes, uint32_t* crc);
void initialize_write(unsigned char * crypto_param, int size);
void write_encrypted(void);

#endif /*AMP_CHALLENGE_05_CAN_LOG_ENCRYPTION_LOGGING_H */