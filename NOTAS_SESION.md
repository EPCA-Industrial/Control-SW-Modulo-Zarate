# Notas de Sesión — Auditoría y Modificaciones del Módulo Rectificador

Documento resumen de los análisis y cambios realizados sobre el firmware del módulo rectificador (ESP32-S3) que forma parte del sistema de 5 módulos coordinados por un rack general por RS485.

---

## 1. Resumen del Sistema

El sistema completo tiene dos firmwares independientes en PlatformIO + Arduino para ESP32-S3:

- **Módulo rectificador** (este proyecto): `-20682-Daber-Modulo`
- **Rack general** (proyecto hermano): `-20682-Daber-Rack-main`

### Topología de comunicaciones

```
SCADA externo  <--Modbus RTU @19200--> Rack (UART2)
                                         |
                                         | (UART0 @28800, protocolo ASCII propio)
                                         v
                            [Módulo1] [Módulo2] [Módulo3] [Módulo4] [Módulo5]
                            (cada uno con su num_modulo único)

Rack <--UART1 @19200--> GPS (NMEA RMC)
Rack <-- PPS GPIO 8 --- GPS (sincronización 1s)
```

### Protocolo ASCII Rack ↔ Módulos

No es Modbus. Es un protocolo propio basado en texto con marcador `$ ... *` y checksum por suma de campos:

- **Lectura (rack → módulo):** `$num_modulo,0,checksum*`
- **Escritura modo/referencia:** `$num_modulo,1,modo,referencia,checksum*`
- **Despolarización:** `$num_modulo,2,tiempo_o_-1,potencial_o_-1,checksum*`
- **Respuesta del módulo:** `$num_modulo,modo,Vcc*10,Icc*10,Pot,Temp1,referencia,HsH,HsL,despol_T,despol_P,checksum*`

El campo `-1` se usa para indicar "no modificar este parámetro".

### Sincronización GPS

La sincronización temporal vive en el rack, no en los módulos. El rack:

1. Detecta automáticamente el baudrate del GPS y lo configura a 19200 bps.
2. Configura el GPS para emitir solo tramas RMC (`PMTK314`).
3. Usa PPS en GPIO 8 + un timer hardware de 1s para mantener cadencia incluso ante pérdidas de PPS.
4. Compara fecha/hora corregida por zona horaria contra `ensayo1.inicio` para arrancar el ciclo ON/OFF en el momento exacto.

---

## 2. Análisis del Módulo

### Estructura del firmware

| Ubicación | Rol |
|---|---|
| `src/main.cpp` | `setup`, `loop`, tarea FreeRTOS `func_com` (RS485) |
| `lib/EstablecePines/` | Macros de pines + `configuraPines()` |
| `lib/varGlobales/varFuncionamiento.*` | Estado global (`referencia`, `modo_funcionamiento`, `regs_entrantes[]`, etc.) |
| `lib/varGlobales/varMedicion.*` | `Vcc`, `Icc`, `Pot`, `Temp1` |
| `lib/rs485/` | UART2 + parser ASCII `$...*` |
| `lib/ADC/` | ADC externo MCP3208 por SPI bit-bang |
| `lib/PWM/` | LEDC canal 1 (disparo de tiristores), canal 0 (LED debug) |
| `lib/Pantallas/` `lib/Display16x2/` | LCD I2C 16x4 + menús + encoder |
| `lib/usuarios/` | NVS (`Preferences`) con mutex |
| `lib/interrupciones/` | ISR en GPIO 8 (interruptor de habilitación) |
| `lib/Funciones/` | Sobrecorriente, despolarización, cuenta horas |

### Flujo de funcionamiento

**`setup()`:**

1. Crea tarea FreeRTOS `func_com` en core 0 (suspendida).
2. CPU a 240 MHz.
3. Configura WDT (`WDT_TIMEOUT = 300s`).
4. `configuraPines()`, inicializa salidas en estado seguro.
5. `ini_UARTs()` — `Serial`@115200, `RS485_ext`@28800 (UART2).
6. I2C @ 100 kHz, NVS con valores nominales y referencia/modo guardados.
7. Pantallas de presentación + ventana de configuración por encoder.
8. PWM, interrupciones, resume de tarea de comunicación.
9. Habilita salida (`ENABLE_1`).

**`loop()`:**

1. Sincroniza `ENABLE_1` con `DISP_INT`.
2. `mideTodo()`: Vcc, Icc, Pot, Temp1.
3. `sobre_I()`: protección por hardware + 110% de I nominal.
4. `ctaHoras()`: cuenta horas si I > umbral.
5. `corrige_PWM(referencia)`: lazo de control proporcional.
6. Refresca LCD.
7. Suspende `func_com`, opera encoder/menú, reanuda.
8. Si `despolariza` está activa, ejecuta `despolarizacionRemota()`.

**Tarea `func_com` (core 0):**

- Si `atender == 0`: llama `recibeYanalizaValores()` para parsear tramas entrantes.
- Si `atender != 0` y `regs_entrantes[1] == num_modulo`: despacha acción (`ESCRITURA=1`, `LECTURA=0`, `DESPOLAR=2`).

### Mapeo de GPIOs

| GPIO | Símbolo | Dirección/Modo | Función |
|---:|---|---|---|
| 1 | `I2C_SCL` | I2C clock | LCD `0x27` |
| 2 | `I2C_SDA` | I2C data | LCD |
| 5 | `OUT_DIG_05` | Salida | Auxiliar libre |
| 6 | `SW_B` | Entrada pull-up | Encoder canal B |
| 7 | `SW_A` | Entrada pull-up | Encoder canal A |
| 8 | `DISP_INT` | Entrada pull-up + ISR `CHANGE` | Interruptor / habilitación |
| 11 | `RS485_RW` | Salida | DE/RE transceiver RS485 |
| 17 | `RX_UART2` | UART RX | RS485 hacia rack |
| 18 | `TX_UART2` | UART TX | RS485 hacia rack |
| 21 | `PWM_pin21` | LEDC canal 1 | Disparo de tiristores |
| 35 | `SPICLOCK` | Salida | Reloj SPI bit-bang al MCP3208 |
| 36 | `DATAIN` | Entrada | MISO ADC |
| 37 | `DATAOUT` | Salida | MOSI ADC |
| 38 | `SELPIN` | Salida | CS del ADC |
| 39 | `ENABLE_1` | Salida | Habilitación física de salida |
| 40 | `LED_DEBUG2` | Salida / LEDC canal 0 | LED debug |
| 41 | `LED_DEBUG1` | Salida | LED debug 1 |
| 42 | `SOBRE_I` | Entrada pull-down | Sobrecorriente por hardware |
| 47 | `SW` | Entrada pull-up | Pulsador encoder |
| 48 | `BUZZER` | Salida | Buzzer |

### Canales del ADC externo (MCP3208)

| Canal | Variable | Uso |
|---:|---|---|
| 1 | `Vcc` | Tensión de salida |
| 2 | `Icc` | Corriente de salida |
| 3 | `Pot` | Potencial |
| 6 | `Temp1` | Temperatura |

VREF utilizada en hardware: **4.096V** (referencia de precisión externa).

---

## 3. Auditoría Estática — Hallazgos Principales

### Bugs de lógica

- En `src/main.cpp` `loop()`: `digitalRead(!ENABLE_1)` equivale a `digitalRead(0)` (lee GPIO0, no el pin de habilitación), porque `!39 == 0`. Bug pre-existente, no se corrigió en esta sesión.
- En `lib/ADC/adc.cpp` `midePotencial()`: la condición `if (millis() == millisAnt + 10)` exige igualdad exacta de tiempo, por lo que `Pot` casi nunca se actualizaba. **CORREGIDO** en esta sesión (ver §7).

### Concurrencia

- `referencia`, `modo_funcionamiento`, `regs_entrantes[]`, `atender`, `despolariza` se acceden desde `loop()` (core 1) y desde la tarea `func_com` (core 0) sin barreras ni `volatile`.
- La ISR `interruptor` y el `loop()` ambos escriben `ENABLE_1`.

### Bloqueos / latencias

- `sobre_I()` ante GPIO `SOBRE_I` activo entra en `do { delay(1000); ... } while (true);` — bucle infinito, requiere reset físico para salir.
- `envia_a_Maestro()` hace `delay(50)` fijo tras cada TX RS485.
- En error de `ledcSetup()`, `configura_PWM()` se bloquea en `while (digitalRead(SW))` sin timeout.

### Memoria / buffers

- `Str_a_char()` usa buffer global `charbuffer[50]` y `toCharArray(charbuffer, _str.length())`. El segundo argumento de `toCharArray` debería ser tamaño total (incluyendo NUL), no longitud. Riesgo de truncamiento.
- `recibeYanalizaValores()` acumula bytes en un `String` y luego copia a `char buffer[50]` local — se cae si la trama supera ese tamaño.

### Hardware

- `digitalWrite(RX_UART2, HIGH)` / `digitalWrite(TX_UART2, HIGH)` en `configuraPines()` sin haber declarado esos pines como salida — comportamiento indefinido al inicio.

### NVS

- `ER_Icc` se declara como `unsigned int` pero se guarda en NVS con `putFloat`/`getFloat`. Funciona pero pierde precisión teórica.
- Solo `guardaNVS_EstadoER` y `guardaNVS_HsFunc` usan mutex; las lecturas no.

### Símbolos huérfanos

- `SLAVE_ID` y `bandEnsayoSinGPS` están declarados como `extern` pero no tienen definición real (única definición está comentada).

---

## 4. Problema: La Referencia de un Módulo se Copia a Otro

### Síntoma reportado

Al modificar la referencia de un módulo, en ocasiones otro módulo termina tomando el mismo valor.

### Hipótesis principal

El problema está casi seguramente en el **rack**, no en el módulo. En `lib/rs485/rs485.cpp` del rack:

```
void recibeYanalizaValores(uint8_t _nroModulo)
{
    ...
    valores_modulos[_nroModulo - 1][c + 1] = values[c];
    ...
}
```

Esta función guarda la trama recibida en el slot del módulo que el rack *cree* estar consultando, **sin verificar** que `values[0] == _nroModulo`. Secuencia que reproduce el síntoma:

1. SCADA cambia referencia del módulo 1 a 500.
2. El módulo 1 responde con su nueva referencia 500.
3. El rack, por timing/retraso, está consultando al módulo 2 cuando llega esa respuesta.
4. La respuesta del módulo 1 se almacena en `valores_modulos[1]` (slot del módulo 2).
5. `variables_a_Modbus(2)` vuelca esa referencia 500 al holding register de referencia del módulo 2.
6. La tarea `func_modbus()` ve que el registro cambió y lo interpreta como **escritura nueva** del SCADA.
7. Envía `$2,1,-1,500,...*` al módulo 2, que ahora toma la misma referencia.

### Causas concurrentes a verificar

- **`num_modulo` duplicado** entre módulos en NVS: si dos módulos tienen el mismo número, ambos aceptan la misma trama.
- **Colisión RS485 interno** del rack: `parametrosModulos()` (loop) y `func_modbus()` (tarea) acceden al mismo bus sin mutex.
- **Sin distinción readback vs comando** en los holding registers del rack: la misma dirección sirve para leer estado del módulo y para escribir orden — el rack se confunde.
- `Str_a_char()` con buffer global puede arrastrar bytes residuales entre tramas consecutivas.

### Decisión

El módulo ya valida `regs_entrantes[1] == num_modulo` antes de aceptar la orden, por lo que la corrección de fondo debe hacerse en el rack. Esta sesión se concentró en el módulo; las correcciones del rack quedan pendientes.

---

## 5. Lógica de Sobrecorriente

### Estado original

- `pinMode(SOBRE_I, INPUT_PULLDOWN)` — pin en bajo por defecto.
- `if (digitalRead(SOBRE_I)) { ... }` — entraba en protección con **nivel alto (3.3V)**.

### Cambio solicitado y aplicado en esta sesión

Inversión de la lógica para que la protección se dispare con **nivel bajo (0V / masa)**, lo que es más seguro: si el cable de la señal de sobrecorriente se corta o se desconecta, el pull-up lo mantiene en alto (estado inactivo) y no genera disparos falsos.

### Modificaciones aplicadas

**`lib/EstablecePines/establecePines.cpp`:**

```
pinMode(SOBRE_I, INPUT_PULLUP);
```

**`lib/Funciones/funciones.cpp`:**

```
if (!digitalRead(SOBRE_I))
```

---

## 6. Ajuste de Escalas de Medición — Cambio Principal

### Contexto

El hardware se modificó con:

- **Tensión**: divisor `0.022` (180V → 4V en el ADC).
- **Corriente**: shunt `50A / 50mV` → IC amplificador con ganancia `×50` → divisor `÷2` → amplificador `×1.8` → ADC. Ganancia total del camino: `0.045 V/A`.
- **Referencia**: pasar de "0–100mA / 0–1A" a **0–20A con 1 decimal**.
- **VREF del MCP3208**: 4.096V (referencia de precisión).

### Cálculo de constantes

Conversión cuentas → V con MCP3208 12 bits y VREF=4.096V:
`V_adc = count × 4.096 / 4095 ≈ count × 0.001`

**Vcc**:
- `V_adc = V_in × 0.022`
- `count ≈ V_in × 22`
- `V_in = count / 22 ≈ count × 0.04546`

Verificación: 180V → 4V → ~3960 cuentas → 3960 × 0.04546 ≈ 180. OK.

**Icc**:
- `V_adc = I × 0.045`
- `count ≈ I × 45`
- `I (A) = count / 45 ≈ count × 0.02223`

Verificación: 20A → 0.9V → ~900 cuentas → 900 × 0.02223 ≈ 20.0. OK.

### Patrón de unidades adoptado

Se replicó el patrón ya usado por `MODO_V_CTE`:

- `Vcc`, `Icc`: en **A / V** como `float`.
- `referencia`: en **décimas** de la unidad (e.g., 200 = 20.0A, 1250 = 125.0V).
- Comparaciones internas: dividir referencia por 10 antes de comparar con `Vcc` o `Icc`.
- Comunicación al rack: `Vcc * 10`, `Icc * 10`, `referencia` (ya en décimas).

### Diagrama del flujo de unidades

```
ADC counts ─── ×0.04546 ──> Vcc (V, float)
ADC counts ─── ×0.02223 ──> Icc (A, float)
Encoder ──── décimas de A ──> referencia
referencia ── /10 ──> compara con Icc en A
Vcc, Icc, ref ── *10 (int) ──> RS485 al rack
```

### Cambios aplicados archivo por archivo

#### `lib/ADC/adc.cpp` — `mideTodo()`

```
Vcc = mide(1, 435) * 0.04546;
Icc = mide(2, 435) * 0.02223;  // ahora en A, sin *1000
```

#### `lib/Pantallas/pantallas.cpp` — `formateaReferencia()` caso `MODO_I_CTE`

```
case MODO_I_CTE:
    txt_referencia = "A ";
    digitos = 4;
    dec = 1;
    maximo = 10 * ER_Icc;
    minimo = 1;
    multiplicador = 10;
    sensibilidad = 80;
    break;
```

#### `lib/Pantallas/pantallas.cpp` — `muestraReferencia()`

Agregado `MODO_I_CTE` al condicional que divide por 10 al mostrar:

```
if (modo_funcionamiento == MODO_V_CTE || modo_funcionamiento == MODO_I_CTE)
{
    _ref /= 10;
}
```

#### `lib/Pantallas/pantallas.cpp` — `configuraInicio()` menú Fabricante

```
case 2: // Corriente de salida (nominal en A, hasta 20A)
    digitos = 2;
    ER_Icc = encoder("Icc: ", ER_Icc, 20, 1, 1, 180, 1, 4, 0);
    break;
```

#### `src/main.cpp` — `func_com()` caso 1 (`MODO_I_CTE`)

```
case 1:
    if (regs_entrantes[4] <= ER_Icc * 10)
    {
        referencia = regs_entrantes[4];
    }
    break;
```

#### `lib/PWM/PWMs.cpp` — `corrige_PWM()` caso `MODO_I_CTE`

Descomentada la línea `_ref = _ref / 10;` para llevar la referencia a A antes de compararla con `Icc`.

#### `lib/rs485/rs485.cpp` — `armaCadenaValores()`

Tanto la cadena como el checksum se actualizaron para enviar Icc en décimas:

```
chkSum = ... + (int)(Icc * 10) + ...
sprintf(..., (int)(Vcc * 10), (int)(Icc * 10), ..., chkSum);
```

### Impacto en SCADA / Modbus externo

Los siguientes holding registers del rack **cambian de unidad** (no de dirección):

| Registro Modbus | Función | Antes | Ahora |
|---:|---|---|---|
| 18, 29, 40, 51, 62 | Icc readback módulos 1–5 | mA | décimas de A |
| 21, 32, 43, 54, 65 | Referencia módulos 1–5 (en modo I) | mA | décimas de A |
| 17, 28, 39, 50, 61 | Vcc módulos 1–5 | décimas de V | décimas de V (sin cambio) |

El SCADA externo debe interpretar los valores de corriente como `valor / 10 = Amperes` (ej. 200 → 20.0A).

---

## 7. Fixes Post-Flasheo

Tras flashear y probar en hardware, se detectaron dos problemas que se corrigieron:

### 7.1. La corriente en el display no mostraba decimales

En `lib/Pantallas/pantallas.cpp` `muestraMedicion()` la llamada era:

```
muestraFloat("I: ", Icc, 4, 0, 8, 0);
```

El cuarto parámetro `0` significa "0 decimales". Cambiado a `1`:

```
muestraFloat("I: ", Icc, 4, 1, 8, 0);
```

Ahora muestra `I: 12.5` en lugar de `I:    12`.

### 7.2. El potencial quedaba estático

En `lib/ADC/adc.cpp` `midePotencial()` la asignación de `Pot` estaba dentro de un `if` con comparación de igualdad exacta de tiempo:

```
if (millis() == millisAnt + 10)
{
    Pot = (2048 - auxPot) * 2;
}
```

Como `millis()` rara vez cae **exactamente** en `millisAnt + 10`, `Pot` casi nunca se actualizaba. Bug pre-existente identificado en la auditoría inicial.

Corregido eliminando el condicional:

```
void midePotencial(void)
{
    float auxPot;

    digitalWrite(LED_DEBUG2, HIGH);
    auxPot = mide(3, 425);   // 425 muestras ~ 10ms para filtrar 50Hz
    digitalWrite(LED_DEBUG2, LOW);

    Pot = (2048 - auxPot) * 2;
}
```

El promedio sobre 425 muestras sigue dando el filtrado a 50Hz que era el objetivo original.

### 7.3. El PWM tardaba mucho en alcanzar la referencia

Tras los cambios de escala se notó en banco que la salida tardaba decenas de segundos en estabilizarse en la referencia configurada.

**Causa raíz**: el control en `corrige_PWM()` cambiaba `duty_cycle` con paso fijo `±1` por iteración del `loop()`. Con un `delay(50)` y `mideTodo()` consumiendo ~30 ms, cada iteración tomaba ~80–100 ms; barrer los 225 pasos efectivos del rango `[5, 230]` requería ~20 s. La sensación previa de "rápido" provenía de que la calibración incorrecta de `Icc` saturaba el controlador en una rampa (no controlaba: empujaba al máximo). Una vez corregida la escala, el control real con paso unitario quedó expuesto.

**Cambios aplicados** (terminan lo que el autor original ya había esbozado en líneas comentadas dentro del switch):

#### a) Control proporcional acotado en `lib/PWM/PWMs.cpp` — `corrige_PWM()`

Se reemplaza el paso fijo `±1` por un incremento proporcional al error, recortado a `±DELTA_MAX (= 10)` por iteración. Se mantienen las bandas muertas originales (`±1%` en I/V, `±0.5%` en P) como condición de disparo del incremento — así no hay hunting alrededor del setpoint.

| Modo | Error | Fórmula del incremento | Origen |
|---|---|---|---|
| `MODO_I_CTE` | `e = _ref/10 − Icc` (A) | `incremento = (int)(255 × e / ER_Icc)` | Descomentada de la línea ya prevista |
| `MODO_V_CTE` | `e = _ref/10 − Vcc` (V) | `incremento = (int)(255 × e / ER_Vcc)` | Descomentada de la línea ya prevista |
| `MODO_P_CTE` | `e = (−_ref) − Pot` (mV, magnitudes) | `incremento = ±(int)(255 × |e| / 6000)` | Línea original ajustada para forzar el signo en cada rama del `if/else` |

El recorte final es:

```
if (incremento >  DELTA_MAX) incremento =  DELTA_MAX;
if (incremento < -DELTA_MAX) incremento = -DELTA_MAX;
duty_cycle += incremento;
```

Se preservan los topes finales `duty_cycle ∈ [5, 230]`.

#### b) `delay(50)` → `delay(10)` en `src/main.cpp` `loop()`

Acelera el lazo de control sin tocar la arquitectura del `loop()`. Cada iteración pasa a ~20–30 ms.

#### c) Corrección del bug `digitalRead(!ENABLE_1)`

Antes:

```
if (digitalRead(!ENABLE_1))    // !ENABLE_1 == 0 → lee GPIO0 (boot), no ENABLE_1
{
    corrige_PWM(referencia);
}
```

Después:

```
// Sólo corrige el ángulo si la salida está habilitada (ENABLE_1 es activo en bajo).
if (digitalRead(ENABLE_1) == LOW)
{
    corrige_PWM(referencia);
}
```

Ahora la corrección de ángulo se inhibe cuando `sobre_I()` o el manejo de `DISP_INT` deshabilitan la salida, coherente con la lógica de protección.

#### d) Rampa de arranque suave (soft-start) en `corrige_PWM()`

Para evitar el "salto" del primer ciclo de control cuando la salida se energiza (firmware fresco o re-habilitación tras una protección por sobrecorriente), el tope efectivo del paso (`delta_max_efectivo`) **arranca en `DELTA_MAX_INICIAL = 1` y crece linealmente hasta `DELTA_MAX = 10` durante `RAMPA_INICIO_MS = 2000` ms**.

Esquema:

```
delta_max_efectivo
       |
  10  -|- - - - - - - - ─────────────────
       |                /
       |              /
       |            /
       |          /
   1  -|────────/
       |________________________________ t [ms]
       0      2000
```

Detección de "arranque" autocontenida — no requiere modificar `main.cpp`.
**Importante**: la rampa sólo se aplica en el primer arranque del firmware
(power-on / reset del MCU). Las pausas intermedias provocadas por
`DISP_INT = LOW` NO la reinician, para permitir ensayos ON-OFF sincronizados
sin perder el estado del lazo (ver §7.7).

```
static int32_t tiempo_inicio_rampa = -1;

if (tiempo_inicio_rampa < 0)
{
    tiempo_inicio_rampa = ahora_ms;   // sólo en el primer arranque
}
```

- **Firmware recién booteado**: `tiempo_inicio_rampa = -1` → la primera llamada inicia la rampa.
- **Re-habilitación intermedia por `DISP_INT`**: el lazo retoma el control con el
  mismo `duty_cycle` y acumulador, sin pasar por soft-start. Comportamiento clave
  para ensayos ON-OFF.

Constantes ajustables en `lib/PWM/PWMs.cpp`:

| Constante | Valor por defecto | Rol |
|---|---|---|
| `DELTA_MAX` | 5 | Paso máximo por iteración en régimen |
| `DELTA_MAX_INICIAL` | 1 | Paso máximo permitido al instante 0 de la rampa |
| `RAMPA_INICIO_MS` | 2000 | Duración total de la rampa de arranque suave |

#### Resultado esperado

| Fase | Comportamiento |
|---|---|
| Primeros ~200 ms tras encendido | Paso máximo ≈ 1 → variación lenta (similar al control original) |
| Hasta ~1 s | Paso máximo crece a ~5 → respuesta intermedia |
| Después de 2 s | Régimen normal con `DELTA_MAX = 10` |
| Escalón 50% en régimen | ~12 iteraciones × 25 ms ≈ **300 ms** |
| Escalón 10–15% en régimen | ~5–10 × 25 ms ≈ **150 ms** |
| Cerca del setpoint | Paso cae a 1 y luego banda muerta corta el bucle (sin hunting) |
| Re-habilitación intermedia por `DISP_INT` | Retoma con el mismo `duty_cycle`, sin rampa (sincronismo ON-OFF) |

Si en banco se observa overshoot, se puede bajar `DELTA_MAX` (p. ej. a `5`) o extender `RAMPA_INICIO_MS`. Si la respuesta sigue siendo lenta para correcciones medianas, conviene mover `corrige_PWM` a una tarea FreeRTOS con `vTaskDelay(20 ms)` en otro core (cambio mayor, no aplicado en esta iteración).

#### e) Eliminación del error de estado estacionario por truncamiento a `int`

Tras flashear el control proporcional + soft-start se detectó en hardware que la salida **no llegaba al valor exacto del setpoint**: con referencia de 10 V quedaba estabilizada en 9.7 V, y al bajar la referencia a 9 V quedaba en 9.3 V. Es decir, ~3% de error fuera de la banda muerta de 1%, pero sin corrección activa.

**Causa**: el cálculo del incremento usa `(int)(255 * error / ER_xx)`. Para errores chicos, el resultado en `float` es menor que 1 y al castear a `int` **se trunca a 0**. La condición del `if/else if` sí dispara el control (error > 1%) pero el `duty_cycle` queda inmóvil. Hay una zona donde el control "intenta" pero no aplica nada:

| Modo | Error mínimo para `int(incremento) ≥ 1` | Banda muerta | Tierra de nadie |
|---|---|---|---|
| `MODO_I_CTE` (ER=20A) | `20/255 ≈ 78 mA` | 1% del setpoint | A 5A: 50 mA a 78 mA |
| `MODO_V_CTE` (ER=180V) | `180/255 ≈ 0.71 V` | 1% del setpoint | A 10V: 0.1 V a 0.71 V |
| `MODO_P_CTE` (div=6000) | `6000/255 ≈ 23.5 mV` | 0.5% del setpoint | A 1000mV: 5 mV a 23.5 mV |

**Solución**: acumulador fraccionario (acción integral discreta) dentro de `corrige_PWM()`. El cálculo en `float` (`incremento_f`) se acumula en `incremento_acum` en cada iteración. Cuando la suma alcanza `1.0`, se aplica al `duty_cycle` y la fracción remanente se conserva.

```
incremento_f  = 255 * error / ER_xx      // float, p. ej. 0.425
incremento_acum += incremento_f          // se acumula
incremento = (int)incremento_acum        // parte entera (puede ser 0)
incremento_acum -= incremento            // guarda la fracción
duty_cycle += incremento
```

Comportamiento para un error que da `incremento_f = 0.425`:

| Iteración | acum antes | acum después | int aplicado | acum restante |
|---:|---:|---:|---:|---:|
| 1 | 0.000 | 0.425 | 0 | 0.425 |
| 2 | 0.425 | 0.850 | 0 | 0.850 |
| 3 | 0.850 | 1.275 | **+1** | 0.275 |
| 4 | 0.275 | 0.700 | 0 | 0.700 |
| 5 | 0.700 | 1.125 | **+1** | 0.125 |

Resultado: se aplica `+1` aproximadamente cada `1/0.425 ≈ 2.4` iteraciones. El control converge hasta el borde de la banda muerta (±1% del setpoint) en lugar de quedar atrapado afuera.

**Detalles del diseño**:

1. **Reset del acumulador al entrar en banda muerta**: evita arrastres residuales que tirarían el control al lado opuesto cuando el error cambia de signo.
2. **Reset también en (re)arranque y cambio de modo**: usa los mismos triggers que ya disparaban el reinicio de la rampa de soft-start.
3. **Anti-windup**: `|incremento_acum| ≤ DELTA_MAX` (10) — el acumulador no puede crecer indefinidamente si el sistema entra en saturación.
4. **Interacción correcta con la rampa**: si la rampa de soft-start recorta el paso aplicado, lo no aplicado se devuelve al acumulador (no se pierde corrección). Esto es importante en los primeros 2 s post-arranque.

**Error de estado estacionario esperado tras este cambio**:

| Setpoint | Banda muerta efectiva (±1%) | Salida esperada |
|---|---|---|
| 10 V | ±0.10 V | 9.9–10.1 V |
| 9 V | ±0.09 V | 8.91–9.09 V |
| 5 A | ±0.05 A | 4.95–5.05 A |
| 1000 mV | ±5 mV (banda 0.5%) | 995–1005 mV |

Si en uso real se requiere precisión por debajo del 1% (improbable para este tipo de equipo de protección catódica), se puede reducir la banda muerta cambiando `1.01` → `1.005` y `0.99` → `0.995` en los `case`. No conviene bajarla más sin filtrado/oversampling adicional de las mediciones, porque el ripple del rectificador puede ser ya del orden del 0.5%.

### 7.4. Protección por sobrecorriente HW se disparaba sin causa aparente

Durante pruebas en `MODO_I_CTE` a ~10 A, tras un tiempo el equipo entraba en el branch de hardware de `sobre_I()` (`!digitalRead(SOBRE_I)`) y quedaba bloqueado en su bucle infinito a la espera de reset físico. La inspección de hardware no mostró sobrecorriente real, por lo que se atribuyó a **glitches por EMI** (acoplamiento capacitivo del bus de los tiristores, conmutación de cargas inductivas) sobre el pin GPIO 42 con pull-up interno (~45 kΩ).

#### a) Debounce por software en `SOBRE_I` (`lib/Funciones/funciones.cpp`)

Se introdujo un contador estático que exige `SOBRE_I_DEBOUNCE_N = 3` lecturas consecutivas LOW para disparar el branch HW. Como el `loop()` corre cada ~20–30 ms, eso traduce a **~60–90 ms de señal sostenida** mínimos para que se considere falla real. Glitches puntuales se descartan.

```
#define SOBRE_I_DEBOUNCE_N 3

void sobre_I(void)
{
    static uint8_t consec_low = 0;

    if (!digitalRead(SOBRE_I)) {
        if (consec_low < 255) consec_low++;
    } else {
        consec_low = 0;
    }

    if (consec_low >= SOBRE_I_DEBOUNCE_N) {
        // ... branch HW (bloqueo + espera de reset físico) ...
    }
    else if (Icc > ER_Icc * 1.1) {
        // ... branch SW ...
    }
}
```

**Importante**: el bucle infinito del branch HW se conserva intencionalmente como medida de seguridad. La regla operativa sigue siendo: ante alarma HW, se requiere intervención humana + reset físico. El debounce sólo evita que un glitch espurio dispare esa secuencia.

Si en operación real se observa que con `N=3` aún aparecen disparos espurios, se puede subir a `5` o `7` (~100–200 ms de señal mínima). Si por el contrario interesa una respuesta más rápida, se baja a `2`.

#### b) Reducción de `DELTA_MAX` de 10 a 5 (`lib/PWM/PWMs.cpp`)

Con `DELTA_MAX = 10`, el acumulador integral podía aplicar saltos grandes de `duty_cycle` frente a disturbios sostenidos, generando overshoot transitorio de corriente que en algunos casos podía activar correctamente la protección por hardware. Bajar el tope a `5` reduce ese overshoot a la mitad.

| Parámetro | Antes | Ahora |
|---|---:|---:|
| `DELTA_MAX` | 10 | **5** |
| Máximo cambio de `duty_cycle` por iteración | ±10 | ±5 |
| Tiempo de barrido completo (escalón 100%) | ~25 × 25 ms ≈ 625 ms | ~45 × 25 ms ≈ 1.1 s |
| Riesgo de pico transitorio de corriente | Mayor | Menor |

El compromiso es una respuesta a escalones grandes algo más lenta (1.1 s vs 0.6 s para barrido completo), aceptable para esta clase de equipo. En régimen normal (errores pequeños cerca del setpoint) no hay diferencia.

#### Pendientes asociados (no aplicados en esta iteración)

- **Branch HW como bucle infinito (`do { } while (true)`)**: se conserva por decisión operativa — el equipo debe requerir reset físico ante sobrecorriente HW. Documentado.
- **`digitalWrite(DISP_INT, salida_OF)` en `sobre_I()`**: `DISP_INT` está configurado como `INPUT_PULLUP`, por lo que ese write no fuerza un nivel de salida. Bug histórico, se evaluará en una próxima iteración (probablemente reemplazar por `digitalWrite(ENABLE_1, HIGH)` para deshabilitar la salida efectivamente).

### 7.5. Corriente no llegaba a la consigna en valores altos (techo del `duty_cycle`)

Durante pruebas en `MODO_I_CTE` con `ER_Icc = 20 A`, al pedir referencia de **13 A** la salida se topaba en **~10.4 A** y no subía más. La protección por software (`Icc > ER_Icc * 1.1 = 22 A`) no se activaba — la corriente real estaba muy por debajo del umbral.

**Causa**: el `duty_cycle` se saturaba en su techo de software (`230`) y el hardware del rectificador necesitaba ángulos de disparo más cerca del máximo del rango PWM (0–255) para entregar 13 A con la carga / tensión de red de la prueba. El control empujaba al tope, ahí se quedaba.

El techo de `230` venía de versiones antiguas con otro hardware y un control P puro sin acumulador. Con el control proporcional + integral + debounce + soft-start actuales, ese margen de ~10% del rango es excesivo.

**Cambio aplicado** en `lib/PWM/PWMs.cpp`:

```
#define DUTY_MAX 250
#define DUTY_MIN 5
```

Subido de **230 → 250**. Sigue dejando un margen de 5 unidades (~2%) para evitar disparos demasiado cerca del cruce por cero. Adicionalmente se cambiaron los `if (duty_cycle > 230) duty_cycle = 230;` por el `#define` correspondiente, así el ajuste futuro es de una sola línea.

| Parámetro | Antes | Ahora | Margen |
|---|---:|---:|---:|
| `DUTY_MAX` | 230 | **250** | ~2% del rango PWM |
| `DUTY_MIN` | 5 | 5 | sin cambio |
| Rango efectivo del control | 225 unidades | **245 unidades** | +9% |

**Lo que esto NO arregla**:
- Si la calibración del ADC sobrestima (`Icc` medido > `Icc` real), el problema persiste — verificar con amperímetro patrón.
- Si el hardware (tiristores / transformador / carga) directamente no puede entregar más corriente, subir el techo no aporta.

Si tras este cambio la corriente sigue tope en ~10.4 A, el límite es físico y conviene revisar el hardware antes de tocar más software.

### 7.6. Protección térmica con LM35 (sensor en canal 5 del MCP3208)

Se agrega medición y protección por temperatura del módulo usando un sensor **LM35** conectado al canal 5 del MCP3208. La estrategia es **derateo lineal** de la referencia entre dos umbrales y **corte total con autorecuperación** cuando se supera un umbral crítico. La temperatura se exhibe permanentemente en el display y se transmite al rack (la trama RS485 ya incluye `Temp1`).

#### Hardware

- LM35 alimentado entre +5 V y GND, salida al **pin 6** del MCP3208 (que corresponde a **CH5** según el datasheet).
- LM35: **10 mV/°C**, salida lineal 0–1 V para 0–100 °C.
- Con VREF = 4.096 V y 12 bits, **1 cuenta = 1 mV** → 1 °C = 10 cuentas.
- Recomendación: capacitor 100 nF entre la salida del LM35 y GND, cerca del sensor.

> **Importante sobre la API local**: la función `mide(channel, ...)` en este proyecto recibe el **número de pin físico del integrado (1-base)**, NO el número de canal CHx del datasheet. Adentro hace `(channel - 1) << 3` para construir el byte de selección. Por eso para leer el pin 6 (CH5 del datasheet) hay que pasar `channel = 6`. Mantener la convención existente del proyecto evita romper el resto de las llamadas.

#### a) Medición (`lib/ADC/adc.cpp` `mideTodo()`)

Filtrado en **dos etapas** para una lectura estable y para evitar que el derateo
oscile en el borde de los umbrales (70 / 85 / 90 °C):

1. Promedio de **50 muestras** dentro de `mide()` (reduce ruido base).
2. **EMA** (exponential moving average) con `α = 0.10` → pondera 10 % la lectura
   nueva y 90 % el histórico. Constante de tiempo ≈ 10 lecturas; suficiente
   para una variable térmica.

```
static float temp_filtrada = 0.0f;
static bool  temp_init     = false;

float temp_raw = mide(6, 50) * 0.1f;

if (!temp_init) { temp_filtrada = temp_raw; temp_init = true; }
else            { temp_filtrada = 0.1f * temp_raw + 0.9f * temp_filtrada; }

Temp1 = temp_filtrada;
```

> Si en una versión futura se decide que la respuesta es demasiado lenta, basta
> con subir `α` (p. ej. 0.2 o 0.3) sin tocar el resto de la lógica.

Tabla rápida de correspondencias (para futura referencia):

| `mide(channel, …)` | Bits seleccionados | Canal CHx (datasheet) | Pin físico | Etiqueta del esquema |
|:---:|:---:|:---:|:---:|---|
| 1 | 000 | CH0 | 1 | MEDICION_V (Vcc) |
| 2 | 001 | CH1 | 2 | MEDICION_I (Icc) |
| 3 | 010 | CH2 | 3 | MEDICION_P (Pot) |
| 6 | 101 | CH5 | 6 | TEMP (LM35) |

La variable `Temp1` ya existía y se transmite al rack en la trama RS485 (`armaCadenaValores()`), por lo que **el rack ve la temperatura sin cambios adicionales** en su firmware (sólo hay que reinterpretar el campo correspondiente como °C).

#### b) Umbrales (`lib/Funciones/funciones.h`)

```
#define TEMP_INICIO_DERATEO 70.0f   // °C: inicia derateo lineal
#define TEMP_FIN_DERATEO    85.0f   // °C: derateo completo (factor = 0)
#define TEMP_CRITICA        90.0f   // °C: corta salida totalmente
#define TEMP_REENGANCHE     75.0f   // °C: histéresis para volver del corte
```

Curva del factor de derateo aplicado a la referencia:

```
factor
  1.0 |─────────.
      |          \
      |           \
      |            \
  0.0 |─────────────●─────────────●─────────
       70°C        85°C          90°C
       (inicio)   (fin lineal)   (corte total)
```

#### c) Lógica (`lib/Funciones/funciones.cpp` — `aplicaDerateoTemp()`)

Estado interno mínimo: una sola variable estática `corte_termico`. Histéresis para evitar parpadeo entre operación y corte.

```
float aplicaDerateoTemp(float ref)
{
    if (corte_termico) {
        if (Temp1 < TEMP_REENGANCHE) corte_termico = false;
        else return 0.0f;
    } else if (Temp1 > TEMP_CRITICA) {
        corte_termico = true;
        return 0.0f;
    }

    if (Temp1 <= TEMP_INICIO_DERATEO) return ref;

    float factor = (TEMP_FIN_DERATEO - Temp1) /
                   (TEMP_FIN_DERATEO - TEMP_INICIO_DERATEO);
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    return ref * factor;
}
```

La función `corteTermicoActivo()` expone el estado de corte para que el display lo visualice.

#### d) Aplicación al control (`src/main.cpp` `loop()`)

Un solo cambio quirúrgico — la referencia que el operador configura (`referencia`) no se toca, sólo se procesa antes de pasarla al PWM:

```
if (digitalRead(ENABLE_1) == LOW)
{
    corrige_PWM(aplicaDerateoTemp(referencia));
}
```

Cuando la temperatura baja, el derateo desaparece automáticamente y el control vuelve a operar contra la referencia completa, sin intervención del operador.

#### e) Display (`lib/Pantallas/pantallas.cpp` `muestraMedicion()`)

Se ocupó el espacio que antes se borraba con `lcd_print_Posicion(9, 2, "       ")` (columnas 9–15 de la fila 2). Layout resultante 16×4:

```
Pos:    0123456789012345
Fila 1: V:XX.X  I:XX.X
Fila 2: P:XXXXX T: XX !
Fila 3: Hs:XXXXX
Fila 4: R:XXX.X A
```

`muestraFloat("T:", Temp1, 3, 0, 9, 1)` → etiqueta (cols 9–10) + 3 chars
(`" 36"`, cols 11–13). Col 14 limpia. Col 15 indicador `!`/`X`/`' '`.

> **⚠ Convenciones del display (¡leer antes de tocar!)**: las funciones
> `muestraFloat()` y `lcd_print_Posicion()` **no usan el mismo origen**:
> - `muestraFloat(_c, _f)` es 0-indexed → llama `setCursor(_c, _f)` directo.
> - `lcd_print_Posicion(c, f)` es 1-indexed → llama `setCursor(c-1, f-1)`.
>
> Para alinear el indicador térmico a la última columna del LCD (col 15
> 0-indexed) hay que pasar `lcd_print_Posicion(16, 2, ...)`. En la primera
> versión usé `lcd_print_Posicion(15, 2, ...)`, lo que hacía que el
> indicador cayera en **col 14** pisando el último dígito de la
> temperatura: con `Temp1 = 36` se veía "T:  3" porque el "6" era
> sobrescrito por el indicador (un espacio mientras T < 70 °C). **Corregido**.

Indicador en col 15 fila 2:

| Símbolo | Significado |
|:---:|---|
| (espacio) | Operación normal, T < 70 °C |
| `!` | En zona de derateo (70 °C ≤ T ≤ 85 °C) — el operador ve que el sistema está bajando corriente |
| `X` | Corte térmico activo (T > 90 °C) — salida en cero hasta que enfríe a < 75 °C |

#### Lo que NO se modificó (decisión "quirúrgica")

- **Menú fabricante**: los umbrales quedan como `#define`. Si en el futuro se desea configurables vía encoder, se agrega una entrada al menú `Pant_Fabricante` y se guardan en NVS.
- **`sobre_I()`**: queda intacta. La protección térmica es ortogonal y vive en una función nueva.
- **Trama RS485**: `Temp1` ya se transmite, el rack la lee sin cambios.

#### Calibración fina

La fórmula `mide(6, 50) * 0.1f` es **teórica** (LM35 ideal + VREF exacta). Si en banco se observa un offset constante respecto a un termómetro patrón (típicamente < 2 °C), conviene ajustar la lectura cruda antes de pasarla al EMA:

```
float temp_raw = mide(6, 50) * 0.1f - OFFSET_CAL;   // si lee de más
float temp_raw = mide(6, 50) * 0.1f + OFFSET_CAL;   // si lee de menos
```

Documentar el offset aplicado en este archivo si se utiliza.

#### Verificaciones tras flashear

1. Confirmar que el display muestra `T:XX.X` con un valor coherente con la temperatura ambiente (típicamente 20–30 °C en banco).
2. Calentar el sensor controladamente y verificar:
   - Al pasar 70 °C aparece `!` y la corriente empieza a bajar proporcionalmente.
   - Al pasar 90 °C cambia a `X` y la salida se va a cero.
   - Al bajar por debajo de 75 °C, el `X` desaparece y la salida vuelve a operar.
3. Verificar que la `referencia` que muestra el menú no cambió — sólo se está modulando la corriente real.

---

### 7.7. `DISP_INT` como sincronismo para ensayos ON-OFF

#### Problema

Para ensayos ON-OFF se quiere usar el interruptor físico conectado a `DISP_INT`
(GPIO 8) como sincronismo: al cerrarlo el módulo apaga la salida y al abrirlo
debe **volver al mismo `duty_cycle`** sin pasar por la rampa de soft-start.

La lógica original de `corrige_PWM()` interpretaba cualquier pausa mayor a
`PAUSA_RESET_MS = 500 ms` como un re-arranque y reiniciaba la rampa
(2 segundos hasta paso completo) y el acumulador fraccionario. Eso era
deseable tras el power-on pero indeseable en cada toggle de DISP_INT.

#### Solución

En `lib/PWM/PWMs.cpp` se eliminó la condición de reinicio por pausa larga.
La rampa de soft-start ahora se aplica **sólo en el primer arranque del
firmware** (`tiempo_inicio_rampa < 0`):

```
static int32_t tiempo_inicio_rampa = -1;

bool reinicio = false;
if (tiempo_inicio_rampa < 0)            // sólo en el power-on / reset del MCU
{
    tiempo_inicio_rampa = (int32_t)ahora_ms;
    reinicio = true;
}
```

Se eliminaron `ultima_llamada_ms` y `PAUSA_RESET_MS`. El reinicio del
acumulador por cambio de modo se conserva (`reinicio || modo != ultimo_modo`).

#### Comportamiento resultante (gating por SW en GPIO 18)

Como `ENABLE_1` aún no está cableado en el hardware, el corte/encendido del
PWM se hace por software directamente sobre el LEDC (GPIO 18). El `loop()`
en `src/main.cpp` aplica:

```
static bool salida_estaba_off = false;
if (digitalRead(ENABLE_1) == LOW)
{
    if (salida_estaba_off) { fija_Angulo(duty_cycle); salida_estaba_off = false; }
    corrige_PWM(aplicaDerateoTemp(referencia));
}
else
{
    fija_Angulo(0);
    salida_estaba_off = true;
}
```

| Evento | Acción |
|---|---|
| `DISP_INT = LOW` (corte) | ISR pone `ENABLE_1 = HIGH`. La próxima iteración del `loop()` ejecuta `fija_Angulo(0)` → LEDC mantiene GPIO 18 en LOW, el SCR no se dispara. `duty_cycle` queda **intacto** en RAM. |
| `DISP_INT = HIGH` (vuelve) | ISR pone `ENABLE_1 = LOW`. La próxima iteración detecta el flanco (`salida_estaba_off == true`) y ejecuta `fija_Angulo(duty_cycle)` **antes** del `corrige_PWM()` → la salida vuelve al ángulo previo de inmediato, sin rampa. |
| Latencia de re-conexión | ≈ 10–30 ms (período del `loop()`). Para sincronismo a 50 Hz convendría llamar `ledcWrite` desde la ISR (no implementado todavía). |

Si en el futuro se cablea `ENABLE_1` por hardware, ambos mecanismos conviven
sin problema: el HW actúa como capa de seguridad redundante.

#### Consideraciones

- Si entre dos ciclos ON-OFF se modifica la `referencia`, el lazo arranca con
  el `duty_cycle` viejo apuntando al setpoint nuevo. La corrección ocurre
  paso a paso acotada por `DELTA_MAX = 5`, no instantánea.
- En cargas inductivas (transformador, motor), el escalón al volver al
  `duty_cycle` previo puede generar di/dt importantes. Verificar en banco
  que el hardware tolera el escalón antes de usar este modo con cargas
  reactivas.

---

## 8. Calibración Fina

Las constantes `0.04546` (Vcc) y `0.02223` (Icc) son **teóricas**. Para compensar tolerancias reales (resistencias del divisor, ganancia del IC, exactitud del VREF) conviene ajustar empíricamente:

```
Constante_nueva = Constante_actual × (valor_real / valor_medido)
```

Aplicar con una fuente patrón / amperímetro patrón y reemplazar los multiplicadores en `lib/ADC/adc.cpp` `mideTodo()`.

---

## 9. Configuración del Entorno de Compilación / Flasheo

### Herramientas requeridas

- VS Code o Cursor.
- Extensión **PlatformIO IDE**.
- Driver USB según placa:
  - **USB Serial/JTAG nativo** del ESP32-S3 (sin driver adicional en Windows 10/11 recientes).
  - **CP210x** si la devkit es Silicon Labs.
  - **CH340** si la devkit es WCH.

### Pasos

1. Abrir la carpeta `-20682-Daber-Modulo`.
2. Esperar a que PlatformIO instale la plataforma `espressif32` y descargue toolchains.
3. Seleccionar el entorno `esp32dev`.
4. Build (`platformio run`).
5. Conectar el ESP32-S3 por USB.
6. Upload (`platformio run --target upload`).
7. Monitor serie a **115200** baud (configurado en `platformio.ini`).

Si el upload no entra en bootloader automáticamente: mantener `BOOT`, pulsar `RESET`, soltar `RESET`, soltar `BOOT` cuando comience la carga.

### Configuración detectada

```
[env:esp32dev]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
```

---

## 10. Recomendaciones Pendientes (Sin Aplicar)

Estas son observaciones de la auditoría que **no se implementaron** en esta sesión. Quedan para un futuro pedido explícito:

### En el rack (`-20682-Daber-Rack-main`)

1. **Validar `values[0] == _nroModulo`** en `recibeYanalizaValores()` antes de guardar en `valores_modulos[]`. Es el fix principal para evitar que la referencia de un módulo se copie a otro.
2. **Mutex sobre `UART0_Modulos` y `RS485_RW_MODULOS`** para evitar colisión entre el polling periódico y las escrituras Modbus.
3. **Ventana anti-rebote** de los holding registers: distinguir "el rack acaba de escribir este valor por refresh" de "el SCADA escribió un comando nuevo". Hoy se mezclan.
4. **Validación robusta de tramas RMC del GPS** antes de usar la hora (`validarRMC()` existe pero no se invoca en el flujo principal).
5. **Aceptar `$GPRMC` y `$GNRMC`** en `lee_GPS()`. Hoy solo acepta `$GNRMC`.
6. **Revisar `sumarSegundos()` cerca de medianoche** — usa `t.hora >= 24` en vez de la variable auxiliar.

### En el módulo

1. ~~**Bug `digitalRead(!ENABLE_1)`** en `src/main.cpp` `loop()` — lee GPIO0 en lugar de `ENABLE_1`.~~ **Corregido en §7.3.c.**
2. **Bucle infinito en `sobre_I()`** — agregar timeout o mecanismo de reseteo controlado.
3. **`Str_a_char()`** — corregir tamaño del buffer y manejar NUL.
4. **Declaración de `volatile`** en variables compartidas entre `loop()` y la tarea `func_com`.
5. **Símbolos huérfanos**: `SLAVE_ID` (declarado, no definido) y `bandEnsayoSinGPS` (definición comentada).
6. **`pinMode()` de `RX_UART2` y `TX_UART2`** antes de hacerles `digitalWrite()` en `configuraPines()`.
7. **Mover `corrige_PWM` a tarea FreeRTOS** con periodicidad fija (p. ej. 20 ms), liberando al `loop()` de display/encoder de marcar el ritmo del control.

---

## 11. Versionado y Trazabilidad

- Versión del firmware del módulo (`varFuncionamiento.cpp`): `V: 1.3.0`.
- Cambios de esta sesión están todos en el módulo. El rack **no fue modificado**.
- Sugerencia: bumpear la versión a `V: 1.4.0` cuando se valide en hardware el rango 0–20A con 1 decimal, y dejar registrado en este documento el cambio de versión.

---

## Anexo: Constantes de Calibración Resumidas

| Magnitud | Constante actual | Fórmula derivada |
|---|---|---|
| Vcc | `0.04546` | `1 / (0.022 × 4095 / 4.096)` |
| Icc | `0.02223` | `1 / (0.045 × 4095 / 4.096)` |
| Pot | `(2048 - count) × 2` | offset 2048 (mitad de 4096), escala ×2 (preexistente) |
| Temp1 | `(count - 500) / 10` | preexistente |
