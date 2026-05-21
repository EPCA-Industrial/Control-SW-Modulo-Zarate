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

// Protección térmica con LM35 (canal 5 del MCP3208).
// Derateo lineal entre TEMP_INICIO_DERATEO y TEMP_FIN_DERATEO; corte total
// si supera TEMP_CRITICA con histéresis de reenganche en TEMP_REENGANCHE.
#define TEMP_INICIO_DERATEO 70.0f
#define TEMP_FIN_DERATEO    85.0f
#define TEMP_CRITICA        90.0f
#define TEMP_REENGANCHE     75.0f

float aplicaDerateoTemp(float ref);
bool  corteTermicoActivo(void);

#endif // __PWM_H__