#include <Arduino.h>
#include "driver/twai.h"
#include "esp_task_wdt.h"
#include "types.h"
#include "ktm_can_ids.h"
#include "pwm_manager.h"
#include "controller_fsm.h"
#include "config_store.h"
#include "web_api.h"

// Pines TWAI / CAN Bus para LILYGO T-CAN485
#define TWAI_RX_PIN       GPIO_NUM_26
#define TWAI_TX_PIN       GPIO_NUM_27
#define PIN_CAN_SE        23  // CAN_SE del transceptor: LOW = modo alta velocidad (T-CAN485 IO23)
#define PIN_5V_ENABLE     16  // Habilita el regulador ME2107 en LILYGO T-CAN485 para energizar el transceptor CAN

// Si no llega ninguna trama CAN estándar durante este tiempo se considera contacto OFF
#define CAN_SILENCE_TIMEOUT_MS   2000
#define WDT_TIMEOUT_S            5

SystemConfig sysConfig;
KtmTelemetry bikeTelemetry;
LiveDimmerState liveDimmer;

static bool needConfigSave = false;
static uint32_t lastTelemetryHeartbeat = 0;
static uint32_t lastCanStatusCheck = 0;

// ---- Modo sniffer (enviar 's' por el monitor serie para activar/desactivar) ----
// Imprime cada trama CAN solo cuando cambia su contenido. Sirve para capturar los IDs reales
// de la moto (pulsa cada mando y anota qué ID/byte/bit cambia) y corregir ktm_can_ids.h.
static bool snifferEnabled = false;
#define SNIFFER_TABLE_SIZE 128
static struct { uint32_t id; uint8_t len; uint8_t data[8]; bool used; } snifferTable[SNIFFER_TABLE_SIZE];

static void snifferHandle(const twai_message_t& m) {
    int free_slot = -1;
    for (int i = 0; i < SNIFFER_TABLE_SIZE; i++) {
        if (snifferTable[i].used && snifferTable[i].id == m.identifier) {
            if (snifferTable[i].len == m.data_length_code &&
                memcmp(snifferTable[i].data, m.data, m.data_length_code) == 0) return; // sin cambios
            free_slot = i;
            break;
        }
        if (!snifferTable[i].used && free_slot < 0) free_slot = i;
    }
    if (free_slot >= 0) {
        snifferTable[free_slot].used = true;
        snifferTable[free_slot].id = m.identifier;
        snifferTable[free_slot].len = m.data_length_code;
        memcpy(snifferTable[free_slot].data, m.data, m.data_length_code);
    }
    Serial.printf("[SNIFF] %lu ms  ID 0x%03X  DLC %d  ", (unsigned long)millis(), (unsigned)m.identifier, m.data_length_code);
    for (int i = 0; i < m.data_length_code; i++) Serial.printf("%02X ", m.data[i]);
    Serial.println();
}

static void handleSerialCommands() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == 's' || c == 'S') {
            snifferEnabled = !snifferEnabled;
            memset(snifferTable, 0, sizeof(snifferTable));
            Serial.printf("[SNIFF] Modo sniffer %s\n", snifferEnabled ? "ACTIVADO" : "DESACTIVADO");
        }
    }
}

// Pone a cero todo el estado "vivo" de la moto (contacto OFF o bus CAN en silencio)
static void resetLiveTelemetry(KtmTelemetry& t) {
    t.ignition_on = false;
    t.engine_running = false;
    t.engine_rpm = 0;
    t.speed_kmh = 0;
    t.high_beam_active = false;
    t.pass_trigger_held = false;
    t.turn_left_active = false;
    t.turn_right_active = false;
    t.hazard_active = false;
    t.turn_cancel_pressed = false;
    t.nav_up_pressed = false;
    t.nav_down_pressed = false;
    t.nav_enter_pressed = false;
    t.horn_active = false;
    t.front_brake_active = false;
    t.rear_brake_active = false;
}

void setupTwaiCanBus() {
    Serial.println("[CAN] Configurando periférico TWAI para KTM 1290 (500 kbps)...");

    // Configuración general: Modo LISTEN_ONLY para máxima seguridad (nunca inyecta tramas de error)
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        TWAI_TX_PIN, 
        TWAI_RX_PIN, 
        TWAI_MODE_LISTEN_ONLY
    );
    g_config.rx_queue_len = 64;

    // Velocidad estándar Euro 5: 500 kbit/s
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

    // Aceptar todas las tramas estándar (sin filtro restrictivo inicial)
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err == ESP_OK) {
        Serial.println("[CAN] Driver TWAI instalado correctamente.");
    } else {
        Serial.printf("[CAN] Error instalando driver TWAI: 0x%x\n", err);
        return;
    }

    err = twai_start();
    if (err == ESP_OK) {
        Serial.println("[CAN] Bus CAN en modo escucha pasiva activo a 500 kbps.");
    } else {
        Serial.printf("[CAN] Error iniciando TWAI: 0x%x\n", err);
    }
}

// Recuperación automática del controlador TWAI si entra en bus-off o se detiene
static void checkCanHealth() {
    twai_status_info_t st;
    if (twai_get_status_info(&st) != ESP_OK) return;
    if (st.state == TWAI_STATE_BUS_OFF) {
        Serial.println("[CAN] Bus-off detectado, iniciando recuperación...");
        twai_initiate_recovery();
    } else if (st.state == TWAI_STATE_STOPPED) {
        Serial.println("[CAN] Controlador detenido, reiniciando...");
        twai_start();
    }
}

static void setupWatchdog() {
    // Si el loop se bloquea > WDT_TIMEOUT_S el ESP32 se reinicia (los LEDC no se quedan con el último PWM)
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    esp_task_wdt_config_t wdt_cfg = {};
    wdt_cfg.timeout_ms = WDT_TIMEOUT_S * 1000;
    wdt_cfg.idle_core_mask = 0;
    wdt_cfg.trigger_panic = true;
    esp_task_wdt_reconfigure(&wdt_cfg);
    #else
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    #endif
    esp_task_wdt_add(NULL);
}

void setup() {
    // Salidas PWM a nivel BAJO lo antes posible (luces apagadas durante el arranque)
    pwmManager.preInitPinsLow();

    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=======================================================");
    Serial.println("  KTM 1290 CANSMART (LILYGO T-CAN485) - FIRMWARE v1.1  ");
    Serial.println("=======================================================");

    // 0. Habilitar alimentación del transceptor CAN en la placa LILYGO T-CAN485 (GPIO 16 HIGH)
    pinMode(PIN_5V_ENABLE, OUTPUT);
    digitalWrite(PIN_5V_ENABLE, HIGH);
    pinMode(PIN_CAN_SE, OUTPUT);
    digitalWrite(PIN_CAN_SE, LOW);
    delay(50);
    Serial.println("[HW] Regulador ME2107 (GPIO 16) activo -> Transceptor CAN energizado.");

    // 1. Cargar configuración persistente
    configStore.init();
    configStore.loadConfig(sysConfig);

    // 2. Inicializar salidas PWM MOSFET
    pwmManager.init();

    // 3. Inicializar máquina de estados de los mandos
    controllerFsm.init();

    // 4. Iniciar bus CAN
    setupTwaiCanBus();

    // 5. Iniciar punto de acceso WiFi y servidor Web
    webApiServer.init(sysConfig, bikeTelemetry, liveDimmer);

    // 6. Watchdog
    setupWatchdog();

    Serial.println("[INIT] Sistema listo y operando. Envía 's' para el modo sniffer CAN.\n");
}

void loop() {
    esp_task_wdt_reset();
    uint32_t now = millis();

    // 1. Lectura no bloqueante del bus CAN
    twai_message_t message;
    while (twai_receive(&message, 0) == ESP_OK) {
        // Las tramas extendidas (29 bits) y remotas no pertenecen al protocolo decodificado
        if (message.extd || message.rtr) continue;

        bikeTelemetry.last_can_ms = now;
        bikeTelemetry.can_seen = true;
        if (snifferEnabled) snifferHandle(message);

        // Decodificar tramas originales de la KTM 1290
        parse_ktm_can_frame(message.identifier, message.data, message.data_length_code, &bikeTelemetry);
    }

    // 2. Si el bus CAN calla (contacto OFF) todo vuelve a estado seguro: luces apagadas
    if (!bikeTelemetry.can_seen || (now - bikeTelemetry.last_can_ms) > CAN_SILENCE_TIMEOUT_MS) {
        resetLiveTelemetry(bikeTelemetry);
    }

    // 3. Salud del controlador CAN (cada segundo) y comandos por Serial
    if (now - lastCanStatusCheck >= 1000) {
        lastCanStatusCheck = now;
        checkCanHealth();
    }
    handleSerialCommands();

    // 4. Actualizar máquina de estados de botones (triple click, ráfagas 3s, dimmer)
    controllerFsm.update(sysConfig, bikeTelemetry, liveDimmer, needConfigSave);

    // 5. Guardar en NVS si hubo cambios que lo requieran
    if (needConfigSave) {
        configStore.saveConfig(sysConfig);
        needConfigSave = false;
    }

    // 6. Actualizar ciclos de trabajo PWM de las luces y accesorios
    pwmManager.update(sysConfig, bikeTelemetry, liveDimmer);

    // 7. Atender peticiones de la interfaz web
    webApiServer.handleClient();

    // 8. Heartbeat por Serial cada 2 segundos para depuración en banco
    if (now - lastTelemetryHeartbeat >= 2000) {
        lastTelemetryHeartbeat = now;
        Serial.printf("[Status] Ign:%d | RPM:%d | Larga:%d | Niebla:%s | Brillo:[%d%%, %d%%, %d%%, %d%%]\n",
            bikeTelemetry.ignition_on,
            bikeTelemetry.engine_rpm,
            bikeTelemetry.high_beam_active,
            sysConfig.light_set_2_enabled ? "ON" : "OFF",
            pwmManager.current_duty[0],
            pwmManager.current_duty[1],
            pwmManager.current_duty[2],
            pwmManager.current_duty[3]
        );
    }
}
