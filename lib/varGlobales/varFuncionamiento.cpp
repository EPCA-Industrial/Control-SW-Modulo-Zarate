#include "varFuncionamiento.h"

const String VERSION = "V: 1.0.0";

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

float referencia;

bool salida_ON = 0;
bool salida_OF = 1;

// número de pantalla a mostrar
uint8_t num_Pantalla = 1;

// para medir, mostrar y corregir ángulo en 'encoder()'
bool configurando = false;

bool swa_presionado, swb_presionado, sw_presionado;

const uint8_t cant_Registros = 11;

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
int regs_entrantes[10] = {
    /*00*/ 0, // Byte inicio '$'
    /*01*/ 0, // # módulo
    /*02*/ 0, // Esc/Lec
    /*03*/ 0, // Modo
    /*04*/ 0, // Referencia
    /*05*/ 0, // Tiempo despolarización
    /*06*/ 0, // Potencial despolarización
    /*07*/ 0, // Check Sum
    /*08*/ 0 // Byte fin '*'
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

// para armar cadena para enviar
char cadena_a_enviar[50];

// para indicar que entró un mensaje
bool atender;

// para la despolarización
unsigned int despol_Tiempo;
int despol_Potencial;

// tiempo [mS] que espera la pulsación del encoder en 'encoder()'
unsigned int esperaEncoder = 5000;