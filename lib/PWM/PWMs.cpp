#include <Arduino.h>
#include "PWMs.h"
#include "establecePines.h"
#include "display16x2.h"
#include "LiquidCrystal_I2C.h"

extern LiquidCrystal_I2C lcd;

// bits de precisión para el timer del PWM
#define LEDC_TIMER_BITS 8

// Frecuencia base del PWM
#define LEDC_BASE_FREQ 40000

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
    //ledcAnalogWrite(PWM_CANAL_1, ang_pwm);

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