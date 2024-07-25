#ifndef __VARFUNCIONAMIENTO_H__
#define __VARFUNCIONAMIENTO_H__

#include <Arduino.h>

//! comentar la línea siguiente para no compliar salidas por consola (debug)
// #define COMPILAR_DEBUG 1

#define WDT_TIMEOUT 300 // segundos
#define WDT_SI 1

#define MODO_I_CTE 1 // Corriente constante
#define MODO_P_CTE 2 // Potencial constante
#define MODO_M_ELE 3 // Manual electrónico
#define MODO_V_CTE 4 // Tensión constante
#define MODO_INTER 5 // Interruptor
#define MODO_DESPO 6 // Despolarización

#define CAMBIO_MODO_FUNCIONAMIENTO 1 // registro modo de funcionamiento
#define CAMBIO_REFERENCIA 9          // registro referencia
#define CAMBIO_TIPO_ENSAYO 12        // registro tipo de ensayo

#define LECTURA 0
#define ESCRITURA 1

extern const String VERSION;

extern HardwareSerial RS485_ext; // UART2, comunicación con el maestro

// Valores nominales
extern unsigned int NS;
extern unsigned int ER_Vca;
extern unsigned int ER_Icc;
extern uint8_t ER_Vcc;
extern uint8_t Ciclos;
extern uint8_t Fases;

extern bool er_modulo; // 1 equipo; 2 módulo
extern uint8_t num_modulo;
extern float Imin;

extern uint8_t modo_funcionamiento;

extern float referencia;

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
// Variables para almacenar los valores de los registros
extern uint16_t regs[];
// Variables para definir registros de solo lectura
extern const bool regs_lectura[];

extern int regs_entrantes[];

// Variables para el cuenta horas
extern unsigned long hs_Func;
extern uint16_t hs_FuncH, hs_FuncL;

extern uint8_t estadoEnsayo;

extern // para ejecutar o no la tarea 'mi_modbus'
    bool tarea_mi_modbus_habilitada;

// para contar cuantas marcas de 1" se perdieron
extern uint8_t cont_faltaPPS;

// registro de alarmas
extern uint8_t alarmas;

// para armar cadena para enviar
extern char cadena_a_enviar[];

// para indicar que entró un mensaje
extern bool atender;

#endif // __VARFUNCIONAMIENTO_H__