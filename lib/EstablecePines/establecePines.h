#ifndef __ESTABLECEPINES_H__
#define __ESTABLECEPINES_H__

// I2C
#define I2C_SCL 1 // Clock
#define I2C_SDA 2 // SDA

// Salida digital
#define OUT_DIG_05 5 

// botones
#define SW_B 6
#define SW_A 7
#define SW 47

// buzzer
#define BUZZER 48

// Habilita/Deshabilita salida (Interruptor y Sobrecorriente)
#define DISP_INT 8 

// uart2 (RS485 - Tibbo - GPRS))
#define TX_UART2 18
#define RX_UART2 17

// RS485 R/W
#define RS485_RW 11

// PWM
#define PWM_pin21 21

// SPI
#define SPICLOCK 35 // Clock
#define DATAIN 36   // MISO
#define DATAOUT 37  // MOSI
#define SELPIN 38   // Selección Pin

// Habilitación de la salida
#define ENABLE_1 39

#define LED_DEBUG2 40 // Led debug 2
#define LED_DEBUG1 41 // Led debug 1

// Sobre Corriente (corto cicuito)
#define SOBRE_I 42

void configuraPines(void);

#endif // __ESTABLECEPINES_H__