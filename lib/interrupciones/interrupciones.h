#ifndef __INTERRUPCIONES_H__
#define __INTERRUPCIONES_H__

#include <Arduino.h>

void configuraInterrupciones(void);

//void IRAM_ATTR swbPresionado();
//void IRAM_ATTR swaPresionado();
//void IRAM_ATTR swPresionado();

void IRAM_ATTR interruptor();

#endif // __INTERRUPCIONES_H__