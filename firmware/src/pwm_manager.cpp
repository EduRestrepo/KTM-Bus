#include "pwm_manager.h"

PwmManager pwmManager;

static const uint8_t channel_pins[4] = {
    PIN_CHANNEL_WHITE,
    PIN_CHANNEL_YELLOW,
    PIN_CHANNEL_BLUE,
    PIN_CHANNEL_RED
};

void PwmManager::preInitPinsLow() {
    for (int i = 0; i < 4; i++) {
        pinMode(channel_pins[i], OUTPUT);
        digitalWrite(channel_pins[i], LOW);
    }
}

void PwmManager::init() {
    last_strobe_toggle_ms = 0;
    strobe_phase = false;
    last_hazard_toggle_ms = 0;
    hazard_phase = false;
    last_ramp_ms = 0;

    for (int i = 0; i < 4; i++) {
        current_duty[i] = 0;
        target_duty[i] = 0;
        estimated_amps[i] = 0.0f;
        test_until_ms[i] = 0;
        test_duty[i] = 0;

        pinMode(channel_pins[i], OUTPUT);
        digitalWrite(channel_pins[i], LOW);

        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcAttach(channel_pins[i], PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
        #else
        ledcSetup(i, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
        ledcAttachPin(channel_pins[i], i);
        #endif
        writePwm(i, 0);
    }
}

void PwmManager::writePwm(uint8_t ch_idx, uint8_t percent) {
    uint8_t duty_val = map(constrain(percent, 0, 100), 0, 100, 0, 255);
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(channel_pins[ch_idx], duty_val);
    #else
    ledcWrite(ch_idx, duty_val);
    #endif
}

void PwmManager::testChannel(uint8_t channel_idx, uint8_t duty_percent) {
    if (channel_idx >= 4) return;
    test_duty[channel_idx] = (uint8_t)constrain(duty_percent, 0, 100);
    // millis() | 1 evita que 0 (= sin prueba) coincida con un instante válido
    test_until_ms[channel_idx] = (millis() + TEST_MODE_TIMEOUT_MS) | 1;
}

uint8_t PwmManager::calculateChannelDuty(uint8_t ch_idx, const ChannelSettings& ch_cfg, 
                                        const SystemConfig& sys_cfg, const KtmTelemetry& telem, 
                                        const LiveDimmerState& dimmer) {
    if (ch_cfg.function == FUNC_DISABLED) return 0;

    // Contacto apagado (o bus CAN en silencio): todo a 0
    if (!telem.ignition_on) {
        return 0;
    }

    bool is_set_1 = (ch_cfg.function == FUNC_LEFT_LIGHT_1 || 
                     ch_cfg.function == FUNC_RIGHT_LIGHT_1 || 
                     ch_cfg.function == FUNC_LIGHT_PAIR_1);

    bool is_set_2 = (ch_cfg.function == FUNC_LEFT_LIGHT_2 || 
                     ch_cfg.function == FUNC_RIGHT_LIGHT_2 || 
                     ch_cfg.function == FUNC_LIGHT_PAIR_2);

    bool is_left  = (ch_cfg.function == FUNC_LEFT_LIGHT_1 || ch_cfg.function == FUNC_LEFT_LIGHT_2);
    bool is_right = (ch_cfg.function == FUNC_RIGHT_LIGHT_1 || ch_cfg.function == FUNC_RIGHT_LIGHT_2);

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

    // 3. Manejo de accesorios permanentes/conmutados (solo con contacto)
    if (ch_cfg.function == FUNC_ACCESSORY || ch_cfg.function == FUNC_HEATED_GEAR) {
        return 100;
    }

    // Funciones reservadas (FUNC_LEFT_TURN / FUNC_RIGHT_TURN): sin implementar, canal apagado
    if (ch_cfg.function == FUNC_LEFT_TURN || ch_cfg.function == FUNC_RIGHT_TURN) {
        return 0;
    }

    // 4. Si el grupo de luces está apagado desde la piña
    if (is_set_1 && !sys_cfg.light_set_1_enabled) return 0;
    if (is_set_2 && !sys_cfg.light_set_2_enabled) return 0;

    // 5. Destello alterno con luces de emergencia (inverse hazard)
    if (telem.hazard_active && ch_cfg.inverse_hazard && (is_left || is_right)) {
        if (is_left) return hazard_phase ? 100 : 0;
        return hazard_phase ? 0 : 100;
    }

    // 6. Estroboscópico de alerta con bocina (Strobe on Horn)
    if (telem.horn_active && ch_cfg.strobe_on_horn) {
        // Estrobo alternado entre izquierda y derecha
        if (is_left) {
            return strobe_phase ? 100 : 0;
        } else {
            return strobe_phase ? 0 : 100;
        }
    }

    // 7. Ráfagas estroboscópicas (Strobe on Flash)
    if (telem.pass_trigger_held && ch_cfg.strobe_on_pass) {
        return strobe_phase ? 100 : 20;
    }

    // 8. Apagado de foco al poner intermitente (Turn signal cutoff)
    if (ch_cfg.off_with_turn_signal) {
        if (is_left && telem.turn_left_active) {
            return 0;
        }
        if (is_right && telem.turn_right_active) {
            return 0;
        }
    }

    // 9. Luz larga activa (High Beam)
    if (telem.high_beam_active) {
        return ch_cfg.brightness_high_beam;
    }

    // 10. Nivel de brillo base (Modo Día vs Modo Noche del TFT de KTM)
    uint8_t base_percent = telem.tft_night_mode ? ch_cfg.brightness_night : ch_cfg.brightness_day;

    // 11. Si el usuario ajustó el dimmer en vivo desde la piña
    if (is_set_1 && sys_cfg.set_1_custom_dim > 0) {
        base_percent = sys_cfg.set_1_custom_dim;
    } else if (is_set_2 && sys_cfg.set_2_custom_dim > 0) {
        base_percent = sys_cfg.set_2_custom_dim;
    }

    return base_percent;
}

void PwmManager::update(const SystemConfig& cfg, const KtmTelemetry& telem, const LiveDimmerState& dimmer) {
    uint32_t now = millis();

    // Oscilador de estroboscópico a 12 Hz (~42 ms por semiperiodo)
    if (now - last_strobe_toggle_ms >= 42) {
        last_strobe_toggle_ms = now;
        strobe_phase = !strobe_phase;
    }

    // Oscilador del destello de emergencia (1 Hz)
    if (now - last_hazard_toggle_ms >= HAZARD_HALF_PERIOD_MS) {
        last_hazard_toggle_ms = now;
        hazard_phase = !hazard_phase;
    }

    // La rampa avanza solo cada PWM_RAMP_INTERVAL_MS (independiente de la velocidad del loop)
    bool ramp_tick = false;
    if (now - last_ramp_ms >= PWM_RAMP_INTERVAL_MS) {
        last_ramp_ms = now;
        ramp_tick = true;
    }

    for (int i = 0; i < 4; i++) {
        // Modo prueba desde la web: tiene prioridad hasta que expira
        if (test_until_ms[i] != 0) {
            if ((int32_t)(now - test_until_ms[i]) < 0) {
                current_duty[i] = test_duty[i];
                target_duty[i] = test_duty[i];
                estimated_amps[i] = (current_duty[i] / 100.0f) * cfg.channels[i].current_limit_amps;
                writePwm(i, current_duty[i]);
                continue;
            }
            test_until_ms[i] = 0;
        }

        uint8_t target_percent = calculateChannelDuty(i, cfg.channels[i], cfg, telem, dimmer);
        target_duty[i] = target_percent;

        // Rampa suave (slew rate limiter). Estrobo, bocina y apagado inmediato: sin rampa
        bool instant = telem.horn_active ||
                       (telem.pass_trigger_held && cfg.channels[i].strobe_on_pass) ||
                       (telem.hazard_active && cfg.channels[i].inverse_hazard) ||
                       !telem.ignition_on;
        if (instant) {
            current_duty[i] = target_percent;
        } else if (ramp_tick) {
            if (current_duty[i] < target_percent) {
                current_duty[i] = (uint8_t)min((int)current_duty[i] + PWM_RAMP_STEP_PCT, (int)target_percent);
            } else if (current_duty[i] > target_percent) {
                current_duty[i] = (uint8_t)max((int)current_duty[i] - PWM_RAMP_STEP_PCT, (int)target_percent);
            }
        }

        // Estimación de amperaje basada en el límite programado y % actual (NO es una medida real)
        estimated_amps[i] = (current_duty[i] / 100.0f) * cfg.channels[i].current_limit_amps;

        writePwm(i, current_duty[i]);
    }
}
