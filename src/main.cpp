#include <Arduino.h>
#include "Wire.h"
#include "establecePines.h"
#include "display16x2.h"
#include "pantallas.h"
#include "funciones.h"
#include "adc.h"
#include "interrupciones.h"
#include "usuarios.h"
#include "LiquidCrystal_I2C.h"
#include <esp_task_wdt.h>
#include "varFuncionamiento.h"
#include "varMedicion.h"
#include "rs485.h"
#include "PWMs.h"

// no olvidar 'Habilitar WDT' y setear el tiempo en 'WDT_TIMEOUT'

// definir Vx.y.z
// x => compatibilidad con anteriores
// y => cambios funcionales
// z => errores corregidos
// Version                                      *
// Version                                      *
// Version  Definir en 'varFuncionamiento.cpp'  *
// Version                                      *
// Version                                      *


TaskHandle_t mi_comunicacion;

// para formatear la referencia y guardar en NVS si cambió algún
uint8_t cambioAlgunReg = 0;

static void func_com(void *pvParameters);
// static void debug(void);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Atiende un ensaje entrante poniendo en la/s variable/s el valor recibido.
/// @example ' $, #módulo, Esc/Lec, modo, referencia, estados, checkSum, * '
/// @warning $, #módulo, Lec/Esc, ..., checkSum y * deben estar siempre incluidos.
/// @warning Las variables: modo, referencia y estados deben ser enviadas en ese orden;
/// @warning si no se quiere cambiar alguna, se enviará en su lugar -1 salvo que esté después de la que se quiere cambiar.
/// @example $, 2, 1, -1, 10, 12, * (no se envía 'estados' por estar después de la referencia que es el valor a establecer)
/// @example    ^  ^   ^   ^   ^
/// @example    |  |   |   |   └ ckeckSum
/// @example    |  |   |   └──── valor de la referencia a establecer
/// @example    |  |   └──────── el modo no se cambia
/// @example    |  └──────────── indica escritura de alguna variable
/// @example    └─────────────── para el módulo #2
/// @param pvParameters
static void func_com(void *pvParameters)
{
    while (1)
    {
#ifdef WDT_SI
        // Resetea el WDT
        esp_task_wdt_reset();
#endif

        if (atender != 1)
        {
            recibeYanalizaValores();
        }
        else
        {
            if (regs_entrantes[1] == num_modulo)
            {
                if (regs_entrantes[2] == ESCRITURA)
                {
                    if (regs_entrantes[3] != -1)
                    {
                        modo_funcionamiento = regs_entrantes[3];
                    }
                    if (regs_entrantes[4] != -1)
                    {
                        referencia = regs_entrantes[4];
                    }

                    formateaReferencia();
                    guardaNVS_EstadoER();

                    envia_a_Maestro("Recibido Ok\n ");

                    Serial.println("Cambio la referencia y el modo");
                }
                else if (regs_entrantes[2] == LECTURA)
                {
                    armaCadenaValores();

                    digitalWrite(RS485_RW, HIGH);
                    delay(100);
                    RS485_ext.write(cadena_a_enviar);
                    delay(100);
                    digitalWrite(RS485_RW, LOW);

                    Serial.println("Valores enviados.");
                }
            }
            atender = 0;
        }
    }
}

void setup()
{
    //! ***********************************************************
    // tarea nucleo 0
    xTaskCreatePinnedToCore(
        func_com,         // función de la tarea
        "ctrl_Angulo",    // nombre de la tarea
        2048,             // tamaño de la pila
        NULL,             // parámetros de entrada
        0,                // prioridad de la tarea
        &mi_comunicacion, // objeto TaskHandle
        0);               // núcleo donde se correrá la tarea

    // suspendo la tarea
    vTaskSuspend(mi_comunicacion);
    //! ***********************************************************

    // Establece Fosc
    //! en caso de modificar la frecuencia ver 'mideTodo()'
    setCpuFrequencyMhz(240);

#ifdef WDT_SI
    // Inicializa el WDT
    esp_task_wdt_init(WDT_TIMEOUT, true); // Timeout de WDT_TIMEOUT segundos, supervisar ambos núcleos

    //  esp_task_wdt_deinit();

    // Agrega el loop de Arduino al WDT
    esp_task_wdt_add(NULL); // Agrega el loop al WDT

    // agrega la tarea a la vigilancia del WDT
    esp_task_wdt_add(mi_comunicacion);
    // esp_task_wdt_delete(mi_comunicacion);
#endif

    // asigna y configura entradas/salidas
    configuraPines();

    // apaga salida
    digitalWrite(DISP_INT, salida_OF);
    digitalWrite(LED_DEBUG1, LOW);

    // Configura UARTs
    ini_UARTs();

    // configura i2c
    Wire.begin(I2C_SDA, I2C_SCL, 1000000);

    guardaNVS_Claves();

    // inicia display y carga caracteres especiales
    inicializaDisplay();

    // carga valores de la Non Volatile Storage
    leeNVS_claves();
    leeNVS_Caracteristicas();
    leeNVS_EstadoER();
    leeNVS_HsFunc();

    // muestra pantallas iniciales: 'Imastec' y 'Características'
    presentacion();

    pip(50);

    borraPantalla();

#ifdef WDT_SI
    // Desalimentar el WDT temporalmente mientras configura
    esp_task_wdt_delete(NULL);
    esp_task_wdt_delete(mi_comunicacion);
#endif

    // espera 5" por configuración
    configuraInicio();
    // Inicializa PWM
    configura_PWMs();

#ifdef WDT_SI
    // Volver a añadir la tarea al WDT
    esp_task_wdt_add(NULL);
    esp_task_wdt_add(mi_comunicacion);
#ifdef WDT_SI
    // Resetea el WDT
    esp_task_wdt_reset();
#endif
#endif

    // formatea la referencia según el modo
    formateaReferencia();

    configuraInterrupciones();

    neopixelWrite(BUILTIN_LED, 0, 0, 0);

    // habilito tarea
    vTaskResume(mi_comunicacion);

    // enciende salida
    digitalWrite(DISP_INT, salida_ON);
    digitalWrite(LED_DEBUG1, HIGH);

    controlFuentes(referencia);
}

void loop()
{
#ifdef WDT_SI
    // Resetea el WDT
    esp_task_wdt_reset();
#endif
    //prueba_PWM();

    mideTodo();

    analizaAlarmas();

    sobre_I();

    ctaHoras();

    controlFuentes(referencia);

    muestraMedicion(1, 4, estadoEnsayo);

    // suspendo la tarea mientras se opera el encoder
    vTaskSuspend(mi_comunicacion);

    referenciaEncoder();
    menuEncoder();

    // habilito tarea
    vTaskResume(mi_comunicacion);

    // delay(1000);
}

/* void debug(void)
{
    Serial.printf("%i", ensayo1.zh);

    while (digitalRead(SW))
        ;

    delay(200);
}
 */