#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define PWM_PERIOD 1000U

void Config_Timer(uint8_t channel, uint16_t duty_cycle);
void ControlPWM_Timer2(uint8_t channel, uint16_t duty_cycle);

#endif