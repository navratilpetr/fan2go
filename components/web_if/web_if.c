#include <stdio.h>
#include <string.h>

#include "esp_http_server.h"

#include "config.h"
#include "fan.h"
#include "web_if.h"

static httpd_handle_t s_server = NULL;

static const char *INDEX_HTML =
"<!DOCTYPE html>"
"<html><head><meta charset='utf-8'/>"
"<title>fan2go</title>"
"<style>"
"body{font-family:sans-serif;padding:1rem;}"
"table{border-collapse:collapse;}"
"td,th{border:1px solid #ccc;padding:0.25rem 0.5rem;}"
"input{width:3rem;}"
"</style>"
"</head><body>"
"<h1>fan2go</h1>"
"<table id='tbl'>"
"<thead><tr><th>Fan</th><th>Connected</th><th>RPM</th><th>Duty</th><th>Set</th></tr></thead>"
"<tbody></tbody>"
"</table>"
"<script>"
"async function loadStatus(){"
"  const r=await fetch('/api/status');"
"  const j=await r.json();"
"  const tb=document.querySelector('#tbl tbody');"
"  tb.innerHTML='';"
"  j.fans.forEach(f=>{"
"    const tr=document.createElement('tr');"
"    tr.innerHTML="
"      `<td>${f.idx}</td>`+"
"      `<td>${f.connected?'yes':'no'}</td>`+"
"      `<td>${f.rpm}</td>`+"
"      `<td>${f.duty}</td>`+"
"      `<td><input type='number' min='0' max='100' value='${f.duty}' "
"onchange='setDuty(${f.idx},this.value)'></td>`;"
"    tb.appendChild(tr);"
"  });"
"}"
"async function setDuty(idx,val){"
"  await fetch(`/api/set?fan=${idx}&duty=${val}`);"
"  setTimeout(loadStatus,500);"
"}"
"loadStatus();"
"setInterval(loadStatus,5000);"
"</script>"
"</body></html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    char buf[512];
    int len = 0;

    len += snprintf(buf + len, sizeof(buf) - len, "{\"fans\":[");
    for (int i = 0; i < CONFIG_MAX_FANS; ++i) {
        if (i > 0) {
            len += snprintf(buf + len, sizeof(buf) - len, ",");
        }
        len += snprintf(buf + len, sizeof(buf) - len,
                        "{\"idx\":%d,\"connected\":%d,\"rpm\":%" PRIu32 ",\"duty\":%d}",
                        i,
                        fan_is_connected(i) ? 1 : 0,
                        fan_get_rpm(i),
                        fan_get_duty(i));
    }
    len += snprintf(buf + len, sizeof(buf) - len, "]}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, len);
    return ESP_OK;
}

static esp_err_t set_get_handler(httpd_req_t *req)
{
    char qbuf[128];
    if (httpd_req_get_url_query_str(req, qbuf, sizeof(qbuf)) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char fan_str[8];
    char duty_str[8];
    int idx = -1;
    int duty = 0;

    if (httpd_query_key_value(qbuf, "fan", fan_str, sizeof(fan_str)) == ESP_OK) {
        idx = atoi(fan_str);
    }
    if (httpd_query_key_value(qbuf, "duty", duty_str, sizeof(duty_str)) == ESP_OK) {
        duty = atoi(duty_str);
    }

    if (idx < 0 || idx >= CONFIG_MAX_FANS) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid fan index");
        return ESP_FAIL;
    }

    fan_set_duty_percent(fan_get_ptr(idx), duty);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

void web_init(void)
{
    if (s_server != NULL) {
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    if (httpd_start(&s_server, &config) != ESP_OK) {
        printf("WEB: httpd_start failed\n");
        s_server = NULL;
        return;
    }

    httpd_uri_t root_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = root_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &root_uri);

    httpd_uri_t status_uri = {
        .uri      = "/api/status",
        .method   = HTTP_GET,
        .handler  = status_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &status_uri);

    httpd_uri_t set_uri = {
        .uri      = "/api/set",
        .method   = HTTP_GET,
        .handler  = set_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &set_uri);

    printf("WEB: server started on port %d\n", config.server_port);
}

