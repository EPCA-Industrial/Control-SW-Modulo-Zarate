#include "varFuncionamiento.h"

const String VERSION = "V: 0.0.0";

HardwareSerial RS485_ext(2); // UART2, comunicación con el maestro

// Valores nominales
unsigned int NS;
unsigned int ER_Vca;
unsigned int ER_Icc;
uint8_t ER_Vcc;
uint8_t Ciclos;
uint8_t Fases;

bool er_modulo; // 1 equipo; 2 módulo
uint8_t num_modulo;
float Imin;

uint8_t modo_funcionamiento = 0;

uint64_t t_Ty = 9000;
float referencia;

bool salida_ON = 0;
bool salida_OF = 1;

// número de pantalla a mostrar
uint8_t num_Pantalla = 1;

// para medir, mostrar y corregir ángulo en 'encoder()'
bool configurando = false;

bool swa_presionado, swb_presionado, sw_presionado;

const uint8_t cant_Registros = 11;

// Variables para almacenar los valores de los registros
uint16_t regs[cant_Registros] = {
    /*00*/ num_modulo,
    /*01*/ MODO_M_ELE,
    /*02*/ 0, // Vcc
    /*03*/ 0, // Icc
    /*04*/ 0, // Pot
    /*05*/ 0, // Temp
    /*06*/ 0, // Ref
    /*07*/ 0, // HsH
    /*08*/ 0, // HsL
    /*09*/ 0, // Estados
    /*10*/ 0  // Alarmas
};
// Variables para definir registros de solo lectura
const bool regs_lectura[cant_Registros] = {
    /*00*/ 0, // N° módulo
    /*01*/ 0, // Modo
    /*02*/ 1, // Vcc
    /*03*/ 1, // Icc
    /*04*/ 1, // Pot
    /*05*/ 1, // Temp
    /*06*/ 0, // Ref
    /*07*/ 1, // HsH
    /*08*/ 1, // HsL
    /*09*/ 1, // Estados
    /*10*/ 1  // Alarmas}
};

// Variables para almacenar mensaje recibido del rack
uint16_t regs_entrantes[9] = {
    /*00*/ 0, // Byte inicio '$'
    /*01*/ 0, // # módulo
    /*02*/ 0, // Esc/Lec
    /*03*/ 0, // Modo
    /*04*/ 0, // Referencia H
    /*05*/ 0, // Referencia L
    /*06*/ 0, // Check Sum
    /*07*/ 0 // Byte fin '*'
};

// Variables globales para el cuenta horas
unsigned long hs_Func;
uint16_t hs_FuncH = 0, hs_FuncL = 0;

// para cambiar pantallas
uint8_t estadoEnsayo = 9;

// para contar cuantas marcas de 1" se perdieron
uint8_t cont_faltaPPS = 0;

// registro de alarmas
uint8_t alarmas = 0;
