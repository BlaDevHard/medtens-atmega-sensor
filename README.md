# medtens — Sensor ATmega (ZMPT101B)

Firmware para Arduino Uno (ATmega328P) que mede tensão AC RMS através do
módulo ZMPT101B e transmite as medições via UART para um gateway ESP01.

Este repositório é **um** dos componentes do sistema **MedTens** de monitoramento
da qualidade do fornecimento de energia elétrica. Os demais:

| Componente | Repositório | Função |
|---|---|---|
| Sensor (este repo) | `medtens-atmega-sensor` | Amostragem ADC, cálculo RMS, emissão UART |
| Gateway WiFi | `medtens-esp01-gateway` | Recebe UART, valida CRC, publica MQTT |
| Coletor | `medtens-esp32-collector` | Assina MQTT, calcula índices de qualidade, grava no MySQL |
| Servidor | `medtens-server` | Banco MySQL + API/consulta de histórico |
| Documentação geral | `medtens-docs` | Arquitetura, protocolo UART, bibliografia, caderno de lab |

## Estado atual

`v0.1.2` — cálculo RMS funcional, saída em texto ASCII via UART 9600 8N1;
`calibration_factor` recalibrado contra multímetro Aneng; limiar de
detecção de queda elevado para 30 V.

A evolução para frame binário com CRC16 está planejada para `v0.2.0`
(ver [CHANGELOG.md](CHANGELOG.md) e issues).

## Hardware

- Placa: Arduino Uno (ATmega328P @ 16 MHz, 5 V)
- Sensor: ZMPT101B (transformador de tensão + amp. operacional) em A0
- UART TX (pino 1) → (via divisor resistivo 5 V→3,3 V) → RX do ESP01

## Build

Requer [PlatformIO Core](https://platformio.org/install/cli).

```bash
pio run                    # compila
pio run -t upload          # grava via /dev/ttyUSB0
pio device monitor -b 9600 # monitora saída
```

## Parâmetros relevantes (src/main.cpp)

- `calibration_factor = 0.8642` — ajuste empírico por instância do ZMPT101B
  (recalibrado em 2026-04-19 contra multímetro Aneng: 121.9 V vs. 119.9 V
  lidos com fator 0.85)
- `OFFSET_ADC = 512` — ponto médio do ADC de 10 bits com Vref = AVcc
- Janela de amostragem: 3000 leituras × ~10 µs ≈ 2 ciclos de 60 Hz

## Licença

A definir.

## Citação

Placeholder — `CITATION.cff` será adicionado quando a autoria for
formalizada.
