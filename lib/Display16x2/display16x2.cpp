#include "display16x2.h"
#include <Arduino.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"

byte Ohm[8] = {0, 4, 14, 17, 17, 10, 27, 0};
byte Preg[8] = {4, 0, 4, 8, 16, 17, 14, 0};
byte a[8] = {2, 4, 14, 1, 15, 17, 15, 0};
byte e[8] = {2, 4, 14, 17, 31, 16, 14, 0};
byte i[8] = {2, 4, 0, 12, 4, 4, 14, 0};
byte o[8] = {2, 4, 14, 17, 17, 17, 14, 0};
byte u[8] = {2, 4, 17, 17, 17, 17, 14, 0};
byte n[8] = {14, 0, 22, 25, 17, 17, 17, 0};

LiquidCrystal_I2C lcd(0x27, 16, 4);

void inicializaDisplay(void)
{
    lcd.init();
    cargaCaracteres();
    //lcd.backlight();
}

/// @brief Carga caracteres especiales. Se imprimen con 'lcd.write(n)'
/// @param
void cargaCaracteres(void)
{
    lcd.createChar(1, a);
    lcd.createChar(2, e);
    lcd.createChar(3, i);
    lcd.createChar(4, o);
    lcd.createChar(5, u);
    lcd.createChar(6, Preg);
    lcd.createChar(7, n);
    lcd.createChar(8, Ohm);
}

void borraPantalla(void)
{
    lcd.clear();
}

void lcd_print_Posicion(uint8_t columna, uint8_t fila, String cadena)
{
    // corrección por bug en la librería /////////
    if (fila > 2)
    {
        columna = columna - 4;
    }
    //////////////////////////////////////////////
    lcd.setCursor(columna - 1, fila - 1);
    lcd.print(cadena);
}

/// @brief extrae la hora, la  y la muestra en el lcd en la fila '_f' y la columna '_c'
/// @param _fechaHora de entrada en formato 'aammddhhmmss'
/// @param _c
/// @param _f
void muestraHora(String _fechaHora, uint8_t _c, uint8_t _f)
{
    String msj_Hora, hora_str;

    msj_Hora = _fechaHora.substring(6, 12);

    // lcd.setCursor(_c, _f);
    // lcd.printf("%s:%s:%s", msj_Hora.substring(0, 2), msj_Hora.substring(2, 4), msj_Hora.substring(4, 6));
    String h_gps = msj_Hora.substring(0, 2) + ":" + msj_Hora.substring(2, 4) + ":" + msj_Hora.substring(4, 6);
    lcd_print_Posicion(_c, _f, h_gps);

#ifdef COMPILAR_DEBUG
// Serial.printf("%s:%s:%s\n", msj_Hora.substring(0, 2), msj_Hora.substring(2, 4), msj_Hora.substring(4, 6));
#endif
}

/// @brief extrae la fecha y la muestra en el lcd en la fila '_f' y la columna '_c'
/// @param _fechaHora de entrada en formato 'aammddhhmmss'
/// @param _c
/// @param _f
void muestraFecha(String _fechaHora, uint8_t _c, uint8_t _f)
{
    String msj_Fecha;

    msj_Fecha = _fechaHora.substring(0, 6);

    // lcd.setCursor(_c, _f);
    // lcd.printf("%s/%s/%s", msj_Fecha.substring(4, 6), msj_Fecha.substring(2, 4), msj_Fecha.substring(0, 2));
    String f_gps = msj_Fecha.substring(4, 6) + "/" + msj_Fecha.substring(2, 4) + "/" + msj_Fecha.substring(0, 2);
    lcd_print_Posicion(_c, _f, f_gps);

#ifdef COMPILAR_DEBUG
// Serial.printf("%s/%s/%s\n", msj_Fecha.substring(4, 6), msj_Fecha.substring(2, 4), msj_Fecha.substring(0, 2));
#endif
}

/// @brief muestra un flotante en lcd
/// @param _etiqueta
/// @param _parametro
/// @param _largo
/// @param _decimales
/// @param _f
/// @param _c
void muestraFloat(String _etiqueta, float _parametro, uint8_t _largo, uint8_t _decimales, uint8_t _c, int _f)
{
    // correción por debug de la librería del display 16x4
    if (_f > 1)
    {
        _c = _c - 4;
    }

    lcd.setCursor(_c, _f);

    if (_etiqueta == "")
    {
        lcd.printf("       ");
    }
    else
    {
        lcd.print(_etiqueta);
        lcd.printf("%*.*f", _largo, _decimales, _parametro);
    }
}
