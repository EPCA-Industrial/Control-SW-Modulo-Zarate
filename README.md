# Control SW Modulo Zarate

Firmware del **módulo rectificador** del equipo Zarate (proyecto 20682 Daber). Plataforma: **ESP32-S3** con framework Arduino sobre PlatformIO.

Cada módulo opera como un rectificador controlado por tiristores y reporta al rack maestro vía RS485 con un protocolo ASCII propio. El equipo completo se compone de **5 módulos** coordinados por un rack general (repositorio hermano, no publicado aquí).

## Modos de funcionamiento

| Modo | Descripción | Rango |
|---|---|---|
| `MODO_I_CTE` | Corriente constante | 0.1 – 20.0 A (1 decimal) |
| `MODO_V_CTE` | Tensión constante | 0.1 – 180.0 V (1 decimal) |
| `MODO_P_CTE` | Potencial constante (protección catódica) | 100 – 3000 mV |
| `MODO_DESPO` | Despolarización por tiempo o por potencial | — |

## Arquitectura

```
SCADA externo  <--Modbus RTU--> Rack general  <--RS485 ASCII--> Módulo (este firmware)
                                    |
                                    | (coordinación, GPS)
                                    v
                                 5 módulos
```

- **Medición**: MCP3208 (ADC 12 bits externo, VREF 4.096 V) por bit-bang SPI.
- **Control**: PWM 8 bits @ 100 kHz como referencia de disparo de tiristores. Algoritmo P + acumulador integral con anti-windup, soft-start de 2 s y banda muerta del 1% (0.5% para potencial).
- **Display**: LCD 16×4 I²C, navegación por encoder rotativo.
- **Persistencia**: NVS (Preferences) con mutex para configuración y horas de funcionamiento.
- **Protecciones**: sobrecorriente por hardware con debounce de software (anti-glitch EMI) + sobrecorriente por software (`Icc > ER_Icc × 1.1`) + WDT.

## Hardware

| Periférico | GPIO |
|---|---:|
| Encoder (SW_A / SW_B / SW) | 7 / 6 / 47 |
| LCD I²C (SDA / SCL) | 2 / 1 |
| RS485 (TX / RX / RW) | 13 / 12 / 11 |
| SPI MCP3208 (CLK / MISO / MOSI / CS) | 35 / 36 / 37 / 38 |
| PWM tiristores | 18 |
| Habilita salida (`ENABLE_1`, activo LOW) | 39 |
| Interruptor salida (`DISP_INT`) | 8 |
| Sobrecorriente HW (`SOBRE_I`, activo LOW) | 42 |
| Buzzer | 48 |
| LEDs debug | 40, 41 |

Detalle completo de mapeo y conexiones en [lib/EstablecePines/establecePines.h](lib/EstablecePines/establecePines.h).

## Compilación y flasheo

Requisitos:

- **VS Code** o **Cursor** con la extensión **PlatformIO IDE**.
- Driver USB del ESP32-S3 (USB Serial/JTAG nativo en Windows 10/11 recientes; CP210x o CH340 según placa).

Comandos típicos:

```
pio run                       # compila
pio run -t upload             # compila y flashea
pio device monitor -b 115200  # monitor serie
```

Configuración detectada en [platformio.ini](platformio.ini):

```
[env:esp32dev]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
```

## Documentación de cambios

Ver [NOTAS_SESION.md](NOTAS_SESION.md) para el historial detallado de la última campaña de mejoras (auditoría, recalibración, control PI con soft-start, eliminación del error de estado estacionario, debounce de sobrecorriente y ajuste del techo del PWM).

## Convenciones

- Cada módulo tiene un `num_modulo` único (1–9) configurado en el menú fabricante y persistido en NVS. El rack lo usa para direccionar comandos.
- La trama RS485 ASCII al rack es:
  ```
  $num_modulo,modo,Vcc*10,Icc*10,Pot,Temp1,referencia,HsH,HsL,despol_T,despol_P,checksum*
  ```
- El checksum es la suma aritmética de los campos (no XOR).
