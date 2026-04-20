# Changelog

Todos os lançamentos relevantes deste firmware serão documentados neste arquivo.

O formato segue [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/)
e o projeto adere a [SemVer](https://semver.org/lang/pt-BR/).

## [0.1.0] - 2026-04-19

### Adicionado
- Firmware inicial do ESP01 no papel **sensor-server** (somente leitura).
- Escuta UART em RX=GPIO3 a 9600 bps, parseia linhas produzidas pelo Arduino
  UNO (`Tensao AC: X V` e `QUEDA NA REDE`) e mantém última leitura em RAM.
- Servidor HTTP em `192.168.1.x:80`:
  - `GET /` — página HTML com auto-refresh de 2 s.
  - `GET /json` — payload JSON `{v_rms, outage, has_reading, age_ms, fw}`.
- `platformio.ini` configurado para `board = esp01_1m`, `framework = arduino`,
  `monitor_speed = 9600`, LD script `eagle.flash.512k64.ld`.

### Notas de compatibilidade
- O ESP01 **não escreve** no próprio TX. Isso preserva o `/dev/ttyACM0` do
  Arduino UNO como canal exclusivo para o `medtens-gadget` continuar lendo
  pelo cabo USB sem contenção.
- Projeto `esp8266-controle-rele` (v1.2.0) permanece intacto — este é um
  firmware independente em `Arduino/medtens/trans uno_esp/`.
