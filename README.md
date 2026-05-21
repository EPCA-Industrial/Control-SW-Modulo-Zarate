# Control SW Módulo Zarate

Firmware del **módulo rectificador** del equipo Zarate (proyecto 20682 Daber).
Plataforma: **ESP32-S3** con framework Arduino sobre PlatformIO.

Cada módulo opera como un rectificador controlado por tiristores y reporta al rack
maestro vía RS485 con un protocolo ASCII propio. El equipo completo se compone de
**5 módulos** coordinados por un rack general (repositorio hermano, no publicado aquí).

---

## Índice

- [Modos de funcionamiento](#modos-de-funcionamiento)
- [Arquitectura](#arquitectura)
- [Hardware](#hardware)
- [Mediciones (MCP3208)](#mediciones-mcp3208)
- [Lazo de control PWM](#lazo-de-control-pwm)
- [Protecciones](#protecciones)
- [Sincronismo ON-OFF por `DISP_INT`](#sincronismo-on-off-por-disp_int)
- [Display y encoder](#display-y-encoder)
- [Comunicación RS485 con el rack](#comunicación-rs485-con-el-rack)
- [Compilación y flasheo](#compilación-y-flasheo)
- [Documentación adicional](#documentación-adicional)

---

## Modos de funcionamiento

| Modo | Descripción | Rango |
|---|---|---|
| `MODO_I_CTE` | Corriente constante | 0.1 – 20.0 A (1 decimal) |
| `MODO_V_CTE` | Tensión constante | 0.1 – 180.0 V (1 decimal) |
| `MODO_P_CTE` | Potencial constante (protección catódica) | 100 – 3000 mV |
| `MODO_DESPO` | Despolarización por tiempo o por potencial | — |

---

## Arquitectura

```
SCADA externo  <--Modbus RTU-->  Rack general  <--RS485 ASCII-->  Módulo (este firmware)
                                      |
                                      | (coordinación, GPS, ensayos sincronizados)
                                      v
                                  5 módulos
```

- **Medición**: MCP3208 (ADC 12 bits externo, VREF 4.096 V) por bit-bang SPI.
- **Control**: PWM 8 bits @ 100 kHz como referencia de disparo de tiristores.
  Lazo proporcional + acumulador integral fraccionario con anti-windup, soft-start
  de 2 s al primer arranque y banda muerta del 1 % (0.5 % para potencial).
- **Protección térmica**: LM35 al MCP3208 con derateo lineal y corte por histéresis.
- **Display**: LCD 16×4 I²C, navegación por encoder rotativo.
- **Persistencia**: NVS (Preferences) con mutex para configuración y horas de funcionamiento.
- **Sincronismo**: `DISP_INT` (GPIO 8) corta y restaura el PWM al instante sin pasar
  por el soft-start, pensado para ensayos ON-OFF.

---

## Hardware

| Periférico | GPIO |
|---|---:|
| Encoder (SW_A / SW_B / SW) | 7 / 6 / 47 |
| LCD I²C (SDA / SCL) | 2 / 1 |
| RS485 (TX / RX / RW) | 13 / 12 / 11 |
| SPI MCP3208 (CLK / MISO / MOSI / CS) | 35 / 36 / 37 / 38 |
| PWM tiristores (`PWM_pin21`) | 18 |
| Habilita salida (`ENABLE_1`, activo LOW) | 39 |
| Interruptor salida (`DISP_INT`, INPUT_PULLUP) | 8 |
| Sobrecorriente HW (`SOBRE_I`, INPUT_PULLUP, activo LOW) | 42 |
| Buzzer | 48 |
| LEDs debug | 40, 41 |

Detalle completo en [lib/EstablecePines/establecePines.h](lib/EstablecePines/establecePines.h).

---

## Mediciones (MCP3208)

El driver `mide(channel, n_muestras)` usa la convención del **número de pin físico
del integrado (1-base)**, no el número de canal CHx del datasheet. Internamente
hace `(channel - 1) << 3` para armar los bits de selección. Tabla de equivalencias:

| `mide(channel, …)` | CHx (datasheet) | Pin físico del IC | Etiqueta del esquema | Variable | Conversión |
|:---:|:---:|:---:|---|---|---|
| 1 | CH0 | 1 | MEDICION_V | `Vcc` | `mide(1,435) * 0.04546` → V |
| 2 | CH1 | 2 | MEDICION_I | `Icc` | `mide(2,435) * 0.02223` → A |
| 3 | CH2 | 3 | MEDICION_P | `Pot`  | `(2048 - mide(3,425)) * 2` → mV |
| 6 | CH5 | 6 | TEMP (LM35) | `Temp1` | EMA(α=0.1) sobre `mide(6,50) * 0.1` → °C |

### Temperatura — LM35 filtrado

Hay dos etapas de filtrado para una lectura estable y para evitar que el derateo
térmico oscile en el borde de los umbrales:

1. Promedio de 50 muestras dentro de `mide()` (reduce ruido base).
2. EMA (exponential moving average) con `α = 0.10`. Constante de tiempo ≈ 10 lecturas.

Si se observa offset constante respecto a un patrón, ajustar:
```
float temp_raw = mide(6, 50) * 0.1f - OFFSET_CAL;
```

---

## Lazo de control PWM

Implementación en [lib/PWM/PWMs.cpp](lib/PWM/PWMs.cpp) `corrige_PWM()`:

- **Proporcional acotado**: paso por iteración limitado a ±`DELTA_MAX = 5`.
- **Acumulador fraccionario**: integra aportes < 1 paso para eliminar error
  de estado estacionario (truncación). Con anti-windup limitado a ±`DELTA_MAX`.
- **Banda muerta**: ±1 % en I/V, ±0.5 % en P, para evitar hunting.
- **Soft-start**: rampa de 2 s desde `DELTA_MAX_INICIAL = 1` hasta `DELTA_MAX`,
  **sólo al primer arranque del firmware** (power-on / reset MCU). Las pausas
  intermedias por `DISP_INT` NO la reinician (ver sincronismo ON-OFF).
- **Cambio de modo**: descarta el acumulador (cambia la magnitud controlada).
- **Rango del duty**: `DUTY_MIN = 5`, `DUTY_MAX = 250` (sobre 0–255).

Constantes ajustables en [lib/PWM/PWMs.cpp](lib/PWM/PWMs.cpp):

| Constante | Valor | Rol |
|---|---:|---|
| `DELTA_MAX` | 5 | Paso máximo por iteración en régimen |
| `DELTA_MAX_INICIAL` | 1 | Paso al instante 0 del soft-start |
| `RAMPA_INICIO_MS` | 2000 | Duración del soft-start |
| `DUTY_MAX` | 250 | Tope del duty (margen anti-saturación) |
| `DUTY_MIN` | 5 | Piso del duty (asegura disparo mínimo) |
| `LEDC_BASE_FREQ` | 100000 | Frecuencia del PWM en Hz |

---

## Protecciones

| Capa | Mecanismo | Umbral / Acción |
|---|---|---|
| HW | Pin `SOBRE_I` (GPIO 42, activo LOW) | Debounce SW de 3 lecturas (~60–90 ms). Dispara apagado y entra a loop infinito esperando reset físico. |
| SW | Sobrecorriente blanda | `Icc > 1.10 × ER_Icc` → referencia a 0, aviso por display 10 s |
| SW | Derateo térmico LM35 | Reduce linealmente la referencia entre 70 °C y 85 °C |
| SW | Corte térmico | `Temp1 ≥ 90 °C` → referencia = 0; reengancha al bajar de 75 °C (histéresis) |
| SW | Validación de referencia RS485 | `regs_entrantes[4] <= ER_Icc·10` (o `ER_Vcc·10`) en escritura |
| SW | Checksum RS485 + filtro `num_modulo` | Sólo se atiende si el destinatario coincide |
| SYS | Watchdog Timer | 300 s ambos núcleos (ver propuesta de mejora en `NOTAS_SESION.md`) |
| SYS | Boot seguro | `ENABLE_1 = HIGH` y `duty = 0` hasta terminar el setup |

El derateo se aplica a la referencia **antes** de entrar al lazo, por lo que actúa
en todos los modos (I_CTE, V_CTE, P_CTE):

```
corrige_PWM(aplicaDerateoTemp(referencia));
```

Indicador en la columna 15 fila 2 del display:
`' '` normal — `'!'` en derateo — `'X'` corte térmico activo.

---

## Sincronismo ON-OFF por `DISP_INT`

El pin `DISP_INT` (GPIO 8, INPUT_PULLUP) se usa como entrada de sincronismo para
ensayos ON-OFF. La ISR `interruptor()` reacciona en cualquier cambio y refleja el
estado en `ENABLE_1`; el `loop()` complementa con polling cada vuelta.

**Gating por software del PWM** (porque `ENABLE_1` aún no está cableado al driver
en hardware):

```cpp
static bool salida_estaba_off = false;
if (digitalRead(ENABLE_1) == LOW) {
    if (salida_estaba_off) { fija_Angulo(duty_cycle); salida_estaba_off = false; }
    corrige_PWM(aplicaDerateoTemp(referencia));
} else {
    fija_Angulo(0);
    salida_estaba_off = true;
}
```

| Evento | Acción | Latencia |
|---|---|---:|
| `DISP_INT → LOW` | `fija_Angulo(0)` → GPIO 18 en LOW, SCR sin disparo. `duty_cycle` queda intacto. | ≤ 30 ms |
| `DISP_INT → HIGH` | `fija_Angulo(duty_cycle)` antes de `corrige_PWM` → salida al ángulo previo sin rampa. | ≤ 30 ms |

Si en el futuro se cablea `ENABLE_1` por hardware, ambos mecanismos conviven sin
problema; el HW actúa como capa de seguridad redundante.

---

## Display y encoder

LCD 16×4 I²C controlado vía librería `LiquidCrystal_I2C`.

**Layout en operación**:

```
Pos:    0123456789012345
Fila 1: V:XX.X  I:XX.X
Fila 2: P:XXXXX T: XX !
Fila 3: Hs:XXXXX
Fila 4: R:XXX.X A
```

- **Indicador térmico** en col 15 fila 2: `' '` normal · `'!'` en derateo · `'X'` corte.

> Convenciones del módulo `Display16x2` — leer antes de tocar el layout:
> - `muestraFloat(_c, _f)` usa coordenadas **0-indexed** (`setCursor(_c, _f)` directo).
> - `lcd_print_Posicion(c, f)` usa **1-indexed** (`setCursor(c-1, f-1)` internamente).
>
> Mezclarlas hace que dos llamadas con el mismo número apunten a columnas distintas.

---

## Comunicación RS485 con el rack

Protocolo ASCII propio sobre RS485, half-duplex, controlado por `RS485_RW`.

Trama enviada por el módulo al rack (respuesta a `LECTURA`):

```
$num_modulo,modo,Vcc*10,Icc*10,Pot,Temp1,referencia,HsH,HsL,despol_T,despol_P,checksum*
```

Trama recibida del rack:

```
$num_modulo,operacion,modo,referencia,checksum*
```
donde `operacion` ∈ {`0=LECTURA`, `1=ESCRITURA`, `2=DESPOLAR`}.

- El **checksum** es la suma aritmética de todos los campos (no XOR).
- El módulo sólo atiende mensajes cuyo `num_modulo` coincide con el suyo
  (configurable en menú fabricante, persistido en NVS).

---

## Compilación y flasheo

Requisitos:

- **VS Code** o **Cursor** con la extensión **PlatformIO IDE**.
- Driver USB del ESP32-S3 (USB Serial/JTAG nativo en Windows 10/11 recientes;
  CP210x o CH340 según placa).

Comandos típicos:

```
pio run                       # compila
pio run -t upload             # compila y flashea
pio device monitor -b 115200  # monitor serie
```

Configuración en [platformio.ini](platformio.ini):

```
[env:esp32dev]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
```

---

## Documentación adicional

- [NOTAS_SESION.md](NOTAS_SESION.md): historial detallado de campañas de mejoras,
  diagnóstico de bugs, justificación de constantes, auditoría de seguridad y
  propuestas de mejora pendientes (priorizadas P1 / P2 / P3).
- Convenciones de versión: `Vx.y.z` definido en `lib/varGlobales/varFuncionamiento.cpp`.
  - `x` → compatibilidad con anteriores
  - `y` → cambios funcionales
  - `z` → correcciones de errores
