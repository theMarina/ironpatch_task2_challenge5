#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include <linux/can.h>
#include <linux/can/raw.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <stdint.h>
// Challenge 04 new includes
#include <signal.h>
#include <err.h>
#include <errno.h>
#include <sys/time.h>

#include "gpio.h"
#include "bumper.h"
#include "local_defs.h"

// Challenge 05 new includes
#include "logging.h"

#define F_USE	1

// Address claim global
#define STATE_INITIAL 1
#define STATE_OPERATIONAL 2
#define STATE_DEFUNCT 3

typedef struct conf_t {
    const char *intf;
    char *ranges;
    uint64_t name;
    // Current Source Address
    uint8_t current_sa;  
    int sig_term;
    int sig_alrm;
    int state;
    struct can_frame claim_msg;
} Conf ;

int init_can(int *sock, char *interface);

// Routine for handling incoming brake CAN frames, vulnerability exists here
void rx_brake_routine( unsigned char buff[], struct Bumper *bumper );
// Routine for handling incoming turn signal CAN frames
void rx_signal_routine( unsigned char buff[], struct Bumper *bumper );

// Challenge 04 Specific
// Used with interrupt to change state to operational if no counter claim was given within 250ms
static void schedule_timer(int msec);
// Installs signals for use cases such as the timer
static void install_signal(int sig);
// A recursive function to choose a new source address from an available pool
void choose_new_sa(int current_index, int iterations, int *avail_addrs);
// The routine ran when an address claim message is received. Handles incoming address claim requests
void rx_claim_routine(uint8_t source_address, unsigned char NAME[], int sock, int *avail_addrs);
// The routine ran when an address request message is received. Handles requests for our address
void rx_request_routine (int sock, int destination);
// Parses J1939 IDs. Returns priority, pgn, destination, source
void parse_J1939(uint32_t can_id, uint8_t *priority, uint32_t *pgn, uint8_t *da, uint8_t *sa);
#endif /* SRC_MAIN_H_ */
