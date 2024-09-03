#include "usuarios.h"
#include <Arduino.h>
#include <Preferences.h>
#include "establecePines.h"
#include "display16x2.h"
#include "varFuncionamiento.h"
#include "varMedicion.h"

Preferences pref;

unsigned int claveFabricante = 1111;

/// @brief guarda clave fijada en 'claveFabricante'
/// @param
void guardaNVS_Claves(void)
{
    pref.begin("claves", false);

    pref.putInt("claveFab", claveFabricante);

    pref.end();
}

void leeNVS_claves(void)
{
    pref.begin("claves", false);

    claveFabricante = pref.getInt("claveFab");

    pref.end();
}

void guardaNVS_Caracteristicas(void)
{
    pref.begin("cartcas", false);

    pref.putUInt("NSerie", NS);
    pref.putUInt("er_vca", ER_Vca);
    pref.putFloat("v_nom", ER_Vcc);
    pref.putFloat("i_nom", ER_Icc);
    pref.putUInt("num_mod", num_modulo);
    pref.putUInt("er_Hz", Ciclos);
    pref.putUInt("er_fas", Fases);

    pref.end();
}

void leeNVS_Caracteristicas(void)
{
    pref.begin("cartcas", false);

    NS = pref.getUInt("NSerie");
    ER_Vca = pref.getUInt("er_vca");
    ER_Vcc = pref.getFloat("v_nom");
    ER_Icc = pref.getFloat("i_nom");
    num_modulo = pref.getUInt("num_mod");
    Ciclos = pref.getUInt("er_Hz");
    Fases = pref.getUInt("er_fas");

    pref.end();

    delay(200);
}

/// @brief Si el modo de funcionamiento NO es Despolarización, guarda en memoria.
/// @param  
void guardaNVS_EstadoER(void)
{
    if (modo_funcionamiento != MODO_DESPO)
    {
        pref.begin("estado", false);

        pref.putFloat("ref", referencia);
        pref.putUInt("modo", modo_funcionamiento);

        pref.end();
    }
}

void leeNVS_EstadoER(void)
{
    pref.begin("estado", false);

    referencia = pref.getFloat("ref");
    modo_funcionamiento = pref.getUInt("modo");

    pref.end();
}

void guardaNVS_HsFunc(void)
{
    pref.begin("horas", false);

    pref.putUInt("hsH", hs_FuncH);
    pref.putUInt("hsL", hs_FuncL);

    pref.end();
}

void leeNVS_HsFunc(void)
{
    pref.begin("horas", false);

    hs_FuncH = pref.getUInt("hsH");
    hs_FuncL = pref.getUInt("hsL");

    pref.end();
}

bool leeNVS_Primer_Ensayo(void)
{
    String _ok;

    pref.begin("iniNVS", false);
    _ok = pref.getString("ok");
    pref.end();

    if (_ok == "guardado")
    {
        return 1;
    }
    else
    {
        return 0;
    }
}