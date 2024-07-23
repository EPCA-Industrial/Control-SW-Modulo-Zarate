#include "interrupciones.h"
#include <Arduino.h>
#include "establecePines.h"
#include "varFuncionamiento.h"
#include "funciones.h"

extern hw_timer_t *TMR_Ty;

extern uint64_t t_Ty;

bool cx0_on;
bool band_PPS; // para habilitar marca de 1" interna

// bool bandEnsayoSinGPS = 0;

/// @brief establece valores para las interrupciones del interruptor.
/// @brief Timmer y por cambio de flanco para marca de 1seg
/// @param
void configuraInterrupciones(void)
{
    /////////////////////////////////////////////////////////////
    // inicializa Timer 'TMR_Ty (TMRnro, prescaler, crece/decrece)'
    TMR_Ty = timerBegin(1, 80, true);
    // deshabilita interrupción por timer
    timerDetachInterrupt(TMR_Ty);
    // indica la función a a ejecutar con la interrupción del Timer
    timerAttachInterrupt(TMR_Ty, &onTimer_Ty, true);
    // se establece el valor a alcanzar por el Timer y si activa/desactiva autoreload.
    timerAlarmWrite(TMR_Ty, t_Ty, true);
    // se habilita el Timer
    timerAlarmEnable(TMR_Ty);
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    // deshabilita interrupción por cambio de flanco
    //  detachInterrupt(digitalPinToInterrupt(PPS));

    attachInterrupt(digitalPinToInterrupt(Cx0), cx0on, RISING);

    // attachInterrupt(digitalPinToInterrupt(SW_A), swaPresionado, FALLING);
    //  attachInterrupt(digitalPinToInterrupt(SW_B), swbPresionado, FALLING);
    //  attachInterrupt(digitalPinToInterrupt(SW), swPresionado, FALLING);
    /////////////////////////////////////////////////////////////
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

void IRAM_ATTR cx0on()
{
    // inicia el contador
    timerStart(TMR_Ty);
}

void IRAM_ATTR onTimer_Ty()
{
    // detiene el contador
    timerStop(TMR_Ty);
    // reinicia el contador del Timer
    timerRestart(TMR_Ty);
    timerAlarmWrite(TMR_Ty, t_Ty, true);

    digitalWrite(PULSO_DISPARO, HIGH);
    // digitalWrite(LED_DEBUG1, HIGH);

    // cada instrucción NOP en el for() = 0.021uS y con FOSC = 240MHz
    for (uint16_t nop = 0; nop < 20000; nop++)
    {
        asm("nop");
    }

    digitalWrite(PULSO_DISPARO, LOW);
    // digitalWrite(LED_DEBUG1, LOW);
}


void IRAM_ATTR onTimer_1s()
{
    if (!band_PPS)
    {
        // digitalWrite(LED_DEBUG2, !digitalRead(LED_DEBUG2));
     
        cont_faltaPPS++;

    }
    else
    {
        band_PPS = 0;
    }
}

void IRAM_ATTR onTimer()
{
    digitalWrite(LED_DEBUG2, !digitalRead(LED_DEBUG2));
    // digitalWrite(DISP_INT, !digitalRead(DISP_INT));

}