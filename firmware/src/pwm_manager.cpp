#include "pwm_manager.h"

PwmManager pwmManager;

static const uint8_t channel_pins[4] = {
    PIN_CHANNEL_WHITE,
    PIN_CHANNEL_YELLOW,
    PIN_CHANNEL_BLUE,
    PIN_CHANNEL_RED
};

void PwmManager::init() {
    last_strobe_toggle_ms = 0;
    strobe_phase = false;

    for (int i = 0; i < 4; i++) {
        current_duty[i] = 0;
        target_duty[i] = 0;
        estimated_amps[i] = 0.0f;

        pinMode(channel_pins[i], OUTPUT);
        digitalWrite(channel_pins[i], LOW);

        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcAttach(channel_pins[i], PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
        #else
        ledcSetup(i, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
        ledcAttachPin(channel_pins[i], i);
        #endif
    }
}

void PwmManager::testChannel(uint8_t channel_idx, uint8_t duty_percent) {
    if (channel_idx >= 4) return;
    uint8_t duty_val = map(constrain(duty_percent, 0, 100), 0, 100, 0, 255);
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(channel_pins[channel_idx], duty_val);
    #else
    ledcWrite(channel_idx, duty_val);
    #endif
    current_duty[channel_idx] = duty_percent;
}

uint8_t PwmManager::calculateChannelDuty(uint8_t ch_idx, const ChannelSettings& ch_cfg, 
                                        const SystemConfig& sys_cfg, const KtmTelemetry& telem, 
                                        const LiveDimmerState& dimmer) {
    if (ch_cfg.function == FUNC_DISABLED) return 0;

    // Si la moto tiene el contacto apagado y no hay retardo, todo a 0
    if (!telem.ignition_on) {
        return 0;
    }

    bool is_set_1 = (ch_cfg.function == FUNC_LEFT_LIGHT_1 || 
                     ch_cfg.function == FUNC_RIGHT_LIGHT_1 || 
                     ch_cfg.function == FUNC_LIGHT_PAIR_1);

    bool is_set_2 = (ch_cfg.function == FUNC_LEFT_LIGHT_2 || 
                     ch_cfg.function == FUNC_RIGHT_LIGHT_2 || 
                     ch_cfg.function == FUNC_LIGHT_PAIR_2);

    // 1. Manejo de bocina (HORN)
    if (ch_cfg.function == FUNC_HORN) {
        return telem.horn_active ? 100 : 0;
    }

    // 2. Manejo de luz de freno (BRAKE LIGHT)
    if (ch_cfg.function == FUNC_BRAKE_LIGHT) {
        if (telem.front_brake_active || telem.rear_brake_active) {
            return 100;
        }
        return ch_cfg.brightness_night; // Luz de posición trasera
    }

    // 3. Manejo de accesorios permanentes/conmutados
    if (ch_cfg.function == FUNC_ACCESSORY || ch_cfg.function == FUNC_HEATED_GEAR) {
        return 100;
    }

    // 4. Si el grupo de luces está apagado desde la piña
    if (is_set_1 && !sys_cfg.light_set_1_enabled) return 0;
    if (is_set_2 && !sys_cfg.light_set_2_enabled) return 0;

    // 5. Estroboscópico de alerta con bocina (Strobe on Horn)
    if (telem.horn_active && ch_cfg.strobe_on_horn) {
        // Estrobo alternado entre izquierda y derecha
        if (ch_cfg.function == FUNC_LEFT_LIGHT_1 || ch_cfg.function == FUNC_LEFT_LIGHT_2) {
            return strobe_phase ? 100 : 0;
        } else {
            return strobe_phase ? 0 : 100;
        }
    }

    // 6. Ráfagas de ráfaga estroboscópica (Strobe on Flash)
    if (telem.pass_trigger_held && ch_cfg.strobe_on_pass) {
        return strobe_phase ? 100 : 20;
    }

    // 7. Apagado de foco al poner intermitente (Turn signal cutoff)
    if (ch_cfg.off_with_turn_signal) {
        if ((ch_cfg.function == FUNC_LEFT_LIGHT_1 || ch_cfg.function == FUNC_LEFT_LIGHT_2) && telem.turn_left_active) {
            return 0;
        }
        if ((ch_cfg.function == FUNC_RIGHT_LIGHT_1 || ch_cfg.function == FUNC_RIGHT_LIGHT_2) && telem.turn_right_active) {
            return 0;
        }
    }

    // 8. Luz larga activa (High Beam) -> 100% de potencia
    if (telem.high_beam_active) {
        return ch_cfg.brightness_high_beam;
    }

    // 9. Nivel de brillo base (Modo Día vs Modo Noche del TFT de KTM)
    uint8_t base_percent = telem.tft_night_mode ? ch_cfg.brightness_night : ch_cfg.brightness_day;

    // 10. Si el usuario ajustó el dimmer en vivo desde la piña
    if (is_set_1 && sys_cfg.set_1_custom_dim > 0) {
        base_percent = sys_cfg.set_1_custom_dim;
    } else if (is_set_2 && sys_cfg.set_2_custom_dim > 0) {
        base_percent = sys_cfg.set_2_custom_dim;
    }

    return base_percent;
}

void PwmManager::update(const SystemConfig& cfg, const KtmTelemetry& telem, const LiveDimmerState& dimmer) {
    // Oscilador de estroboscópico a 12 Hz (~42 ms por semiperiodo)
    uint32_t now = millis();
    if (now - last_strobe_toggle_ms >= 42) {
        last_strobe_toggle_ms = now;
        strobe_phase = !strobe_phase;
    }

    for (int i = 0; i < 4; i++) {
        uint8_t target_percent = calculateChannelDuty(i, cfg.channels[i], cfg, telem, dimmer);
        target_duty[i] = target_percent;

        // Rampa suave de transición (Slew rate limiter) para encendido progresivo
        // Si hay estroboscópico o bocina, transición instantánea
        if (telem.horn_active || (telem.pass_trigger_held && cfg.channels[i].strobe_on_pass)) {
            current_duty[i] = target_percent;
        } else {
            if (current_duty[i] < target_percent) {
                current_duty[i] = (uint8_t)min((int)current_duty[i] + 5, (int)target_percent);
            } else if (current_duty[i] > target_percent) {
                current_duty[i] = (uint8_t)max((int)current_duty[i] - 5, (int)target_percent);
            }
        }

        // Estimación de amperaje basada en el límite programado y % actual
        estimated_amps[i] = (current_duty[i] / 100.0f) * cfg.channels[i].current_limit_amps;

        // Escribir valor PWM al hardware (0 - 255)
        uint8_t duty_val = map(current_duty[i], 0, 100, 0, 255);
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(channel_pins[i], duty_val);
        #else
        ledcWrite(i, duty_val);
        #endif
    }
}
