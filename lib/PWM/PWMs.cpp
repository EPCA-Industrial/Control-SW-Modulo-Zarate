#include <Arduino.h>
#include "PWMs.h"
#include "establecePines.h"
#include "display16x2.h"
#include "LiquidCrystal_I2C.h"
#include "varFuncionamiento.h"
#include "varMedicion.h"

extern LiquidCrystal_I2C lcd;

// bits de precisión para el timer del PWM
#define LEDC_TIMER_BITS 8

// Frecuencia base del PWM
//#define LEDC_BASE_FREQ 90000
// Se redefine la frecuencia por cambio de escala de corriente que modificó el hardware
#define LEDC_BASE_FREQ 100000

#define PWM_CANAL_0 0
#define PWM_CANAL_1 1

int ang_pwm = 0;    // angulo del PWM
int fadeAmount = 1; // pasos de incremento/decremento

void configura_PWMs(void)
{
    // ledcWrite(PWM_CANAL_0, 0);
    // ledcWrite(PWM_CANAL_1, 0);
    configura_PWM(LED_DEBUG2, PWM_CANAL_0);
    configura_PWM(PWM_pin21, PWM_CANAL_1);
}

void configura_PWM(uint8_t pin, uint8_t canal)
{ // configura PWM

    if (ledcSetup(canal, LEDC_BASE_FREQ, LEDC_TIMER_BITS))
    {
        Serial.printf("\nPWM configurado OK.\n\n\r");
    }
    else
    {
        Serial.printf("\nERROR configurando PWM.\n\n\r");

        lcd.clear();
        lcd_print_Posicion(5, 2, "ERROR PWM");
        while (digitalRead(SW))
            ;
    }
    // configura el timer y lo vincula al canal
    ledcAttachPin(pin, canal);
}

// el valor tiene que ser entre 0 y valorMax
void ledcAnalogWrite(uint8_t canal, uint32_t valor, uint32_t valorMax = 255)
{
    // calcula duty a partir de 2 ^ LEDC_TIMER_BITS - 1
    uint32_t duty = ((pow(2, LEDC_TIMER_BITS) - 1) / valorMax * min(valor, valorMax));

    Serial.printf("Brillo: %3i Duty: %5i\r", valor, duty);

    // pone el duty en el canal
    ledcWrite(canal, duty);
}

void prueba_PWM(void)
{
    // set the angulo on LEDC
    ledcAnalogWrite(PWM_CANAL_0, ang_pwm);
    ledcAnalogWrite(PWM_CANAL_1, ang_pwm);

    // cambia el angulo la próxima entrada al bucle:
    ang_pwm = ang_pwm + fadeAmount;

    // cambia la deirección y el límite del incremento:
    if (ang_pwm <= 0 || ang_pwm >= 255)
    {
        fadeAmount = -fadeAmount;
    }
}

void fija_Angulo(int _angulo)
{
    ledcWrite(PWM_CANAL_1, _angulo);
}

// Tope máximo de cambio por iteración (paso adaptativo acotado).
// Evita saltos bruscos si el error es muy grande y reduce overshoot.
// Bajado de 10 a 5 para minimizar picos transitorios que puedan activar
// la protección de sobrecorriente por hardware.
#define DELTA_MAX 5

// Rango efectivo del duty_cycle (PWM 8 bits = 0..255).
// Se deja un margen pequeño en los extremos para evitar disparos en el cruce
// por cero (techo) y duty muy chico que pueda quedar sin disparar (piso).
// Subido de 230 a 250 para aprovechar más rango y permitir alcanzar la
// corriente nominal cuando la carga / tensión de red lo exigen.
#define DUTY_MAX 250
#define DUTY_MIN 5

// Rampa de arranque suave (soft-start).
// Durante RAMPA_INICIO_MS desde el primer ajuste — o desde la re-habilitación de la
// salida tras una pausa larga — el tope efectivo crece linealmente de
// DELTA_MAX_INICIAL hasta DELTA_MAX, evitando un salto brusco al energizar.
#define RAMPA_INICIO_MS     2000
#define DELTA_MAX_INICIAL   1
// Si el llamador deja de invocar corrige_PWM por más de PAUSA_RESET_MS,
// se asume que la salida estuvo deshabilitada y se reinicia la rampa.
#define PAUSA_RESET_MS      500

/// @brief Ajusta el ángulo del disparo para tiristores con control proporcional acotado
///        más acumulador fraccionario (elimina error de estado estacionario).
///        El paso es proporcional al error y queda limitado a ±DELTA_MAX por iteración.
///        Se mantiene la banda muerta (±1% en I/V, ±0.5% en P) para evitar hunting.
///        Durante los primeros RAMPA_INICIO_MS desde el (re)arranque, el tope del paso
///        crece linealmente desde DELTA_MAX_INICIAL hasta DELTA_MAX (soft-start).
///        El acumulador fraccionario `incremento_acum` integra los aportes <1 paso entero
///        que de otra forma se truncarían a 0, asegurando que el control siga reduciendo
///        el error hasta entrar en la banda muerta.
/// @param _ref referencia recibida (en décimas de A o décimas de V según modo)
void corrige_PWM(float _ref)
{
    static int32_t tiempo_inicio_rampa = -1;
    static uint32_t ultima_llamada_ms  = 0;
    static float   incremento_acum    = 0.0f;
    static uint8_t ultimo_modo        = 0;
    uint32_t ahora_ms = millis();

    // (Re)inicio: firmware fresco, pausa larga (re-habilitación tras protección) o cambio de modo.
    bool reinicio = false;
    if (tiempo_inicio_rampa < 0 || (ahora_ms - ultima_llamada_ms) > PAUSA_RESET_MS)
    {
        tiempo_inicio_rampa = (int32_t)ahora_ms;
        reinicio = true;
    }
    ultima_llamada_ms = ahora_ms;

    if (reinicio || modo_funcionamiento != ultimo_modo)
    {
        incremento_acum = 0.0f;
    }
    ultimo_modo = modo_funcionamiento;

    // Cálculo del aporte de esta iteración en float (sin truncar todavía).
    float incremento_f = 0.0f;
    bool en_banda = true;

    switch (modo_funcionamiento)
    {
    case MODO_I_CTE:
        _ref = _ref / 10;

        if (_ref > Icc * 1.01)
        {
            incremento_f = 255.0f * (_ref - Icc) / (float)ER_Icc;
            en_banda = false;
        }
        else if (_ref < Icc * 0.99)
        {
            incremento_f = 255.0f * (_ref - Icc) / (float)ER_Icc;
            en_banda = false;
        }

        break;
    case MODO_P_CTE:
        // Pot y _ref son magnitudes positivas que representan potencial negativo,
        // por eso se compara contra -_ref. El signo del incremento se fuerza
        // para respetar la dirección del control que ya tenía el código original.
        if (-_ref > Pot * 1.005)
        {
            incremento_f = -255.0f * fabs(-_ref - Pot) / 6000.0f;
            en_banda = false;
        }
        else if (-_ref < Pot * 0.995)
        {
            incremento_f = 255.0f * fabs(-_ref - Pot) / 6000.0f;
            en_banda = false;
        }

        break;
    case MODO_V_CTE:
        _ref = _ref / 10;

        if (_ref > Vcc * 1.01)
        {
            incremento_f = 255.0f * (_ref - Vcc) / (float)ER_Vcc;
            en_banda = false;
        }
        else if (_ref < Vcc * 0.99)
        {
            incremento_f = 255.0f * (_ref - Vcc) / (float)ER_Vcc;
            en_banda = false;
        }

        break;
    default:
        break;
    }

    // Acumulador fraccionario: si el error es chico el aporte puede ser <1, se va
    // sumando para no perderse al truncar a int. Dentro de la banda muerta se reinicia
    // para evitar arrastres residuales que puedan llevar al control al otro lado.
    if (en_banda)
    {
        incremento_acum = 0.0f;
    }
    else
    {
        incremento_acum += incremento_f;
        // Anti-windup: el acumulador no puede crecer más allá de DELTA_MAX.
        if (incremento_acum >  (float)DELTA_MAX) incremento_acum =  (float)DELTA_MAX;
        else if (incremento_acum < -(float)DELTA_MAX) incremento_acum = -(float)DELTA_MAX;
    }

    // Parte entera del acumulador = paso a aplicar; la fracción queda para la próxima.
    int incremento = (int)incremento_acum;
    incremento_acum -= (float)incremento;

    // Soft-start: durante los primeros RAMPA_INICIO_MS el tope efectivo crece
    // linealmente desde DELTA_MAX_INICIAL hasta DELTA_MAX.
    int32_t transcurrido_ms = (int32_t)ahora_ms - tiempo_inicio_rampa;
    int delta_max_efectivo;
    if (transcurrido_ms < RAMPA_INICIO_MS)
    {
        delta_max_efectivo = DELTA_MAX_INICIAL +
            ((DELTA_MAX - DELTA_MAX_INICIAL) * transcurrido_ms) / RAMPA_INICIO_MS;
    }
    else
    {
        delta_max_efectivo = DELTA_MAX;
    }

    // Acota el paso final; lo no aplicado por la rampa se devuelve al acumulador
    // para no perder corrección efectiva.
    if (incremento > delta_max_efectivo)
    {
        incremento_acum += (float)(incremento - delta_max_efectivo);
        incremento = delta_max_efectivo;
    }
    else if (incremento < -delta_max_efectivo)
    {
        incremento_acum += (float)(incremento + delta_max_efectivo);
        incremento = -delta_max_efectivo;
    }

    duty_cycle = duty_cycle + incremento;

    if (duty_cycle > DUTY_MAX)
    {
        duty_cycle = DUTY_MAX;
    }

    if (duty_cycle < DUTY_MIN)
    {
        duty_cycle = DUTY_MIN;
    }

    fija_Angulo(duty_cycle);

#ifdef COMPILAR_DEBUG
// Serial.printf("V: %5.2f I: %5.2f P: %05.0f Ref: %5.2f TMR: %05u\r", Vcc, Icc, Pot, _ref, (uint64_t)valor);
#endif
}