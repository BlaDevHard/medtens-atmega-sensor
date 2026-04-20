# MedTens — trans uno_esp (firmware ESP01 sensor-server)

Firmware de teste para o ESP-01 atuar como **servidor HTTP somente** que
publica na rede local as leituras do ZMPT101B medidas pelo Arduino UNO.

Versão atual: **v0.1.0** (ver `CHANGELOG.md`).

## Arquitetura

```
┌──────────────┐   UART TX (9600 bps)   ┌───────────────┐   HTTP (Wi-Fi)
│ Arduino UNO  │ ───────────────────▶   │   ESP-01      │ ───────────────▶ Browser
│ ZMPT101B A0  │                        │  sensor-server│                  (192.168.1.x:80)
│ src/main.cpp │                        │  v0.1.0       │
└──────┬───────┘                        └───────┬───────┘
       │ USB (ttyACM0 @ 9600)                   │
       ▼                                        │
  medtens-gadget ◀────────────────── (leitura pelo cabo continua igual)
```

O UNO permanece com o firmware atual (`Arduino/medtens/src/main.cpp`, C puro
para ATmega). Suas linhas serial seguem dois caminhos:

1. **USB → PC**: `/dev/ttyACM0` → `medtens-gadget` (inalterado).
2. **UART TX → ESP-01 RX**: o ESP-01 escuta passivamente e serve via HTTP.

## Ligação física

| UNO            | ESP-01       | Observação                                 |
|----------------|--------------|--------------------------------------------|
| TX0 (pino 1)   | RX (GPIO3)   | via divisor 5V→3.3V (1kΩ + 2kΩ ou nível)   |
| GND            | GND          | comum                                      |
| —              | VCC 3.3V     | fonte externa 3.3V (ESP-01 puxa ~250 mA)   |

> **Não** conectar RX do UNO ao TX do ESP-01. Neste firmware o ESP-01 é
> silencioso no serial para não conflitar com o `ttyACM0`.

## Build e upload

```bash
cd "Arduino/medtens/trans uno_esp"
~/.platformio/penv/bin/pio run                 # compila
~/.platformio/penv/bin/pio run -t upload       # grava (ESP-01 em modo flash: GPIO0=GND no boot)
```

O `platformio.ini` está fixado em `/dev/ttyUSB0` para upload (adaptador
USB-TTL do gravador de ESP-01).

## Endpoints

- `GET /` — HTML com auto-refresh de 2 s (tensão RMS, status QUEDA, idade da leitura).
- `GET /json` — `{"v_rms":127.43,"outage":false,"has_reading":true,"age_ms":812,"fw":"0.1.0"}`.

## Testes rápidos

```bash
# descobrir IP: no roteador ou pelo broadcast ARP
curl http://192.168.1.19/json

# enquanto isso, gadget pelo cabo continua rodando:
medtens-gadget --port /dev/ttyACM0 --baud 9600
```

## O que este firmware NÃO faz (v0.1.0)

- Não publica MQTT (fica para a etapa E3/E4 da integração MedTens).
- Não calcula métricas — só repassa a última linha válida recebida do UNO.
- Não persiste em flash — reboot zera o estado.
