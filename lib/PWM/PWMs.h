#ifndef __PWMS_H__
#define __PWMS_H__

#include <Arduino.h>

void configura_PWMs(void);
void configura_PWM(uint8_t pin, uint8_t canal);
void ledcAnalogWrite(uint8_t canal, uint32_t valor, uint32_t valorMax);
void prueba_PWM(void);
void fija_Angulo(uint32_t _angulo);

#endif // __PWMS_H__