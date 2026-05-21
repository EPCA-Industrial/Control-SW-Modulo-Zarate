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
    float auxPot;

    digitalWrite(LED_DEBUG2, HIGH);
    //  mide canal 3 y promedia n veces (425 muestras ~ 10ms para filtrar 50Hz)
    auxPot = mide(3, 425);
    digitalWrite(LED_DEBUG2, LOW);

    Pot = (2048 - auxPot) * 2;
}
void mideTodo(void)
{
    // Vcc: divisor 0.022, VREF=4.096V -> 1/(0.022*999.76) ~= 0.04546 V/cuenta
    Vcc = mide(1, 435) * 0.04546;

    // Icc: shunt 50A/50mV * IC(50) / 2 * 1.8 = 0.045 V/A, VREF=4.096V -> 1/(0.045*999.76) ~= 0.02223 A/cuenta
    //Icc = mide(2, 435) * 0.2841;  //! para modulo 1A
    //Icc = mide(2, 435) * 0.02841; //! para modulo 100mA
    //Icc = mide(2, 435) * 0.0028;  //! para shunt 10A
    //Icc = mide(2, 435) * 0.00568; //! para shunt 20A
    Icc = mide(2, 435) * 0.02223;   //! para shunt 50A/50mV + IC x50 + /2 + x1.8 (Icc en A)

    midePotencial();

    // LM35 conectado al pin 6 del MCP3208 (canal CH5 del datasheet).
    // La función mide() usa el número de pin físico (1-base), por eso se pasa 6.
    // LM35: 10 mV/°C; VREF=4.096V -> 1 mV/cuenta -> 0.1 °C/cuenta.
    //
    // Filtrado en dos etapas para una lectura visualmente estable y para evitar
    // que el derateo térmico oscile en el borde de los umbrales (70/85/90 °C):
    //   1) Promedio de 50 muestras dentro de mide() -> reduce ruido base.
    //   2) EMA (exponential moving average) con alpha = 0.10  ->  pondera 10%
    //      la lectura nueva y 90% el histórico. Constante de tiempo ~10 lecturas,
    //      suficiente para una variable térmica.
    {
        static float temp_filtrada = 0.0f;
        static bool  temp_init     = false;

        float temp_raw = mide(6, 50) * 0.1f;

        if (!temp_init)
        {
            temp_filtrada = temp_raw;
            temp_init     = true;
        }
        else
        {
            temp_filtrada = 0.1f * temp_raw + 0.9f * temp_filtrada;
        }

        Temp1 = temp_filtrada;
    }
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
