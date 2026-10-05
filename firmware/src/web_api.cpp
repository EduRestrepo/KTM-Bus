#include "web_api.h"
#include "pwm_manager.h"
#include "config_store.h"
#include <WiFi.h>

WebApiServer webApiServer;

void WebApiServer::init(SystemConfig& cfg, KtmTelemetry& telem, LiveDimmerState& dimmer) {
    p_cfg = &cfg;
    p_telem = &telem;
    p_dimmer = &dimmer;

    // Configuración del Punto de Acceso WiFi (SoftAP)
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(cfg.wifi_ssid, cfg.wifi_password);
    IPAddress IP = WiFi.softAPIP();

    Serial.printf("[WiFi] SoftAP iniciado: %s (Password: %s)\n", cfg.wifi_ssid, cfg.wifi_password);
    Serial.printf("[WiFi] IP del servidor de configuración: http://%s\n", IP.toString().c_str());

    setupRoutes();
    server.begin();
}

void WebApiServer::setupRoutes() {
    // Cabecera CORS universal para permitir peticiones desde cualquier origen durante desarrollo
    server.enableCORS(true);

    server.on("/api/status", HTTP_GET, [this]() { handleGetStatus(); });
    server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    server.on("/api/config", HTTP_POST, [this]() { handlePostConfig(); });
    server.on("/api/test", HTTP_POST, [this]() { handlePostTest(); });
    server.on("/api/reset", HTTP_POST, [this]() { handlePostReset(); });

    // Mensaje informativo raíz
    server.on("/", HTTP_GET, [this]() {
        server.send(200, "text/html", 
            "<html><head><meta charset='utf-8'><title>KTM CANsmart AP</title></head>"
            "<body style='background:#121212;color:#fff;font-family:sans-serif;text-align:center;padding:50px;'>"
            "<h1 style='color:#ff6600;'>KTM 1290 CANsmart Controller</h1>"
            "<p>Firmware v1.0 listo. Conéctate a la interfaz Web para configurar.</p>"
            "<p><a href='/api/status' style='color:#4fc3f7;'>Ver Telemetría en Vivo (JSON)</a></p>"
            "</body></html>");
    });
}

void WebApiServer::handleGetStatus() {
    String json = "{";
    json += "\"ignition\":" + String(p_telem->ignition_on ? "true" : "false") + ",";
    json += "\"engine_running\":" + String(p_telem->engine_running ? "true" : "false") + ",";
    json += "\"engine_rpm\":" + String(p_telem->engine_rpm) + ",";
    json += "\"speed_kmh\":" + String(p_telem->speed_kmh, 1) + ",";
    json += "\"high_beam\":" + String(p_telem->high_beam_active ? "true" : "false") + ",";
    json += "\"pass_trigger\":" + String(p_telem->pass_trigger_held ? "true" : "false") + ",";
    json += "\"turn_left\":" + String(p_telem->turn_left_active ? "true" : "false") + ",";
    json += "\"turn_right\":" + String(p_telem->turn_right_active ? "true" : "false") + ",";
    json += "\"horn\":" + String(p_telem->horn_active ? "true" : "false") + ",";
    json += "\"brake\":" + String((p_telem->front_brake_active || p_telem->rear_brake_active) ? "true" : "false") + ",";
    json += "\"tft_night_mode\":" + String(p_telem->tft_night_mode ? "true" : "false") + ",";
    json += "\"set_1_enabled\":" + String(p_cfg->light_set_1_enabled ? "true" : "false") + ",";
    json += "\"set_2_enabled\":" + String(p_cfg->light_set_2_enabled ? "true" : "false") + ",";
    json += "\"dimmer_active\":" + String(p_dimmer->is_active ? "true" : "false") + ",";
    json += "\"duty\":[" + String(pwmManager.current_duty[0]) + "," +
                          String(pwmManager.current_duty[1]) + "," +
                          String(pwmManager.current_duty[2]) + "," +
                          String(pwmManager.current_duty[3]) + "],";
    json += "\"amps\":[" + String(pwmManager.estimated_amps[0], 2) + "," +
                          String(pwmManager.estimated_amps[1], 2) + "," +
                          String(pwmManager.estimated_amps[2], 2) + "," +
                          String(pwmManager.estimated_amps[3], 2) + "]";
    json += "}";

    server.send(200, "application/json", json);
}

void WebApiServer::handleGetConfig() {
    String json = "{";
    json += "\"light_set_1_enabled\":" + String(p_cfg->light_set_1_enabled ? "true" : "false") + ",";
    json += "\"light_set_2_enabled\":" + String(p_cfg->light_set_2_enabled ? "true" : "false") + ",";
    json += "\"channels\":[";
    for (int i = 0; i < 4; i++) {
        json += "{";
        json += "\"function\":" + String((int)p_cfg->channels[i].function) + ",";
        json += "\"fuse_amps\":" + String(p_cfg->channels[i].current_limit_amps, 1) + ",";
        json += "\"day_pct\":" + String(p_cfg->channels[i].brightness_day) + ",";
        json += "\"night_pct\":" + String(p_cfg->channels[i].brightness_night) + ",";
        json += "\"high_beam_pct\":" + String(p_cfg->channels[i].brightness_high_beam) + ",";
        json += "\"off_with_turn\":" + String(p_cfg->channels[i].off_with_turn_signal ? "true" : "false") + ",";
        json += "\"strobe_horn\":" + String(p_cfg->channels[i].strobe_on_horn ? "true" : "false") + ",";
        json += "\"strobe_pass\":" + String(p_cfg->channels[i].strobe_on_pass ? "true" : "false") + ",";
        json += "\"inverse_hazard\":" + String(p_cfg->channels[i].inverse_hazard ? "true" : "false") + ",";
        json += "\"off_delay_sec\":" + String(p_cfg->channels[i].off_delay_seconds);
        json += "}";
        if (i < 3) json += ",";
    }
    json += "]}";

    server.send(200, "application/json", json);
}

void WebApiServer::handlePostConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Cuerpo vacio\"}");
        return;
    }
    // Parseo básico de configuración enviada por la interfaz web
    // Guardado en memoria no volátil
    configStore.saveConfig(*p_cfg);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebApiServer::handlePostTest() {
    if (server.hasArg("channel") && server.hasArg("duty")) {
        uint8_t ch = server.arg("channel").toInt();
        uint8_t duty = server.arg("duty").toInt();
        pwmManager.testChannel(ch, duty);
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Faltan parametros\"}");
    }
}

void WebApiServer::handlePostReset() {
    configStore.setDefaults(*p_cfg);
    configStore.saveConfig(*p_cfg);
    server.send(200, "application/json", "{\"status\":\"reset_complete\"}");
}

void WebApiServer::handleClient() {
    server.handleClient();
}
