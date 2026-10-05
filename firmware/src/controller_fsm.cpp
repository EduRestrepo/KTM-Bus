#include "controller_fsm.h"

ControllerFsm controllerFsm;

void ControllerFsm::init() {
    cancel_press_start_ms = 0;
    cancel_prev_state = false;
    cancel_click_count = 0;
    last_cancel_release_ms = 0;

    pass_press_start_ms = 0;
    pass_prev_state = false;

    nav_up_prev_state = false;
    nav_down_prev_state = false;
}

void ControllerFsm::handleCancelButton(SystemConfig& cfg, KtmTelemetry& telem, bool& need_save) {
    uint32_t now = millis();
    bool pressed = telem.turn_cancel_pressed;

    // Flanco de subida (se presiona el botón cancelar intermitente)
    if (pressed && !cancel_prev_state) {
        cancel_press_start_ms = now;
    }

    // Botón sostenido continuamente: Detección de HOLD 3 SECONDS -> ON/OFF LIGHT SET 1
    if (pressed && (now - cancel_press_start_ms >= 3000) && cancel_press_start_ms != 0) {
        cfg.light_set_1_enabled = !cfg.light_set_1_enabled;
        need_save = true;
        cancel_press_start_ms = 0; // Evitar disparos repetidos
        cancel_click_count = 0;
        Serial.printf("[FSM] Light Set 1 Conmutado -> %s\n", cfg.light_set_1_enabled ? "ON" : "OFF");
    }

    // Flanco de bajada (se suelta el botón)
    if (!pressed && cancel_prev_state) {
        uint32_t press_duration = now - cancel_press_start_ms;
        if (press_duration < 1000 && cancel_press_start_ms != 0) {
            cancel_click_count++;
            last_cancel_release_ms = now;

            // Detección de TRIPLE CLIC -> ON/OFF LIGHT SET 2 (Luces de niebla)
            if (cancel_click_count >= 3) {
                cfg.light_set_2_enabled = !cfg.light_set_2_enabled;
                need_save = true;
                cancel_click_count = 0;
                Serial.printf("[FSM] Light Set 2 (Nieblas) Conmutado -> %s\n", cfg.light_set_2_enabled ? "ON" : "OFF");
            }
        }
        cancel_press_start_ms = 0;
    }

    // Timeout de clic múltiple (si pasa más de 600ms sin otro clic, se resetea la cuenta)
    if (cancel_click_count > 0 && (now - last_cancel_release_ms > 600)) {
        cancel_click_count = 0;
    }

    cancel_prev_state = pressed;
}

void ControllerFsm::handlePassTrigger(SystemConfig& cfg, KtmTelemetry& telem, LiveDimmerState& dimmer) {
    uint32_t now = millis();
    bool pressed = telem.pass_trigger_held;

    if (pressed && !pass_prev_state) {
        pass_press_start_ms = now;
    }

    // Si se mantiene presionado el gatillo durante 3 segundos
    if (pressed && (now - pass_press_start_ms >= 3000) && pass_press_start_ms != 0) {
        // Si el gatillo se empujó hacia arriba o si la luz larga está activa -> SET 1
        // Si fue una pulsación hacia abajo -> SET 2
        dimmer.is_active = true;
        dimmer.expires_at_ms = now + 5000; // 5 segundos de ventana de ajuste

        if (telem.high_beam_active) {
            dimmer.target = DIM_TARGET_SET_1;
            Serial.println("[FSM] Entrado en Modo DIMMER para Set 1 (Luces Principales)");
        } else {
            dimmer.target = DIM_TARGET_SET_2;
            Serial.println("[FSM] Entrado en Modo DIMMER para Set 2 (Luces de Niebla)");
        }

        pass_press_start_ms = 0; // Evitar reentrada
    }

    if (!pressed) {
        pass_press_start_ms = 0;
    }

    pass_prev_state = pressed;
}

void ControllerFsm::handleDimmerAdjustment(SystemConfig& cfg, KtmTelemetry& telem, LiveDimmerState& dimmer, bool& need_save) {
    // Los flancos de la cruceta se siguen siempre, para no disparar una pulsación "fantasma"
    // si el modo dimmer se activa con la tecla ya pulsada
    bool up_edge = telem.nav_up_pressed && !nav_up_prev_state;
    bool down_edge = telem.nav_down_pressed && !nav_down_prev_state;
    nav_up_prev_state = telem.nav_up_pressed;
    nav_down_prev_state = telem.nav_down_pressed;

    if (!dimmer.is_active) return;

    uint32_t now = millis();

    // Comprobar si expiró la ventana de inactividad de 5 segundos
    if (now >= dimmer.expires_at_ms) {
        dimmer.is_active = false;
        dimmer.target = DIM_TARGET_NONE;
        need_save = true;
        Serial.println("[FSM] Modo DIMMER finalizado y guardado.");
        return;
    }

    // Tecla '+' de la cruceta (Aumentar brillo en 10%)
    if (up_edge) {
        dimmer.expires_at_ms = now + 5000; // Reiniciar temporizador
        if (dimmer.target == DIM_TARGET_SET_1) {
            cfg.set_1_custom_dim = (uint8_t)min((int)cfg.set_1_custom_dim + 10, 100);
            Serial.printf("[FSM] Set 1 Brillo -> %d%%\n", cfg.set_1_custom_dim);
        } else if (dimmer.target == DIM_TARGET_SET_2) {
            cfg.set_2_custom_dim = (uint8_t)min((int)cfg.set_2_custom_dim + 10, 100);
            Serial.printf("[FSM] Set 2 Brillo -> %d%%\n", cfg.set_2_custom_dim);
        }
    }

    // Tecla '-' de la cruceta (Disminuir brillo en 10%)
    if (down_edge) {
        dimmer.expires_at_ms = now + 5000; // Reiniciar temporizador
        if (dimmer.target == DIM_TARGET_SET_1) {
            cfg.set_1_custom_dim = (uint8_t)max((int)cfg.set_1_custom_dim - 10, 10);
            Serial.printf("[FSM] Set 1 Brillo -> %d%%\n", cfg.set_1_custom_dim);
        } else if (dimmer.target == DIM_TARGET_SET_2) {
            cfg.set_2_custom_dim = (uint8_t)max((int)cfg.set_2_custom_dim - 10, 10);
            Serial.printf("[FSM] Set 2 Brillo -> %d%%\n", cfg.set_2_custom_dim);
        }
    }
}

void ControllerFsm::update(SystemConfig& cfg, KtmTelemetry& telem, LiveDimmerState& dimmer, bool& need_config_save) {
    handleCancelButton(cfg, telem, need_config_save);
    handlePassTrigger(cfg, telem, dimmer);
    handleDimmerAdjustment(cfg, telem, dimmer, need_config_save);
}
