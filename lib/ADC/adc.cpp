#include "adc.h"
#include <Arduino.h>
#include "Display16x2.h"
#include "varMedicion.h"
#include "varFuncionamiento.h"
#include <esp_task_wdt.h>

uint8_t estadoAnteriorAlarmas = 0;

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

void midePotencial(void)
{
    uint32_t millisAnt;
    float auxPot = Pot;

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
}
void mideTodo(void)
{
    // digitalWrite(LED_DEBUG2, HIGH);
    //   mide canal 1 y promedia n veces
    Vcc = mide(1, 435) * 0.0125;
    // digitalWrite(LED_DEBUG2, LOW);

    // digitalWrite(LED_DEBUG2, HIGH);
    //  mide canal 2 y promedia n veces
    Icc = mide(2, 435) * 0.0028; //! para shunt 10A
    //Icc = mide(2, 435) * 0.00568; //! para shunt 20A
    
    // digitalWrite(LED_DEBUG2, LOW);

    midePotencial();

    // mide canal 6 y promedia n veces
    Temp1 = (mide(6, 10) - 500) / 10;
    // digitalWrite(LED_DEBUG2, LOW);
}

void analizaAlarmas(void)
{
    alarmas = alarmas & 0b00000000; // Todo en cero

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
                alarmas = alarmas & 0b11110111;
            }
            else
            {
                // No alcanzó la histéresis del valor, se mantiene la alarma
                alarmas = alarmas | 0b00001000;
            }
        }
    }
    else
    {
        // Hay alarma I < UMBRAL_I0
        alarmas = alarmas | 0b00001000;
    }

    //  Si está por debajo del umbral IMax OK (definida por el usuario)
    if (Icc <= (ER_Icc))
    {
        // ...pero viene de una alarma IMax anterior, espera alcanzar el valor más la histéresis
        if ((estadoAnteriorAlarmas & 0b00010000) == 16)
        {
            // Viene de alarma Imax anterior
            if (Icc < ER_Icc * 0.98)
            {
                // Alcanzó la histéresis del valor
                alarmas = alarmas & 0b11101111;
            }
            else
            {
                // No alcanzó la histéresis del valor, se mantiene la alarma
                alarmas = alarmas | 0b00010000;
            }
        }
    }
    else
    {
        // Hay alarma I > Imax
        alarmas = alarmas | 0b00010000;
    }

    estadoAnteriorAlarmas = alarmas;
}

/* int readADCPrueba(int channel) {
    if (channel < 0 || channel > 7) {
        return -1; // Canal fuera de rango
    }

    // Configurar el comando para el MCP3208
    byte command = 0b00000110 | ((channel & 0x04) >> 2);
    byte command2 = ((channel & 0x03) << 6);

    digitalWrite(SELPIN, LOW); // Activa el chip

    // Enviar el comando y leer los valores
    SPI.transfer(command);
    byte highByte = SPI.transfer(command2);
    byte lowByte = SPI.transfer(0);

    digitalWrite(SELPIN, HIGH); // Desactiva el chip

    // Combinar los bytes recibidos
    int result = ((highByte & 0x0F) << 8) | lowByte;
    return result;
} */
