#include "web_server.h"

#include <stdio.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "led_control.h"
#include "rpm_sensor.h"
#define HISTORY_SIZE 30

static float rpm_history[HISTORY_SIZE];
static int history_index = 0;

static const char *TAG = "web_server";
static httpd_handle_t s_server;

static const char INDEX_HTML[] =
"<!DOCTYPE html><html lang='es'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>ESP32-C6 Dashboard</title>"

"<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"

"<style>"

"*{margin:0;padding:0;box-sizing:border-box;font-family:Arial,sans-serif}"

"body{"
"background:#0f172a;"
"color:white;"
"padding:20px;"
"}"

"h1{"
"font-size:32px;"
"margin-bottom:20px;"
"font-weight:700;"
"}"

".dashboard{"
"display:grid;"
"grid-template-columns:repeat(auto-fit,minmax(300px,1fr));"
"gap:20px;"
"}"

".card{"
"background:#1e293b;"
"border-radius:20px;"
"padding:24px;"
"box-shadow:0 4px 20px rgba(0,0,0,0.3);"
"}"

".rpm-value{"
"font-size:72px;"
"font-weight:bold;"
"color:#22c55e;"
"margin-top:10px;"
"}"

".subtitle{"
"color:#94a3b8;"
"margin-top:10px;"
"}"

".buttons{"
"display:flex;"
"gap:15px;"
"margin-top:20px;"
"}"

"button{"
"border:none;"
"padding:14px 22px;"
"border-radius:12px;"
"font-size:16px;"
"cursor:pointer;"
"font-weight:bold;"
"transition:0.2s;"
"}"

".on{"
"background:#22c55e;"
"color:white;"
"}"

".off{"
"background:#ef4444;"
"color:white;"
"}"

"button:hover{"
"transform:scale(1.05);"
"}"

"canvas{"
"margin-top:20px;"
"}"

"</style>"
"</head>"

"<body>"

"<h1>ESP32-C6 RPM Dashboard</h1>"

"<div class='dashboard'>"

"<div class='card'>"
"<h2>RPM ACTUALES</h2>"
"<div class='rpm-value' id='rpm'>0</div>"
"<div class='subtitle'>Actualización en tiempo real</div>"
"</div>"

"<div class='card'>"
"<h2>CONTROL LED</h2>"

"<div class='buttons'>"
"<button class='on' onclick=\"setLed('on')\">ENCENDER</button>"
"<button class='off' onclick=\"setLed('off')\">APAGAR</button>"
"</div>"

"<div class='subtitle' id='ledStatus'>Sistema listo</div>"
"</div>"

"<div class='card' style='grid-column:1/-1;'>"
"<h2>HISTÓRICO RPM</h2>"
"<canvas id='chart'></canvas>"
"</div>"

"</div>"

"<script>"

"const ctx=document.getElementById('chart').getContext('2d');"

"const chart=new Chart(ctx,{"
"type:'line',"
"data:{"
"labels:Array(30).fill(''),"
"datasets:[{"
"label:'RPM',"
"data:[],"
"borderColor:'#22c55e',"
"backgroundColor:'rgba(34,197,94,0.2)',"
"fill:true,"
"tension:0.3"
"}]"
"},"
"options:{"
"responsive:true,"
"plugins:{legend:{labels:{color:'white'}}},"
"scales:{"
"x:{ticks:{color:'white'}},"
"y:{ticks:{color:'white'}}"
"}"
"}"
"});"

"async function updateRpm(){"

"try{"

"const r=await fetch('/rpm');"
"const d=await r.json();"

"document.getElementById('rpm').textContent=d.rpm.toFixed(1);"

"const h=await fetch('/history');"
"const hd=await h.json();"

"chart.data.datasets[0].data=hd.values;"
"chart.update();"

"}catch(e){"
"console.log(e);"
"}"
"}"

"async function setLed(state){"

"try{"

"const r=await fetch('/led?state='+state);"
"const d=await r.json();"

"document.getElementById('ledStatus').textContent="
"'LED '+(d.led?'ENCENDIDO':'APAGADO');"

"}catch(e){"
"console.log(e);"
"}"
"}"

"updateRpm();"
"setInterval(updateRpm,1000);"

"</script>"

"</body>"
"</html>";

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t rpm_handler(httpd_req_t *req)
{
    char response[64];
    float rpm = rpm_sensor_get_rpm();

    rpm_history[history_index] = rpm;
    history_index = (history_index + 1) % HISTORY_SIZE;

    snprintf(response, sizeof(response), "{\"rpm\":%.2f}", rpm);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, response);
}

static esp_err_t history_handler(httpd_req_t *req)
{
    char response[512];
    int offset = 0;

    offset += snprintf(response + offset,
                       sizeof(response) - offset,
                       "{\"values\":[");

    for (int i = 0; i < HISTORY_SIZE; i++) {

        int idx = (history_index + i) % HISTORY_SIZE;

        offset += snprintf(response + offset,
                           sizeof(response) - offset,
                           "%.2f%s",
                           rpm_history[idx],
                           (i < HISTORY_SIZE - 1) ? "," : "");
    }

    offset += snprintf(response + offset,
                       sizeof(response) - offset,
                       "]}");

    httpd_resp_set_type(req, "application/json");

    return httpd_resp_sendstr(req, response);
}

static esp_err_t led_handler(httpd_req_t *req)
{
    char query[32];
    char state[8];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "state", state, sizeof(state)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Use /led?state=on or /led?state=off");
        return ESP_FAIL;
    }

    if (strcmp(state, "on") == 0) {
        ESP_ERROR_CHECK(led_control_set(true));
    } else if (strcmp(state, "off") == 0) {
        ESP_ERROR_CHECK(led_control_set(false));
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid LED state");
        return ESP_FAIL;
    }

    char response[32];
    snprintf(response, sizeof(response), "{\"led\":%s}", led_control_get() ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, response);
}

esp_err_t web_server_start(void)
{
    if (s_server != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server start failed: %s", esp_err_to_name(err));
        return err;
    }

    const httpd_uri_t uris[] = {
        {.uri = "/", .method = HTTP_GET, .handler = index_handler},
        {.uri = "/rpm", .method = HTTP_GET, .handler = rpm_handler},
        {.uri = "/led", .method = HTTP_GET, .handler = led_handler},
        {.uri = "/history", .method = HTTP_GET, .handler = history_handler},
    };

    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &uris[i]));
    }

    ESP_LOGI(TAG, "HTTP server listening on port %u", config.server_port);
    return ESP_OK;
}

esp_err_t web_server_stop(void)
{
    if (s_server == NULL) {
        return ESP_OK;
    }

    esp_err_t err = httpd_stop(s_server);
    if (err == ESP_OK) {
        s_server = NULL;
    }
    return err;
}
