// MedTens — Firmware ESP01 "sensor-server" v0.1.0
//
// Papel: ponte HTTP somente-leitura. O ESP01 escuta a UART do Arduino UNO
// (TX do UNO  →  RX/GPIO3 do ESP01, 9600 bps) e expõe a última leitura via
// HTTP na rede local. NÃO escreve no próprio Serial: isso deixa o UNO como
// dono exclusivo do /dev/ttyACM0 para o medtens-gadget.
//
// Linhas esperadas do UNO (produzidas por medtens/src/main.cpp):
//   "Tensao AC: 127.43 V\r\n"
//   "QUEDA NA REDE\r\n"
//
// Endpoints:
//   GET /      → página HTML auto-refresh com última leitura
//   GET /json  → {"v_rms": 127.43, "outage": false, "age_ms": 812, "fw": "0.1.0"}

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ── Credenciais (mesma rede do controle-rele para facilitar teste) ──
const char *ssid     = "CABANA 8883";
const char *password = "eusouamor88";

// ── Estado do sensor ────────────────────────────────────────────────
static float    g_vrms       = 0.0f;   // última tensão RMS válida
static bool     g_outage     = false;  // último status foi QUEDA NA REDE
static bool     g_has_reading = false; // já recebeu alguma leitura?
static uint32_t g_last_ms    = 0;      // millis() da última leitura

// Buffer de linha vindo da UART
static char     g_line[64];
static uint8_t  g_line_len = 0;

ESP8266WebServer server(80);

// ── Template HTML em PROGMEM ────────────────────────────────────────
static const char PAGE_TPL[] PROGMEM =
  "<!DOCTYPE html>"
  "<html><head><meta charset=\"UTF-8\">"
  "<meta http-equiv=\"refresh\" content=\"2\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>MedTens Sensor</title>"
  "<style>"
  "body{font-family:Arial,sans-serif;display:flex;flex-direction:column;"
  "align-items:center;justify-content:center;min-height:100vh;margin:0;"
  "background:#1a1a2e;color:#fff}"
  "h1{margin:0 0 24px;font-size:22px;opacity:.8}"
  ".v{font-size:64px;font-weight:700;margin:8px 0}"
  ".u{font-size:24px;opacity:.7;margin-left:8px}"
  ".ok{color:#4caf50}.bad{color:#f44336}.wait{color:#ffb74d}"
  ".s{font-size:18px;margin-top:12px}"
  ".meta{margin-top:32px;font-size:12px;opacity:.5}"
  "</style></head><body>"
  "<h1>MedTens &mdash; Monitor ZMPT101B</h1>"
  "<div class=\"v %s\">%s<span class=\"u\">%s</span></div>"
  "<div class=\"s\">%s</div>"
  "<div class=\"meta\">fw %s &middot; última leitura há %lu ms</div>"
  "</body></html>";

static const char JSON_TPL[] PROGMEM =
  "{\"v_rms\":%s,\"outage\":%s,\"has_reading\":%s,\"age_ms\":%lu,\"fw\":\"%s\"}";

// ── Parse de uma linha recebida do UNO ──────────────────────────────
static void parse_line(const char *line) {
  // "QUEDA NA REDE"
  if (strstr(line, "QUEDA NA REDE") != nullptr) {
    g_outage      = true;
    g_has_reading = true;
    g_last_ms     = millis();
    return;
  }
  // "Tensao AC: 127.43 V"
  const char *p = strstr(line, "Tensao AC:");
  if (p != nullptr) {
    p += 10; // pula "Tensao AC:"
    while (*p == ' ') p++;
    float v = strtod(p, nullptr);
    if (v > 0.0f) {
      g_vrms        = v;
      g_outage      = false;
      g_has_reading = true;
      g_last_ms     = millis();
    }
  }
}

// ── Lê UART sem bloquear, quebra em linhas, chama parse_line ────────
static void pump_serial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      g_line[g_line_len] = '\0';
      if (g_line_len > 0) parse_line(g_line);
      g_line_len = 0;
    } else if (g_line_len < sizeof(g_line) - 1) {
      g_line[g_line_len++] = c;
    } else {
      // overflow: descarta linha
      g_line_len = 0;
    }
  }
}

// ── Handlers HTTP ───────────────────────────────────────────────────
void handleRoot() {
  char vbuf[16];
  if (g_has_reading && !g_outage) {
    dtostrf(g_vrms, 0, 2, vbuf);
  } else {
    strcpy(vbuf, "--");
  }
  const char *klass =
    !g_has_reading ? "wait" :
    g_outage       ? "bad"  : "ok";
  const char *unit = (g_has_reading && !g_outage) ? "V" : "";
  const char *msg =
    !g_has_reading ? "aguardando dados do UNO..." :
    g_outage       ? "QUEDA NA REDE"              : "rede OK";

  uint32_t age = g_has_reading ? (millis() - g_last_ms) : 0;

  char buf[1024];
  snprintf_P(buf, sizeof(buf), PAGE_TPL,
             klass, vbuf, unit, msg, FW_VERSION, (unsigned long)age);
  server.send(200, F("text/html"), buf);
}

void handleJson() {
  char vbuf[16];
  dtostrf(g_vrms, 0, 2, vbuf);
  uint32_t age = g_has_reading ? (millis() - g_last_ms) : 0;

  char buf[192];
  snprintf_P(buf, sizeof(buf), JSON_TPL,
             g_has_reading ? vbuf : "null",
             g_outage      ? "true" : "false",
             g_has_reading ? "true" : "false",
             (unsigned long)age,
             FW_VERSION);
  server.send(200, F("application/json"), buf);
}

void setup() {
  // IMPORTANTE: o ESP01 usa a mesma UART para RX do UNO e para log.
  // Aqui operamos em modo RX-only @ 9600 para casar com o UNO. Não
  // imprimimos nada para não injetar ruído no /dev/ttyACM0 (que é
  // somente TX do UNO — o ESP01 só escuta).
  Serial.begin(9600);
  delay(10);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 30000) {
    delay(250);
  }

  server.on(F("/"),     handleRoot);
  server.on(F("/json"), handleJson);
  server.begin();
}

void loop() {
  pump_serial();
  server.handleClient();
}
