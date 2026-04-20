# Changelog

Todas as mudanças notáveis deste projeto são documentadas aqui.

Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/);
versionamento segue [SemVer](https://semver.org/lang/pt-BR/).

## [Unreleased]

### Planejado
- Protocolo UART binário com SOF + LEN + TYPE + PAYLOAD + CRC16-CCITT
- Tipo `MEAS_V1` contendo `seq`, `vrms`, `flags`
- Extensibilidade para novos `TYPE` (THD, frequência, flicker) sem quebrar parsers antigos

## [0.1.2] - 2026-04-19

### Corrigido
- Limiar de detecção de queda: 10 V → 30 V. Com a entrada do ZMPT101B
  desconectada da rede, o sensor capta ruído/acoplamento e o RMS
  calculado fica entre 10 e 14 V, nunca cruzando o antigo limiar de
  10 V. Uma rede real nunca cai abaixo de 30 V (limite crítico PRODIST
  em 127 V nominal fica em torno de 109 V), então o novo limiar
  distingue com segurança "rede ausente" de "rede com subtensão".

## [0.1.1] - 2026-04-19

### Alterado
- `calibration_factor`: 0.85 → 0.8642. Recalibração empírica contra
  multímetro Aneng (referência 121.9 V, leitura do gadget 119.9 V com
  fator anterior). Novo fator = 0.85 × (121.9 / 119.9).

## [0.1.0] - 2026-04-17

### Adicionado
- Cálculo de tensão RMS a partir do ZMPT101B em A0 (3000 amostras, ~2 ciclos de 60 Hz)
- Saída em texto ASCII via UART: `Tensao AC: XX.XX V` ou `QUEDA NA REDE`
- Leitura ADC com prescaler 128 e referência AVcc

### Corrigido
- Inclusão de `<stdlib.h>` para disponibilizar `dtostrf`
- Guard `#ifndef F_CPU` para evitar redefinição quando PlatformIO injeta `-DF_CPU`
