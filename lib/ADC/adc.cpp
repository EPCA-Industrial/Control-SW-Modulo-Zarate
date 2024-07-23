#include "adc.h"
#include <Arduino.h>
#include "Display16x2.h"
#include "varMedicion.h"
#include "varFuncionamiento.h"
#include <esp_task_wdt.h>

uint8_t estadoAnteriorAlarmas = 0;
uint8_t estadoActualAlarmas;

extern TaskHandle_t mi_modbus;
extern uint8_t alarmasHabilitadas;
extern int umbralPmax, umbralPmin;
extern uint8_t umbralImax;
extern bool estadoGPS;

int read_adc(int channel)
{
    int adcvalue = 0;
    byte commandbits = B11000000; // command bits - start, mode, chn (3), dont care (3)

    // allow channel selection
    commandbits |= ((channel - 1) << 3);

    digitalWrite(SELPIN, LOW); // Select adc
    // setup bits to be written
    for (int i = 7; i >= 3; i--)
    {
        digitalWrite(DATAOUT, commandbits & 1 << i);
        // cycle clock
        digitalWrite(SPICLOCK, HIGH);
        digitalWrite(SPICLOCK, LOW);
    }

    digitalWrite(SPICLOCK, HIGH); // ignores 2 null bits
    digitalWrite(SPICLOCK, LOW);
    digitalWrite(SPICLOCK, HIGH);
    digitalWrite(SPICLOCK, LOW);

    // read bits from adc
    for (int i = 11; i >= 0; i--)
    {
        adcvalue += digitalRead(DATAIN) << i;
        // cycle clock
        digitalWrite(SPICLOCK, HIGH);
        digitalWrite(SPICLOCK, LOW);
    }
    digitalWrite(SELPIN, HIGH); // turn off device
    return adcvalue;
}

float mide(int _canal, int _veces)
{
    int numADC;
    float sum = 0;
    float prom = 0;

    for (int i = 0; i < _veces; i++)
    {
        numADC = read_adc(_canal);
        sum = numADC + sum;
    }

    prom = sum / _veces;

    return prom;
}

/// @brief
/// @param
/// @warning //! IMPORTANTE: asegurar la cantidad de muestreos para promediar ('n'),
//!             que correspondan a 10mSeg o multiplo de 10mSeg

void mideTodo(void)
{
    uint32_t millisAnt;
    float auxPot = Pot;

    // digitalWrite(LED_DEBUG2, HIGH);
    //   mide canal 1 y promedia n veces
    Vcc = mide(1, 435) * 0.04544; // 0.02272;
    // digitalWrite(LED_DEBUG2, LOW);

    // digitalWrite(LED_DEBUG2, HIGH);
    //  mide canal 2 y promedia n veces
    Icc = mide(2, 435) * 0.0225;
    // digitalWrite(LED_DEBUG2, LOW);

    millisAnt = millis();
    digitalWrite(LED_DEBUG2, HIGH);
    //  mide canal 3 y promedia n veces
    auxPot = mide(3, 425);
    digitalWrite(LED_DEBUG2, LOW);

    // guarda 'Pot' sólo si tardó 10mSeg midiendo.
    if (millis() == millisAnt + 10)
    {
        Pot = (2048 - auxPot) * 2;
    }

    // digitalWrite(LED_DEBUG2, HIGH);
    //  mide canal 4 y promedia n veces
    Vca = mide(4, 435) * 0.0625;
    // digitalWrite(LED_DEBUG2, LOW);

    Vca = Vca + (220 - Vca) * 0.26;
    // mide canal 5 y promedia n veces
    VBat = mide(5, 10) / 280;

    // mide canal 6 y promedia n veces
    Temp1 = (mide(6, 10) - 500) / 10;
    // digitalWrite(LED_DEBUG2, LOW);
}

/*bool analizaAlarmas(void)
{
    bool enviar = 0;
    estadoActualAlarmas = estadoActualAlarmas & 0b10000000; // En cero todo, menos el GPS

    // Puerta abierta
    if (digitalRead(IN_DIG_04))
    {
        estadoActualAlarmas = estadoActualAlarmas | 0b00000001;
    }
    else
    {
        estadoActualAlarmas = estadoActualAlarmas & 0b11111110;
    }

    // Si supera el umbral (Vca OK)...
    if (Vca > UMBRAL_VCA)
    {
        // ...pero viene una alarma Vca anterior, espera alcanzar el valor más la histéresis
        if ((estadoAnteriorAlarmas & 0b00000010) == 2)
        {
            // Alcanzó la histéresis del valor
            if (Vca > UMBRAL_VCA * 1.1)
            {
                // Apaga la alarma
                estadoActualAlarmas = estadoActualAlarmas & 0b11111101;
            }
            // No alcanzó la histéresis del valor, se mantiene la alarma
            else
            {
                // mantiene la alarma
                estadoActualAlarmas = estadoActualAlarmas | 0b00000010;
            }
        }
    }
    else
    {
        // Hay alarma Vca < AlarmaVca
        estadoActualAlarmas = estadoActualAlarmas | 0b00000010;
    }

    // Si supera el umbral (Vbat OK)...
    if (VBat > UMBRAL_VBT)
    {
        // ...pero viene una alarma Vbat anterior, espera alcanzar el valor más la histéresis
        if ((estadoAnteriorAlarmas & 0b00000100) == 4)
        {
            // Alcanzó la histéresis del valor
            if (VBat > HISTERESIS_VBT)
            {
                // Apaga la alarma
                estadoActualAlarmas = estadoActualAlarmas & 0b11111011;
            }
            else
            {
                // No alcanzó la histéresis del valor, se mantiene la alarma
                estadoActualAlarmas = estadoActualAlarmas | 0b00000100;
            }
        }
    }
    else
    {
        // Hay alarma VBat < AlarmaVbat
        estadoActualAlarmas = estadoActualAlarmas | 0b00000100;
    }

    // Si está por debajo del umbral (I0 OK)...
    if (Icc > UMBRAL_I0)
    {
        // ...pero viene de una alarma de Corriente cero anterior, espera alcanzar el valor más la histéresis
        if ((estadoAnteriorAlarmas & 0b00001000) == 8)
        {
            // Alcanzó la histéresis del valor
            if (Icc > HISTERESIS_I0)
            {
                // Apaga la alarma
                estadoActualAlarmas = estadoActualAlarmas & 0b11110111;
            }
            else
            {
                // No alcanzó la histéresis del valor, se mantiene la alarma
                estadoActualAlarmas = estadoActualAlarmas | 0b00001000;
            }
        }
    }
    else
    {
        // Hay alarma I < UMBRAL_I0
        estadoActualAlarmas = estadoActualAlarmas | 0b00001000;
    }

    //  Si está por debajo del umbral IMax OK (definida por el usuario)
    if (Icc <= (umbralImax))
    {
        // ...pero viene de una alarma IMax anterior, espera alcanzar el valor más la histéresis
        if ((estadoAnteriorAlarmas & 0b00010000) == 16)
        {
            // Viene de alarma Imax anterior
            if (Icc < umbralImax * 0.98)
            {
                // Alcanzó la histéresis del valor
                estadoActualAlarmas = estadoActualAlarmas & 0b11101111;
            }
            else
            {
                // No alcanzó la histéresis del valor, se mantiene la alarma
                estadoActualAlarmas = estadoActualAlarmas | 0b00010000;
            }
        }
    }
    else
    {
        // Hay alarma I > Imax
        estadoActualAlarmas = estadoActualAlarmas | 0b00010000;
    }

    // Si tiene hemipila evalua alarmas de potencial -- Define usuario
    if (Pot >= -umbralPmin)
    {
        // No hay alarma Pmin
        if ((estadoAnteriorAlarmas & 0b00100000) == 32)
        {
            // Viene de alarma Pmin anterior
            if (Pot > -umbralPmin * 0.9)
            {
                // Alcanzó la histéresis del valor
                estadoActualAlarmas = estadoActualAlarmas & 0b11011111;
            }
            else
            {
                // No alcanzó la histéresis del valor, se mantiene la alarma
                estadoActualAlarmas = estadoActualAlarmas | 0b00100000;
            }
        }
    }
    else
    {
        // Hay alarma P < -Pmin
        estadoActualAlarmas = estadoActualAlarmas | 0b00100000;
    }

    if (Pot <= (-umbralPmax))
    {
        // No hay alarma PMax
        if ((estadoAnteriorAlarmas & 0b01000000) == 64)
        {
            // Viene de alarma Pmax anterior
            if (Pot < -umbralPmax * 1.1)
            {
                // Alcanzó la histéresis del valor
                estadoActualAlarmas = estadoActualAlarmas & 0b10111111;
            }
            else
            {
                // No alcanzó la histéresis del valor, se mantiene la alarma
                estadoActualAlarmas = estadoActualAlarmas | 0b01000000;
            }
        }
    }
    else
    {
        // Hay alarma P > -Pmax
        estadoActualAlarmas = estadoActualAlarmas | 0b01000000;
    }

    if (!estadoGPS)
    {
        estadoActualAlarmas = estadoActualAlarmas | 0b10000000;
    }

    if ((estadoAnteriorAlarmas & alarmasHabilitadas) != (estadoActualAlarmas & alarmasHabilitadas))
    {
        enviar = 1;
#ifdef COMPILAR_DEBUG
        Serial.printf("\rActual: %03d Anterior: %03d", estadoActualAlarmas, estadoAnteriorAlarmas);
#endif
        estadoAnteriorAlarmas = estadoActualAlarmas;
    }
#ifdef COMPILAR_DEBUG
    // Serial.printf("\rActual: %03d Anterior: %03d", estadoActualAlarmas, estadoAnteriorAlarmas);
#endif
    return enviar;
}
*/