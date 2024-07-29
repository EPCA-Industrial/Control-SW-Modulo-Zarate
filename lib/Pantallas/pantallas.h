#ifndef __PANTALLAS_H__
#define __PANTALLAS_H__

#include <Arduino.h>
#include "varFuncionamiento.h"


#define OPERADOR 1
#define FABRICANTE 2

void EstableceAcentos(void (*Acentos_a_Ejecutar)(void));
void CargaPantalla(String* PN);
void presentacion(void);
void CargaValoresNominales(void);
void configuraInicio(void);
void CargaMenu_x(String* nm, bool TituloSi_No);
void suelteBoton(void);

void pip(unsigned int mS_duracion);
float encoder(String nombre, float num, int max, int min, uint8_t mult, uint8_t senc, uint8_t co, uint8_t fi, uint8_t _dec);
void menuEncoder(void);
void referenciaEncoder(void);

void formateaReferencia(void);
void muestraReferencia(String etiqueta, float _ref, uint8_t _c, uint8_t _f);
void muestraMedicion(uint8_t _coRef, uint8_t _fiRef, uint8_t _est);
bool valida_Clave(void);
void eligeModoFuncionamiento(void);
void eligeModoDespolarizacion(void);

void Acentos_NO(void);
void AcentosP_Fabricante(void);
void AcentosP_Pant_ER_Mod(void);
void AcentosP_ModoFto(void);
void AcentosPant_Operador_Inicio(void);
/*
void MuestraFinEnsayo6_7 (void);
void MuestraMsjSobreI(void);
void MuestraRef(void); */

unsigned short int EligeOpcionMenu(bool TituloSiNo, String *NombreMenu, char Lineas, char Scl, char Fla);

const char Ohm[8]= {0,4,14,17,17,10,27,0};
const char    a[8]= {2,4,14,1,15,17,15,0};
const char    e[8]= {2,4,14,17,31,16,14,0};
const char    i[8]= {2,4,0,12,4,4,14,0};
const char    o[8]= {2,4,14,17,17,17,14,0};
const char    u[8]= {2,4,17,17,17,17,14,0};
const char    n[8]= {14,0,22,25,17,17,17,0};
const char Preg[8]= {4,0,4,8,16,17,14,0};

/*

const char Pant_Factores[9][20]={
                        "   FACTORES A/D    ",
                        "Vca                ",
                        "Ica                ",
                        "Vcc                ",
                        "Icc                ",
                        "Pot                ",
                        "Temp1              ",
                        "Temp2              ",
                        "Salir              "};

const char Pant_LimitesFto[7][20]={
                        "      LIMITES      ",
                        "V Bateria minima   ",
                        "Io (min. de funto.)",
                        "Pot Off minimo     ",
                        "Pot Off Maximo     ",
                        "Config. remota     ",
                        "Salir              "};
*/

/*
const char Pant_UmbralesOp[7][20]={
                        " UMBRALES ALARMAS  ",
                        "Icc Maxima         ",
                        "Icc minima         ",
                        "Vca minima         ",
                        "Pot. Off minimo    ",
                        "Pot. Off Maximo    ",
                        "Salir              "};

const char Pant_RTC[4][20]={
                        "    HORAS RELOJ    ",     
                        "Pone en hora       ",
                        "Envios automaticos ",
                        "Salir              "};



const char Pant_Si_No[4][20]={
                        "                   ",     
                        "SI                 ",
                        "NO                 ",
                        "                   "};

const char TipoEnsayo[4][20]={
                        "  TIPO DE ENSAYO   ",     
                        "Sin GPS            ",
                        "Con GPS            ",
                        "                   "};

const char Pant_Conf_Ensayo[5][20]={
                        "ENSAYO CARGADO:    ", 
                        "Configura          ",                        
                        "Graba              ",                        
                        "Muestra cargado    ",
                        "Salir              "};

const char Pant_1_2_3[4][20]={                    Para usar si se configura
                        "   N° de ENSAYO    ",      mas de un esnayo     
                        "1                  ",
                        "2                  ",
                        "3                  "};

const char Pant_ParametrosEnsayo[7][20]={
                        "PARAMETROS ENSAYO  ",
                        "Zona Horaria       ",
                        "Estados            ",
                        "Fechas             ",
                        "Horas              ",
                        "Tiempos  On/Off    ",
                        "Salir              "};

const char Pant_Fechas_Ensayo[4][20]={
                        "   FECHAS ENSAYO   ",     
                        "Inicio             ",
                        "Fin                ",
                        "Salir              "};

const char Pant_FechaFin[4][20]={
                        "  DETIENE ENSAYO   ",     
                        "el operador        ",
                        "con fecha fin      ",
                        "                   "};

const char Pant_Horas_Ensayo[4][20]={
                        "   HORAS ENSAYO    ",     
                        "Inicio             ",
                        "Fin                ",
                        "Salir              "};

const char Pant_HusosHorarios[26][20]={
                        "   HUSO HORARIO    ",
                        "UTC -12            ",
                        "UTC -11            ",
                        "UTC -10            ",
                        "UTC - 9            ",
                        "UTC - 8            ",
                        "UTC - 7            ",
                        "UTC - 6            ",
                        "UTC - 5            ",
                        "UTC - 4            ",
                        "UTC - 3            ",
                        "UTC - 2            ",
                        "UTC - 1            ",
                        "UTC   0            ",
                        "UTC + 1            ",
                        "UTC + 2            ",
                        "UTC + 3            ",
                        "UTC + 4            ",
                        "UTC + 5            ",
                        "UTC + 6            ",
                        "UTC + 7            ",
                        "UTC + 8            ",
                        "UTC + 9            ",
                        "UTC +10            ",
                        "UTC +11            ",
                        "UTC +12            "};

const char Pant_EstadosEnsayo[6][20]={
                        "      ESTADOS      ", 
                        "Espera inicio      ",
                        "Inicia ciclo       ",
                        "Finaliza ensayo    ",
                        "Espera nuevo dia   ",
                        "Salir              "};

const char Pant_EstadoON_OFF[4][20]={
                        "                   ",     
                        "OFF                ",
                        "ON                 ",
                        "                   "};

const char Pant_DiaSemana[9][20]={
                        "   DIA DE SEMANA   ",
                        "Domingo            ",
                        "Lunes              ",
                        "Martes             ",
                        "Miercoles          ",
                        "Jueves             ",
                        "Viernes            ",
                        "Sabado             ",                      
                        "Salir              "};

const char Pant_Despolarizacion[4][20]={
                        "  ESPOLARIZACION   ",     
                        "Fija potencial     ",
                        "Fija tiempo        ",
                        "Salir              "}; */

#endif // __PANTALLAS_H__