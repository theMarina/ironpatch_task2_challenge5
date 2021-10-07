// Header file to store all common defs
#ifndef LOCAL_DEFS_H
#define LOCAL_DEFS_H

/* PWM Driver */
#define PWM_PERIOD 1000
#define PWM_LOW 10
#define PWM_HIGH 100
#define PWM_PERIOD_STR "1000"
#define PWM_LOW_STR "10"
#define PWM_HIGH_STR "100"

// These are the PGN values, not the EXT ID values. The PGN is extracted
// and the priority/source address are ignored. (We listen for broadcasts)
#define PGN_CruiseControlVehicleSpeed1     0xFEF1
#define PGN_OperatorsExternalLightControls 0xFDCC
#define J1939_PGN_REQUEST 0xEA00
#define J1939_PGN_CLAIM   0xEE00

#define PRIORITY_MASK  0x1C000000
#define EDP_MASK       0x02000000
#define DP_MASK        0x01000000
#define PF_MASK        0x00FF0000
#define PS_MASK        0x0000FF00
#define SA_MASK        0x000000FF
#define PDU1_PGN_MASK  0x03FF0000
#define PDU2_PGN_MASK  0x03FFFF00

// Address Claim related values
#define J1939_IDLE_ADDR 0xFE
#define DYNAMIC_MIN 127
#define DYNAMIC_MAX 247

#endif /* LOCAL_DEFS_H */
