#ifndef __ESTABLECEPINES_H__
#define __ESTABLECEPINES_H__

//I2C
#define I2C_SCL 1   // Clock
#define I2C_SDA 2   // SDA

#define IN_DIG_04 4     //Entrada digital
#define OUT_DIG_05 5    //Salida digital

//botones
#define SW_B 6
#define SW_A 7
#define SW 47

//buzzer
#define BUZZER 48

//RS485 R/W
#define RS485_RW 9

//Cx0
#define Cx0 14

//GPS
#define PPS       8
#define ON_GPS 15
#define RST_GPS 16

//Tibbo
#define Modo_EM203 9
#define RST_EM203 10

//uart2 (RS485 - Tibbo - GPRS))
#define TX_UART2 13
#define RX_UART2 12

//PWM
#define PMM_pin18 18

//Interruptor
#define PULSO_DISPARO 21    //PWM o Timer

//SPI
#define SPICLOCK 35 // Clock
#define DATAIN 36   // MISO
#define DATAOUT 37  // MOSI
#define SELPIN 38   // Selección Pin

#define DISP_INT 39   // Habilita/Deshabilita salida (Interruptor y Sobrecorriente)

#define LED_DEBUG2 40  // Led debug 2 
#define LED_DEBUG1 41  // Led debug 1

//Sobre Corriente (corto cicuito)
#define SOBRE_I 42

void configuraPines(void);

#endif // __ESTABLECEPINES_H__