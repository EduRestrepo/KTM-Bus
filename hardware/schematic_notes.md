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
     * Pin 3 (+12V de moto): **Aislado / Desconectado** (la potencia se toma de la batería).

2. **Convertidor Reductor DC-DC Automotriz (12V a 5V 3A)**:
   * **Modelo**: Carcasa sellada en resina epoxi impermeable (IP67).
   * **Enlace AliExpress**: [Item 1005007820939213](https://es.aliexpress.com/item/1005007820939213.html)
   * **Entrada**: Cable Rojo a +12V protegido (post-fusible), Cable Negro a Borne (-) batería.
   * **Salida**: 5V estables (hasta 3A) hacia bornas `VIN` y `GND` de la LILYGO T-CAN485.

3. **Portafusible Aéreo Estanco con Fusible de 30A**:
   * **Modelo**: Portafusible aéreo de goma impermeable para automoción con fusible Maxi/Mini de 30A.
   * **Enlace AliExpress**: [Item 32813530925](https://es.aliexpress.com/item/32813530925.html)
   * **Ubicación**: Intercalado en el cable positivo AWG 14 directo del borne (+) de la batería antes de alimentar la placa MOSFET y el convertidor.

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
     * Control de entrada: `PNP Input 3.3-5V` (compatible con 3.3V nativo de los GPIOs del ESP32).
     * Salida conmutada: `PNP Output` (**High-Side**: conmuta +12V con masa común al chasis).
     * Capacidad: Hasta 5A continuos por canal (60W por foco).

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
| **PWM Canal 4 (Rojo)** | `GPIO 14` | Bocina / Accesorio (LEDC Channel 3, 1 kHz) |
| **Alimentación LILYGO** | Bornera `VIN` (5V-12V) y `GND` | Alimentación de la placa (vía regulador o batería protegida) |
| **LED RGB WS2812** | `GPIO 4` (Interno) | Indicador visual de estado CAN (Verde = Escuchando) |

---

## 5. Cableado hacia los Accesorios (Conectores Estancos Comprados)

Para igualar la robustez y calidad del DENALI CANsmart, la distribución en los 4 conectores **Superseal 1.5** comprados se realiza de forma estricta:

### A. Conectores de 2 Pines (Juego 1 de Focos Auxiliares)
* **Conector 1 (Foco Auxiliar Izquierdo - Set 1)**:
  * **Pin 1 (Cable Rojo AWG 16)**: Salida `OUT 1` (+12V conmutado por PWM, LEDC Canal 0). Dimmer día/noche, ráfagas y apagado al activar intermitente izquierdo.
  * **Pin 2 (Cable Negro AWG 16)**: Masa común `GND` directa al borne negativo de la batería.
* **Conector 2 (Foco Auxiliar Derecho - Set 1)**:
  * **Pin 1 (Cable Rojo AWG 16)**: Salida `OUT 2` (+12V conmutado por PWM, LEDC Canal 1). Dimmer día/noche, ráfagas y apagado al activar intermitente derecho.
  * **Pin 2 (Cable Negro AWG 16)**: Masa común `GND` directa al borne negativo de la batería.

### B. Conectores de 3 Pines (Nieblas con DRL y Freno/Accesorio)
* **Conector 3 (Faros de Niebla Set 2 + Aro DRL)**:
  * **Pin 1 (Cable Rojo AWG 16)**: Salida `OUT 3` (+12V conmutado por PWM, LEDC Canal 2). Encendido con triple clic en botón cancelar intermitente o dimmer dedicado.
  * **Pin 2 (Cable Negro AWG 16)**: Masa común `GND` de retorno.
  * **Pin 3 (Cable Amarillo AWG 18)**: **Luz Diurna / Aro DRL (Halo)**. Conectado directamente a la **Línea DRL / Posición (+12V bajo contacto protegido)** para que el halo ámbar o blanco permanezca encendido con la moto en marcha.
* **Conector 4 (Luz de Freno Estroboscópica / Bocina / Accesorio)**:
  * **Pin 1 (Cable Rojo AWG 16)**: Salida `OUT 4` (+12V conmutado por PWM, LEDC Canal 3). Destello estroboscópico de alerta al frenar o activar la bocina.
  * **Pin 2 (Cable Negro AWG 16)**: Masa común `GND` de retorno.
  * **Pin 3 (Cable Amarillo AWG 18)**: **Luz de Posición Trasera (Running Light)**. Conectado a la **Línea DRL / Posición (+12V bajo contacto protegido)** para iluminación tenue fija de posición.

---

## 6. Documentación Gráfica y Planos de Conexión

* 📊 **Plano de Conexionado Detallado**: Consulta [EsquematicoConexiones.md](file:///c:/APPS-DEV/zzz/KTM-Bus/EsquematicoConexiones.md) para el pinout paso a paso.
* 🖼️ **Esquema Visual de Componentes y Cableado (Alta Resolución)**: [hardware/ktm_wiring_diagram_v2.jpg](file:///c:/APPS-DEV/zzz/KTM-Bus/hardware/ktm_wiring_diagram_v2.jpg) y [hardware/ktm_wiring_diagram_v2.png](file:///c:/APPS-DEV/zzz/KTM-Bus/hardware/ktm_wiring_diagram_v2.png).
* 📐 **Diagrama Técnico Vectorial SVG**: [hardware/ktm_wiring_schematic.svg](file:///c:/APPS-DEV/zzz/KTM-Bus/hardware/ktm_wiring_schematic.svg).


