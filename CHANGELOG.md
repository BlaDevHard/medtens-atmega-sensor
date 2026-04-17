# Changelog

Todas as mudanças notáveis deste projeto são documentadas aqui.

Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/);
versionamento segue [SemVer](https://semver.org/lang/pt-BR/).

## [Unreleased]

### Planejado
- Protocolo UART binário com SOF + LEN + TYPE + PAYLOAD + CRC16-CCITT
- Tipo `MEAS_V1` contendo `seq`, `vrms`, `flags`
- Extensibilidade para novos `TYPE` (THD, frequência, flicker) sem quebrar parsers antigos

## [0.1.0] - 2026-04-17

### Adicionado
- Cálculo de tensão RMS a partir do ZMPT101B em A0 (3000 amostras, ~2 ciclos de 60 Hz)
- Saída em texto ASCII via UART: `Tensao AC: XX.XX V` ou `QUEDA NA REDE`
- Leitura ADC com prescaler 128 e referência AVcc

### Corrigido
- Inclusão de `<stdlib.h>` para disponibilizar `dtostrf`
- Guard `#ifndef F_CPU` para evitar redefinição quando PlatformIO injeta `-DF_CPU`
