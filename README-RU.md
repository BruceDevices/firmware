# WiFite fix для Bruce (LilyGo T-Embed CC1101 Plus)

Дата сборки: 09.11.2026
Устройство: LilyGo T-Embed CC1101 Plus (ESP32-S3, 16 MB flash, 8 MB PSRAM)
Плата PlatformIO: `lilygo-t-embed-cc1101`

## Проблема

При запуске WiFite появлялось сообщение:

```
Cannot allocate WiFite worker
```

Причина: воркер создавался через `xTaskCreate(task_main, "WIFITE", 24576, ...)`.
`xTaskCreate` всегда выделяет стек задачи из внутренней DRAM и требует один
непрерывный блок 24 КБ. После инициализации Wi-Fi внутренняя heap фрагментирована,
поэтому создать задачу не удавалось, и `wifite_start()` возвращал `NULL`.

Дополнительно крупные буферы WiFite (сессия WPS, кадры, очереди, таблицы
сканирования) выделялись во внутренней RAM и усиливали фрагментацию.

## Что изменено

Изменён только файл:

```
lib/BruceWifite/src/radio.c
```

1. Добавлены `wf_alloc()` / `wf_calloc()` / `wf_free()`, которые сначала
   пытаются выделить память в PSRAM (`MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT`),
   а при неудаче откатываются на обычный `malloc`/`calloc`.
2. Крупные буферы переведены на эти функции: `WfSession`, кадры
   `scratch`/`eap_scratch`/`frame`, storage RX-очереди, таблица целей
   сканирования, список клиентов, буферы WPS-обмена.
3. Очередь событий стала статической (`StaticQueue_t` в `WifiteJob`),
   её storage выделяется в PSRAM.
4. Сам `WifiteJob`, TCB (`StaticTask_t`) и стек 24 КБ перенесены в `.bss`:
   `static WifiteJob job_storage; static StackType_t wifite_stack[24576];`
5. Задача создаётся через `xTaskCreateStatic()` и больше не требует heap.
6. `task_main()` после работы не удаляет себя, а «паркуется» через
   `vTaskDelay(portMAX_DELAY)`. `wifite_free()` удаляет задачу синхронно
   перед повторным использованием статического стека.
7. Добавлен флаг `job_in_use`, запрещающий одновременный запуск двух воркеров.

## Файлы в архиве

```
README-RU.md
build-info.txt
patches/0001-wifite-psram-static-stack.patch   — unified diff против upstream
lib/BruceWifite/                                — полная изменённая библиотека
```

Оригинальный upstream: `BruceDevices/firmware`, ветка `dev`,
коммит `22e72a1ba812ab19c9e1fc63dd1cbf467780d576` (тот же код WiFite,
что и в установленной прошивке `4cde470`).

## Как применить

Вариант 1 — заменить папку:

```
lib/BruceWifite/  ->  <проект>/lib/BruceWifite/
```

Вариант 2 — применить патч из корня проекта:

```
git apply patches/0001-wifite-psram-static-stack.patch
```

## Сборка

```
pio run -e lilygo-t-embed-cc1101
```

Окружение сборки:

- PlatformIO Core 6.x, Python 3.13
- pioarduino platform 55.03.39
- Arduino core 3.3.9
- framework-arduinoespressif32-libs: bmorcelli
  `main_esp32-arduino-libs-20260826-181233`
- toolchain: xtensa-esp-elf 14.2.0+20260121

Результат: app-образ `.pio/build/lilygo-t-embed-cc1101/firmware.bin` и
merged-образ `Bruce-lilygo-t-embed-cc1101.bin`.

## Проверка

- Прошивка собрана без ошибок.
- `radio.c` компилируется, линковка проходит.
- Занятость памяти: DRAM 73.34% (250 656 / 341 760), RAM 47.8%,
  Flash 27.3%.
- На устройстве WiFite запускается; полноценная атака не проверялась,
  так как рядом не было тестовых точек доступа.

## SHA256 прошивок

```
9F4C4F860886AF87C478FF56ACDA8906DC3B334621BECD5C3ED5D124D953A2EA  Bruce-lilygo-t-embed-cc1101-wifite-fix.bin
1FD1DAAF645431C7F853AA68B3A227D5709DDA4F1C06D67F4A58065C79066115  Bruce-lilygo-t-embed-cc1101-wifite-fix-app-only.bin
```
