#include "varFuncionamiento.h"

const String VERSION = "V: 1.2.0";

HardwareSerial RS485_ext(2); // UART2, comunicación con el maestro

// Valores nominales
unsigned int ER_Icc;
uint8_t ER_Vcc;
uint8_t num_modulo;

int chkSum;

uint8_t modo_funcionamiento = 0;
uint8_t aux_modo;

float referencia;
float aux_refe;

bool salida_ON = 0;
bool salida_OF = 1;

// número de pantalla a mostrar
uint8_t num_Pantalla = 1;

// para medir, mostrar y corregir ángulo en 'encoder()'
bool configurando = false;

bool swa_presionado, swb_presionado, sw_presionado;

const uint8_t cant_Registros = 11;

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
    /*08*/ 0  // Byte fin '*'
};

// Variables globales para el cuenta horas
unsigned long hs_Func;
uint16_t hs_FuncH = 0, hs_FuncL = 0;

// para cambiar pantallas
uint8_t estadoEnsayo = 9;

// registro de alarmas
uint8_t alarmas = 0;

// para armar cadena para enviar
char cadena_a_enviar[50];

// para indicar que entró un mensaje y está OK
uint8_t atender;

// para la despolarización
unsigned int despol_Tiempo;
int despol_Potencial;

// tiempo [mS] que espera la pulsación del encoder en 'encoder()'
unsigned int esperaEncoder = 5000;

int duty_cycle;