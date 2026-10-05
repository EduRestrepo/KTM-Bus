#include <Arduino.h>
#include "driver/twai.h"
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

SystemConfig sysConfig;
KtmTelemetry bikeTelemetry;
LiveDimmerState liveDimmer;

static bool needConfigSave = false;
static uint32_t lastTelemetryHeartbeat = 0;

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

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=======================================================");
    Serial.println("  KTM 1290 CANSMART (LILYGO T-CAN485) - FIRMWARE v1.0  ");
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

    Serial.println("[INIT] Sistema listo y operando.\n");
}

void loop() {
    // 1. Lectura no bloqueante del bus CAN
    twai_message_t message;
    while (twai_receive(&message, 0) == ESP_OK) {
        if (!(message.rtr)) {
            // Decodificar tramas originales de la KTM 1290
            parse_ktm_can_frame(message.identifier, message.data, message.data_length_code, &bikeTelemetry);
        }
    }

    // 2. Actualizar máquina de estados de botones (triple click, ráfagas 3s, dimmer)
    controllerFsm.update(sysConfig, bikeTelemetry, liveDimmer, needConfigSave);

    // 3. Guardar en NVS si hubo cambios que lo requieran
    if (needConfigSave) {
        configStore.saveConfig(sysConfig);
        needConfigSave = false;
    }

    // 4. Actualizar ciclos de trabajo PWM de las luces y accesorios
    pwmManager.update(sysConfig, bikeTelemetry, liveDimmer);

    // 5. Atender peticiones de la interfaz web
    webApiServer.handleClient();

    // 6. Heartbeat por Serial cada 2 segundos para depuración en banco
    uint32_t now = millis();
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
