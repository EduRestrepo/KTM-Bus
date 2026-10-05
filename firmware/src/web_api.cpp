#include "web_api.h"
#include "pwm_manager.h"
#include "config_store.h"
#include <WiFi.h>
#include <ArduinoJson.h>

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
    // No se admite configuración con la moto en marcha
    if (p_telem->engine_running || p_telem->speed_kmh > 3.0f) {
        server.send(409, "application/json", "{\"error\":\"Moto en marcha\"}");
        return;
    }

    DynamicJsonDocument doc(3072);
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "application/json", "{\"error\":\"JSON invalido\"}");
        return;
    }

    // Se trabaja sobre una copia: solo se aplica si todo el documento es válido
    SystemConfig newCfg = *p_cfg;

    if (doc.containsKey("light_set_1_enabled")) newCfg.light_set_1_enabled = doc["light_set_1_enabled"].as<bool>();
    if (doc.containsKey("light_set_2_enabled")) newCfg.light_set_2_enabled = doc["light_set_2_enabled"].as<bool>();

    if (doc.containsKey("channels")) {
        JsonArray arr = doc["channels"].as<JsonArray>();
        if (arr.size() != 4) {
            server.send(400, "application/json", "{\"error\":\"Se esperan 4 canales\"}");
            return;
        }
        for (int i = 0; i < 4; i++) {
            JsonObject c = arr[i];
            ChannelSettings& ch = newCfg.channels[i];
            if (c.containsKey("function")) {
                int f = c["function"].as<int>();
                if (f < 0 || f > (int)FUNC_RIGHT_TURN) {
                    server.send(400, "application/json", "{\"error\":\"Funcion fuera de rango\"}");
                    return;
                }
                ch.function = (CircuitFunction)f;
            }
            if (c.containsKey("fuse_amps"))      ch.current_limit_amps = c["fuse_amps"].as<float>();
            if (c.containsKey("day_pct"))        ch.brightness_day = (uint8_t)constrain(c["day_pct"].as<int>(), 0, 100);
            if (c.containsKey("night_pct"))      ch.brightness_night = (uint8_t)constrain(c["night_pct"].as<int>(), 0, 100);
            if (c.containsKey("high_beam_pct"))  ch.brightness_high_beam = (uint8_t)constrain(c["high_beam_pct"].as<int>(), 0, 100);
            if (c.containsKey("off_with_turn"))  ch.off_with_turn_signal = c["off_with_turn"].as<bool>();
            if (c.containsKey("strobe_horn"))    ch.strobe_on_horn = c["strobe_horn"].as<bool>();
            if (c.containsKey("strobe_pass"))    ch.strobe_on_pass = c["strobe_pass"].as<bool>();
            if (c.containsKey("inverse_hazard")) ch.inverse_hazard = c["inverse_hazard"].as<bool>();
            if (c.containsKey("off_delay_sec"))  ch.off_delay_seconds = (uint8_t)constrain(c["off_delay_sec"].as<int>(), 0, 120);
        }
    }

    // Cambio opcional de credenciales WiFi (se aplican tras reiniciar)
    if (doc.containsKey("wifi_password")) {
        String pw = doc["wifi_password"].as<String>();
        if (pw.length() < 8 || pw.length() >= sizeof(newCfg.wifi_password)) {
            server.send(400, "application/json", "{\"error\":\"Contrasena WiFi 8-31 caracteres\"}");
            return;
        }
        strncpy(newCfg.wifi_password, pw.c_str(), sizeof(newCfg.wifi_password));
    }

    configStore.sanitize(newCfg);
    *p_cfg = newCfg;
    configStore.saveConfig(*p_cfg);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebApiServer::handlePostTest() {
    // Prueba de canales solo con la moto parada (el modo prueba caduca solo a los 5 s)
    if (p_telem->engine_running || p_telem->speed_kmh > 3.0f) {
        server.send(409, "application/json", "{\"error\":\"Moto en marcha\"}");
        return;
    }
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
    if (p_telem->engine_running || p_telem->speed_kmh > 3.0f) {
        server.send(409, "application/json", "{\"error\":\"Moto en marcha\"}");
        return;
    }
    configStore.setDefaults(*p_cfg);
    configStore.saveConfig(*p_cfg);
    server.send(200, "application/json", "{\"status\":\"reset_complete\"}");
}

void WebApiServer::handleClient() {
    server.handleClient();
}
