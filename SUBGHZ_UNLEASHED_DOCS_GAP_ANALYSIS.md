# SubGHz Unleashed Docs Gap Analysis (Bruce)

Updated: 2026-05-23  
Scope: mapear funcionalidades documentadas no Unleashed para plano de implementacao no Bruce.

## Fontes analisadas

- `ref/unleashed-firmware/documentation/SubGHzSupportedSystems.md`
- `ref/unleashed-firmware/documentation/SubGHzRemoteProg.md`
- `ref/unleashed-firmware/documentation/SubGHzCounterMode.md`
- `ref/unleashed-firmware/documentation/SubGHzSettings.md`
- `ref/unleashed-firmware/documentation/SubGHzRemotePlugin.md`
- `ref/unleashed-firmware/ReadMe.md` (secoes Sub-GHz Library/HAL e Sub-GHz Main App)

## Resumo executivo

- Protocolos/sistemas documentados no `SubGHzSupportedSystems.md`: `63`.
- Runtime `FULL` atual no Bruce (`SubGhzAdvancedDecoderAdapter`): `56` protocolos ativos (com `RAW/BinRAW` fora do profile FULL por decisao de v1).
- Mapeamento documentacao -> runtime:
- `59/63` cobertos diretamente ou via protocolo base equivalente.
- `4/63` ainda sem cobertura direta no runtime atual: `Prastel`, `Airforce`, `HCS101`, `ZKTeco`.
- Fluxo unificado ja cobre RX/TX/Analyze/Recent/Settings e coexistencia com legado.
- Paridade completa de cenas do app SubGHz do Unleashed ainda esta parcial (principalmente `Add manually`, `CounterMode` UI, e configuracoes de frequencias/hopper via arquivo de settings).

## Cobertura de protocolos (doc -> Bruce)

### Cobertos diretamente no runtime FULL

- CAME, CAME TWEE, CAME Atomo
- KeeLoq, FAAC SLH, Nice FLO, Nice FloR-S
- Somfy Telis, Somfy Keytis
- Gate TX, Princeton, Linear, Linear Delta3
- Security+ 1.0, Security+ 2.0
- Holtek, Holtek HT12X, SMC5326
- Doitrand, Dooya, Power Smart
- Marantec, Marantec24, Mastercode, MegaCode
- Honeywell, Honeywell WDB, Magellan, Legrand
- Ansonic, BETT, Clemsa, Dickert MAHS
- Alutech AT-4N, KingGates Stylo 4k
- Beninca ARC, Ditec GOL4, Phoenix V2
- Jarolift, KeyFinder, Nord ICE, Allstar Firefly
- Hollarm, GangQi, Feron, Roger, Revers RB2, Elplast, Treadmill37, iDO, Nero Sketch, Nero Radio, Hormann, Intertechno V3, Hay21

### Cobertos por equivalencia de protocolo base (nao protocolo dedicado)

- AN-Motors AT4 -> base `KeeLoq` / fluxo rolling.
- BFT Mitto -> base `KeeLoq` com seed/manufacturer.
- Erreka -> base `KeeLoq` com manufacturer.
- Nice One -> tratado hoje na familia `Nice FloR-S` (validar diferencas de campo/UX no roadmap).

### Nao cobertos no runtime atual

- Prastel
- Airforce
- HCS101
- ZKTeco

## Paridade de funcionalidades de app/UX (ReadMe + docs)

| Funcionalidade documentada no Unleashed | Estado no Bruce | Observacao |
| --- | --- | --- |
| Menu SubGHz dedicado | done | `SubGHz Unified` ativo no menu principal |
| RX avancado com identificacao de protocolo | done | `Scan & Identify`, `Scan/Copy`, `Decoder UI` |
| TX de `.sub` com encoder protocol-native | done | via `SubGhzAdvancedTransmitterAdapter` com fallback legado |
| Analyze `.sub` (Bruce + Flipper) | done | `Analyze (Advanced)` |
| Rolling tools/UI | partial | preview/send/emulate existentes; falta paridade de cenas/wizards por protocolo |
| Botoes custom por protocolo rolling (setas/hidden/prog) | partial | campo `Button` existe, mas falta UX dedicada estilo Unleashed |
| `Add manually` expandido com protocolos rolling | missing | nao ha cena avancada de criacao manual equivalente |
| CounterMode por protocolo rolling | partial | parse em `.sub` suportado no vendor; falta editor/UI no Bruce |
| Save last used settings (SubGHz app state) | partial | freq/range existem; profile/filter/estado de cenas sem persistencia completa |
| Naming com `protocol+timestamp` ao salvar | missing | fluxo atual salva por nome padrao |
| Received list com delete rapido | missing | nao ha gesto equivalente no fluxo atual |
| Ignore options por categoria/protocolo | missing | nao implementado no menu atual |
| User frequencies/hopper via `subghz/assets/setting_user` | missing | Bruce usa lista interna; nao carrega `setting_user` do Unleashed |
| SubGHz Remote (map de 5 botoes) | missing | funcionalidade/plugin ainda nao portada |

## Observacoes de estabilidade para rollout

- Existe incidente de reboot/panic em RX avancado com profile `FULL` no board `lilygo-t-embed-cc1101`, com stack apontando decode de `jarolift` em feed ao vivo.
- Isso bloqueia considerarmos o `FULL` como pronto para campo sem hardening adicional.

## Backlog recomendado (priorizado)

### P0 - Estabilidade e seguranca operacional

1. Reproduzir panic em build debug (`env:lilygo-t-embed-cc1101-debug`) e registrar stack simbolizado.
2. Introduzir modo de mitigacao no FULL:
   - blacklist temporaria de protocolos instaveis em live RX, ou
   - toggle de profile seguro (`FULL_SAFE`) para campo.
3. Adicionar guardrails no loop de decode ao vivo:
   - limite de duracao/pulsos por frame,
   - reset rigoroso da instancia receiver entre capturas.

### P1 - Paridade funcional com docs do Unleashed

1. Criar `Add manually (Advanced)` para protocolos prioritarios:
   - FAAC SLH, BFT Mitto, Somfy Telis, Nice FloR-S, CAME Atomo, Alutech AT-4N.
2. Criar editor de `CounterMode` por arquivo `.sub` (com validacao por protocolo).
3. Criar UI de botoes custom por protocolo rolling (atalhos de setas por perfil).
4. Implementar naming opcional `protocol+timestamp` nos saves de capture.
5. Portar opcoes de ignore por categoria/protocolo no decoder avancado.
6. Implementar carregamento opcional de frequencias/hopper via arquivo estilo `setting_user`.

### P2 - Expansao de cobertura e consolidacao

1. Tratar os 4 protocolos ainda sem cobertura (`Prastel`, `Airforce`, `HCS101`, `ZKTeco`):
   - checar se existem implementacoes upstream novas e sincronizar vendor,
   - caso nao existam, manter como backlog de protocolo novo.
2. Empacotar keystore default de fabricantes para melhorar UX KeeLoq out-of-box.
3. Adicionar teste automatizado de conformidade doc->runtime em CI (lista de protocolos + aliases).

## Definicao de pronto para esta trilha

- `SubGHz Unified` cobre fluxos legado+avancado sem perda funcional.
- RX/TX avancado em hardware CC1101 sem reboot em bateria de sinais reais.
- Cenas prioritarias de rolling/manual (P1) entregues com UX consistente.
- Checklist de cobertura contra docs atualizado a cada sync de vendor.
