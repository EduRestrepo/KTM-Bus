#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "types.h"

class WebApiServer {
public:
    void init(SystemConfig& cfg, KtmTelemetry& telem, LiveDimmerState& dimmer);
    void handleClient();

private:
    WebServer server{80};
    SystemConfig* p_cfg;
    KtmTelemetry* p_telem;
    LiveDimmerState* p_dimmer;

    void setupRoutes();
    void handleGetStatus();
    void handleGetConfig();
    void handlePostConfig();
    void handlePostTest();
    void handlePostReset();
};

extern WebApiServer webApiServer;
