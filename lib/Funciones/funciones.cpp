#include "funciones.h"
#include <Arduino.h>
#include "adc.h"
#include "pantallas.h"
#include "establecePines.h"
#include "varFuncionamiento.h"
#include "varMedicion.h"
#include "usuarios.h"
#include <esp_task_wdt.h>
#include "PWMs.h"
#include "display16x2.h"
#include "LiquidCrystal_I2C.h"

extern LiquidCrystal_I2C lcd;

// Variables locales para el cuenta horas
unsigned long previousMillis = 0;      // Tiempo de referencia para la temporización
unsigned long interval = 3600000;      // Intervalo de tiempo en milisegundos (1 hora)
int accumulatedSeconds = 0;            // Variable para acumular los segundos
const float umbral_Icc_cuentaHs = 0.5; // umbral de corriente para acumular horas de funcionamiento

extern String msj_GPS;

extern String txt_referencia;

void ctaHoras(void)
{
    unsigned long currentMillis = millis();

    if (Icc > umbral_Icc_cuentaHs) // Comprobar si está por encima del umbral
    {
        if (currentMillis - previousMillis >= interval)
        {
            previousMillis = currentMillis; // Actualizar el tiempo de referencia
            hs_FuncL++;                     // Incrementar la variable de horas acumuladas

            guardaNVS_HsFunc();
        }

        if (hs_FuncL > 32767)
        {
            hs_FuncH++;
            hs_FuncL = 0;
        }

#ifdef COMPILAR_DEBUG
        Serial.printf("Minutos acumulados: %i\r", hs_FuncL);
#endif
    }
    else
    {
        previousMillis = currentMillis; // Resetear el tiempo de referencia si está por debajo del umbral
    }
}

/// @brief Función para obtener el estado de un bit en una posición específica
/// @param numero número a analizar
/// @param posicion bit consultado (7... 0)
/// @return estado del bit consultado
bool obtenerEstadoBit(uint8_t numero, uint8_t posicion)
{
    // Desplazar el bit deseado a la posición más baja y realizar una operación AND con 1
    return (numero >> posicion) & 1;
}

void sobre_I(void)
{
    // detecta sobre corriente por hard
    if (digitalRead(SOBRE_I))
    {
        digitalWrite(DISP_INT, salida_OF);
        digitalWrite(LED_DEBUG1, LOW);
        delay(200);

        alarmas = alarmas | 0b00000100;

        Serial.println("\r\nEsperando atencion del operador...\r\n");

        estadoEnsayo = 11;
        muestraMedicion(1, 4, estadoEnsayo);

        do
        {
            delay(1000);

#ifdef WDT_SI
            // Resetea el WDT
            esp_task_wdt_reset();
#endif
        } while (true);
    }
}

void controlFuentes(float _referencia)
{
    static float fdoEscala;

    if (digitalRead(VIN_DETECTADA))
    {
        digitalWrite(OUTVCCX, HIGH);
    }
    else
    {
        digitalWrite(OUTVCCX, LOW);
    }

    switch (modo_funcionamiento)
    {
    case MODO_I_CTE:
        fdoEscala = ER_Icc * 10;
        break;
    case MODO_P_CTE:
        fdoEscala = 3000;
        break;
    case MODO_M_ELE:
        fdoEscala = 100;
        break;
    case MODO_V_CTE:
        fdoEscala = ER_Vcc * 10;
        break;
    default:
        break;
    }

    fija_Angulo(255 / fdoEscala * _referencia);
}

void ensayoDespolarizacion(uint8_t _ToP)
{
    char auxtxt[16];
    esperaEncoder = 500;

    uint8_t hs, ms;

    lcd.clear();

    switch (_ToP)
    {
    case 1:
        txt_referencia = "' ";

        lcd_print_Posicion(1, 1, "Tiempo          ");
        lcd_print_Posicion(1, 2, "a alcanzar:     ");

        do
        {
            despol_Tiempo = encoder("T: ", despol_Tiempo, 65500, 1, 10, 40, 4, 4, 0);
            // delay(500);

            hs = (int)((float)despol_Tiempo / 60);
            ms = despol_Tiempo - hs * 60;

            sprintf(auxtxt, "%4ihs %2i'", hs, ms);
            lcd_print_Posicion(3, 4, auxtxt);

        } while (digitalRead(SW));

        despolarizacionxTiempo();

        break;

    case 2:
        txt_referencia = "mV";

        lcd_print_Posicion(1, 1, "Potencial       ");
        lcd_print_Posicion(1, 2, "a alcanzar:     ");

        despol_Potencial = -Pot;
        do
        {
            despol_Potencial = encoder("P: -", despol_Potencial, 1500, 400, 10, 40, 4, 4, 0);
            Serial.printf("%i\r", despol_Potencial);

        } while (digitalRead(SW));

        despol_Potencial = -despol_Potencial;

        despolarizacionxPotencial();

        break;

    default:
        break;
    }

    esperaEncoder = 5000;
}

void despolarizacionxTiempo(void)
{
    char aux_pot[16];
    unsigned long cuentaMinutos = millis();
    unsigned long despol_Tiempo_mS = despol_Tiempo * 60000;

    Serial.println("Despolarizando x Tiempos");

    lcd.clear();
    lcd_print_Posicion(1, 1, "Despol. x Tiempo");

    delay(500);
    sprintf(aux_pot, "Tr:%05i'", despol_Tiempo);
    lcd_print_Posicion(1, 3, aux_pot);

    do
    {
#ifdef WDT_SI
        // Resetea el WDT
        esp_task_wdt_reset();
#endif
        midePotencial();

        sprintf(aux_pot, "P: %4.0f", Pot);
        lcd_print_Posicion(1, 4, aux_pot);

        sprintf(aux_pot, "%05li'", (millis() - cuentaMinutos) / 60000);
        lcd_print_Posicion(11, 3, aux_pot);

        sprintf(aux_pot, "%6lis", (millis() - cuentaMinutos) / 1000);
        lcd_print_Posicion(10, 4, aux_pot);

    } while (digitalRead(SW) && (millis() - cuentaMinutos) < despol_Tiempo_mS);

    despol_Potencial = Pot;

    lcd_print_Posicion(11, 3, "      ");
    lcd_print_Posicion(10, 4, "       ");
    delay(500);

    while (digitalRead(SW))
        ;
    delay(200);
}

void despolarizacionxPotencial(void)
{
    char aux_potp[17];
    unsigned long cuentaMinutos = millis();

    Serial.println("Despolarizando x Potencial");

    lcd.clear();
    lcd_print_Posicion(1, 1, "Despol. x Poten.");
    delay(500);

    sprintf(aux_potp, "Pr:%04imV", despol_Potencial);
    lcd_print_Posicion(1, 3, aux_potp);

    do
    {
#ifdef WDT_SI
        // Resetea el WDT
        esp_task_wdt_reset();
#endif
        midePotencial();

        sprintf(aux_potp, "P: %4.0f", Pot);
        lcd_print_Posicion(1, 4, aux_potp);

        sprintf(aux_potp, "%05li'", (millis() - cuentaMinutos) / 60000);
        lcd_print_Posicion(11, 3, aux_potp);

        sprintf(aux_potp, "%6lis", (millis() - cuentaMinutos) / 1000);
        lcd_print_Posicion(10, 4, aux_potp);

    } while (digitalRead(SW) && (Pot < despol_Potencial));

    despol_Tiempo = (millis() - cuentaMinutos) / 60000;

    lcd_print_Posicion(11, 3, "      ");
    sprintf(aux_potp, "Tr: %05i'      ", despol_Tiempo);
    lcd_print_Posicion(1, 4, aux_potp);

    delay(500);
    while (digitalRead(SW))
        ;
    delay(200);
}
