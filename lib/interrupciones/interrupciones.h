#ifndef __INTERRUPCIONES_H__
#define __INTERRUPCIONES_H__

#include <Arduino.h>

void configuraInterrupciones(void);

//void IRAM_ATTR swbPresionado();
//void IRAM_ATTR swaPresionado();
//void IRAM_ATTR swPresionado();

void IRAM_ATTR cx0on();
void IRAM_ATTR onTimer_Ty();

void IRAM_ATTR pps();
void IRAM_ATTR onTimer_1s();
void IRAM_ATTR onTimer();

#endif // __INTERRUPCIONES_H__