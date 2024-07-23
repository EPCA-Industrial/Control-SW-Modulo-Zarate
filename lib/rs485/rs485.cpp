#include <Arduino.h>
#include "establecePines.h"
#include "varFuncionamiento.h"
#include "rs485.h"

char charbuffer[50];

// Varables para recibir mensaje del rack superior
const unsigned long timeoutRS485 = 1000; // Timeout en milisegundos
uint8_t largoMsj_Recibido;               // para recbir Modo, Referencia y Estados
uint16_t buffer[9];
byte byteInicio = 0x24; // '$'
byte byteFin = 0x2A;    // '*'

void ini_UARTs(void)
{
    Serial.begin(115200);
    RS485_ext.begin(19200, SERIAL_8N1, RX_UART2, TX_UART2);
}

String atiendeUart_2(void)
{
    String msj_Uart2;

    /*     while (RS485.available() > 0)
        {
            char c = RS485.read();
            msj_Uart2 = msj_Uart2 + c;

            Serial.printf("%02X",c);

            //Serial.write(c);
        } */

    return msj_Uart2;
}

void envia_a_Maestro(String _msj)
{
    digitalWrite(RS485_RW, HIGH);

    // RS485_ext.write(Str_a_char(_msj));

    // digitalWrite(RS485_RW, LOW);
}

void envia_a_Modulos(String _msj)
{
    digitalWrite(RS485_RW, HIGH);

    RS485_int.write(Str_a_char(_msj));

    // digitalWrite(RS485_RW, LOW);
}

/// @brief Pasa un 'String' a 'char'
/// @param _str
/// @return charbuffer
char *Str_a_char(String _str)
{
    unsigned int numDeElementos = (_str.length());

    // char charbuffer[numDeElementos];
    _str.toCharArray(charbuffer, numDeElementos);

    return charbuffer;
}

/// @brief Sí el mensaje comienza con '$' y finaliza con '*' deja los valores en regs_entrantes[]
/// @param
bool atiendoPeticion(void)
{
    bool _ok = 0;
    uint8_t indice = 0;
    bool recibiendo = false;
    unsigned long startTime;

    while (RS485_ext.available())
    {
        uint16_t byteRecibido = RS485_ext.read();

        if (byteRecibido == byteInicio)
        { // BYTE_INICIO '$'
            indice = 0;
            recibiendo = true;
            startTime = millis(); // inicializa hora de inicio de recepción
        }
        else if (byteRecibido == byteFin)
        { // BYTE_FIN '*'
            largoMsj_Recibido = indice;

            indice++;

            if (check_Sum() == buffer[6])
            {
                _ok = 1;

                for (int i = 1; i < indice; i++)
                {
                    regs_entrantes[i] = buffer[i];
                    buffer[i] = 0;
                }
            }

            if (recibiendo)
            {
                recibiendo = false;
            }

            Serial.printf("CheckSum2: %i Largo: %i\n", check_Sum(), indice);
        }
        else if (recibiendo)
        {
            indice++;
            buffer[indice] = byteRecibido;

            Serial.printf("Recibido %i: %i\n\r", indice, byteRecibido);
        }
    }

    if ((millis() - startTime) > timeoutRS485 && recibiendo)
    {
        // Timeout alcanzado, descarta el mensaje
        recibiendo = false;
        Serial.println("Timeout alcanzado, mensaje descartado");
        indice = 0;
    }

    return _ok;
}

/// @brief calcula el checksum del mensaje incluyendo el byte del inicio ($ = 0x24 = 36)
/// @param
/// @return checkSum
uint16_t check_Sum(void)
{
    uint16_t checkSum = byteInicio; // 36 en decimal

    for (int i = 0; i < largoMsj_Recibido; i++)
    {
        byte lowByte = buffer[i] & 0xFF;         // Byte menos significativo
        byte highByte = (buffer[i] >> 8) & 0xFF; // Byte más significativo

        checkSum += lowByte + highByte;
    }

    return checkSum;
}