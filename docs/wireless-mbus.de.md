# Wireless M-Bus für Bruce

[English instructions](wireless-mbus.md)

Diese experimentelle Erweiterung ergänzt das RF-Menü von Bruce um einen rein
empfangenden Wireless-M-Bus-Empfänger für den CC1101. Sie sendet nicht, nimmt keine
Schlüssel entgegen und entschlüsselt keine geschützten Zählerwerte.
Es ist eine native Firmware-Erweiterung, kein JavaScript zum Kopieren auf die
SD-Karte. Die vorhandene Bruce-JavaScript-API stellt den benötigten Paketempfang
nicht bereit.

## Wofür Wireless M-Bus verwendet wird

Wireless M-Bus (wM-Bus, EN 13757-4) überträgt Zählerdaten per Funk, zum Beispiel
von Wasser-, Gas-, Wärme- und Stromzählern oder Heizkostenverteilern zu einem
Lesegerät oder Gateway. Dadurch lassen sich Verbrauchsdaten aus der Ferne erfassen.

Es ist besonders in Europa verbreitet. Regionale Profile und Frequenzbänder
unterscheiden sich; diese Implementierung empfängt gezielt T1/C1 auf 868,95 MHz.
Siehe die [M-Bus-Übersicht](https://m-bus.com/) und
[TIs Wireless-M-Bus-Übersicht](https://www.ti.com/en-us/technologies/wired-wireless-connectivity/wireless-m-bus).

## Hardware und Teststand

- Getestet wurde ein **LilyGO T-Embed CC1101 Plus** mit dem Build-Ziel
  `lilygo-t-embed-cc1101`. Ein eigenes Plus-Ziel gibt es in diesem Quellstand nicht.
- Implementiert sind **T1 und C1 auf 868,95 MHz**, bei C1 die Frameformate A und B.
- Der Gerätebesitzer hat am **11. November 2026** einen sehr gut funktionierenden
  Funkempfang bestätigt. Damit sind noch nicht alle Modi, Zählertypen,
  Datenfelder und SD-Funktionen einzeln geprüft.
- Andere Boards und externe CC1101-Aufbauten wurden hier nicht auf Hardware getestet.
- Für Decoder und Empfänger gibt es lokale C++-Tests; siehe unten.

## Firmware bauen und installieren

Baue den Checkout, der diese Erweiterung enthält. Die ursprüngliche Entwicklungsbasis ist
der offizielle Bruce-Commit `a59213f3ec6cd302fa32c2042b2700c9c24d991a`.

Der lokale Build wurde mit **Python 3.13**, **PlatformIO 6.2.0** und dem Ziel
`lilygo-t-embed-cc1101` durchgeführt. Der Branch enthält außerdem die für den
lokalen Windows-Build benötigte Festlegung auf FastLED 3.10.3 und eine Korrektur
am objcopy-Build-Hook.

Beispiel für Windows PowerShell im Repository-Verzeichnis, mit verfügbarem Python 3.13:

```powershell
py -3.13 -m venv .tools\venv
$env:PLATFORMIO_CORE_DIR = "$PWD\.tools\platformio"
$env:PLATFORMIO_CACHE_DIR = "$PWD\.tools\platformio-cache"
$env:TEMP = "$PWD\.tools\tmp"
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Path $env:TEMP -Force | Out-Null
& .\.tools\venv\Scripts\python.exe -m pip install platformio==6.2.0 --cache-dir .tools\pip-cache
& .\.tools\venv\Scripts\python.exe -m platformio run -e lilygo-t-embed-cc1101
```

Danach liegen das reine App-Image unter
`.pio/build/lilygo-t-embed-cc1101/firmware.bin` und das zusammengefügte Komplettimage
`Bruce-lilygo-t-embed-cc1101.bin` im Repository-Hauptverzeichnis.
Für eine normale Installation als einzige Firmware auf dem ESP32-S3 wird das
Komplettimage mit esptool ab Adresse **`0x0`** geschrieben. Das reine App-Image darf
**nicht** an Adresse `0x0` geschrieben werden. Wähle den COM-Port deines Geräts.

Beispiel zum Flashen als einzelne Firmware:

```powershell
& .\.tools\venv\Scripts\python.exe -m pip install esptool --cache-dir .tools\pip-cache
$devicePort = 'COM7' # Durch den COM-Port deines Geräts ersetzen.
& .\.tools\venv\Scripts\python.exe -m esptool --chip esp32s3 --port $devicePort write-flash 0x0 Bruce-lilygo-t-embed-cc1101.bin
```

Das Ende des Schreibvorgangs und die Datenprüfung abwarten. Anschließend das
RF-Menü auf dem Gerät öffnen.

Das Komplettimage enthält Bootloader, Partitionstabelle und Anwendung für Bruce
als einzelne Firmware auf dem ESP32-S3 mit 16 MB Flash. Beim dokumentierten
Hardwaretest wurde der interne Flash gelöscht und Bruce anschließend neu installiert.

Beim Build bleiben Warnungen aus vorhandenem Bruce-, Display-, Bluetooth- und
Audiocode bestehen. Ein erfolgreicher Build bestätigt nicht alle Funktionen der
gesamten Firmware.

## Bedienung

1. Falls erforderlich unter **Radio Frequency > Config** den CC1101 als RF-Modul auswählen.
2. **Radio Frequency > Wireless M-Bus > Scan T1/C1 868.95** öffnen.
3. Auf vollständige Telegramme mit gültigen Link-Layer-Prüfsummen warten.
   Die Sendeintervalle eines Zählers können lang sein.
4. Mit dem Drehrad ein Gerät auswählen und durch Klicken die Details öffnen.
5. In den Details mit dem Drehrad scrollen. Währenddessen pausiert der Empfang.
   Klicken oder Zurück setzt ihn fort.
6. Zurück in der Scanansicht stoppt den Empfang und öffnet das App-Menü.
7. **Export devices to SD** speichert die Geräteübersicht; **Clear results** leert
   sie. Beim Verlassen der App endet die Sitzung und die RAM-Liste wird verworfen.

Die Liste enthält höchstens **96 unterschiedliche Geräte**. Gruppiert wird nach
Hersteller und vollständiger Link-Layer-Adresse aus ID, Version und Typ.
Angezeigt werden unter anderem unterstützte Header, RSSI, LQI, Telegrammzahl und
Empfangszeit seit Gerätestart. Bekannte numerische DIF/VIF-Datensätze werden nur
bei ausdrücklich unverschlüsselten Anwendungsdaten interpretiert.
Unbekannte Formate bleiben als Rohdaten sichtbar. Der Hexdump kann auch
verschlüsselte Bytes enthalten und ist entsprechend beschriftet.

## SD-Export

Die Dateien heißen `/wmbus/devices-<boot-ms>-<nummer>.jsonl`. Jede JSON-Zeile
enthält ein Gerät mit seinem letzten gültigen Telegramm und Metadaten. Das ist
eine Momentaufnahme, keine lückenlose Telegrammaufzeichnung.
Bei `frame_hex` sind die DLL-Prüfsummen entfernt und das L-Byte normalisiert;
es handelt sich nicht um eine unveränderte Aufzeichnung des Funksignals.
Der ursprüngliche L-Wert bleibt in `wire_l` erhalten. Zeiten sind Millisekunden
seit Gerätestart, keine Uhrzeiten.

## Grenzen und Rückmeldungen

Unterstützt werden nur die genannten Modi und die genannte Frequenz. S-, R- und
N-Modi, 169 MHz und andere Frequenzen sind nicht implementiert. Geschützte Werte
bleiben geschützt; Herstellerformate, Kompakttelegramme und nicht unterstützte
Datensätze lassen sich nicht universell interpretieren.
Display, SD und Funkmodul teilen sich einen SPI-Bus. Bei Bildschirmupdates sind
Empfangslücken möglich. Ungültige Prüfsummen, unvollständige Telegramme und
FIFO-Überläufe werden verworfen. Die Tabelle ist auf 96 Geräte begrenzt.
Eine gültige CRC ist keine Authentifizierung des Senders.

Für weitere Tests sind besonders hilfreich: Vergleich von T1/C1-A/C1-B mit einem
Referenzempfänger, wiederholte Telegramme, offene und geschützte Datensätze,
SD-Export samt Fehlerfällen, Bedienung und andere RF-Apps nach dem Verlassen.
Bei Rückmeldungen bitte Board, Firmware-Commit, bekannten Empfangsmodus und
Schritte zum Nachstellen nennen. Geräte-IDs und persönliche Verbrauchsdaten in
öffentlichen Beispielen unkenntlich machen.

## Lokale Tests und Quellen

Mit Python 3 und einem GCC-/Clang-C++17-Compiler mit UndefinedBehaviorSanitizer:

```sh
python tests/wmbus/run.py
```

Mit `--cxx /pfad/zum/compiler` lässt sich ein anderer Compiler wählen.
Alternativ das Python-Paket `ziglang` in einer lokalen virtuellen Umgebung
installieren und `python tests/wmbus/run.py --zig` ausführen. Dieser Weg wurde
für die lokalen Windows-Tests verwendet. Ausgaben liegen unter
`.pio/wmbus-host-tests` und gehören nicht zum Quellcode.

Geprüft werden CRC, T1-/C1-A/B-Framing, beschädigte und abgeschnittene Telegramme,
Verschlüsselungsgrenzen, 20.000 Zufallseingaben, FIFO-Stückelung, lange Frames,
Erholung nach Überlauf/Timeout und Wiederherstellung der Funkregister.
Der Test mit simuliertem CC1101 übersetzt die tatsächliche Receiver-Implementierung;
er prüft weder Funkausbreitung noch Benutzeroberfläche.

Referenzen: [TI AN067](https://www.ti.com/lit/an/swra234a/swra234a.pdf),
[TI C/T-Framing](https://www.ti.com/lit/an/swra522d/swra522d.pdf),
[CC1101-Datenblatt](https://www.ti.com/lit/ds/symlink/cc1101.pdf),
[CC1101-Errata](https://www.ti.com/lit/er/swrz020e/swrz020e.pdf),
[wMbus-lib](https://github.com/SzczepanLeon/wMbus-lib),
[wmbusmeters](https://github.com/wmbusmeters/wmbusmeters) und
[libmbus](https://github.com/rscada/libmbus).
Die Quellen dienten dem Abgleich von Funkparametern und Telegrammformaten;
es wird keine Entschlüsselungsbibliothek eingebunden. Die Implementierung
entstand mit Unterstützung von Codex. Lizenz: AGPL-3.0-or-later;
siehe [LICENSE](../LICENSE).
