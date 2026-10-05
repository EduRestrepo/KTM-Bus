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

class PwmManager {
public:
    void init();
    void update(const SystemConfig& cfg, const KtmTelemetry& telem, const LiveDimmerState& dimmer);
    void testChannel(uint8_t channel_idx, uint8_t duty_percent);

    // Valores calculados actuales (para telemetría web)
    uint8_t current_duty[4];
    float estimated_amps[4];

private:
    uint8_t target_duty[4];
    uint32_t last_strobe_toggle_ms;
    bool strobe_phase;

    uint8_t calculateChannelDuty(uint8_t ch_idx, const ChannelSettings& ch_cfg, 
                                 const SystemConfig& sys_cfg, const KtmTelemetry& telem, 
                                 const LiveDimmerState& dimmer);
};

extern PwmManager pwmManager;
