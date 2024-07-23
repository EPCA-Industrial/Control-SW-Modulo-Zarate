#include "funciones.h"
#include <Arduino.h>
#include "adc.h"
#include "pantallas.h"
#include "establecePines.h"
#include "varFuncionamiento.h"
#include "varMedicion.h"
#include "usuarios.h"
#include <esp_task_wdt.h>

// Variables locales para el cuenta horas
unsigned long previousMillis = 0;      // Tiempo de referencia para la temporización
unsigned long interval = 3600000;      // Intervalo de tiempo en milisegundos (1 hora)
int accumulatedSeconds = 0;            // Variable para acumular los segundos
const float umbral_Icc_cuentaHs = 0.5; // umbral de corriente para acumular horas de funcionamiento

extern String msj_GPS;

float valor = 9000;

int seg_ant, seg_act;
uint16_t contErrGPS = 0;

/// @brief Ajusta el ángulo del disparo para tiristores
/// @param _ref
/// @return
uint64_t corrige_angulo_TMR(float _ref)
{
    float incremento = 0;

    switch (modo_funcionamiento)
    {
    case MODO_I_CTE:
        _ref = _ref / 10;

        incremento = 100 * (_ref - Icc) / _ref;

        valor -= incremento;

        break;
    case MODO_P_CTE:
        incremento = 100 * (_ref + Pot) / _ref;

        if (Icc < ER_Icc)
        {
            valor -= incremento * 5;
        }
        else
        {
            valor += 50;
        }
        // valor -= incremento * 5;
        break;

    case MODO_V_CTE:
        _ref = _ref / 10;

        incremento = 100 * (_ref - Vcc) / _ref;

        if (Icc < ER_Icc)
        {
            valor -= incremento * 5;
        }
        else
        {
            valor += 50;
        }

        // valor -= incremento * 5;
        break;

    case MODO_M_ELE:
        // incremento = 100 * (_ref - t_Ty) / _ref;
        float t_ty_ref;

        // convierto el valor de la referencia al número que corresponde
        // al disparo para comparar con el valor actual del disparo
        // 9000 (1 - ref/100)
        t_ty_ref = 9000 - 90 * _ref;

        incremento = 100 * (t_ty_ref - t_Ty) / t_ty_ref;

        if (incremento > 0)
        {
            valor += incremento * 10;
        }
        else
        {
            if (Icc < ER_Icc)
            {
                valor += incremento * 5;
            }
            else
            {
                valor += 50;
            }
        }

        //! valor = 9000 - (9000 * _ref / 100);
        break;

    case MODO_INTER:
        valor = 9000 - (9000 * _ref / 100);
        break;

    default:
        break;
    }

    // TMR máximo -> Salida mínima
    if (valor > 9000)
    {
        valor = 9000;
    }
    // TMR mínimo -> Salida Máxima
    if (valor < 50)
    {
        valor = 50;
    }
#ifdef COMPILAR_DEBUG
// Serial.printf("V: %5.2f I: %5.2f P: %05.0f Ref: %5.2f TMR: %05u\r", Vcc, Icc, Pot, _ref, (uint64_t)valor);
#endif

    return (uint64_t)valor;
}

/// @brief pasa valores desde el vector regs[] a variables de funcionamiento
/// @param
void cargaVariables_desdeVector(void)
{
    // carga las variables con el vector 'regs[]' (no los registros Modbus)
    modo_funcionamiento = regs[1];

    referencia = regs[9];

    hs_FuncH = regs[10];
    hs_FuncL = regs[11];
}

/// @brief carga en la variable regs[] (no los registros Modbus), con los valores de las variables
/// @param
void cargaVector_desdeVariables(void)
{
    regs[1] = modo_funcionamiento;

    regs[9] = referencia;
    regs[10] = hs_FuncH;
    regs[11] = hs_FuncL;
/*     regs[12] = ensayo1.tipo;
    regs[13] = ensayo1.zh;
    regs[14] = ensayo1.inicio.hora * 100 + ensayo1.inicio.minuto;
    regs[15] = ensayo1.fin.hora * 100 + ensayo1.fin.minuto;
    regs[16] = ensayo1.t_on;
    regs[17] = ensayo1.t_of;
    regs[18] = ensayo1.inicio.dia * 100 + ensayo1.inicio.mes;
    regs[19] = ensayo1.inicio.ano;
    regs[20] = ensayo1.fin.dia * 100 + ensayo1.fin.mes;
    regs[21] = ensayo1.fin.ano;
    regs[22] = ensayo1.estados; */
}

/// @brief verifica que no se pierdan lecturas del GPS, comparando el segundo actual con el anterior + 1
/// @param
/// @return cont_ERR con la cantidad de errores desde que se conectó el GPS. Descartar los dos primeros
uint16_t validaSegundoSiguiente(void)
{
    seg_act = msj_GPS.substring(10, 12).toInt();

    if (seg_ant != 59)
    {
        if (seg_act != seg_ant + 1)
        {
            // Serial.printf("Error1: %s %i\n", msj_GPS, seg_act);
            contErrGPS++;

#ifdef COMPILAR_DEBUG
            Serial.printf("\nErrores GPS: %i\n", contErrGPS);
#endif
        }
        else
        {
#ifdef COMPILAR_DEBUG
            Serial.printf("OK: %s %02i\r", msj_GPS, seg_act);
#endif
        }
    }

    else if (seg_act != 0)
    {
        // Serial.printf("Error2: %s %i\n", msj_GPS, seg_act);
        contErrGPS++;

#ifdef COMPILAR_DEBUG
        Serial.printf("\nErrores: %i\n", contErrGPS);
#endif
    }
    else
    {
#ifdef COMPILAR_DEBUG
        Serial.printf("OK: %s %02i\r", msj_GPS, seg_act);
#endif
    }
    seg_ant = seg_act;

    return contErrGPS;
}

void ctaHoras(void)
{
    unsigned long currentMillis = millis();

    if (modo_funcionamiento != MODO_INTER)
    {
        if (Icc > umbral_Icc_cuentaHs) // Comprobar si está por encima del umbral
        {
            if (currentMillis - previousMillis >= interval)
            {
                previousMillis = currentMillis; // Actualizar el tiempo de referencia
                hs_FuncL++;                     // Incrementar la variable de horas acumuladas

                guardaNVS_HsFunc();
            }

            if (hs_FuncL > 32767)
            {
                hs_FuncH++;
                hs_FuncL = 0;
            }

#ifdef COMPILAR_DEBUG
            Serial.printf("Minutos acumulados: %i\r", hs_FuncL);
#endif
        }
        else
        {
            previousMillis = currentMillis; // Resetear el tiempo de referencia si está por debajo del umbral
        }
    }
}

/// @brief Función para obtener el estado de un bit en una posición específica
/// @param numero número a analizar
/// @param posicion bit consultado (7... 0)
/// @return estado del bit consultado
bool obtenerEstadoBit(uint8_t numero, uint8_t posicion)
{
    // Desplazar el bit deseado a la posición más baja y realizar una operación AND con 1
    return (numero >> posicion) & 1;
}

void sobre_I(void)
{
    // detecta sobre corriente por hard
    if (digitalRead(SOBRE_I))
    {
        digitalWrite(DISP_INT, salida_OF);
        digitalWrite(LED_DEBUG1, LOW);
        delay(200);

        alarmas = alarmas | 0b00000100;
        regs[23] = alarmas;

        // deshabilita interrupciones
        //! detachInterrupt(digitalPinToInterrupt(PPS));
        //!    timerDetachInterrupt(My_timer);

        // apaga el GPS
        digitalWrite(ON_GPS, LOW);

        Serial.println("\r\nEsperando atencion del operador...\r\n");

        estadoEnsayo = 11;
        muestraMedicion(1, 4, estadoEnsayo);

        do
        {
            delay(1000);

#ifdef WDT_SI
            // Resetea el WDT
            esp_task_wdt_reset();
#endif
        } while (true);
    }
}