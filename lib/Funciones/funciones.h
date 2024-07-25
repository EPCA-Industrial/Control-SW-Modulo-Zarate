#ifndef __PWM_H__
#define __PWM_H__

#include <Arduino.h>

void cargaVector_desdeVariables(void);
uint16_t validaSegundoSiguiente(void);
void ctaHoras(void);
bool obtenerEstadoBit(uint8_t numero, uint8_t posicion);
void sobre_I(void);
void controlFuentes(float _referencia);

#endif // __PWM_H__