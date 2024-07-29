#ifndef __ADC_H__
#define __ADC_H__

#include "establecePines.h"

#define UMBRAL_VCA 180
#define UMBRAL_VBT 6.99
#define HISTERESIS_VBT 7.05
#define UMBRAL_I0 0.2
#define HISTERESIS_I0 0.8

int read_adc(int channel);
float mide(int _canal, int _veces);
void midePotencial(void);
void mideTodo(void);
void analizaAlarmas(void);
//int readADCPrueba(int channel);
//void alarmaVca_Sleep(void);

#endif // __ADC_H__