#pragma once
#include <Arduino.h>
#include "types.h"

// Definición de pines físicos por canal para LILYGO T-CAN485 (Header de expansión)
#define PIN_CHANNEL_WHITE     25  // Canal 1: Foco Izquierdo Set 1 (Pin Header GPIO 25)
#define PIN_CHANNEL_YELLOW    32  // Canal 2: Foco Derecho Set 1   (Pin Header GPIO 32)
#define PIN_CHANNEL_BLUE      33  // Canal 3: Focos de Niebla Set 2 (Pin Header GPIO 33)
#define PIN_CHANNEL_RED       18  // Canal 4: Bocina / Accesorio   (Pin Header GPIO 18)

#define PWM_FREQ_HZ           1000 // 1 kHz para evitar zumbido en bobinas/LEDs
#define PWM_RESOLUTION_BITS   8    // 8 bits (0 a 255)

#define PWM_RAMP_STEP_PCT     5    // Paso de la rampa suave (%)
#define PWM_RAMP_INTERVAL_MS  10   // Intervalo entre pasos de la rampa (5 % cada 10 ms = 0 a 100 % en 200 ms)
#define HAZARD_HALF_PERIOD_MS 500  // Semiperiodo del destello alterno de emergencia (1 Hz)
#define TEST_MODE_TIMEOUT_MS  5000 // Duración del modo de prueba de canal desde la web

class PwmManager {
public:
    // Fuerza los pines de salida a LOW lo antes posible (luces apagadas durante el arranque)
    void preInitPinsLow();
    void init();
    void update(const SystemConfig& cfg, const KtmTelemetry& telem, const LiveDimmerState& dimmer);

    // Fuerza un canal a un % durante TEST_MODE_TIMEOUT_MS (update() lo respeta y luego recupera el control)
    void testChannel(uint8_t channel_idx, uint8_t duty_percent);

    // Valores calculados actuales (para telemetría web)
    uint8_t current_duty[4];
    float estimated_amps[4];

private:
    uint8_t target_duty[4];
    uint32_t last_strobe_toggle_ms;
    bool strobe_phase;
    uint32_t last_hazard_toggle_ms;
    bool hazard_phase;
    uint32_t last_ramp_ms;

    // Modo prueba (por canal)
    uint32_t test_until_ms[4];
    uint8_t test_duty[4];

    void writePwm(uint8_t ch_idx, uint8_t percent);
    uint8_t calculateChannelDuty(uint8_t ch_idx, const ChannelSettings& ch_cfg, 
                                 const SystemConfig& sys_cfg, const KtmTelemetry& telem, 
                                 const LiveDimmerState& dimmer);
};

extern PwmManager pwmManager;
