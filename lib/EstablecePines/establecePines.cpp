#include "establecePines.h"
#include <Arduino.h>

void configuraPines(void)
{
    // encoder
    pinMode(SW_A, INPUT_PULLUP);
    pinMode(SW_B, INPUT_PULLUP);
    pinMode(SW, INPUT_PULLUP);

    pinMode(ENABLE_1, OUTPUT);

    // buzzer
    pinMode(BUZZER, OUTPUT);

    // establece pines SPI
    pinMode(SELPIN, OUTPUT);
    pinMode(DATAOUT, OUTPUT);
    pinMode(DATAIN, INPUT);
    pinMode(SPICLOCK, OUTPUT);

    // entrada/salida libres
    pinMode(OUT_DIG_05, OUTPUT);

    // leds
    pinMode(LED_DEBUG1, OUTPUT);
    pinMode(LED_DEBUG2, OUTPUT);

    // PWM
    pinMode(PWM_pin18, OUTPUT);

    // Interruptor
    pinMode(DISP_INT, INPUT_PULLDOWN);
    
    // RS485 R/W
    pinMode(RS485_RW, OUTPUT);

    // Sobre corriente por hard
    pinMode(SOBRE_I, INPUT_PULLDOWN);

    // desactiva el dispositivo SPI para empezar
    digitalWrite(SELPIN, HIGH);
    digitalWrite(DATAOUT, LOW);
    digitalWrite(SPICLOCK, LOW);

    // desactiva UART2 para empezar
    digitalWrite(RX_UART2, HIGH);
    digitalWrite(TX_UART2, HIGH);
}