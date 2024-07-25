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

float valor = 9000;

int seg_ant, seg_act;
uint16_t contErrGPS = 0;

/// @brief verifica que no se pierdan lecturas del GPS, comparando el segundo actual con el anterior + 1
/// @param
/// @return cont_ERR con la cantidad de errores desde que se conectó el GPS. Descartar los dos primeros
uint16_t validaSegundoSiguiente(void)
{
    seg_act = msj_GPS.substring(10, 12).toInt();

    if (seg_ant != 59)
    {
        if (seg_act != seg_ant + 1)
        {
            // Serial.printf("Error1: %s %i\n", msj_GPS, seg_act);
            contErrGPS++;

#ifdef COMPILAR_DEBUG
            Serial.printf("\nErrores GPS: %i\n", contErrGPS);
#endif
        }
        else
        {
#ifdef COMPILAR_DEBUG
            Serial.printf("OK: %s %02i\r", msj_GPS, seg_act);
#endif
        }
    }

    else if (seg_act != 0)
    {
        // Serial.printf("Error2: %s %i\n", msj_GPS, seg_act);
        contErrGPS++;

#ifdef COMPILAR_DEBUG
        Serial.printf("\nErrores: %i\n", contErrGPS);
#endif
    }
    else
    {
#ifdef COMPILAR_DEBUG
        Serial.printf("OK: %s %02i\r", msj_GPS, seg_act);
#endif
    }
    seg_ant = seg_act;

    return contErrGPS;
}

void ctaHoras(void)
{
    unsigned long currentMillis = millis();

    if (modo_funcionamiento != MODO_INTER)
    {
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