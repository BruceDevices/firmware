# Roadmap de Migracao SubGHz (Bruce <- Unleashed)

Data de atualizacao: 2026-05-23  
Branch de trabalho: `subghz_improvements`

## Objetivo

Integrar a stack `subghz` do Unleashed no Bruce sem alterar vendor, mantendo compatibilidade com fluxos legados (`rf,subghz`) e evoluindo identificacao de protocolo, analise de chave/counter e TX protocol-native.

## Status Geral

- [x] Base arquitetural da migracao concluida (vendor + compat + bridge + adapter).
- [x] Menu principal `SubGHz` avancado integrado e funcional.
- [x] RX avancado (scan/decode/analyze/recent/filter/profile) funcional.
- [x] TX avancado por `.sub` funcional com fallback seguro para legado.
- [~] Paridade de todas as cenas/fluxos do Unleashed em andamento.
- [ ] Fechamento formal da v1 (testes de hardware completos + docs finais).
- [x] Gap analysis baseado na documentacao oficial do Unleashed (`SUBGHZ_UNLEASHED_DOCS_GAP_ANALYSIS.md`).

## Implementado

### 1) Vendor, Compat e Bridge

- [x] Snapshot vendorizado em `lib/subghz_unleashed_vendor/`.
- [x] Politica de vendor imutavel aplicada (adaptacoes fora da pasta vendor).
- [x] Camada compat em `lib/subghz_unleashed_compat/` para subset de `furi`, `furi_hal`, `FuriString`, `FlipperFormat`, `level_duration` e keystore.
- [x] Bridge de build em `lib/subghz_unleashed_bridge/` com `environment`, `receiver`, `transmitter` e protocolos usados pelos perfis.
- [x] Suporte a novos protocolos trazidos do upstream atual: `allstar_firefly`, `keyfinder`, `nord_ice`, ajustes em `keeloq` e correlatos.

### 2) Modulo SubGHz Avancado

- [x] `SubGhzAdvancedEngine` criado com:
- [x] inicializacao e selecao de profile (`CORE`/`FULL`);
- [x] filtro de protocolo;
- [x] historico `Recent`;
- [x] analise online e offline de `.sub`;
- [x] TX com caminho avancado e fallback legado.
- [x] `SubGhzAdvancedDecoderAdapter` criado com:
- [x] decode de capturas RAW para protocolo real;
- [x] normalizacao de frame (`protocol_name`, `frequency_hz`, `bit_count`, `key_hex`, `counter`, `button`, `serial`, `raw_summary`);
- [x] exposicao de registry e validacao de protocolo habilitado por profile.
- [x] `SubGhzAdvancedTransmitterAdapter` criado com:
- [x] `subghz_transmitter_deserialize + yield`;
- [x] TX protocol-native sem alterar vendor;
- [x] mapeamento de preset/modulacao para hardware Bruce.
- [x] `SubGhzAdvancedSubFileCodec` criado para leitura Bruce + Flipper e enriquecimento com `Detected_*`.

### 3) UI, CLI e JS

- [x] Novo menu `SubGHz` no menu principal (sem remover `RF` legado).
- [x] Estrutura `SubGHz Unified` com coexistencia de fluxos avancados e legado otimizado.
- [x] Cenas implementadas no menu avancado:
- [x] RX (`Scan & Identify`, `Scan/Copy`, `Decoder UI`, `Rolling UI`);
- [x] TX (`Replay Last RX`, `Transmit .sub`, `Transmit Last`);
- [x] Analyze (`Analyze .sub`, `Analyze Last TX`);
- [x] `Recent` e `Settings` (`CORE/FULL`, filtro, frequencia, range).
- [x] Submenu `Legacy+ Tools` no `SubGHz` com funcionalidades herdadas do `RF`:
- [x] `Scan/copy`, `Record RAW`, `Custom SubGhz`, `Spectrum`, `RSSI Spectrum`, `SquareWave Spec`, `Spectogram`, `Listen`, `Bruteforce`, `Jammer`, `RF Config`.
- [x] CLI `subghz_adv` aditiva:
- [x] `rx`, `tx_file`, `analyze_file`, `protocols`, `profile`, `filter`.
- [x] JS aditivo em `subghzAdvanced`:
- [x] `read(timeoutSec?)`, `analyzeFile(path)`, `transmitFile(path, hideDefaultUI?)`.
- [x] Checklist de paridade legado->unificado em `SUBGHZ_PARITY_CHECKLIST.md`.

### 4) Otimizacoes no Legado (sem breaking changes)

- [x] Enriquecimento do legado com decode avancado em RX (`Detected_Protocol`, `Detected_Key`, `Detected_Serial`, `Detected_Button`, `Detected_Counter`).
- [x] Persistencia de metadados detectados em arquivos `.sub` salvos pelo legado.
- [x] `subghz tx_from_file` e `subghz tx_from_buffer` agora tentam TX avancado primeiro e fazem fallback para RcSwitch.
- [x] `sendCustomRF()` atualizado para aproveitar TX avancado quando o arquivo permitir.

### 5) Cobertura de Protocolo e Validacao

- [x] Script `tools/subghz_protocol_coverage_check.py` criado.
- [x] `FULL` sincronizado com vendor com exclusao intencional de `RAW`/`BinRAW`.
- [x] Matriz de validacao de amostras upstream gerada em `SUBGHZ_V1_SAMPLE_MATRIX.md`.
- [x] Cobertura de fixtures: `CORE 17/17`, `FULL 46/56` (10 protocolos sem fixture upstream).
- [x] Nota de release v1 gerada em `SUBGHZ_V1_RELEASE_NOTES.md`.
- [x] Build validado em:
- [x] `m5stack-cplus2`;
- [x] `lilygo-t-embed-cc1101`.

## Pendente

### P0 (necessario para fechar v1)

- [ ] Rodar validacao de hardware com sinais reais (minimo 3 capturas) e registrar resultados.
- [x] Rodar bateria de amostras `.sub` do upstream (minimo 10) e registrar matriz protocolo esperado vs detectado.
- [x] Consolidar nota de release da v1 para equipe (escopo, limites, comandos e menus).

### P1 (proxima iteracao recomendada)

- [ ] Paridade mais profunda de interface rolling (estado, incrementos e UX de emulacao) comparada ao fluxo da biblioteca de origem.
- [ ] Melhorar telas de `Scan/Copy` com mais contexto de key/counter e acao rapida por protocolo.
- [ ] Implementar cenas `Add manually (Advanced)` para protocolos rolling prioritarios (FAAC SLH, BFT Mitto, Somfy Telis, Nice FloR-S, CAME Atomo, Alutech AT-4N).
- [ ] Implementar editor de `CounterMode` por arquivo `.sub` (com validacao por protocolo).
- [ ] Implementar opcoes de save com nome por `protocol+timestamp`.
- [ ] Implementar suporte opcional a user frequencies/hopper (estilo `setting_user`).
- [ ] Expandir testes automatizados para parser/transcoder de `.sub` (Bruce/Flipper e `Detected_*`).
- [ ] Instrumentar comparativo de acuracia entre fluxo legado e avancado em cenarios selecionados.

### P2 (hardening e manutencao)

- [ ] Opcional: habilitar `RAW`/`BinRAW` no profile `FULL` via shims faltantes no compat (`float_tools`, helpers de storage e ajustes adicionais de API Furi).
- [ ] Normalizar metadados de pin do vendor (`UPSTREAM_PIN.txt` e `README.md` do vendor apontam commits diferentes).
- [ ] Cobrir protocolos ainda sem suporte direto no runtime (`Prastel`, `Airforce`, `HCS101`, `ZKTeco`) via sync upstream ou implementacao dedicada.
- [ ] Documentar no wiki do projeto os novos fluxos `SubGHz` e comandos `subghz_adv`.

## Riscos Conhecidos

- Uso de protocolos que exigem recursos extras de compat pode requerer shims adicionais.
- Cenarios rolling dependem de qualidade da captura, sincronismo de counter e material de keystore.
- Sem bateria automatizada em hardware, regressao de campo ainda pode passar despercebida.

## Criterio de Conclusao da v1

- [ ] `SubGHz` avancado operacional no menu principal com RX/TX/Analyze/Recent/Settings.
- [ ] Legado `rf,subghz` sem quebra de comportamento e com fallback funcional.
- [ ] `subghz_adv` e `subghzAdvanced` estaveis nas rotas principais.
- [ ] Build verde nos targets definidos e evidencias de teste de protocolo/hardware anexadas.
