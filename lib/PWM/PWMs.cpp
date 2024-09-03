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
#define LEDC_BASE_FREQ 120000

#define PWM_CANAL_0 0
#define PWM_CANAL_1 1

int ang_pwm = 0;    // angulo del PWM
int fadeAmount = 1; // pasos de incremento/decremento

void configura_PWMs(void)
{
    // ledcWrite(PWM_CANAL_0, 0);
    // ledcWrite(PWM_CANAL_1, 0);
    configura_PWM(LED_DEBUG2, PWM_CANAL_0);
    configura_PWM(PWM_pin18, PWM_CANAL_1);
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

/// @brief Ajusta el ángulo del disparo para tiristores
/// @param _ref
/// @return
void corrige_PWM(float _ref)
{
    int incremento = 0;

    switch (modo_funcionamiento)
    {
    case MODO_I_CTE:
        _ref = _ref / 10;

        if (_ref > Icc * 1.01)
        {
            duty_cycle = duty_cycle + 1;
        }
        else if (_ref < Icc * 0.99)
        {
            duty_cycle = duty_cycle - 1;
        }

        //incremento = (int)(255 * (_ref - Icc) / ER_Icc);

        break;
    case MODO_P_CTE:
        incremento = (int)(255 * (_ref + Pot) / 6000);

        break;
    case MODO_V_CTE:
        _ref = _ref / 10;

        if (_ref > Vcc * 1.01)
        {
            duty_cycle = duty_cycle + 1;
        }
        else if (_ref < Vcc * 0.99)
        {
            duty_cycle = duty_cycle - 1;
        }

        //incremento = (int)(255 * (_ref - Vcc) / ER_Vcc);

        break;
    default:
        break;
    }

    //duty_cycle = duty_cycle + incremento;

    if (duty_cycle > 255)
    {
        duty_cycle = 255;
    }

    if (duty_cycle < 1)
    {
        duty_cycle = 1;
    }

    fija_Angulo(duty_cycle);

#ifdef COMPILAR_DEBUG
// Serial.printf("V: %5.2f I: %5.2f P: %05.0f Ref: %5.2f TMR: %05u\r", Vcc, Icc, Pot, _ref, (uint64_t)valor);
#endif
}