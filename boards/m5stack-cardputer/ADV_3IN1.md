# Bruce 1.16.1 para Cardputer ADV y módulo RF 3 en 1

Entorno PlatformIO: `m5stack-cardputer-adv-3in1`. Parte de la release estable
1.16.1 de Bruce y conserva la licencia AGPL-3.0 del repositorio. Este entorno
fija el cableado del módulo CC1101 + nRF24L01 + SX1262. No requiere ajustar
los pines desde el menú después de arrancar.

## Cableado

| Señal | GPIO | Uso |
| --- | ---: | --- |
| SPI SCK | 40 | CC1101, nRF24, SX1262 y SD |
| SPI MISO | 39 | CC1101, nRF24, SX1262 y SD |
| SPI MOSI | 14 | CC1101, nRF24, SX1262 y SD |
| CC1101 CS | 15 | Selección de CC1101 |
| CC1101 GDO0 | 13 | Interrupción/datos CC1101 |
| nRF24 CE | 8 | Compartido con teclado SDA |
| nRF24 CS | 9 | Compartido con teclado SCL |
| SX1262 CS | 5 | Selección de SX1262 |
| SX1262 RST | 3 | Reset SX1262 |
| SX1262 DIO1 | 4 | Interrupción SX1262 |
| SX1262 BUSY | 6 | Estado SX1262 |
| Teclado TCA8418 SDA | 8 | Compartido con nRF24 CE |
| Teclado TCA8418 SCL | 9 | Compartido con nRF24 CS |
| Teclado TCA8418 INT | 11 | Interrupción de teclado |
| Tarjeta SD CS | 12 | Selección de SD existente |

## Transición del teclado al nRF24

En reposo, GPIO8/9 pertenecen a `Wire1` para el teclado. Al iniciar una función
SPI de nRF24, el firmware bloquea el lector de teclas, desconecta la interrupción
TCA8418, termina `Wire1`, establece CE bajo y CS alto y configura ambos GPIO
como salidas. Luego usa `acquireSPIBus()`, el gestor SPI de Bruce, para compartir
el bus de la SD. El botón lateral GPIO0 sirve para salir de la operación activa
cuando el teclado I2C está suspendido. Al terminar, se apaga el nRF24, se dejan
CE bajo y CS alto, se liberan GPIO8/9, se reinicia `Wire1` y se reinicializan la
matriz y la interrupción del teclado. Un fallo de reinicialización se reintenta.

Las operaciones de CC1101, nRF24 y LoRa preparan sus CS antes de inicializar la
radio. El perfil usa el soporte SX1262 ya incluido en Bruce, con BUSY en GPIO6.
La variante SX1262 se guarda en `/lora_settings.json`. Los pines fijos del
perfil se reimponen al leer `/brucePins.conf`, incluyendo después de reinicio o
salida de deep sleep; un fichero antiguo con otro pinout se corrige y guarda.
Una asignación GPS antigua en GPIO15/13 se mueve a Grove GPIO1/2 para no
activar accidentalmente CS o GDO0 de CC1101.

## Límite eléctrico del montaje indicado

GPIO8 y GPIO9 están conectados **directamente** tanto al TCA8418 como al
nRF24L01. Durante una transacción I2C, SDA cambia el nivel de CE y SCL lleva
CS a bajo. El firmware no puede mantener CS alto ni CE bajo en el nRF24 y, a la
vez, usar esas mismas líneas para el teclado. El nRF24 podría ver tráfico SPI
ajeno mientras CS está bajo, incluso cuando el software haya apagado la radio.
Por tanto, la exclusión eléctrica de CS y el funcionamiento RF fiable **no están
garantizados** con este cableado. Hace falta aislar físicamente las dos líneas
del nRF24 (con un multiplexor, interruptor o cableado alternativo) para cumplir
ese requisito. Ese cambio de hardware necesitaría especificar su pin de control
antes de adaptar el firmware.

Mientras se usa nRF24, el teclado I2C no proporciona navegación; el botón
lateral permite terminar la función activa. Las funciones nRF24 que requieren
teclas simultáneas para cambiar opciones conservan el código original, pero su
control completo necesita aislamiento externo. Esto también afecta a cualquier
uso simultáneo de `Wire1` por otro periférico en los mismos GPIO.

## Compilación y comprobaciones

```
pio run -e m5stack-cardputer-adv-3in1
python -m unittest discover -s tests -p 'test_cardputer_adv_3in1.py'
```

Las pruebas automatizadas verifican pinout, reimposición tras cargar configuración,
secuencia de transición, preparación de CS, BUSY/IRQ SX1262 y presencia en los
menús. Son comprobaciones de código; no sustituyen la validación sobre placa.

## Pruebas físicas pendientes

1. Medir con osciloscopio o analizador lógico GPIO8/9, CE y CS durante escritura
   I2C y tráfico SD/SPI; comprobar que no se selecciona nRF24 indebidamente.
2. Arrancar en frío, reiniciar y despertar de deep sleep; leer el teclado y
   confirmar los pines efectivos de las tres radios en cada arranque.
3. Entrar y salir repetidamente de Spectrum, Jammer y MouseJack; comprobar
   restauración del teclado, incluso tras fallo de detección nRF24.
4. Confirmar CC1101 RX/TX con un equipo de prueba legal y SX1262 TX/RX con
   otra radio configurada a la misma frecuencia y región.
5. Validar nRF24 RX/TX con un segundo nRF24, después de incorporar el aislamiento
   eléctrico; repetir las pruebas de teclado durante actividad RF.
