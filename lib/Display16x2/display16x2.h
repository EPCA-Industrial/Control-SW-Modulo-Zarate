#ifndef __DISPLAY16X2_H__
#define __DISPLAY16X2_H__

#include <Arduino.h>

void inicializaDisplay(void);
void cargaCaracteres(void);
void borraPantalla(void);
void lcd_print_Posicion(uint8_t columna, uint8_t fila, String cadena);
void muestraHora(String _fechaHora, uint8_t _c, uint8_t _f);
void muestraFecha(String _fechaHora, uint8_t _c, uint8_t _f);
void muestraFloat(String _etiqueta, float _parametro, uint8_t _largo, uint8_t _decimales, uint8_t _c, int _f);

#endif // __DISPLAY16X2_H__