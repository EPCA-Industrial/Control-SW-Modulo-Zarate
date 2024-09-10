#ifndef __VARFUNCIONAMIENTO_H__
#define __VARFUNCIONAMIENTO_H__

#include <Arduino.h>

//! comentar la línea siguiente para no compliar salidas por consola (debug)
// #define COMPILAR_DEBUG 1

#define WDT_TIMEOUT 300 // segundos
#define WDT_SI 1

#define MODO_I_CTE 1 // Corriente constante
#define MODO_P_CTE 2 // Potencial constante
#define MODO_V_CTE 3 // Tensión constante
#define MODO_DESPO 4 // Despolarización

#define DESP_TIEMPO 1 // Tiempo despolarización
#define DESP_POTENC 2 // Potencial despolarización

#define LECTURA 0
#define ESCRITURA 1
#define DESPOLAR 2

extern const String VERSION;

extern HardwareSerial RS485_ext; // UART2, comunicación con el maestro

// Valores nominales
extern unsigned int ER_Icc;
extern uint8_t ER_Vcc;
extern uint8_t num_modulo;

extern int chkSum;

extern uint8_t modo_funcionamiento;
extern uint8_t aux_modo;

extern float referencia;
extern float aux_refe;

// número de pantalla a mostrar
extern uint8_t num_Pantalla;

// para medir, mostrar y corregir ángulo en 'encoder()'
extern bool configurando;

extern bool swa_presionado, swb_presionado, sw_presionado;

extern bool salida_ON;
extern bool salida_OF;

// Definir la dirección del esclavo Modbus
extern const uint8_t SLAVE_ID;
extern const uint8_t cant_Registros;

extern int regs_entrantes[];

// Variables para el cuenta horas
extern unsigned long hs_Func;
extern uint16_t hs_FuncH, hs_FuncL;

extern uint8_t estadoEnsayo;

// registro de alarmas
extern uint8_t alarmas;

// para armar cadena para enviar
extern char cadena_a_enviar[];

// para indicar que entró un mensaje
extern uint8_t atender;

// para la despolarización
extern unsigned int despol_Tiempo;
extern int despol_Potencial;

// tiempo que espera la pulsación del encoder 'encoder()'
extern unsigned int esperaEncoder;

extern int duty_cycle;

#endif // __VARFUNCIONAMIENTO_H__