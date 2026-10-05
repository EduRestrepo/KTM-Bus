#include "config_store.h"

ConfigStore configStore;

void ConfigStore::init() {
    prefs.begin("ktm_bus", false);
}

void ConfigStore::setDefaults(SystemConfig& cfg) {
    // Configuración recomendada para KTM 1290 con luces principales + nieblas
    // Canal 0 (Blanco): Foco Izquierdo Set 1
    cfg.channels[0].function = FUNC_LEFT_LIGHT_1;
    cfg.channels[0].current_limit_amps = 10.0f;
    cfg.channels[0].brightness_day = 40;
    cfg.channels[0].brightness_night = 20;
    cfg.channels[0].brightness_high_beam = 100;
    cfg.channels[0].off_with_turn_signal = true;
    cfg.channels[0].strobe_on_horn = false;
    cfg.channels[0].strobe_on_pass = false;
    cfg.channels[0].inverse_hazard = true;
    cfg.channels[0].off_delay_seconds = 0;

    // Canal 1 (Amarillo): Foco Derecho Set 1
    cfg.channels[1].function = FUNC_RIGHT_LIGHT_1;
    cfg.channels[1].current_limit_amps = 10.0f;
    cfg.channels[1].brightness_day = 40;
    cfg.channels[1].brightness_night = 20;
    cfg.channels[1].brightness_high_beam = 100;
    cfg.channels[1].off_with_turn_signal = true;
    cfg.channels[1].strobe_on_horn = false;
    cfg.channels[1].strobe_on_pass = false;
    cfg.channels[1].inverse_hazard = true;
    cfg.channels[1].off_delay_seconds = 0;

    // Canal 2 (Azul): Focos de Niebla Set 2 (Light Pair 2)
    cfg.channels[2].function = FUNC_LIGHT_PAIR_2;
    cfg.channels[2].current_limit_amps = 10.0f;
    cfg.channels[2].brightness_day = 50;
    cfg.channels[2].brightness_night = 30;
    cfg.channels[2].brightness_high_beam = 100;
    cfg.channels[2].off_with_turn_signal = false;
    cfg.channels[2].strobe_on_horn = false;
    cfg.channels[2].strobe_on_pass = false;
    cfg.channels[2].inverse_hazard = false;
    cfg.channels[2].off_delay_seconds = 0;

    // Canal 3 (Rojo): luz de freno / posición auxiliar (conector 4)
    // Estroboscopios desactivados por defecto (suelen estar prohibidos en vía pública).
    cfg.channels[3].function = FUNC_BRAKE_LIGHT;
    cfg.channels[3].current_limit_amps = 10.0f;
    cfg.channels[3].brightness_day = 100;
    cfg.channels[3].brightness_night = 100;
    cfg.channels[3].brightness_high_beam = 100;
    cfg.channels[3].off_with_turn_signal = false;
    cfg.channels[3].strobe_on_horn = false;
    cfg.channels[3].strobe_on_pass = false;
    cfg.channels[3].inverse_hazard = false;
    cfg.channels[3].off_delay_seconds = 0;

    cfg.light_set_1_enabled = true;
    cfg.light_set_2_enabled = false; // Nieblas apagadas por defecto al arrancar
    cfg.set_1_custom_dim = 0;        // 0 indica usar perfil día/noche
    cfg.set_2_custom_dim = 0;

    strncpy(cfg.wifi_ssid, "KTM-CANSMART", sizeof(cfg.wifi_ssid));
    // Contraseña única por equipo (derivada de la MAC): "ktm-XXXXXX". Se imprime por Serial al arrancar.
    snprintf(cfg.wifi_password, sizeof(cfg.wifi_password), "ktm-%06X", (unsigned)((ESP.getEfuseMac() >> 24) & 0xFFFFFF));
}

// Valida/sanea la configuración leída de NVS o recibida por la web (rangos y cadenas terminadas en \0)
void ConfigStore::sanitize(SystemConfig& cfg) {
    for (int i = 0; i < 4; i++) {
        ChannelSettings& c = cfg.channels[i];
        if ((int)c.function < 0 || (int)c.function > (int)FUNC_RIGHT_TURN) c.function = FUNC_DISABLED;
        if (!(c.current_limit_amps >= 0.5f)) c.current_limit_amps = 0.5f;
        if (c.current_limit_amps > 10.0f) c.current_limit_amps = 10.0f;
        if (c.brightness_day > 100) c.brightness_day = 100;
        if (c.brightness_night > 100) c.brightness_night = 100;
        if (c.brightness_high_beam > 100) c.brightness_high_beam = 100;
        if (c.off_delay_seconds > 120) c.off_delay_seconds = 120;
    }
    if (cfg.set_1_custom_dim > 100) cfg.set_1_custom_dim = 100;
    if (cfg.set_2_custom_dim > 100) cfg.set_2_custom_dim = 100;
    cfg.wifi_ssid[sizeof(cfg.wifi_ssid) - 1] = '\0';
    cfg.wifi_password[sizeof(cfg.wifi_password) - 1] = '\0';
    // WPA2 exige >= 8 caracteres; si no, se restaura la contraseña por defecto
    if (strlen(cfg.wifi_password) < 8 || strlen(cfg.wifi_ssid) == 0) {
        strncpy(cfg.wifi_ssid, "KTM-CANSMART", sizeof(cfg.wifi_ssid));
        snprintf(cfg.wifi_password, sizeof(cfg.wifi_password), "ktm-%06X", (unsigned)((ESP.getEfuseMac() >> 24) & 0xFFFFFF));
    }
}

void ConfigStore::loadConfig(SystemConfig& cfg) {
    if (!prefs.isKey("initialized")) {
        setDefaults(cfg);
        saveConfig(cfg);
        return;
    }

    size_t read_bytes = prefs.getBytes("sys_cfg", &cfg, sizeof(SystemConfig));
    if (read_bytes != sizeof(SystemConfig)) {
        Serial.println("[Config] Tamaño inconsistente en NVS, reestableciendo valores de fábrica.");
        setDefaults(cfg);
        saveConfig(cfg);
    } else {
        sanitize(cfg);
        Serial.println("[Config] Configuración cargada correctamente desde memoria NVS.");
    }
}

void ConfigStore::saveConfig(const SystemConfig& cfg) {
    prefs.putBytes("sys_cfg", &cfg, sizeof(SystemConfig));
    prefs.putBool("initialized", true);
    Serial.println("[Config] Configuración guardada en NVS.");
}
