/* ENGENDRE par boards/gen_bruce.py depuis boards/isno-super.json. Ne pas modifier a la
 * main : relancer le generateur.
 *
 * Carte RF HEADLESS (pas d'ecran, pas de clavier, pas de batterie) : elle se pilote
 * par le port serie ou l'UI web de Bruce. Methode de portage « dev » : le brochage
 * radio vit ici, dans _setup_gpio(), et l'ecran est neutralise par -D USE_DUMMY_TFT
 * dans l'.ini. Seul InputHandler() est obligatoire (pas de defaut faible) ; tout le
 * reste herite des defauts de src/. */

// globals.h amene tout ce qu'on touche : bruceConfigPins (extern), l'enum
// CC1101_SPI_MODULE (via core/configPins.h) et Arduino.h (pinMode/digitalWrite,
// gpio_num_t). interface.h declare les prototypes _setup_gpio()/InputHandler().
#include <globals.h>
#include <interface.h>

void _setup_gpio() {
    // Bus I2C Qwiic (modules externes) et bus systeme interne : memes broches.
    bruceConfigPins.i2c_bus = {(gpio_num_t)12, (gpio_num_t)13}; // sda, scl
    bruceConfigPins.sys_i2c = {(gpio_num_t)12, (gpio_num_t)13}; // sda, scl
    // UART, du point de vue de l'ESP32 : {rx, tx}.
    bruceConfigPins.uart_bus = {(gpio_num_t)18, (gpio_num_t)17}; // rx, tx
    // 2 CC1101 sur le meme SPI (868 + 433 MHz), CS et GDO distincts. Radio active
    // par defaut : 868 MHz. L'utilisateur bascule au runtime via le menu RF :
    // selectionner un preset repose CS/GDO et reinit le driver. Un seul
    // emetteur actif a la fois (contrainte Super).
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)4, (gpio_num_t)5, (gpio_num_t)6,
        (gpio_num_t)7, (gpio_num_t)1, (gpio_num_t)2
    }; // defaut : 868 MHz
    bruceConfigPins.CC1101_presets.push_back({"868 MHz", {
        (gpio_num_t)4, (gpio_num_t)5, (gpio_num_t)6,
        (gpio_num_t)7, (gpio_num_t)1, (gpio_num_t)2
    }});
    bruceConfigPins.CC1101_presets.push_back({"433 MHz", {
        (gpio_num_t)4, (gpio_num_t)5, (gpio_num_t)6,
        (gpio_num_t)15, (gpio_num_t)10, (gpio_num_t)11
    }});
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    // Parquer le(s) CS a HIGH : un demarrage bruyant ne corrompt pas le bus.
    pinMode((gpio_num_t)7, OUTPUT); digitalWrite((gpio_num_t)7, HIGH);
    pinMode((gpio_num_t)15, OUTPUT); digitalWrite((gpio_num_t)15, HIGH);
}

// Obligatoire, sans defaut faible. Carte sans entree locale : rien a lire.
void InputHandler(void) {}
