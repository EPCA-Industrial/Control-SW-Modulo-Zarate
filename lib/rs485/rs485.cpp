#include <Arduino.h>
#include "establecePines.h"
#include "varFuncionamiento.h"
#include "varMedicion.h"
#include "rs485.h"

char charbuffer[50];

// Varables para recibir mensaje del rack superior
const unsigned long timeoutRS485 = 1000; // Timeout en milisegundos
uint8_t cantVariablesRecibidas;          // para recibir Modo, Referencia y Estados
uint16_t buffer[9];
byte byteInicio = 0x24; // '$'
byte byteFin = 0x2A;    // '*'

void ini_UARTs(void)
{
    Serial.begin(115200);
    RS485_ext.begin(28800, SERIAL_8N1, RX_UART2, TX_UART2);
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
    delay(1);
    RS485_ext.write(Str_a_char(_msj));
    delay(50);
    digitalWrite(RS485_RW, LOW);
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

/// @brief Sí el mensaje comienza con '$'; finaliza con '*' y chksum OK
/// @brief eja los valores en regs_entrantes[]
/// @param
/// @return _ok
bool atiendoPeticion(void)
{
    bool _ok = 0;
    uint8_t indice = 0;
    bool recibiendo = false;
    unsigned long startTime = 0;

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
            cantVariablesRecibidas = indice;

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

    for (int i = 0; i < cantVariablesRecibidas; i++)
    {
        byte lowByte = buffer[i] & 0xFF;         // Byte menos significativo
        byte highByte = (buffer[i] >> 8) & 0xFF; // Byte más significativo

        checkSum += lowByte + highByte;
    }

    return checkSum;
}

void armaCadenaValores(void)
{
    chkSum = num_modulo + modo_funcionamiento + (int)(Vcc * 10) + (int)(Icc * 10) + (int)Pot + (int)Temp1 + (int)referencia + hs_FuncH + hs_FuncL + despol_Tiempo + despol_Potencial;

    // Crear una cadena con los valores separados por comas
    sprintf(cadena_a_enviar, "$%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i*\n", num_modulo, modo_funcionamiento, (int)(Vcc * 10), (int)(Icc * 10), (int)Pot, (int)Temp1, (int)referencia, hs_FuncH, hs_FuncL, despol_Tiempo, despol_Potencial, chkSum);

    // Enviar la cadena por Serial1 (o cualquier UART configurada)
    Serial.print("Enviando: ");
    Serial.println(cadena_a_enviar);
}

/**
 * Función: recibeYanalizaValores()

 * Descripción detallada:
 * - La función se encarga de leer datos del puerto RS485_ext, esperando recibir válido para este módulo.
 * - El mensaje debe comenzar con el carácter de inicio (byteInicio, '$') y terminar con el carácter de fin (byteFin, '*').
 * - Durante la recepción:
 *   • Se activa la bandera 'recibiendo' cuando se detecta el byte de inicio y se reinicia el contador 
 *     de variables recibidas (cantVariablesRecibidas).
 *   • Cada carácter recibido se analiza:
 *       - Si es una coma (','), se asume que es un separador entre variables y se incrementa el contador.
 *       - Los demás caracteres se van concatenando a la cadena 'receivedData'.
 * - Al detectar el byte de fin ('*'), se finaliza la recepción y se establece la bandera 'endDetected'.
 * - Una vez completada la recepción, si se detectó correctamente el inicio y se recibió información:
 *   • Se muestra en la consola el mensaje completo recibido.
 *   • Se convierte la cadena recibida en un array de caracteres para facilitar su análisis.
 *   • Se utiliza la función strtok para separar la cadena en tokens usando la coma como delimitador.
 *   • Cada token se convierte a entero y se almacena en el array 'values'.
 * - Se extrae el checksum recibido (último valor en el array) y se calcula un checksum como la suma
 *   de todos los valores anteriores.
 * - Si el checksum calculado coincide con el recibido:
 *   • Se verifica que el primer valor coincida con el identificador del módulo (num_modulo).
 *   • Si es correcto, se copian los valores al arreglo global 'regs_entrantes' y se activa la bandera 'atender'
 *     para el posterior análisis en 'func_com()' en el arcivo 'main.cpp'.
 * - Si el checksum no coincide o la información recibida es inválida, se informa a través de mensajes en
 *   el monitor serial.
 */
void recibeYanalizaValores(void)
{
    bool recibiendo = false;
    String receivedData = "";
    bool startDetected = false;
    bool endDetected = false;
    int calculatedChecksum = 0;

    while (RS485_ext.available())
    {
        char byteRecibido = RS485_ext.read();
        
        //Muestra caracter por caracter recibido
        //Serial.print(byteRecibido);

        if (byteRecibido == byteInicio)
        { // BYTE_INICIO '$'
            startDetected = true;
            recibiendo = true;
            cantVariablesRecibidas = 0;
            receivedData = "";
            endDetected = false;
            calculatedChecksum = 0;
        }
        else if (byteRecibido == byteFin)
        { // BYTE_FIN '*'
            endDetected = true;

            if (recibiendo)
            {
                recibiendo = false;
            }
        }
        else if (recibiendo)
        {
            if (byteRecibido == ',')
            {
                cantVariablesRecibidas++;
            }

            receivedData += byteRecibido;
        }

        if (endDetected)
        {
            // Se ha recibido un mensaje completo
            Serial.println("Se detectó el fin del mensaje $");

            if (startDetected && receivedData.length() > 0)
            {
                Serial.println("startDetected = 1 y receivedData.length() > 0");
                Serial.println(receivedData);

                // Convertir la cadena a un array de caracteres (buffer)
                char buffer[50];
                receivedData.toCharArray(buffer, 50);

                // Separar los valores usando strtok
                char *token = strtok(buffer, ",");
                int values[cantVariablesRecibidas + 1];
                int index = 0;

                while (token != nullptr && index < (cantVariablesRecibidas + 1))
                {
                    values[index] = atoi(token); // Convertir cada token a entero
                    token = strtok(nullptr, ",");
                    index++;
                }

                // Verificar el checksum
                int receivedChecksum = values[cantVariablesRecibidas];
                // int calculatedChecksum = values[0] + values[1] + values[2] + values[3];
                for (uint8_t c = 0; c < (cantVariablesRecibidas); c++)
                {
                    calculatedChecksum = calculatedChecksum + values[c];
                }

                if (receivedChecksum == calculatedChecksum)
                {
                    // Checksum válido, imprimir los valores
                    Serial.print("Valores recibidos correctamente: ");
                    if (values[0] == num_modulo)
                    {
                        for (uint8_t c = 0; c <= (cantVariablesRecibidas); c++)
                        {
                            Serial.print(values[c]);
                            Serial.print(", ");

                            regs_entrantes[c + 1] = values[c];
                        }
                        Serial.println();

                        atender = 1;
                        Serial.println("Atendiendo");
                    }
                    else
                    {
                        atender = 0;
                        Serial.println("No es para este modulo");
                    }
                }
                else
                {
                    // Checksum incorrecto
                    Serial.println("Error: checksum incorrecto");
                }
            }else
            {
                Serial.println("startDetected <> 1 ó receivedData.length() = 0");
            }
        }
    }
}