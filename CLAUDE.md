# CLAUDE.md

## Projeto
Bruce Firmware — firmware multifuncional para ESP32, foco em pesquisa de
segurança/laboratório (uso autorizado/controlado). Stack: C/C++,
PlatformIO, Arduino-ESP32 / ESP-IDF. Multi-board (ESP32, S3, C5, P4,
M5Stack Cardputer, M5StickC Plus2, LilyGo T-Embed/T-Deck/T-Display, CYD).

## Regras de economia de tokens (prioridade máxima)
Estas regras valem para QUALQUER tarefa neste repo, não só o refactor de RF.

1. Nunca leia um arquivo inteiro se a tarefa só exige uma função/trecho.
   Primeiro localize com `rg -n "padrão" caminho/`, depois abra só o
   intervalo de linhas necessário (offset+limit / view_range).
2. Nunca cole o conteúdo de um arquivo grande na resposta só para
   "mostrar" — edite direto com Edit/str_replace.
3. Prefira edição pontual (Edit) a reescrever o arquivo inteiro (Write).
4. Ao entrar numa pasta desconhecida, mapeie a estrutura com `ls`/Glob
   antes de abrir arquivos individuais.
5. Para histórico, use `git log -p --follow -n 5 <arquivo>` — nunca
   `git log -p` sem limite.
6. Registre decisões de design relevantes em `.claude/decisions.md` em
   vez de re-explicar o raciocínio inteiro em cada resposta nova.
7. Ao concluir um milestone/etapa grande, rode `/compact` antes de
   seguir para a próxima, mantendo só o essencial em contexto.
8. Nunca clone repositórios externos de novo se já existem em
   `.claude/source/` — leia só os arquivos pontuais necessários ali.

## Arquitetura relevante
- Módulo RF em refactor: `src/modules/rf/**`
- Cores de UI: usar SOMENTE `bruceConfig.priColor` (foreground),
  `bruceConfig.bgColor` (background) e
  `getComplementaryColor2(bruceConfig.priColor)` (destaque).
  Exceção: `rf_waterfall.cpp` (natureza visual diferente).
- Pontos de entrada que NÃO podem mudar de assinatura/comportamento
  externo: chamadas vindas do Menu, do CLI Serial e da API JavaScript.

## Workflow de execução do plano (RF refactor)
- Plano mestre: `.claude/plan.md` — leia uma vez no início da tarefa
  para orientação geral. Não releia o arquivo inteiro depois disso.
- Cada etapa tem arquivo próprio: `.claude/milestone_<n>.md`, com
  critérios de entrega e testes operacionais.
- Ao executar o milestone N: leia **apenas** `.claude/milestone_<n>.md`.
  Não abra os demais milestones.
- Ao concluir, edite o próprio arquivo do milestone adicionando uma
  seção `## Resultado` (arquivos alterados, decisões, resultado dos
  testes, pendências). Não crie relatórios extras em outros arquivos.

## Requisitos fixos do refactor de RF (não reabrir discussão — só seguir)
- Usar somente RMT nativo do framework Espressif; remover a dependência
  do RCSwitch ao final.
- Decodificar/executar todos os protocolos hoje suportados pela
  referência clonada em `.claude/source/momentum-firmware`.
- Centralizar definições de protocolo em um único local documentado
  (ex.: `src/modules/rf/protocols/`).
- Assinaturas das funções consumidas por Menu/CLI Serial/JS: alteração
  mínima.
- UI: alteração mínima; paleta de cores restrita conforme acima.
- Reduzir duplicidade de código/helpers dentro de `src/modules/rf/**`.

## Build / teste
- Build de um env específico: `pio run -e lilygo-t-embed-cc1101`
- Flash: `pio run -e lilygo-t-embed-cc1101 -t upload`
- Compile sempre o env mínimo necessário para validar a mudança, não
  todos os envs do projeto.

## Skills do projeto
- `.claude/skills/rf-protocol-decoder/` — referência condensada de
  protocolos RF, lida por demanda (ver SKILL.md).
- `.claude/skills/milestone-runner/` — fluxo de execução de milestone
  com contexto mínimo (ver SKILL.md).
