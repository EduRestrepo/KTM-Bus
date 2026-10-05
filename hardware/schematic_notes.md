# Guía de Hardware y Esquema Eléctrico: Controlador CAN-Bus para KTM 1290 (2024)

Este documento detalla el diseño electrónico y de hardware para construir un clon DIY de alto rendimiento del **HEX ezCAN / DENALI CANsmart**, diseñado específicamente para la **KTM 1290 Super Adventure S (2024)** con conector de diagnóstico **Euro 5**.

---

## 1. Conector KTM Euro 5 (ISO 19689 - 6 Pines Rojo)

En los modelos 2021-2024, KTM sustituyó los antiguos conectores propietarios y DWA por el conector estándar de diagnóstico Euro 5 (color rojo, ubicado bajo el asiento del pasajero junto a la batería y fusibles).

### Pinout del Conector Euro 5 en KTM 1290:
```
       ┌───────────────┐
       │   [ 1 ] [ 2 ] │     Pin 1: CAN High (CAN-H)  -> Cable Naranja/Negro
   ┌───┤   [ 3 ] [ 4 ] ├───┐ Pin 2: CAN Low (CAN-L)   -> Cable Naranja/Marrón
   │   │   [ 5 ] [ 6 ] │   │ Pin 3: +12V Positivo Contacto (KL15)
   └───┴───────────────┴───┘ Pin 4: Tierra / Masa (GND)
                             Pin 5 / 6: K-Line / Diagnóstico Auxiliar
```

> **IMPORTANTE DE SEGURIDAD**: Para máxima fiabilidad y evitar caídas de tensión en accesorios de alta potencia (luces de 60W-120W), la alimentación de potencia **NO** debe tomarse del pin 3 del conector Euro 5. La potencia se toma **directamente de los bornes de la batería (+12V permanente KL30) con un fusible aéreo de 30A**. El conector Euro 5 solo se utiliza para leer las líneas **CAN-H**, **CAN-L** y masa **GND**.

---

## 2. Diagrama de Bloques del Sistema DIY

```
                              ┌───────────────────────────────────────────────┐
                              │                 KTM-BUS V1.0                  │
                              │                                               │
   Batería Moto (+12V) ──────►│ Fusible 30A ──┬──► Regulador DC-DC (Automotriz)
   (Borne Directo)            │               │    (12V a 5V / 3.3V con TVS)  │
                              │               │         │                     │
                              │               │         ▼                     │
   KTM Euro 5 (CAN-H/L) ─────►│ Transceptor   │    ESP32 MCU                  │
                              │ SN65HVD230 ───┼──► (TWAI CAN @ 500kbps)       │
                              │ (Listen-Only) │    (WiFi AP + Web Server)     │
                              │               │    (4x Salidas PWM LEDC)      │
                              │               │         │                     │
                              │               ▼         ▼                     │
                              │     Etapa de Conmutación High-Side            │
                              │     (4x PROFETs / MOSFETs Inteligentes)       │
                              │        │         │         │         │        │
                              └────────┼─────────┼─────────┼─────────┼────────┘
                                       │         │         │         │
                                       ▼         ▼         ▼         ▼
                                    Canal 1   Canal 2   Canal 3   Canal 4
                                    (Blanco) (Amarillo)  (Azul)    (Rojo)
                                     Max 10A   Max 10A   Max 10A   Max 10A
                                    (Set 1 L) (Set 1 R) (Niebla)  (Accesorio)
```

---

## 3. Lista de Componentes Recomendados (BOM)

### A. Unidad de Control (Cerebro)
* **Placa Principal Recomendada**: **ESP32 DevKit V1 (ESP-WROOM-32 / 30 o 38 pines)** o **ESP32-WROOM-32U**:
  * **Arquitectura Dual-Core (240 MHz)**: Imprescindible en automoción. El **Core 1** atiende las interrupciones del periférico TWAI en tiempo real para procesar el bus CAN y modular el PWM de las luces sin retrasos. El **Core 0** se encarga de la pila de red (SoftAP WiFi y servidor web de configuración).
  * **Periférico TWAI nativo**: 100% compatible con CAN 2.0B a 500 kbps.
  * **Lógica nativa de 3.3V**: Conexión directa a los pines del transceptor SN65HVD230 sin conversores de nivel.
  * **Opción con Antena Externa (ESP32-WROOM-32U)**: Si se aloja el módulo en una caja metálica o muy envuelta en el subchasis de la KTM 1290, se recomienda el modelo con conector IPEX para colocar una antena adhesiva externa bajo los plásticos del colín.
* **Alternativas Todo-en-Uno (All-in-One)**:
  * **LILYGO T-CAN485** o **Waveshare ESP32-S3-CAN**: Ya integran el transceptor CAN y el regulador en la misma placa, simplificando el cableado interno.
* **Alternativa Miniatura**:
  * **Seeed Studio XIAO ESP32-S3** (21 x 17.5 mm): Para instalaciones con espacio extremadamente crítico bajo el asiento.
* **Rango Térmico de Operación**: Grado industrial (-40°C a +85°C) para soportar el calor radiado por el cilindro trasero del motor LC8 en verano.

### B. Transceptor CAN Bus
* **Módulo SN65HVD230 (3.3V)**:
  * Compatible nativo con la lógica de 3.3V del ESP32 (no requiere level shifter como el viejo TJA1050 de 5V).
  * Conexión a pines ESP32:
    * `CTX` (CAN TX) -> `GPIO 5`
    * `CRX` (CAN RX) -> `GPIO 4`
    * `VCC` -> `3.3V`
    * `GND` -> `GND`
    * `CAN-H` -> Pin 1 del conector Euro 5
    * `CAN-L` -> Pin 2 del conector Euro 5
  * *Nota de terminación*: Si el módulo trae una resistencia de 120Ω entre CAN-H y CAN-L (R2), se recomienda desoldarla o dejar el jumper abierto si te estás intercalando en un bus que ya tiene terminación activa en la moto.

### C. Etapa de Potencia (Salidas de 12V conmutable)
Para automoción en moto se debe usar conmutación **High-Side** (cortar el positivo de 12V y dejar la masa común al chasis):
* **Opción Profesional (Recomendada)**: **Infineon BTS7008-1EPP** o **BTS724G / PROFET**:
  * Integra MOSFET High-Side, driver de puerta, sensor de corriente y protección térmica / cortocircuito automática.
  * Soporta control directo con señales PWM de 3.3V desde el ESP32.
* **Opción DIY Accesible**: **MOSFETs de Canal P (ej. IRF4905)** o módulo comercial de 4 MOSFETs optoacoplados:
  * Transistor NPN (ej. 2N2222 o BC547) como excitador de puerta desde los 3.3V del ESP32 hacia los 12V.
  * Resistencia pull-up de 10kΩ a +12V en la puerta (Gate).

### D. Fuente de Alimentación de Automoción Robusta
La red eléctrica de una moto tiene picos inductivos (*load dump*) del alternador que pueden alcanzar los 35V-40V.
* Regulador Step-Down: **LM2596HV** (High Voltage, soporta hasta 60V de entrada) o **MP1584** con filtro previo.
* Diodo de protección contra inversión de polaridad: **Schottky SS34** (3A / 40V) o **1N5822**.
* Diodo TVS supresor de picos transitorios: **SMAJ24A** o **SMBJ28A** en paralelo con la entrada de 12V.
* Condensador electrolítico de filtro: **470µF 35V** de bajo ESR.

---

## 4. Asignación de Pines y Conexión para LILYGO T-CAN485

La placa **LILYGO T-CAN485** ya integra el microcontrolador ESP32, el transceptor CAN bus y un regulador de tensión en la misma PCB con bornes de conexión.

### Conexión del Bus CAN (Bornera Integrada en la Placa):
* **Borne CAN_H** -> Conector Euro 5 Rojo Pin 1 (Cable Naranja/Negro de la KTM)
* **Borne CAN_L** -> Conector Euro 5 Rojo Pin 2 (Cable Naranja/Marrón de la KTM)
* **Borne GND**   -> Conector Euro 5 Rojo Pin 4 (Tierra / Masa)
* *(Nota: El transceptor interno está cableado a `GPIO 26` (RX) y `GPIO 27` (TX). El firmware activa automáticamente `GPIO 16 = HIGH` al arrancar para energizar el convertidor elevador ME2107 de la placa).*

### Asignación de Salidas PWM en la T-CAN485 (Header de Expansión):

| Periférico / Canal | Pin Header T-CAN485 | Función / Dispositivo Conectado |
| :--- | :--- | :--- |
| **PWM Canal 1 (Blanco)** | `GPIO 25` | Foco Izquierdo Set 1 (LEDC Channel 0, 1 kHz) |
| **PWM Canal 2 (Amarillo)**| `GPIO 32` | Foco Derecho Set 1 (LEDC Channel 1, 1 kHz) |
| **PWM Canal 3 (Azul)** | `GPIO 33` | Focos de Niebla Set 2 (LEDC Channel 2, 1 kHz) |
| **PWM Canal 4 (Rojo)** | `GPIO 14` | Bocina / Accesorio (LEDC Channel 3, 1 kHz) |
| **Alimentación LILYGO** | Bornera `VIN` (5V-12V) y `GND` | Alimentación de la placa (vía regulador o batería protegida) |
| **LED RGB WS2812** | `GPIO 4` (Interno) | Indicador visual de estado CAN (Verde = Escuchando) |

---

## 5. Cableado hacia los Accesorios (Conectores Estancos Comprados)
Para igualar la calidad del DENALI CANsmart:
* Utilizar los conectores comprados **Superseal 1.5**:
  * **2 Conectores de 2 Pines**: Para los focos auxiliares izquierdo y derecho (Set 1).
  * **2 Conectores de 3 Pines**: Para los faros de niebla (Set 2 con DRL) y bocina/freno.
* Cableado siliconado AWG 16 (para canales de luces hasta 5A-10A) y AWG 14 (para alimentación principal y masa general).
* 👉 **Plano de Conexión Completo**: Consulta [EsquematicoConexiones.md](file:///c:/APPS-DEV/zzz/KTM-Bus/EsquematicoConexiones.md) y la imagen técnica verificada [ktm_wiring_diagram_v2.jpg](file:///c:/APPS-DEV/zzz/KTM-Bus/hardware/ktm_wiring_diagram_v2.jpg).

