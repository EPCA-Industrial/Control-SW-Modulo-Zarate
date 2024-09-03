#ifndef __PWM_H__
#define __PWM_H__

#include <Arduino.h>

void ctaHoras(void);
bool obtenerEstadoBit(uint8_t numero, uint8_t posicion);
void sobre_I(void);
void ensayoDespolarizacion(uint8_t _ToP);
void despolarizacionxTiempo(void);
void despolarizacionxPotencial(void);
void despolarizacionRemota(void);

#endif // __PWM_H__