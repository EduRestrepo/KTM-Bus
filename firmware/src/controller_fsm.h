#pragma once
#include <Arduino.h>
#include "types.h"

class ControllerFsm {
public:
    void init();
    void update(SystemConfig& cfg, KtmTelemetry& telem, LiveDimmerState& dimmer, bool& need_config_save);

private:
    // Timers para detección de pulsaciones largas y clics múltiples
    uint32_t cancel_press_start_ms;
    bool cancel_prev_state;
    uint8_t cancel_click_count;
    uint32_t last_cancel_release_ms;

    uint32_t pass_press_start_ms;
    bool pass_prev_state;

    bool nav_up_prev_state;
    bool nav_down_prev_state;

    void handleCancelButton(SystemConfig& cfg, KtmTelemetry& telem, bool& need_save);
    void handlePassTrigger(SystemConfig& cfg, KtmTelemetry& telem, LiveDimmerState& dimmer);
    void handleDimmerAdjustment(SystemConfig& cfg, KtmTelemetry& telem, LiveDimmerState& dimmer, bool& need_save);
};

extern ControllerFsm controllerFsm;
