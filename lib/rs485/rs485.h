#ifndef __RS485_H__
#define __RS485_H__
#include "Arduino.h"

void ini_UARTs(void);
String atiendeUart_2(void);
void envia_a_Maestro(String _msj);
char *Str_a_char(String _str);
bool atiendoPeticion(void);
uint16_t check_Sum(void);
void armaCadenaValores(void);
void recibeYanalizaValores(void);

#endif // __RS485_H__