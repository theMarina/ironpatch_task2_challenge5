#ifndef AMP_CHALLENGE_05_CAN_LOG_ENCRYPTION_CONF_H
#define AMP_CHALLENGE_05_CAN_LOG_ENCRYPTION_CONF_H

/* This will enable a call to fflush after every write operation, eliminating buffering */
#define FLUSH_AT_RUNTIME 1
/* This is the amount of CAN frames that will be allocated in the Mblock buffer */
#define BUFFER_SIZE 2
/* Setting LOGSELF to 1 will enable the logging of sent CAN messages as well */
#define LOGSELF 0

#endif /*AMP_CHALLENGE_05_CAN_LOG_ENCRYPTION_CONF_H */
