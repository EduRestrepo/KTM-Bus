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

> **IMPORTANTE DE SEGURIDAD**: Para máxima fiabilidad y evitar caídas de tensión en accesorios de alta potencia (luces de 60W-120W), la alimentación de potencia **NO** debe tomarse del pin 3 del conector Euro 5. La potencia se toma **de los bornes de la batería (+12V permanente KL30) con un fusible aéreo de 15A** y pasa por un **relé de contacto** que solo cierra con la moto encendida (así el consumo en reposo es ≈ 0 y no se descarga la batería). El pin 3 del Euro 5 (KL15) se usa **solo como señal** de mando de ese relé (a través de un transistor), nunca como potencia; **mídelo antes** (0 V con contacto OFF, ≈ 12 V con ON). El resto del conector Euro 5 se usa para leer **CAN-H**, **CAN-L** y masa **GND**.

---

## 2. Diagrama de Bloques del Sistema DIY

```
                              ┌───────────────────────────────────────────────┐
                              │                 KTM-BUS V1.0                  │
                              │                                               │
   Batería Moto (+12V) ──────►│ Fusible 15A + Relé contacto ──┬──► Regulador DC-DC (Automotriz)
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

## 3. Lista de Componentes del Proyecto (BOM Real de ListaCompras.md)

### A. Componentes Comprados y Verificados ✅
1. **Conector KTM Euro 5 (ISO 19689 - 6 Pines Rojo)**:
   * **Modelo**: Sumitomo OEM `6189-7963 / MWTPB-06-1A-R` Set Macho + Hembra con cables flexibles.
   * **Enlace AliExpress**: [Item 1005012171131942](https://es.aliexpress.com/item/1005012171131942.html)
   * **Conexión Pasante (*Pass-Through*)**: Permite enchufar la máquina de diagnosis del concesionario sin desconectar la centralita.
   * **Uso de Pines**:
     * Pin 1 (Naranja/Negro): `CAN_H` ➔ Borna CAN_H de la LILYGO.
     * Pin 2 (Naranja/Marrón): `CAN_L` ➔ Borna CAN_L de la LILYGO.
     * Pin 4 (Marrón/Negro): `GND` ➔ Borna GND de la LILYGO.
     * Pin 3 (+12V contacto, KL15): **solo señal** hacia el driver NPN del relé de contacto (W32). Nunca como potencia.

2. **Convertidor Reductor DC-DC Automotriz (12V a 5V 3A)**:
   * **Modelo**: Carcasa sellada en resina epoxi impermeable (IP67).
   * **Enlace AliExpress**: [Item 1005007820939213](https://es.aliexpress.com/item/1005007820939213.html)
   * **Entrada**: Cable Rojo a +12V protegido (post-fusible), Cable Negro a Borne (-) batería.
   * **Salida**: 5V estables (hasta 3A) hacia bornas `VIN` y `GND` de la LILYGO T-CAN485.

3. **Portafusible Aéreo Estanco (con fusible de 15A)**:
   * **Modelo**: Portafusible aéreo de goma impermeable para automoción con fusible de **15A** (si trae uno de 30A, cámbialo: no protegería los cables AWG 14/16/18).
   * **Enlace AliExpress**: [Item 32813530925](https://es.aliexpress.com/item/32813530925.html)
   * **Ubicación**: Intercalado en el cable positivo AWG 14 directo del borne (+) de la batería antes del relé de contacto que alimenta la placa MOSFET y el convertidor (este último con un fusible en línea de 3A).

4. **Conectores Estancos para Luces y Accesorios**:
   * **Modelo**: Conectores automotrices impermeables Superseal 1.5 (IP67).
   * **Enlace AliExpress**: [Item 1005008334229391](https://es.aliexpress.com/item/1005008334229391.html)
   * **Cantidad y Configuración**:
     * **2 Conectores de 2 Pines (2 holes)**: Set 1 de Focos Auxiliares (Foco Izquierdo y Foco Derecho).
     * **2 Conectores de 3 Pines (3 holes)**: Set 2 de Focos de Niebla (con hilo DRL) y Conector 4 para Bocina / Luz de Freno estroboscópica.

5. **Etapa de Potencia MOSFETs (4 Canales High-Side)**:
   * **Modelo**: Placa amplificadora PLC de 4 Canales con aislamiento optoacoplado.
   * **Configuración Seleccionada**:
     * Formato: `4CH Only Board` (compacto para caja bajo asiento).
     * Control de entrada: `PNP Input 3.3-5V` (compatible con 3.3V nativo de los GPIOs del ESP32). Placa eletechsup OPMSA04_PNP: bornes de potencia VCC, GND, Y1-Y4; bornes de entrada X1-X4 y COM. Puentes de soldadura: Input Level = PNP y Trigger Voltage = 5V. Con nivel PNP, COM va a GND.
     * Salida conmutada: `PNP Output` (**High-Side**: conmuta +12V con masa común al chasis).
     * Capacidad: el anuncio indica hasta 5A por canal, pero el serigrafiado de la placa menciona "<2A". **Confirma la corriente real en la ficha** y protege cada salida con un fusible en línea de 3–5A. Para cargas inductivas o de más de 3A (bocina de aire) usa un relé excitado por la salida.

### B. Componentes Restantes de la Instalación ⏳
1. **Controlador ESP32**: **LILYGO T-CAN485** ([Item 1005003624034092](https://www.aliexpress.com/item/1005003624034092.html)) con transceptor CAN SN65HVD231 y borneras a tornillo.
2. **Caja Estanca IP65 ABS**: Aprox. 100 x 68 x 50 mm para alojar bajo el asiento del acompañante.
3. **Cable Siliconado de Alta Temperatura (200°C)**:
   * **AWG 14** (Rojo y Negro): Batería a portafusible y masa principal.
   * **AWG 16**: Salidas de potencia hacia los conectores de los faros.
4. **Terminales de Anilla M6**: Para asegurar la conexión a los bornes de la batería.

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
| **PWM Canal 4 (Rojo)** | `GPIO 18` | Bocina / Accesorio (LEDC Channel 3, 1 kHz) |
| **Alimentación LILYGO** | Bornera `VIN` (5V-12V) y `GND` | Alimentación de la placa (vía regulador o batería protegida) |
| **LED RGB WS2812** | `GPIO 4` (Interno) | Indicador visual de estado CAN (Verde = Escuchando) |

---

## 5. Cableado hacia los Accesorios (Conectores Estancos Comprados)

Distribución en los 4 conectores **Superseal 1.5** comprados. Los IDs (W16…W26) son los de la Hoja 4 del esquema.

### A. Conectores de 2 Pines (Set 1 de focos auxiliares)
* **Conector 1 (Foco izquierdo)**:
  * **Pin 1 (rojo AWG 16, W16)**: `Y1` (+12 V conmutado por PWM, LEDC canal 0). Dimmer día/noche, ráfagas y apagado con intermitente izquierdo.
  * **Pin 2 (negro AWG 16, W23)**: GND desde la regleta de masa.
* **Conector 2 (Foco derecho)**:
  * **Pin 1 (rojo AWG 16, W17)**: `Y2` (+12 V conmutado por PWM, LEDC canal 1). Dimmer día/noche, ráfagas y apagado con intermitente derecho.
  * **Pin 2 (negro AWG 16, W24)**: GND desde la regleta de masa.

### B. Conectores de 3 Pines (Nieblas y Bocina/Freno)
* **Conector 3 (Faros de niebla Set 2 + aro DRL)**:
  * **Pin 1 (rojo AWG 16, W18)**: `Y3` (+12 V conmutado por PWM, LEDC canal 2).
  * **Pin 2 (negro AWG 16, W25)**: GND desde la regleta de masa.
  * **Pin 3 (amarillo AWG 18, W20)**: aro DRL del foco, **en paralelo con el Pin 1** (mismo canal `Y3`).
* **Conector 4 (Bocina / luz de freno estroboscópica / accesorio)**:
  * **Pin 1 (rojo AWG 16, W19)**: `Y4` (+12 V conmutado por PWM, LEDC canal 3).
  * **Pin 2 (negro AWG 16, W26)**: GND desde la regleta de masa.
  * **Pin 3 (amarillo AWG 18, W21)**: luz de posición/aux, **en paralelo con el Pin 1** (mismo canal `Y4`).

> ⚠️ **El Pin 3 no se alimenta de +12 V permanente.** Se puentea al Pin 1 del mismo conector para que se apague con la moto y respete el dimmer. Conectarlo a +12 V directo de batería lo dejaría siempre encendido y descargaría la batería. Pin 1 + Pin 3 comparten el límite de 5 A del canal. Para un DRL independiente haría falta un quinto canal.

* La **regleta de masa** es un conector de palanca tipo Wago 221-415 (5 vías): 1 entrada desde el borne (−) de la batería (W22) y 4 salidas, una por conector (W23–W26).

---

## 6. Documentación Gráfica y Planos de Conexión

Los esquemas están en 4 hojas (cada una en PNG y en SVG, el mismo dibujo) dentro de [`hardware/diagramas/`](diagramas/):

| Hoja | Contenido | PNG | SVG |
| :---: | :--- | :--- | :--- |
| 1 | Alimentación y masas (W1–W7) | [01_alimentacion.png](diagramas/01_alimentacion.png) | [01_alimentacion.svg](diagramas/01_alimentacion.svg) |
| 2 | Bus CAN (W8–W10) | [02_bus_can.png](diagramas/02_bus_can.png) | [02_bus_can.svg](diagramas/02_bus_can.svg) |
| 3 | Señales PWM (W11–W15) | [03_senales_pwm.png](diagramas/03_senales_pwm.png) | [03_senales_pwm.svg](diagramas/03_senales_pwm.svg) |
| 4 | Salidas a conectores (W16–W26) | [04_salidas_conectores.png](diagramas/04_salidas_conectores.png) | [04_salidas_conectores.svg](diagramas/04_salidas_conectores.svg) |

* 📊 Tablas de cables y pinout completo: [EsquematicoConexiones.md](../EsquematicoConexiones.md).
* 🔁 Para regenerar las imágenes tras cambiar algo: `python hardware/generate_diagrams.py` (requiere `playwright` con Chromium para los PNG).


