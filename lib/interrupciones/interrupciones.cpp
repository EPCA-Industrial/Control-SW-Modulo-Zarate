#include "interrupciones.h"
#include <Arduino.h>
#include "establecePines.h"

// bool bandEnsayoSinGPS = 0;

/// @brief establece valores para las interrupciones del interruptor.
/// @brief Timmer y por cambio de flanco para marca de 1seg
/// @param
void configuraInterrupciones(void)
{
    /////////////////////////////////////////////////////////////
    // deshabilita interrupción por cambio de flanco
    //  detachInterrupt(digitalPinToInterrupt(PPS));

    attachInterrupt(digitalPinToInterrupt(8), interruptor, CHANGE);

    // attachInterrupt(digitalPinToInterrupt(SW_A), swaPresionado, FALLING);
    //  attachInterrupt(digitalPinToInterrupt(SW_B), swbPresionado, FALLING);
    //  attachInterrupt(digitalPinToInterrupt(SW), swPresionado, FALLING);
    /////////////////////////////////////////////////////////////
}

void IRAM_ATTR interruptor()
{
    digitalWrite(ENABLE_1,digitalRead(DISP_INT));
}

/* void IRAM_ATTR swbPresionado()
{
    swb_presionado = true;
}*/

/* void IRAM_ATTR swaPresionado()
{
    swa_presionado = true;
} */

/*void IRAM_ATTR swPresionado()
{
    sw_presionado = true;
}*/


