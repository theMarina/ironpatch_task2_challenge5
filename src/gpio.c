
#include "gpio.h"

pin_t pinOL; // pin 46
pin_t pinIL; // pin 34
pin_t pinIR; // pin 36
pin_t pinOR; // pin 45

void initialize_pins() {
    pinOL.state = open("/sys/devices/platform/ocp/ocp:P8_46_pinmux/state",O_RDWR);
    pinOL.enable = open("/sys/devices/platform/ocp/48304000.epwmss/48304200.pwm/pwm/pwmchip7/pwm-7:1/enable",O_RDWR);
    pinOL.period = open("/sys/devices/platform/ocp/48304000.epwmss/48304200.pwm/pwm/pwmchip7/pwm-7:1/period",O_RDWR);
    pinOL.polarity = open("/sys/devices/platform/ocp/48304000.epwmss/48304200.pwm/pwm/pwmchip7/pwm-7:1/polarity",O_RDWR);
    pinOL.duty_cycle = open("/sys/devices/platform/ocp/48304000.epwmss/48304200.pwm/pwm/pwmchip7/pwm-7:1/duty_cycle",O_RDWR);

    pinIL.state = open("/sys/devices/platform/ocp/ocp:P8_34_pinmux/state",O_RDWR);
    pinIL.enable = open("/sys/devices/platform/ocp/48302000.epwmss/48302200.pwm/pwm/pwmchip4/pwm-4:1/enable",O_RDWR);
    pinIL.period = open("/sys/devices/platform/ocp/48302000.epwmss/48302200.pwm/pwm/pwmchip4/pwm-4:1/period",O_RDWR);
    pinIL.polarity = open("/sys/devices/platform/ocp/48302000.epwmss/48302200.pwm/pwm/pwmchip4/pwm-4:1/polarity",O_RDWR);
    pinIL.duty_cycle = open("/sys/devices/platform/ocp/48302000.epwmss/48302200.pwm/pwm/pwmchip4/pwm-4:1/duty_cycle",O_RDWR);

    pinIR.state = open("/sys/devices/platform/ocp/ocp:P8_36_pinmux/state",O_RDWR);
    pinIR.enable = open("/sys/devices/platform/ocp/48302000.epwmss/48302200.pwm/pwm/pwmchip4/pwm-4:0/enable",O_RDWR);
    pinIR.period = open("/sys/devices/platform/ocp/48302000.epwmss/48302200.pwm/pwm/pwmchip4/pwm-4:0/period",O_RDWR);
    pinIR.polarity = open("/sys/devices/platform/ocp/48302000.epwmss/48302200.pwm/pwm/pwmchip4/pwm-4:0/polarity",O_RDWR);
    pinIR.duty_cycle = open("/sys/devices/platform/ocp/48302000.epwmss/48302200.pwm/pwm/pwmchip4/pwm-4:0/duty_cycle",O_RDWR);

    pinOR.state = open("/sys/devices/platform/ocp/ocp:P8_45_pinmux/state",O_RDWR);
    pinOR.enable = open("/sys/devices/platform/ocp/48304000.epwmss/48304200.pwm/pwm/pwmchip7/pwm-7:0/enable",O_RDWR);
    pinOR.period = open("/sys/devices/platform/ocp/48304000.epwmss/48304200.pwm/pwm/pwmchip7/pwm-7:0/period",O_RDWR);
    pinOR.polarity = open("/sys/devices/platform/ocp/48304000.epwmss/48304200.pwm/pwm/pwmchip7/pwm-7:0/polarity",O_RDWR);
    pinOR.duty_cycle = open("/sys/devices/platform/ocp/48304000.epwmss/48304200.pwm/pwm/pwmchip7/pwm-7:0/duty_cycle",O_RDWR);
}

inline pin_t get_pin(int pin){
    if (pin == 1) {
        return pinOL;
    }
    else if (pin == 2) {
        return pinIL;
    }
    else if (pin == 3) {
        return pinIR;
    }
    else if (pin == 4) {
        return pinOR;
    }
    else {
        return pinOL;
    }
}

// Helper functions/wrappers
void set_power(int pin, int state){
    pin_t p = get_pin(pin);
    lseek(p.enable, 0, SEEK_SET);
    if (state) {
        write(p.enable, "1", 1);
    }
    else {
        write(p.enable, "0", 1);
    }
}

void set_state(int pin) {
    pin_t p = get_pin(pin);
    lseek(p.state, 0, SEEK_SET);
    write(p.state,"pwm",3);
}


void enable_pin(int pin) {
    pin_t p = get_pin(pin);
    lseek(p.enable, 0, SEEK_SET);
    write(p.enable,"1",1);
}


void disable_pin(int pin) {
    pin_t p = get_pin(pin);
    lseek(p.enable, 0, SEEK_SET);
    write(p.enable,"0",1);
}


void polarity_normal(int pin) {
    pin_t p = get_pin(pin);
    lseek(p.polarity, 0, SEEK_SET);
    write(p.polarity,"normal",6);
}


void set_period(int pin, char * period, unsigned char len){
    pin_t p = get_pin(pin);
    lseek(p.period, 0, SEEK_SET);
    write(p.period,period,len);
}


void get_period(int pin, char * period){
    pin_t p = get_pin(pin);
    lseek(p.period, 0, SEEK_SET);
    read(p.period,period,100);
}


void set_duty(int pin, int state){
    pin_t p = get_pin(pin);
    lseek(p.duty_cycle, 0, SEEK_SET);
    if (state == PWM_HIGH) {
        write(p.duty_cycle, PWM_HIGH_STR, sizeof(PWM_HIGH_STR));
    } else {
        write(p.duty_cycle, PWM_LOW_STR, sizeof(PWM_LOW_STR));
    }
}


void get_duty(int pin, char * duty){
    pin_t p = get_pin(pin);
    lseek(p.duty_cycle, 0, SEEK_SET);
    read(p.duty_cycle,duty,100);
}
