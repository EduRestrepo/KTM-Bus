#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "types.h"

/**
 * Identificadores CAN Bus estándar observados en KTM 1290 Super Adventure (2021-2024 Euro 5)
 * Velocidad de bus: 500 kbps (11-bit standard ID)
 * Modo de conexión: LISTEN_ONLY (Sólo escucha pasiva para 100% seguridad)
 *
 * ATENCIÓN: los IDs y máscaras de bits de este archivo son una HIPÓTESIS DE PARTIDA, NO están
 * verificados contra una captura real de la moto. Antes de usarlos hay que activar el modo sniffer
 * del firmware (enviar 's' por el monitor serie), pulsar cada mando de la moto y anotar qué ID,
 * byte y bit cambia. Después corregir las constantes de este archivo.
 */

#define KTM_CAN_SPEED_KBPS             500

// IDs de tramas CAN estándar KTM Euro 5
#define KTM_CAN_ID_WHEEL_SPEED         0x110  // Velocidad de rueda delantera y trasera
#define KTM_CAN_ID_ENGINE_DATA         0x120  // RPM motor, posición acelerador
#define KTM_CAN_ID_BRAKE_STATUS        0x140  // Sensores de maneta y pedal de freno
#define KTM_CAN_ID_HANDLEBAR_LEFT      0x240  // Ráfagas, largas, intermitentes, bocina
#define KTM_CAN_ID_HANDLEBAR_NAV       0x420  // Cruceta de navegación (+, -, Enter, Atrás)
#define KTM_CAN_ID_DASHBOARD_AMBIENT   0x500  // Sensor fotosensible TFT (Día / Noche)

// Máscaras de bits para la trama de la piña izquierda (KTM_CAN_ID_HANDLEBAR_LEFT - 0x240)
// Byte 0: Luces y ráfagas
#define KTM_MASK_HIGH_BEAM             (1 << 0)  // Luz larga fija
#define KTM_MASK_PASS_TRIGGER          (1 << 1)  // Gatillo de ráfagas presionado
#define KTM_MASK_HORN                  (1 << 2)  // Bocina / Claxon presionado

// Byte 1: Intermitentes
#define KTM_MASK_TURN_LEFT             (1 << 0)  // Intermitente izquierdo activo
#define KTM_MASK_TURN_RIGHT            (1 << 1)  // Intermitente derecho activo
#define KTM_MASK_TURN_CANCEL_BTN       (1 << 2)  // Pulsación del botón cancelar intermitente
#define KTM_MASK_HAZARDS               (1 << 3)  // Luces de emergencia 4 intermitentes

// Máscaras de bits para la cruceta de navegación (KTM_CAN_ID_HANDLEBAR_NAV - 0x420)
// Byte 0: Teclas de menú
#define KTM_MASK_NAV_UP                (1 << 0)  // Tecla '+' / Arriba
#define KTM_MASK_NAV_DOWN              (1 << 1)  // Tecla '-' / Abajo
#define KTM_MASK_NAV_ENTER             (1 << 2)  // Tecla 'Enter' / Centro
#define KTM_MASK_NAV_BACK              (1 << 3)  // Tecla 'Volver'

// Máscaras de bits para la trama de frenos (KTM_CAN_ID_BRAKE_STATUS - 0x140)
#define KTM_MASK_BRAKE_FRONT           (1 << 0)  // Interruptor de maneta de freno delantera
#define KTM_MASK_BRAKE_REAR            (1 << 1)  // Interruptor de pedal de freno trasero
#define KTM_MASK_ABS_INTERVENTION      (1 << 2)  // ABS actuando (frenada violenta)

/**
 * Función decodificadora de tramas entrantes
 */
static inline void parse_ktm_can_frame(uint32_t id, const uint8_t* data, uint8_t dlc, KtmTelemetry* telem) {
    if (!telem || !data || dlc == 0) return;

    switch (id) {
        case KTM_CAN_ID_WHEEL_SPEED:
            if (dlc >= 4) {
                // Rueda delantera en bytes 0-1 (formato big-endian o little-endian en 0.1 km/h)
                uint16_t raw_speed = (data[0] << 8) | data[1];
                telem->speed_kmh = raw_speed * 0.1f;
            }
            break;

        case KTM_CAN_ID_ENGINE_DATA:
            if (dlc >= 3) {
                // RPM motor en bytes 0-1
                telem->engine_rpm = ((data[0] << 8) | data[1]) / 4;
                telem->engine_running = (telem->engine_rpm > 400);
                telem->ignition_on = true;
            }
            break;

        case KTM_CAN_ID_HANDLEBAR_LEFT:
            if (dlc >= 2) {
                // Byte 0: Ráfagas, Larga, Bocina
                telem->high_beam_active = (data[0] & KTM_MASK_HIGH_BEAM) != 0;
                telem->pass_trigger_held = (data[0] & KTM_MASK_PASS_TRIGGER) != 0;
                telem->horn_active = (data[0] & KTM_MASK_HORN) != 0;

                // Byte 1: Intermitentes
                telem->turn_left_active = (data[1] & KTM_MASK_TURN_LEFT) != 0;
                telem->turn_right_active = (data[1] & KTM_MASK_TURN_RIGHT) != 0;
                telem->turn_cancel_pressed = (data[1] & KTM_MASK_TURN_CANCEL_BTN) != 0;
                telem->hazard_active = (data[1] & KTM_MASK_HAZARDS) != 0 || 
                                       (telem->turn_left_active && telem->turn_right_active);
            }
            break;

        case KTM_CAN_ID_HANDLEBAR_NAV:
            if (dlc >= 1) {
                telem->nav_up_pressed = (data[0] & KTM_MASK_NAV_UP) != 0;
                telem->nav_down_pressed = (data[0] & KTM_MASK_NAV_DOWN) != 0;
                telem->nav_enter_pressed = (data[0] & KTM_MASK_NAV_ENTER) != 0;
            }
            break;

        case KTM_CAN_ID_BRAKE_STATUS:
            if (dlc >= 1) {
                telem->front_brake_active = (data[0] & KTM_MASK_BRAKE_FRONT) != 0;
                telem->rear_brake_active = (data[0] & KTM_MASK_BRAKE_REAR) != 0;
            }
            break;

        case KTM_CAN_ID_DASHBOARD_AMBIENT:
            if (dlc >= 1) {
                // Byte 0 suele indicar 0 = Modo Día (fondo blanco TFT), 1 = Modo Noche (fondo negro TFT)
                telem->tft_night_mode = (data[0] & 0x01) != 0;
            }
            break;

        default:
            // Trama no procesada
            break;
    }
}
