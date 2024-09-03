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
// Version      PLACA 4005(Ctr-Tel-ER)          *

TaskHandle_t mi_comunicacion;

// para formatear la referencia y guardar en NVS si cambió algún
uint8_t cambioAlgunReg = 0;

bool despolariza = 0;

static void func_com(void *pvParameters);

// static void debug(void);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Atiende un ensaje entrante poniendo en la/s variable/s el valor recibido.
/// @example ' $ #módulo, Esc/Lec/Desp, modo, referencia ó Tiempo Despolarización, estados ó Potencial Despolarización, checkSum * '
/// @warning $ #módulo, Lec/Esc/Desp, ..., checkSum y * deben estar siempre incluidos.
/// @warning Las variables: modo, referencia y estados deben ser enviadas en ese orden;
/// @warning si no se quiere cambiar alguna, se enviará en su lugar -1 salvo que esté después de la que se quiere cambiar.
/// @example $ 2, 1, -1, 10, 12 * (no se envía 'estados' por estar después de la referencia que es el valor a establecer)
/// @example    ^  ^   ^   ^   ^
/// @example    |  |   |   |   └ ckeckSum
/// @example    |  |   |   └──── valor de la referencia a establecer
/// @example    |  |   └──────── el modo no se cambia
/// @example    |  └──────────── indica escritura de alguna variable (= 0 Lee valores; = 1 escribe modo referncia estados; = 2 despolarización)
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

        if (atender == 0)
        {
            // mientras no entre un mensaje queda atento
            recibeYanalizaValores();
        }
        else
        {
            if (regs_entrantes[1] == num_modulo)
            {
                delay(10);
                switch (regs_entrantes[2])
                {
                case ESCRITURA:
                    if (regs_entrantes[3] != -1)
                    {
                        referencia = 0;
                        modo_funcionamiento = regs_entrantes[3];
                    }
                    if (regs_entrantes[4] != -1)
                    {
                        switch (modo_funcionamiento)
                        {
                        case 1:
                            if (regs_entrantes[4] <= ER_Icc * 10)
                            {
                                referencia = regs_entrantes[4];
                            }
                            break;
                        case 3:
                            if (regs_entrantes[4] <= ER_Vcc * 10)
                            {
                                referencia = regs_entrantes[4];
                            }
                            break;
                        default:
                            referencia = regs_entrantes[4];
                            break;
                        }
                    }

                    formateaReferencia();
                    guardaNVS_EstadoER();

                    envia_a_Maestro("Recibido Ok\n ");

                    Serial.println("Cambio la referencia y el modo");
                    Serial.println("Recibido Ok");
                    break;

                case LECTURA:
                    armaCadenaValores();

                    envia_a_Maestro(cadena_a_enviar);

                    Serial.println("Valores enviados.");
                    break;

                case DESPOLAR:
                    despolariza = 1;
                    break;

                default:
                    break;
                }
            }
            //! ********************** BORRAR **************************
            //! else if (regs_entrantes[1] == 2)
            //!{
            //!    envia_a_Maestro("$2,1,28,22,-3000,15,25,1,1,1,0,-2904* ");
            //!    Serial.println("$2,1,28,22,-3000,15,25,1,1,1,0,-2904*");
            //!}
            //! ********************** BORRAR **************************
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

    // Deshabilita el canal PWM asociado al pin BUILTIN_LED
    ledcDetachPin(38);

    //! en algunos ESP32 BUILTIN_LED = 38 y en otros BUILTIN_LED = 48
    //! OJO en este caso se interfiere con SELPIN del SPI
    // neopixelWrite(38, 0, 0, 0);

    // asigna y configura entradas/salidas
    configuraPines();

    // deshabilita salida
    digitalWrite(ENABLE_1, LOW);

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
    // asegura salida en cero
    fija_Angulo(0);

#ifdef WDT_SI
    // Volver a añadir la tarea al WDT
    esp_task_wdt_add(NULL);
    esp_task_wdt_add(mi_comunicacion);
    // Resetea el WDT
    esp_task_wdt_reset();
#endif

    // formatea la referencia según el modo
    formateaReferencia();

    //! configuraInterrupciones();

    // habilito tarea
    vTaskResume(mi_comunicacion);

    // pone un ángulo mínimo para empezar
    fija_Angulo(25);
    delay(500);
    // habilita salida
    digitalWrite(ENABLE_1, HIGH);
}

void loop()
{
#ifdef WDT_SI
    // Resetea el WDT
    esp_task_wdt_reset();
#endif

    // prueba_PWM();
    mideTodo();
    //! analizaAlarmas();
    sobre_I();
    ctaHoras();

    delay(50);
    corrige_PWM(referencia);

    muestraMedicion(1, 4, estadoEnsayo);

    // suspendo la tarea mientras se opera el encoder
    vTaskSuspend(mi_comunicacion);

    esperaEncoder = 1000;
    referenciaEncoder();
    esperaEncoder = 5000;
    menuEncoder();

    // habilito tarea
    vTaskResume(mi_comunicacion);

    if (despolariza)
    {
        despolarizacionRemota();
        despolariza = 0;
    }
}

/* void debug(void)
{
    Serial.printf("%i", ensayo1.zh);

    while (digitalRead(SW))
        ;

    delay(200);
}
 */