# 🛒 Lista de Compras y Componentes: KTM 1290 CANsmart (AliExpress)

Guía completa y estado actualizado de compras para la construcción del clon DIY de **HEX ezCAN / DENALI CANsmart** para la **KTM 1290 Super Adventure S (2024 Euro 5)**.

---

## 📦 Estado Actual de tu Cesta de Compra

| Componente | Estado | Producto Exacto / Opciones Seleccionadas | Enlace de Compra AliExpress |
| :--- | :---: | :--- | :--- |
| **Conector KTM Euro 5** | ✅ **COMPRADO** | Conector OEM Sumitomo 6 pines con cables flexibles (**1 Set Macho + Hembra**) | [Ver Producto en AliExpress](https://es.aliexpress.com/item/1005012171131942.html) |
| **Convertidor 12V a 5V (3A)** | ✅ **COMPRADO** | Convertidor DC-DC impermeable sellado (**Open Wire** o **Type-C**) | [Ver Producto en AliExpress](https://es.aliexpress.com/item/1005007820939213.html) |
| **Portafusible Aéreo** | ✅ **COMPRADO** | Portafusible aéreo estanco con tapa de goma + fusible de **30A** | [Ver Producto en AliExpress](https://es.aliexpress.com/item/32813530925.html) |
| **Conectores Estancos Focos** | ✅ **COMPRADO** | Conectores automotrices impermeables (**2x de 2 vías + 2x de 3 vías**) | [Ver Producto en AliExpress](https://es.aliexpress.com/item/1005008334229391.html) |
| **Etapa de Potencia MOSFETs** | ✅ **ELEGIDO** | Placa PLC 4CH MOS aislamiento (**4CH Only Board**, **PNP Input 3.3-5V**, **PNP Output**) | [Ver Producto en AliExpress](https://es.aliexpress.com/w/wholesale-4-channel-mosfet-module-3.3v-plc.html) |
| **Controlador ESP32 + CAN** | ⏳ *Pendiente* | **LILYGO T-CAN485** (ESP32 con chip CAN y borneras integradas) | [Comprar LILYGO T-CAN485](https://www.aliexpress.com/item/1005003624034092.html) |
| **Caja Estanca de Montaje** | ⏳ *Pendiente* | Caja ABS impermeable IP65 (aprox. 100 x 68 x 50 mm para bajo el asiento) | [Comprar Caja Estanca IP65](https://es.aliexpress.com/w/wholesale-waterproof-junction-box-ip65.html) |
| **Cable Siliconado** | ⏳ *Pendiente* | Cable de silicona flexible resistente al calor: **AWG 14** (batería) y **AWG 16** (focos) | [Comprar Cable Siliconado](https://es.aliexpress.com/w/wholesale-silicone-wire-awg14-awg16.html) |
| **Terminales de Anilla M6** | ⏳ *Pendiente* | Terminales de anilla M6 para bornes de la batería | [Comprar Terminales M6](https://es.aliexpress.com/w/wholesale-ring-terminals-m6.html) |
| **Regleta de masa (GND)** | ⏳ *Pendiente* | Conectores de palanca **Wago 221-415** (5 vías) o equivalente, para repartir la masa a los 4 conectores | [Buscar Wago 221-415](https://es.aliexpress.com/w/wholesale-wago-221-415.html) |

---

## 1. ⚡ Detalle de los Componentes Comprados / Seleccionados

### A. Conector KTM Euro 5 (ISO 19689 - 6 Pines Rojo) ✅
* **Producto:** Conector OBDII de 6 pines con cable flexible (Sumitomo OEM `6189-7963 / MWTPB-06-1A-R`).
* **Enlace directo:** [https://es.aliexpress.com/item/1005012171131942.html](https://es.aliexpress.com/item/1005012171131942.html)
* **Variante comprada:** `1 Set (Male + Female)` con cables flexibles integrados.
* **Función:** Se conecta directamente a la toma de diagnosis roja bajo el asiento del acompañante sin cortar cables originales. Permite hacer un puente pasante (*pass-through*) para que en el taller oficial puedan enchufar la máquina de diagnosis KTM sin desconectar tu centralita.
  * **Pin 1:** `CAN-High` (Cable Naranja/Negro) ➔ Borna CAN_H de la LILYGO
  * **Pin 2:** `CAN-Low` (Cable Naranja/Marrón) ➔ Borna CAN_L de la LILYGO
  * **Pin 4:** `GND / Masa` ➔ Borna GND de la LILYGO

---

### B. Convertidor Reductor DC-DC Automotriz (12V a 5V 3A) ✅
* **Producto:** Convertidor reductor DC-DC impermeable sellado en resina epoxi.
* **Enlace directo:** [https://es.aliexpress.com/item/1005007820939213.html](https://es.aliexpress.com/item/1005007820939213.html)
* **Variante comprada:** `Open Wire` (cables pelados para borneras) o `Type-C`.
* **Función:** Transforma los 12V–14.4V de la batería de la moto en 5V estables (hasta 3A) para alimentar de forma segura la placa LILYGO ESP32, filtrando ruidos del alternador.

---

### C. Portafusible Aéreo Estanco con Fusible de 30A ✅
* **Producto:** Portafusible aéreo automotriz estanco con tapa de goma impermeable.
* **Enlace directo:** [https://es.aliexpress.com/item/32813530925.html](https://es.aliexpress.com/item/32813530925.html)
* **Variante comprada:** Con fusible de **30A**.
* **Función:** Protección principal de toda la instalación eléctrica. Va intercalado en el cable positivo directo desde el borne (+) de la batería antes de alimentar la placa de potencia.

---

### D. Conectores Estancos para Focos y Accesorios ✅
* **Producto:** Conectores impermeables de automoción (tipo Superseal).
* **Enlace directo:** [https://es.aliexpress.com/item/1005008334229391.html](https://es.aliexpress.com/item/1005008334229391.html)
* **Variante comprada:** **2 x 2 holes** (2 pines) y **2 x 3 holes** (3 pines).
* **Función:**
  * Los **2 de 2 pines** para el Set 1 (Foco Izquierdo y Foco Derecho).
  * Los **2 de 3 pines** para el Set 2 de antinieblas (o luces auxiliares con luz diurna DRL de 3 hilos o luz de freno estroboscópica).

---

### E. Etapa de Potencia MOSFETs (4 Canales High-Side) ✅
* **Producto:** Placa Amplificadora de Señal PLC de 4 Canales, 5A, Módulo MOS de Aislamiento Optoacoplado.
* **Enlace directo de búsqueda:** [Buscar Módulo MOSFET 4CH 3.3V PLC](https://es.aliexpress.com/w/wholesale-4-channel-mosfet-module-3.3v-plc.html)
* **Opciones validadas:**
  * **Color / Formato:** `4CH Only Board` (más compacta para alojar en caja bajo el asiento).
  * **Voltaje de Entrada:** `PNP Input 3.3-5V` (placa eletechsup OPMSA04_PNP; bornes X1-X4 + COM; puentes: Input Level = PNP, Trigger Voltage = 5V; se activa con nivel alto a 3.3V desde los pines PWM del ESP32 con COM a GND).
  * **Voltaje de Salida:** `PNP Output` (**High-Side**: conmuta el cable positivo +12V hacia las luces con masa común al chasis).
* **Potencia:** Hasta 5A continuos por canal a 12V (60W por canal).

---

## 2. ⏳ Componentes Restantes por Comprar

### 1. Cerebro: LILYGO T-CAN485 (ESP32 con CAN Integrado)
Es la placa principal para la que está preconfigurado el firmware de este proyecto:
* 👉 **Enlace oficial:** [LILYGO T-CAN485 en AliExpress](https://www.aliexpress.com/item/1005003624034092.html)
* **Qué incluye:** Microcontrolador ESP32 Dual-Core a 240 MHz, transceptor CAN SN65HVD231 a 3.3V, puerto USB-C y borneras a tornillo para `CAN_H`, `CAN_L` y alimentación.

### 2. Caja Estanca de Alojamiento (Bajo el Asiento)
Caja de plástico ABS con junta de estanqueidad para proteger las dos placas y el convertidor del agua y polvo bajo el asiento de la KTM:
* 👉 **Enlace:** [Caja de Conexiones Impermeable IP65 (aprox. 100 x 68 x 50 mm)](https://es.aliexpress.com/w/wholesale-waterproof-junction-box-ip65.html)

### 3. Cableado Siliconado y Terminales de Batería
* **Cables de silicona flexible (200°C):**
  * **AWG 14 (Rojo y Negro):** Para los cables gruesos de batería a portafusible y masa principal.
  * **AWG 16:** Para los cables que van a los conectores de los faros.
  * 👉 **Enlace:** [Cable de Silicona AWG 14 y AWG 16](https://es.aliexpress.com/w/wholesale-silicone-wire-awg14-awg16.html)
* **Terminales de anilla M6:** Para atornillar firmemente al borne de la batería.
  * 👉 **Enlace:** [Terminales de anilla M6](https://es.aliexpress.com/w/wholesale-ring-terminals-m6.html)

### 4. Regleta de Masa (Wago 221-415)
La Hoja 4 del esquema reparte la masa del borne (−) de la batería a los 4 conectores de luces (cables W22–W26). Se hace con un conector de palanca de 5 vías (1 entrada + 4 salidas):
* 👉 **Enlace:** [Wago 221-415 (5 vías)](https://es.aliexpress.com/w/wholesale-wago-221-415.html)
