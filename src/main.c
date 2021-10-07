#include "main.h"
#include "../glue.h" // Marina: added for NR_ITERATIONS

struct conf_t conf = {
    .claim_msg.can_id = 0x18EEFF00,
    .claim_msg.can_dlc = 8,
    .state = STATE_INITIAL,
    .current_sa  = DYNAMIC_MIN
};

void usage(char * program) {
    fprintf(stderr,"Usage %s [can_interface] [-f <path to logfile>]\n", program);
}

int main(int argc, char * argv[]){

    // Set defaults
    char * log_file = "/tmp/challenge05.log";
    char * iter_name = "vcan0";

    // Configure logfile path from commandline
    int opt;
    while ((opt = getopt(argc, argv, "hf:")) != -1) {
        switch(opt) {
            case 'f': 
                log_file = optarg;
                break;
            case 'h': 
                usage(argv[0]);
                exit(0);
            case '?': 
                usage(argv[0]);
                exit(1);
        }
    }
    for (uint8_t i = optind; i < argc; i++)
        iter_name = argv[i];
      
    // Setup logging
    logging_setup(log_file);

    // Init CAN Socket
    int s = socket (PF_CAN, SOCK_RAW, CAN_RAW);
    while (init_can(&s, iter_name) != 0) {
        printf("Failed to create socket\n");
        sleep(3000);
    }
    printf("CAN Socket created\n");

    // Init GPIO
    initialize_pins();
    for (int pin = 1; pin < 5; pin++) {
        set_period(pin, PWM_PERIOD_STR, sizeof(PWM_PERIOD_STR));
        set_duty(pin, PWM_LOW);
        set_power(pin, 1);
    }

    // Keep track of state
    struct Bumper* bumper;
    bumper = (struct Bumper*) malloc(sizeof(struct Bumper));
    init_bumper(bumper);

    // Setup Address Claim variables
    // Address claim NAME (data) describing device and priority
    conf.name = 0x110409000C020040;
    // array with each byte representing a potential address.
    int *addr_pool = calloc(J1939_IDLE_ADDR, sizeof(int));
    for (int i = DYNAMIC_MIN; i <= DYNAMIC_MAX; i++){ addr_pool[i] = F_USE; }
    addr_pool[conf.current_sa] &= !F_USE;


    // Install signals
    install_signal(SIGTERM);
    install_signal(SIGINT);
    install_signal(SIGALRM);

    // Send out initial address claim
    if (write(s, &conf.claim_msg, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        err(1, "could not sent initial request");
    }
    schedule_timer(250);

    // Variables for main loop
    int nfds = s + 1;
    fd_set  readfds;
    struct timeval timeout;
    struct can_frame cf;
    ssize_t nbytes;
    uint8_t led1, led2, led3, led4;
    clock_t report_timer = clock();
    double report_timer_diff;

    //while (!conf.sig_term) {
    for(int i = 0 ; i < NR_ITERATIONS ; i ++) { // Marina - we need to bound execution to something finite
        FD_ZERO(&readfds);
        FD_SET(s,&readfds);

        // Our tick if not ready
        timeout.tv_sec = 0;
        timeout.tv_usec = 50;  // milliseconds

        int ready = select(nfds, &readfds, NULL, NULL, &timeout);
        if ( ready > 0 ){
            nbytes = read(s, &cf, sizeof(struct can_frame));

            if (nbytes < 0 ) {
                perror("CAN read");
                return 1;
            }
            // Challenge 05 log handler
            logging_handler(cf);

            // first 3 bits Reserved for control flow; See https://www.kernel.org/doc/Documentation/networking/can.txt
            cf.can_id &= 0x1FFFFFFF;

            uint8_t priority, da, sa;
            uint32_t pgn;
            parse_J1939(cf.can_id, &priority, &pgn, &da, &sa);

            if (pgn == PGN_CruiseControlVehicleSpeed1) {
                rx_brake_routine(cf.data, bumper);
            } else if (pgn == PGN_OperatorsExternalLightControls) {
                rx_signal_routine(cf.data, bumper);
            } else if (pgn == J1939_PGN_CLAIM) {
                rx_claim_routine(sa, cf.data, s, addr_pool);
            } else if (pgn == J1939_PGN_REQUEST) {
                rx_request_routine(s, da);
            }
        }
        turn_signal_routine(bumper);
        brake_routine(bumper);
        // GPIO toggle
        led1 = bumper->outer_left;
        led2 = bumper->inner_left;
        led3 = bumper->inner_right;
        led4 = bumper->outer_right;
        set_duty(1, led1 ? PWM_HIGH : PWM_LOW );
        set_duty(2, led2 ? PWM_HIGH : PWM_LOW );
        set_duty(3, led3 ? PWM_HIGH : PWM_LOW );
        set_duty(4, led4 ? PWM_HIGH : PWM_LOW );
        printf("LEDs: [%s] [%s] [%s] [%s] State: [%d] SA: [%03d]\r",
            led1 ? "o" : "x",
            led2 ? "o" : "x",
            led3 ? "o" : "x",
            led4 ? "o" : "x",
            conf.state,
            conf.current_sa
        );
        // Challenge 04 Specific
        if (conf.state == STATE_INITIAL) {
            if (conf.sig_alrm){
                conf.state = STATE_OPERATIONAL;
            }
        } else if (conf.state == STATE_OPERATIONAL){
            // Send status update over CAN
            cf.data[0] = 0x00;
            cf.data[1] = led1 ? 0xFF : 0x00;
            cf.data[2] = 0x00;
            cf.data[3] = led2 ? 0xFF : 0x00;
            cf.data[4] = 0x00;
            cf.data[5] = led3 ? 0xFF : 0x00;
            cf.data[6] = 0x00;
            cf.data[7] = led4 ? 0xFF : 0x00;
            // CAN ID derived from J1939 spec, lowest priority with PGN 65088.
            cf.can_id = 0x1CFE4000U | (0xFF & conf.current_sa) | CAN_EFF_FLAG;
            cf.can_dlc = 8;
            report_timer_diff = ((double) (clock() - report_timer));
            if (report_timer_diff > 100000){
                if (write(s, &cf, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
                    err(1, "could not sent initial request");
                }
                report_timer = clock();
            }
        } else {
        /* State defunct, idle addr in use, no report */
        }
    }
    terminate_logging_gracefully();
    close(s);
}


/**
  * @brief  Handles the initialization of the  CAN socket. It applies a filter and binds the socket.
  *
  * @param  sock: a pointer to a socket, that will be initialized
  * @retval 0 on successful bind, -1 on failure
  */
int init_can( int *sock, char *interface ) {
    struct ifreq ifr;
    struct sockaddr_can addr;
    struct can_filter rfilter[2];
    int ro = LOGSELF;

    strcpy(ifr.ifr_name, interface);
    ioctl(*sock, SIOCGIFINDEX, &ifr);  // Retrieve the interface index of the interface into ifr_ifindex

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    setsockopt(*sock, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &ro, sizeof(ro));

    // We're going to be listening
    if (bind(*sock,(struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return -1;
    }
    return 0;
}


void rx_brake_routine( unsigned char buff[], struct Bumper *bumper ) {
    uint16_t speed_value;  
    uint8_t brake_switch;
    uint8_t prev_brake_state = bumper->brake_state;
    // Extract Speed and brake switch from frame
    speed_value  = (buff[3] << 8) + buff[2];  // buf[3] = speed integer, buf[2] = speed decimal
    brake_switch = (buff[4] & 0b00001100) >> 2;
    // update related bumper members
    bumper->brake_state = (brake_switch) ? 1 : 0;

    // This segment would ideally be moved to bumper method
    if (bumper->brake_state) {
        // Can also make speed != 0 here
        if ((speed_value > 0) && (!bumper->has_flashed)){  // speed > 0 and brakes were off last
            bumper->flash_lock = 1;
        }
    }
    else {
        bumper->flash_lock = 0;
        bumper->has_flashed = 0;
    }
}


void rx_signal_routine( unsigned char buff[], struct Bumper *bumper ) {
    bumper->signal = (buff[1] >>4);
}


// Challenge 04 specific functions
/**
  * @brief  This processes an incoming name request. Any name requests that overlaps with our
  *         dynamic range will be marked as in use, given they are higher priority. If a name
  *         request contains our current name, and has a higher priority, we relinquish our
  *         address and revert to an initial state with a new SA.
  *
  * @param can_id: The address of the incoming name request
  * @param buff:   The data of the request, in this case it will be priority
  * @param sock:   Socket to send claim on
  * @param avail_addrs: Pool of addresses with flag set to available or not
  */
void rx_claim_routine(uint8_t source_address, unsigned char NAME[], int sock, int *avail_addrs) {
    uint64_t contending_name = 0;   // will store priority of incoming claim

    memcpy(&contending_name, NAME, sizeof(contending_name));
    contending_name = be64toh(contending_name);

    if (source_address == conf.current_sa) {
        // priority is higher when the number is lower. In this case we loose
        if (conf.name > contending_name) {
            choose_new_sa(conf.current_sa, 0, avail_addrs);
            if (conf.current_sa == J1939_IDLE_ADDR) {
                conf.state = STATE_DEFUNCT;
                fprintf(stderr, "%s", "\nAddresses exhausted!\n");
            }
        }
        // Write out claim for new address
        conf.claim_msg.can_id = (conf.claim_msg.can_id & 0xffffff00) | conf.current_sa | CAN_EFF_FLAG;
        if (write(sock, &conf.claim_msg, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            err(1, "could not sent claim for request");
        }
    } else if ((DYNAMIC_MIN <= source_address) && (source_address <= DYNAMIC_MAX)) {
        // Someone is claiming a dynamic address, if they have higher priority, mark as claimed
        if (!(contending_name >= conf.name)) {
            avail_addrs[source_address] &= !F_USE;
        }
    }
}


/**
  * @brief  Function to determine if an incoming request is asking for us.
  *
  * @param sock: CAN socket for sending claim
  * @param destination: Destination address of request message
  */
void rx_request_routine (int sock, int destination) {
    if (destination == conf.current_sa || destination == 0xFF) {
        if (write(sock, &conf.claim_msg, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            err(1, "could not sent claim for request");
        }
    }
}


/**
  * @brief  Choose a new source address from the dynamic pool, recursively
        conf.current_sa = J1939_IDLE_ADDR;
  *
  * @param  current_index: The current index in the address pool
  * @param  iterations: The iterations past, shouldn't exceed pool size
  * @param  avail_addrs: A pool of ints representing available addresses
  */
void choose_new_sa(int current_index, int iterations, int *avail_addrs) {
    if (iterations > 0xFF) {
        return;
    }
    if (avail_addrs[current_index] == F_USE) {
        conf.current_sa = current_index;
        avail_addrs[current_index] &= !F_USE;
        return;
    }
    current_index++;
    iterations++;
    current_index = current_index % J1939_IDLE_ADDR;
    choose_new_sa(current_index, iterations, avail_addrs);
}


/**
  * @brief  Parses out the priority, pgn, destination address, and source address
  *         from a can_id.
  *
  * @param can_id: J1939 Can ID to parse
  * @param priority: returned priority value
  * @param pgn: returned Parameter Group Number
  * @param da: returned Destination Address
  * @param sa: returned Source Address
  */
void parse_J1939(uint32_t can_id, uint8_t *priority, uint32_t *pgn, uint8_t *da, uint8_t *sa) {
    // Parse J1939
    int pf, ps;
    // Priority
    *priority = (PRIORITY_MASK & can_id) >> 26;
    // Protocol Data Unit (PDU) Format
    pf = (PF_MASK & can_id) >> 16;
    // Protocol Data Unit (PDU) Specific
    ps = (PS_MASK & can_id) >> 8;
    // Determine the Parameter Group Number and Destination Address
    if (pf >= 0xF0) {
        // PDU 2 format, include the PS as a group extension
        *da = 255;
        *pgn = (can_id & PDU2_PGN_MASK) >> 8;
    } else {
        *da = ps;
        *pgn = (can_id & PDU1_PGN_MASK) >> 8;
    }
    // source address
    *sa = (can_id & SA_MASK);
}


/**
  * @brief  On getting a sig to kill program, set exit status to gracefully close program. sig_alrm is used for
  *         handling address claim timeouts
  *
  * @param  sig:  Signal given
  * @param  info: Additional signal info
  * @param  vp:   signal context structure
  */
static void sighandler(int sig, siginfo_t *info, void *vp) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:  // gracefully exit (close socket)
            conf.sig_term = 1;
            break;
        case SIGALRM:
            conf.sig_alrm = 1;
            break;
    }
}


/**
  * @brief  Installs signals with the sighandler function
  *
  * @param  sig:  Signal to install
  */
static void install_signal(int sig) {
    int ret;
    struct sigaction sigact = {
            .sa_sigaction = sighandler,
            .sa_flags = SA_SIGINFO,
    };

    sigfillset(&sigact.sa_mask);
    ret = sigaction(sig, &sigact, NULL);
    if (ret < 0)
        err(1, "sigaction for signal %i", sig);
}


/**
  * @brief  Sets up 250ms timer that we are out of operation. If it ends without a new request
  *         (address in use), then we proceed to operational state with the current source address
  *
  * @param  msec: Milliseconds set timer to
  */
static void schedule_timer(int msec) {
    int ret;
    struct itimerval val = {};

    val.it_value.tv_sec = msec / 1000;
    val.it_value.tv_usec = (msec % 1000) * 1000;

    conf.sig_alrm = 0;
    do {
        ret = setitimer(ITIMER_REAL, &val, NULL);
    } while ((ret < 0) && (errno == EINTR));
    if (ret < 0)
        err(1, "setitimer %i msec", msec);
}
