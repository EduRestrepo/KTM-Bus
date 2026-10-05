#pragma once
#include <stdint.h>
#include <stdbool.h>

// Definición de las funciones disponibles por canal (idéntico a la matriz DENALI / HEX)
enum CircuitFunction {
    FUNC_DISABLED = 0,
    FUNC_LEFT_LIGHT_1,
    FUNC_RIGHT_LIGHT_1,
    FUNC_LIGHT_PAIR_1,
    FUNC_LEFT_LIGHT_2,
    FUNC_RIGHT_LIGHT_2,
    FUNC_LIGHT_PAIR_2,
    FUNC_HORN,
    FUNC_BRAKE_LIGHT,
    FUNC_ACCESSORY,
    FUNC_HEATED_GEAR,
    FUNC_LEFT_TURN,
    FUNC_RIGHT_TURN
};

// Estructura de configuración por cada uno de los 4 canales de potencia
struct ChannelSettings {
    CircuitFunction function;
    float current_limit_amps;      // Fusible electrónico (1.0A a 10.0A)
    uint8_t brightness_day;        // 0% a 100% con luz de cruce diurna
    uint8_t brightness_night;      // 0% a 100% con luz de cruce nocturna
    uint8_t brightness_high_beam;  // 0% a 100% con luz larga activada
    bool off_with_turn_signal;     // Apagar foco del lado del intermitente activo
    bool strobe_on_horn;           // Destello estroboscópico de alerta al sonar bocina
    bool strobe_on_pass;           // Ráfaga estroboscópica al presionar ráfagas
    bool inverse_hazard;           // Destello alterno con luces de emergencia
    uint8_t off_delay_seconds;     // Retardo de apagado tras quitar contacto (0 a 120s)
};

// Configuración global del sistema persistida en NVS
struct SystemConfig {
    ChannelSettings channels[4];   // 0: Blanco, 1: Amarillo, 2: Azul, 3: Rojo
    bool light_set_1_enabled;      // Estado ON/OFF del Juego 1
    bool light_set_2_enabled;      // Estado ON/OFF del Juego 2 (nieblas)
    uint8_t set_1_custom_dim;      // Ajuste en vivo desde la piña (%)
    uint8_t set_2_custom_dim;      // Ajuste en vivo desde la piña (%)
    char wifi_ssid[32];
    char wifi_password[32];
};

// Estado en tiempo real decodificado del Bus CAN de la KTM 1290
struct KtmTelemetry {
    bool ignition_on;
    bool engine_running;
    uint16_t engine_rpm;
    float speed_kmh;
    float battery_voltage;
    
    // Piña izquierda y mandos
    bool high_beam_active;
    bool pass_trigger_held;
    uint32_t pass_trigger_duration_ms;
    
    bool turn_left_active;
    bool turn_right_active;
    bool hazard_active;
    
    bool turn_cancel_pressed;
    uint8_t cancel_click_count;
    uint32_t last_cancel_click_ms;
    
    // Cruceta de navegación (+ / -)
    bool nav_up_pressed;
    bool nav_down_pressed;
    bool nav_enter_pressed;
    
    // Frenos y bocina
    bool horn_active;
    bool front_brake_active;
    bool rear_brake_active;
    
    // Sensor de iluminación ambiental del cuadro TFT
    bool tft_night_mode;
};

// Estado de la máquina de estados del modo regulación rápida (On-The-Fly Dimming)
enum LiveDimmerTarget {
    DIM_TARGET_NONE = 0,
    DIM_TARGET_SET_1,
    DIM_TARGET_SET_2
};

struct LiveDimmerState {
    bool is_active;
    LiveDimmerTarget target;
    uint32_t expires_at_ms;
};
