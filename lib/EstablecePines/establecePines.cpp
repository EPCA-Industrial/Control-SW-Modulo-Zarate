#include "establecePines.h"
#include <Arduino.h>

void configuraPines(void)
{
    // encoder
    pinMode(SW_A, INPUT_PULLUP);
    pinMode(SW_B, INPUT_PULLUP);
    pinMode(SW, INPUT_PULLUP);

    // buzzer
    pinMode(BUZZER, OUTPUT);

    // Cx0
    pinMode(Cx0, INPUT_PULLUP);

    // establece pines SPI
    pinMode(SELPIN, OUTPUT);
    pinMode(DATAOUT, OUTPUT);
    pinMode(DATAIN, INPUT);
    pinMode(SPICLOCK, OUTPUT);

    // entrada/salida libres
    pinMode(IN_DIG_04, INPUT_PULLUP);
    pinMode(OUT_DIG_05, OUTPUT);

    // leds
    pinMode(LED_DEBUG1, OUTPUT);
    pinMode(LED_DEBUG2, OUTPUT);

    // Interruptor
    pinMode(DISP_INT, OUTPUT);

    // Pulso de disparo
    pinMode(PULSO_DISPARO, OUTPUT);

    // GPS
    pinMode(PPS, INPUT_PULLDOWN);
    pinMode(ON_GPS, OUTPUT);
    pinMode(RST_GPS, OUTPUT);

    //Tibbo
    pinMode(Modo_EM203, OUTPUT);
    pinMode(RST_EM203, OUTPUT);

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