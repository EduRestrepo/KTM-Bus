# KTM 1290 CANsmart // Gestor Inteligente de Accesorios y Bus CAN (2024)

Proyecto integral de desarrollo de hardware, firmware y panel web para el control inteligente de accesorios y luces auxiliares en la **KTM 1290 Super Adventure S (modelo 2024 Euro 5)** mediante lectura pasiva del Bus CAN original a **500 kbps**.

Inspirado en la arquitectura de **HEX ezCAN Gobi** y **DENALI CANsmart Gen II**.

---

## 🚀 Arquitectura del Proyecto

```
c:\APPS-DEV\zzz\KTM-Bus\
├── firmware/                   # Firmware C++ para ESP32 (Arduino / PlatformIO)
│   ├── platformio.ini          # Configuración de compilación con 1 clic
│   ├── include/
│   │   ├── types.h             # Estructuras de canales, telemetría y configuración
│   │   └── ktm_can_ids.h       # Mapeo y decodificación de tramas CAN KTM Euro 5 (500 kbps)
│   └── src/
│       ├── main.cpp            # Setup, inicialización TWAI (Listen-Only) y bucles
│       ├── pwm_manager.h/.cpp  # Control de 4 salidas MOSFET / PROFET con rampa suave (LEDC)
│       ├── controller_fsm.h/.cpp # Máquina de estados: Triple Clic, Hold 3s, Dimmer en vivo
│       ├── config_store.h/.cpp # Persistencia en memoria NVS / Flash no volátil
│       └── web_api.h/.cpp      # Servidor REST API + SoftAP WiFi "KTM-CANSMART"
├── web/                        # Interfaz Web UI Embebida (KTM Racing Dark Theme)
│   ├── index.html              # Panel de 4 pestañas: Circuitos, Ajustes, Simulador Piña, Monitor CAN
│   ├── style.css               # Diseño CSS moderno, glassmorphism, micro-animaciones y resplandores
│   └── app.js                  # Lógica reactiva, simulador de luces y cliente REST API
├── hardware/                   # Diseño electrónico, esquemáticos y pinouts
│   ├── schematic_notes.md      # Pinout Euro 5 (ISO 19689), BOM de componentes y notas de cableado
│   ├── generate_diagrams.py    # Genera las 4 hojas de cableado (SVG + PNG)
│   └── diagramas/              # Hojas 1-4: alimentación, bus CAN, señales PWM, salidas (SVG + PNG)
├── EsquematicoConexiones.md    # Guía de cableado paso a paso (cables W1-W26)
├── ListaCompras.md             # Lista de compras con enlaces
└── README.md                   # Documentación general del proyecto
```

---

## 🛠️ 1. Mapeo de Mandos en la Moto (KTM 1290 Switchgear)

El sistema replica fielmente la lógica de control desde la piña izquierda original de la KTM:

| Grupo de Luces | Encendido / Apagado (ON/OFF) | Ajuste de Brillo en Vivo (*DIM Mode*) | Teclas de Regulación |
| :--- | :--- | :--- | :--- |
| **Light Set 1** *(Luces Principales)* | Mantener presionado **3 segundos** el botón central de cancelar intermitente | Mantener presionado **hacia ARRIBA 3 segundos** el gatillo de ráfagas | Teclas **`+`** y **`-`** de la cruceta (+10% / -10%) |
| **Light Set 2** *(Luces de Niebla)* | **Triple clic rápido** en el botón central de cancelar intermitente | Mantener presionado **hacia ABAJO 3 segundos** el gatillo de ráfagas | Teclas **`+`** y **`-`** de la cruceta (+10% / -10%) |

* **Confirmación**: Las luces emiten un destello breve al entrar en modo Dimmer. Tras 5 segundos sin tocar la cruceta, se guardan los cambios automáticamente en la memoria no volátil y la cruceta recupera su función normal en la pantalla TFT.
* **Seguridad Activa**:
  * **Estroboscópico de Bocina**: Parpadeo alterno a 12 Hz al tocar el claxon.
  * **Corte por Intermitente**: El foco del lado que señaliza se apaga temporalmente para garantizar máxima visibilidad del indicador de giro.
  * **Luz Larga**: Al activar las largas o ráfagas, las auxiliares suben automáticamente al 100%.

---

## ⚡ 2. Conexión Hardware en la KTM 1290 (2024 Euro 5)

* **Bus CAN**: Se conecta intercalado en el conector de diagnóstico rojo Euro 5 (ISO 19689 de 6 pines) bajo el asiento del acompañante:
  * Pin 1: `CAN-High` (Cable Naranja/Negro)
  * Pin 2: `CAN-Low` (Cable Naranja/Marrón)
  * Pin 4: `GND` (Masa)
* **Alimentación de Potencia**: Directa a los bornes de la batería (+12V permanente) con **fusible aéreo de 30A**.
* **Protección del Microcontrolador**: El driver TWAI del ESP32 opera en **`TWAI_MODE_LISTEN_ONLY`** (escucha pasiva). Nunca transmite pulsos de ACK ni inyecta datos al bus, garantizando cero interferencias con la ECU o el ABS Bosch de la moto.
* 👉 **Esquema Completo de Cableado**: 4 hojas con cada cable numerado (W1–W26) en [EsquematicoConexiones.md](EsquematicoConexiones.md); imágenes en [hardware/diagramas/](hardware/diagramas/).

---


## 🌐 3. Panel de Configuración Web

La interfaz web está disponible localmente en el servidor de desarrollo:
👉 **[http://localhost:8092/index.html](http://localhost:8092/index.html)**

### Funcionalidades del Panel:
1. **Asignación de Circuitos**: Matriz interactiva de 13 funciones con selección de canal físico (Blanco, Amarillo, Azul, Rojo) y límite de corriente digital (1A a 10A por canal).
2. **Comportamiento y Brillo**: Deslizadores Día/Noche calibrados con el sensor TFT de la moto, potencia con largas y conmutadores de seguridad activa.
3. **Simulador de Piña KTM 1290**: Réplica visual e interactiva de la piña de mandos y faros de la moto para probar el triple clic, la pulsación de 3s, el modo Dimmer y la bocina en tiempo real.
4. **Telemetría y Monitor CAN**: Registro en vivo de las tramas decodificadas a 500 kbps, voltímetro y amperímetro virtual.

---

## 🧠 4. Selección del Controlador ESP32: LILYGO T-CAN485 (Seleccionado)

El firmware de este proyecto viene **preconfigurado de fábrica para la placa [LILYGO T-CAN485](https://github.com/Xinyuan-LilyGO/T-CAN485)**, la cual simplifica enormemente la instalación al integrar el transceptor CAN y borneras de conexión en una sola PCB.

### Configuración de Hardware en la LILYGO T-CAN485:
* **Líneas CAN**:
  * `CAN_RX` en **GPIO 26** y `CAN_TX` en **GPIO 27** (cableados internamente al chip SN65HVD231).
  * Borne `CAN_H` -> Pin 1 Euro 5 Rojo KTM (Naranja/Negro).
  * Borne `CAN_L` -> Pin 2 Euro 5 Rojo KTM (Naranja/Marrón).
  * Borne `GND`   -> Pin 4 Euro 5 Rojo KTM (Masa).
* **Alimentación del Transceptor**: La placa requiere activar en el arranque **`GPIO 16 = HIGH`** para encender el regulador elevador ME2107 (ya configurado en [`main.cpp`](file:///c:/APPS-DEV/zzz/KTM-Bus/firmware/src/main.cpp#L62-L66)).
* **Salidas PWM para MOSFETs (Header de expansión)**:
  * Canal Blanco (Foco Izq Set 1): **GPIO 25**
  * Canal Amarillo (Foco Der Set 1): **GPIO 32**
  * Canal Azul (Nieblas Set 2): **GPIO 33**
  * Canal Rojo (Bocina / Accesorio): **GPIO 18**

### Tabla Comparativa de Referencia:

| Placa ESP32 | Formato | Ventajas | Desventajas / Observaciones | Precio Aprox. |
| :--- | :--- | :--- | :--- | :--- |
| **ESP32 DevKit V1 (ESP-WROOM-32)** *(Recomendada)* | Placa estándar 30/38 pines | Máxima compatibilidad, miles de ejemplos, muy económica y fácil de soldar | Requiere conectar el transceptor SN65HVD230 por cables | ~4 € – 6 € |
| **ESP32-WROOM-32U (Con conector IPEX)** | Placa con antena externa | **Ideal para moto:** si la caja va bajo el asiento junto al subchasis metálico, puedes sacar una pequeña antena adhesiva exterior | Requiere comprar antenita IPEX/U.FL aparte | ~5 € – 7 € |
| **LILYGO T-CAN485** | Todo en Uno (All-in-One) | Incluye el transceptor CAN y regulador de voltaje en la misma PCB (menos cables) | Tamaño algo mayor que un XIAO | ~18 € – 22 € |
| **Waveshare ESP32-S3-CAN** | Todo en Uno (All-in-One) | Chip ESP32-S3 moderno + transceptor integrado con aislamiento galvánico | Requiere puerto USB-C para flasheo | ~19 € – 24 € |
| **Seeed Studio XIAO ESP32-S3** | Ultra-miniatura (21x17 mm) | Tamaño similar a una moneda, cabe en cualquier rincón bajo el asiento | Requiere conectar transceptor CAN externo | ~7 € – 9 € |

### Lista de Compra Completa (BOM del Proyecto DIY):
> 🛒 **Guía detallada con enlaces directos de compra en AliExpress:** Consulta [ListaCompras.md](file:///c:/APPS-DEV/zzz/KTM-Bus/ListaCompras.md).

1. **Microcontrolador**: **LILYGO T-CAN485** (preconfigurada de fábrica) o **ESP32 DevKit V1** / **ESP32-WROOM-32U**.
2. **Transceptor CAN (3.3V)**: Módulo **SN65HVD230** *(Evitar el TJA1050 de 5V para no dañar los pines del ESP32)*.
3. **Etapa de Potencia 12V**: Módulo de **4 MOSFETs High-Side** (Canal P tipo IRF4905) o placa **PROFET Infineon (BTS724G o BTS7008)** con protección térmica y cortocircuito automática.
4. **Fuente de Alimentación**: Convertidor reductor automotriz **LM2596HV** (High Voltage) o impermeable 12V a 5V con TVS **SMAJ24A**.
5. **Portafusible Aéreo y Fusible**: Fusible Maxi/Mini de **30A** directo a borne de batería.
6. **Conector KTM**: Clavija macho estanca **Euro 5 de 6 pines (ISO 19689)**.
7. **Conectores a Luces**: Conectores estancos **Superseal 1.5** de 2 y 3 pines (IP67).


