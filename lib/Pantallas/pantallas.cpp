#include "pantallas.h"
#include <Arduino.h>
#include "display16x2.h"
#include "LiquidCrystal_I2C.h"
#include "establecePines.h"
#include "usuarios.h"
#include "varFuncionamiento.h"
#include "varMedicion.h"
#include "funciones.h"
#include "adc.h"

extern LiquidCrystal_I2C lcd;

extern unsigned int claveFabricante;

extern uint64_t t_Ty;

String txt_referencia;
uint8_t digitos, dec;
uint8_t sensibilidad, multiplicador; // para cambiar reacción del encoder
unsigned int maximo, minimo;

// para alternar pantallas en 'muestraMedicion()'
unsigned long muestra_ant;
unsigned int intervaloMuestreo = 2000;
uint8_t pant = 1;

int Scroll = 0;
int Flecha = 2;

bool Vca_Tp1;
char AuxTxt[16];

float aux_referencia;

extern uint8_t estadoEnsayo;
extern bool bandEnsayoSinGPS;
// extern ensayoOnOff ensayo1;

void (*Acentos_que_se_ejecutara)(void);

void EstableceAcentos(void (*Acentos_a_Ejecutar)(void))
{
    Acentos_que_se_ejecutara = Acentos_a_Ejecutar;
}

String Pant_Presentacion[3] = {
    "  EPCA+ImasTec  ",
    "www.imastec.com ",
    "                "};

String Pant_Caracteristicas[4] = {
    "NS:        M:   ",
    "Vc:     Ic:     ",
    "Va:     Cs:     ",
    "Fases:          "};

String Pant_Contsenias[4] = {
    "    ACCESOS     ",
    "Operador        ",
    "Fabricante      ",
    "Salir           "};

/* String Pant_ER_Mod[4] = {
    "      TIPO      ",
    "Equipo          ",
    "Modulo          ",
    "Salir           "}; */

String Pant_Fabricante[10] = {
    "CARACTERISTICAS ",
    "Nro. de Serie   ",
    "Tension entrada ",
    "Frecuencia      ",
    "Fases           ",
    "Tension salida  ",
    "Corrte. salida  ",
    "Corrte. minima  ",
    "Nro. de modulo  ",
    "Salir           "};

String Pant_ModoFto[5] = {
    "      MODO      ",
    "Icc constante   ",
    "Pot constante   ",
    "Manual elect.   ",
    "Vcc constante   "};

String Pant_Operador_Inicio[4] = {
    "   CONFIGURA    ",
    "Modo funcionam. ",
    "Nro. modulo     ",
    "Salir           "};

String Pant_EstadoON_OFF[4] = {
    "                ",
    "OFF             ",
    "ON              ",
    "                "};

void CargaPantalla(String *PN)
{
    // Carga la pantalla 'PN' definida en 'Pantallas.h'
    lcd_print_Posicion(1, 1, PN[0]);
    lcd_print_Posicion(1, 2, PN[Scroll + 1]);
    lcd_print_Posicion(1, 3, PN[Scroll + 2]);
    lcd_print_Posicion(1, 4, PN[Scroll + 3]);
}

void presentacion(void)
{
    // muestra pantalla inicial
    CargaPantalla(Pant_Presentacion);
    lcd_print_Posicion(7, 4, VERSION);
    delay(2000);
    CargaPantalla(Pant_Caracteristicas);
    CargaValoresNominales();
    delay(3000);
}

void CargaValoresNominales(void)
{
    sprintf(AuxTxt, "%5u", NS);
    lcd_print_Posicion(5, 1, AuxTxt);

    sprintf(AuxTxt, "%3u", ER_Vcc);
    lcd_print_Posicion(5, 2, AuxTxt);

    sprintf(AuxTxt, "%3u", ER_Icc);
    lcd_print_Posicion(12, 2, AuxTxt);

    sprintf(AuxTxt, "%3u", ER_Vca);
    lcd_print_Posicion(5, 3, AuxTxt);

    sprintf(AuxTxt, "%3u", Ciclos);
    lcd_print_Posicion(12, 3, AuxTxt);

    sprintf(AuxTxt, "%2u", Fases);
    lcd_print_Posicion(7, 4, AuxTxt);

    sprintf(AuxTxt, "%u", num_modulo);
    lcd_print_Posicion(14, 1, AuxTxt);
}

void configuraInicio(void)
{
    uint8_t menu_Nromodulo = 0;
    uint8_t opcion_0 = 0;
    uint8_t opcion_01 = 0;
    uint8_t opcion_02 = 0;

    char auxTxt[16];
    // unsigned long tant;
    uint8_t cta = 6;
    bool config = 0;
    // unsigned short int opcion = 0;

    lcd.clear();

    do
    {
        lcd_print_Posicion(1, 1, "Presione para   ");
        sprintf(auxTxt, "configurar   %i", cta);
        lcd_print_Posicion(1, 2, auxTxt);
        delay(1000);
        cta--;

        if (!digitalRead(SW))
        {
            config = 1;
            break;
        }
    } while (digitalRead(SW) && cta > 0);

    if (config)
    {
        configurando = true;

        suelteBoton();

        EstableceAcentos(Acentos_NO);
        opcion_0 = EligeOpcionMenu(1, Pant_Contsenias, sizeof(Pant_Contsenias) / sizeof(Pant_Contsenias[0]), 0, 1);

        lcd.clear();

        do
        {
            switch (opcion_0)
            {
            case OPERADOR: // Menú operador
                EstableceAcentos(AcentosPant_Operador_Inicio);
                menu_Nromodulo = EligeOpcionMenu(1, Pant_Operador_Inicio, sizeof(Pant_Operador_Inicio) / sizeof(Pant_Operador_Inicio[0]), 0, 1);

                if (menu_Nromodulo == 1)
                {
                    EstableceAcentos(AcentosP_ModoFto);
                    modo_funcionamiento = EligeOpcionMenu(1, Pant_ModoFto, sizeof(Pant_ModoFto) / sizeof(Pant_ModoFto[0]), 0, 1);

                    // pasa el modo_funcionamiento al
                    regs[1] = modo_funcionamiento;

                    referencia = 0;
                    regs[9] = referencia;

                    cargaVector_desdeVariables();

                    guardaNVS_EstadoER();

                    formateaReferencia();

                    lcd.clear();
                }
                else
                {
                    lcd.clear();
                    digitos = 1;
                    num_modulo = encoder("# Mod.: ", num_modulo, 9, 1, 1, 180, 1, 4, 0);
                }

                opcion_0 = 3; // para salir del menú superior

                break;
            case FABRICANTE: // Menú fabricante

                txt_referencia = "";
                digitos = 4;
                dec = 0;
                maximo = 9999;
                minimo = 1;
                multiplicador = 1;
                sensibilidad = 80;

                if (valida_Clave())
                {
                    do
                    {
                        EstableceAcentos(AcentosP_Fabricante);
                        opcion_02 = EligeOpcionMenu(1, Pant_Fabricante, sizeof(Pant_Fabricante) / sizeof(Pant_Fabricante[0]), 0, 1);

                        lcd.clear();

                        switch (opcion_02)
                        {
                        case 1: // Número de serie
                            digitos = 5;
                            NS = encoder("NS: ", NS, 40000, 20000, 10, 50, 1, 4, 0);

                            break;
                        case 2: // Tensión de entrada
                            digitos = 3;
                            ER_Vca = encoder("Vca: ", ER_Vca, 2000, 50, 10, 80, 1, 4, 0);

                            break;
                        case 3: // Frecuencia
                            digitos = 2;
                            Ciclos = encoder("Hz: ", Ciclos, 60, 50, 1, 180, 1, 4, 0);

                            break;
                        case 4: // Fases
                            digitos = 1;
                            Fases = encoder("Fases: ", Fases, 6, 1, 1, 180, 1, 4, 0);

                            break;

                        case 5: // Tensión de salida
                            digitos = 3;
                            ER_Vcc = encoder("Vcc: ", ER_Vcc, 300, 1, 10, 180, 1, 4, 0);

                            break;
                        case 6: // Corriente de salida
                            digitos = 3;
                            ER_Icc = encoder("Icc: ", ER_Icc, 300, 1, 10, 180, 1, 4, 0);

                            break;
                        case 7: // Corriente mínima
                            digitos = 3;
                            Imin = encoder("Imin: ", Imin, 300, 1, 10, 180, 1, 4, 1);

                            break;
                        case 8: // Número de módulo
                            digitos = 1;
                            num_modulo = encoder("# Mod.: ", num_modulo, 9, 1, 1, 180, 1, 4, 0);

                            break;
                        case 9: // sale
                            // sale
                            break;
                        }

                        guardaNVS_Caracteristicas();

                    } while (opcion_02 != 9);

                    opcion_0 = 3; // para salir del menú superior

                    delay(200);
                }

                break;

            case 3:
                /* code */
                break;

            default:
                break;
            }
        } while (opcion_0 != 3);

        configurando = false;
    }

    lcd.clear();

    muestraReferencia("R: ", referencia, 1, 4);
}

void CargaMenu_x(String *NM, bool TituloSi_No)
{
    // Carga la pantalla 'MN' definida en 'Pantallas.h'
    // con o sin la 1ra línea 'TituloSi_No' y posiciona la flecha
    lcd_print_Posicion(1, 1, " ");
    lcd_print_Posicion(1, 2, " ");
    lcd_print_Posicion(1, 3, " ");
    lcd_print_Posicion(1, 4, " ");

    if (TituloSi_No)
    {
        lcd_print_Posicion(1, 1, NM[0]);
    }
    lcd_print_Posicion(2, 2, NM[Scroll + 1]);
    lcd_print_Posicion(2, 3, NM[Scroll + 2]);
    lcd_print_Posicion(2, 4, NM[Scroll + 3]);

    if (Acentos_que_se_ejecutara)
    {
        Acentos_que_se_ejecutara();
    }

    // lcd.setCursor(0, Flecha);
    // lcd.write(246);

    lcd_print_Posicion(1, Flecha + 1, ">");
}

void Acentos_NO(void)
{
    // sin acentos
}

void AcentosP_Fabricante(void)
{
    if ((3 - Scroll) > 1 && (3 - Scroll) < 5)
    {
        lcd_print_Posicion(7, 3 - Scroll, "");
        // lcd.setCursor(9, 2 - Scroll);
        lcd.write(4);
    }
    /*     if ((6 - Scroll) > 1 && (6 - Scroll) < 5)
        {
            lcd_print_Posicion(12, 6 - Scroll, "");
            lcd.write(4);
        } */
    if ((6 - Scroll) > 1 && (6 - Scroll) < 5)
    {
        lcd_print_Posicion(7, 6 - Scroll, "");
        lcd.write(4);
    }
    if ((8 - Scroll) > 1 && (8 - Scroll) < 5)
    {
        lcd_print_Posicion(11, 8 - Scroll, "");
        lcd.write(3);
    }
    if ((9 - Scroll) > 1 && (9 - Scroll) < 5)
    {
        lcd_print_Posicion(11, 9 - Scroll, "");
        lcd.write(4);
    }
}

void AcentosP_Pant_ER_Mod(void)
{
    if ((3 - Scroll) > 1 && (3 - Scroll) < 5)
    {
        lcd_print_Posicion(3, 3 - Scroll, "");
        lcd.write(4);
    }
}

void AcentosP_ModoFto(void)
{
    if ((7 - Scroll) > 1 && (7 - Scroll) < 5)
    {
        lcd_print_Posicion(15, 7 - Scroll, "");
        lcd.write(4);
    }
}

void AcentosPant_Operador_Inicio(void)
{
    if ((3 - Scroll) > 1 && (3 - Scroll) < 5)
    {
        lcd_print_Posicion(8, 3 - Scroll, "");
        lcd.write(4);
    }
}
/*
void MuestraFinEnsayo6_7(void)
{
    int CuentaDelay;

    lcd_clear_display();

    lcd_print_with_position(0, 1, "Despolarizado.      ");

    if (ModoFunc == DespolarPot)
    {
        sprintf(AuxTxt, "Pr:  -%i", PotEnsayo6_7);
        lcd_print_with_position(0, 2, AuxTxt);

        sprintf(AuxTxt, "Pot: %5.0f", PotRtaEnsayo_7);
        lcd_print_with_position(0, 3, AuxTxt);

        sprintf(AuxTxt, "Tt: %i min", TpoRtaEnsayo_6);
        lcd_print_with_position(0, 4, AuxTxt);
    }
    else if (ModoFunc == DespolarTpo)
    {
        sprintf(AuxTxt, "Tr: %i min", TiempoEnsayo6_7);
        lcd_print_with_position(0, 2, AuxTxt);

        sprintf(AuxTxt, "Pot: %5.0f", PotRtaEnsayo_7);
        lcd_print_with_position(0, 4, AuxTxt);

        sprintf(AuxTxt, "Tt: %i min", TpoRtaEnsayo_6);
        lcd_print_with_position(0, 3, AuxTxt);
    }

    do
    {
        __delay_ms(100);
        CuentaDelay++;
    } while (SW && CuentaDelay < 500);
    __delay_ms(200);

    CuentaDelay = 0;
}
void MuestraMsjSobreI(void)
{
    PIE11bits.TMR4IE = 0;
    LD_Si;

    lcd_print_with_position(0, 1, "    ATENCION        ");
    lcd_print_with_position(0, 2, "Salida interrumpida ");
    lcd_print_with_position(0, 3, "Exceso de corriente ");
    lcd_print_with_position(0, 4, " Apague y VERIFIQUE ");
}
*/

unsigned short int EligeOpcionMenu(bool TituloSiNo, String *NombreMenu, char Lineas, char Scl, char Fla)
{
    int ValAct = 0;
    int ValAnt = 0;
    // Carga Pantalla con la flecha en la opción que corresponda
    Scroll = Scl;
    Flecha = Fla;
    CargaMenu_x(NombreMenu, TituloSiNo);

    // espera que se suelte el botón
    while (!digitalRead(SW))
        ;
    delay(100);

    //--------LEE EL GIRO DEL ENCODER HASTA QUE SE PRESIONE ----------------------//
    do
    {
        ValAnt = ValAct; // Igualamos 'ValAnt' y 'ValAct' para luego comparar cuando cambie 'ValAct'.

        ValAct = digitalRead(SW_A) + digitalRead(SW_B) * 2; // Aislamos encoder como un número de 2 bits y se carga en la variable 'ValAct'.

        delay(20);

        if ((ValAnt == 3) && (ValAct == 2))
        { // Si en la comparación hay flanco de subida,
            if (Flecha > 2)
            {
                Scroll++;
                if (Scroll > Lineas - 4)
                    Scroll = Lineas - 4;
            }
            else
            {
                lcd_print_Posicion(0, Flecha, " ");
                Flecha++;
                if (Flecha > 3)
                    Flecha = 3;
                if (Flecha > Lineas)
                    Flecha--;
            }

            CargaMenu_x(NombreMenu, TituloSiNo);
        }
        if ((ValAnt == 3) && (ValAct == 1))
        { // Si en la comparación hay flanco de bajada,
            if (Flecha > 1)
            {
                lcd_print_Posicion(0, Flecha, " ");
                Flecha--;
            }
            else
            {
                Scroll--;
                if (Scroll < 0)
                    Scroll = 0;
            }

            CargaMenu_x(NombreMenu, TituloSiNo);
        }

        // sprintf(AuxTxt, "S:%02i F:%02i", Scroll, Flecha);
        // lcd_print_Posicion(10, 3, AuxTxt);

    } while (digitalRead(SW));

    //!  ClrWdt();

    delay(500);
    return (unsigned short int)Scroll + (unsigned short int)Flecha;
}

void pip(unsigned int mS_duracion)
{
    digitalWrite(BUZZER, 1);
    delay(mS_duracion);
    digitalWrite(BUZZER, 0);
}

/// @brief
/// @param nombre
/// @param num
/// @param max
/// @param min
/// @param mult
/// @param senc
/// @param co
/// @param fi
/// @param _dec
/// @return
float encoder(String nombre, float num, int max, int min, uint8_t mult, uint8_t senc, uint8_t co, uint8_t fi, uint8_t _dec)
{
    // char auxTxt[20];
    float N = num * pow(10, _dec);
    unsigned long t_ant, t_act;
    int inc = 1;
    uint8_t cont = 0;

    int ValAnt, ValAct;

    t_ant = millis();

    while (true)
    {
        ValAnt = ValAct; // Igualamos 'ValAnt' y 'ValAct' para luego comparar cuando cambie 'ValAct'.

        ValAct = digitalRead(SW_A) + digitalRead(SW_B) * 2; // Aislamos encoder como un número de 2 bits y se carga en la variable 'ValAct'.

        t_act = millis();

        if ((ValAnt == 3) && (ValAct == 2))
        { // Si en la comparación hay flanco de subida,
            t_ant = millis();
            N = N + inc;
            if (N > max)
            {
                N = max;
            }
        }

        if ((ValAnt == 3) && (ValAct == 1))
        { // Si en la comparación hay flanco de bajada,
            t_ant = millis();
            N = N - inc;
            if (N < min)
            {
                N = min;
            }
        }

        if (t_act < (t_ant + senc))
        {
            cont++;

            if (cont > 15)
            {
                inc = inc * mult;
                if (inc > 1000)
                {
                    inc = 1000;
                }
                cont = 0;
            }
        }
        else
        {
            inc = 1;
        }

        // delay(10);

        if (!configurando)
        {
            mideTodo();

            t_Ty = corrige_angulo_TMR(N / pow(10, _dec));

            muestraMedicion(1, 4, estadoEnsayo);
            muestraReferencia(nombre, N / pow(10, _dec), co, fi);
        }
        else
        {
            // Serial.printf("%6i\r", N);
            muestraReferencia(nombre, N, co, fi);
        }

        if (modo_funcionamiento != MODO_INTER)
        {

            if (millis() > (t_ant + 5000))
            {
                break;
            }
        }

        if (!digitalRead(SW))
        {
            break;
        }
    }
    delay(100);
    return (N / pow(10, _dec));
}

/// @brief Elige ítem del menú o Fija la referencia
/// @param
void menuEncoder(void)
{
    char auxTxt[16];
    unsigned long tant;
    uint8_t cta = 6;
    bool ajustar = 0;
    // unsigned short int opcion = 0;

    if (!digitalRead(SW))
    {
        ajustar = 1;
        lcd.clear();
    }

    tant = millis();
    do
    {
        if (millis() > tant + 1000)
        {
            sprintf(auxTxt, "Resetea en: %i", cta);
            lcd_print_Posicion(1, 2, auxTxt);
            delay(1000);
            cta--;
        }
    } while (!digitalRead(SW));

    if (ajustar)
    {
        EstableceAcentos(AcentosP_ModoFto);
        modo_funcionamiento = EligeOpcionMenu(1, Pant_ModoFto, sizeof(Pant_ModoFto) / sizeof(Pant_ModoFto[0]), 0, 1);
        // pasa el modo_funcionamiento al
        regs[1] = modo_funcionamiento;

        referencia = 0;
        regs[9] = referencia;

        cargaVector_desdeVariables();

        guardaNVS_EstadoER();

        formateaReferencia();

        lcd.clear();
    }

    num_Pantalla = 1;

    muestraReferencia("R: ", referencia, 1, 4);
}

void suelteBoton(void)
{
    lcd.clear();

    lcd_print_Posicion(1, 1, "Suelte el boton");
    lcd.setCursor(13, 0);
    lcd.write(4);

    delay(50);
    while (!digitalRead(SW))
        ;
    delay(200);

    lcd.clear();
}

void formateaReferencia(void)
{
    switch (modo_funcionamiento)
    {
    case MODO_I_CTE:
        txt_referencia = "A";
        digitos = 4;
        dec = 1;
        maximo = 10 * ER_Icc;
        minimo = 1;
        multiplicador = 10;
        sensibilidad = 200;

        break;

    case MODO_P_CTE:
        txt_referencia = "mV";
        digitos = 4;
        dec = 0;
        maximo = 3000;
        minimo = 100;
        multiplicador = 10;
        sensibilidad = 250;

        break;
    case MODO_M_ELE:
        txt_referencia = "%";
        digitos = 2;
        dec = 0;
        maximo = 99;
        minimo = 1;
        multiplicador = 1;
        sensibilidad = 180;

        break;
    case MODO_V_CTE:
        txt_referencia = "V";
        digitos = 4;
        dec = 1;
        maximo = 10 * ER_Vcc /*  * pow10(dec) */;
        minimo = 1;
        multiplicador = 10;
        sensibilidad = 200;

        break;
    case MODO_INTER:
        break;
    case MODO_DESPO:
        break;

    default:
        break;
    }
}

void muestraReferencia(String etiqueta, float _ref, uint8_t _c, uint8_t _f)
{
    char txt[16];

    if (modo_funcionamiento == MODO_P_CTE)
    {
        etiqueta = "R:-";
    }
    if (!configurando)
    {
        if (modo_funcionamiento == MODO_I_CTE || modo_funcionamiento == MODO_V_CTE)
        {
            _ref /= 10;
        }
    }
    sprintf(txt, "%s%0*.*f", etiqueta, digitos, dec, _ref);
    lcd_print_Posicion(_c, _f, txt + txt_referencia);
}

void muestraMedicion(uint8_t _coRef, uint8_t _fiRef, uint8_t _est)
{
    char bufferTxt[16];

    if (_est == 11)
    {
        lcd.clear();
        lcd_print_Posicion(1, 2, "SOBRE-CORRIENTE ");
        lcd_print_Posicion(1, 3, "Atender-Resetear");

        return;
    }

    if ((muestra_ant + intervaloMuestreo) < millis())
    {
        if (pant == 1)
        {
            lcd_print_Posicion(0, 1, "        ");
            muestraFloat("V: ", Vcc, 4, 1, 0, 0);
            muestraFloat("I: ", Icc, 4, 1, 8, 0);
            // borra atrás del potencial
            lcd_print_Posicion(9, 2, "       ");
            muestraFloat("P: ", Pot, 5, 0, 0, 1);

            pant = 2;
        }
        else if (pant == 2)
        {
            // muestraFloat("Va: ", Vca, 3, 0, 0, 0);
            lcd_print_Posicion(8, 1, "         ");
            muestraFloat("Bt: ", VBat, 4, 1, 0, 0);

            sprintf(bufferTxt, "Hs: %li", (unsigned long)hs_FuncH * 32767 + (unsigned long)hs_FuncL);
            lcd_print_Posicion(1, 2, "        ");
            lcd_print_Posicion(1, 2, bufferTxt);

            if (t_Ty > 3150)
            {
                lcd_print_Posicion(1, 3, "Baje regulacion ");
                lcd.setCursor(9, 2);
                lcd.write(4);
            }
            else
            {
                lcd_print_Posicion(1, 3, "Salida continua ");
                lcd.setCursor(7, 2);
                lcd.write(3);
            }

            pant = 1;
        }

        muestra_ant = millis();
    }
}

/// @brief ingresa clave y la compara con la guardad en NVS
/// @param
/// @return 1 si son iguales, sino 0
bool valida_Clave(void)
{
    bool _ok = 0;
    unsigned int numClave;
    uint8_t intentos = 1;

    do
    {
        lcd.clear();

        lcd_print_Posicion(1, 1, "Ingrese clave:");
        numClave = encoder("", 0, 9999, 0, 10, 100, 5, 3, 0);

        if (claveFabricante == numClave)
        {
            lcd.clear();
            lcd_print_Posicion(1, 1, "Clave:");
            lcd_print_Posicion(8, 3, "OK");
            _ok = 1;
            break;
        }
        else
        {

            lcd.clear();
            lcd_print_Posicion(1, 1, "Clave:");
            lcd_print_Posicion(8, 3, "ERROR");

            delay(100);
            while (digitalRead(SW))
                ;
            delay(300);

            intentos++;
        }
    } while (intentos < 4);

    return _ok;
}

void referenciaEncoder(void)
{
    if (!digitalRead(SW_A) || !digitalRead(SW_B))
    {
        formateaReferencia();

        referencia = encoder("R: ", referencia, maximo, minimo, multiplicador, sensibilidad, 1, 4, 0);

        // pasa la referencia al registro
        regs[9] = referencia;
        // delay(20);
        guardaNVS_EstadoER();
    }
}