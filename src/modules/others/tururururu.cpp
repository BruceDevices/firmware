#include "tururururu.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <globals.h>

// By: @IncursioHack / github.com/IncursioHack

// Configuração do personagem principal (tubarão)
int sharkX = 40;
int sharkY = 80;
int sharkSize = 14;
bool sharkUp = false;
bool sharkDown = false;

// Configuração dos peixes
struct Fish {
    int x, y, size;
};
Fish fish[5]; // Array de peixes

// Configuração de pontuação
int score = 0;

void initSprites() {
    // sprite para desenhar a tela toda
    // tft.deleteSprite();
    // tft.createSprite(tftWidth,tftHeight);
    tft.fillScreen(bruceConfig.bgColor);

    // menu_op para desenhar o tubarao
    sprite.deleteSprite();
    sprite.createSprite(32, 30);
    sprite.fillScreen(bruceConfig.bgColor);
    sprite.fillEllipse(19, 17, 10, 5, TFT_DARKGREY);
    sprite.fillCircle(17, 24, 5, TFT_LIGHTGREY);
    sprite.fillTriangle(0, 10, 0, 22, 9, 17, TFT_DARKGREY);
    sprite.fillTriangle(17, 6, 17, 14, 22, 14, TFT_DARKGREY);
    sprite.fillCircle(25, 14, 1, TFT_RED);
    sprite.fillTriangle(23, 18, 29, 18, 24, 21, TFT_RED);
    sprite.fillRect(0, 21, 32, 15, bruceConfig.bgColor);

    // draw para desenhar o peixe
    draw.deleteSprite();
    draw.createSprite(20, 8);
    draw.fillScreen(bruceConfig.bgColor);
    draw.fillEllipse(6, 4, 6, 3, TFT_ORANGE);
    draw.fillTriangle(16, 0, 16, 8, 11, 5, TFT_ORANGE);
    draw.drawFastVLine(6, 1, 7, TFT_WHITE);
    draw.drawFastVLine(10, 1, 7, TFT_WHITE);
    draw.drawFastVLine(15, 1, 7, TFT_WHITE);
    draw.fillCircle(3, 3, 1, TFT_BLACK);
}

// Função para desenhar o tubarão
void drawShark() {
    sprite.pushSprite(sharkX - sharkSize, sharkY - 7);
    // sprite.pushToSprite(&sprite,sharkX-sharkSize, sharkY,TFT_TRANSPARENT);
}

// Função para desenhar peixes
void drawFish(Fish &f) {
    draw.pushSprite(f.x, f.y);
    // draw.pushToSprite(&sprite,f.x,f.y,TFT_TRANSPARENT);
}
#define STEP (tftHeight) / 44

// Função para mover o tubarão
void detectInputs() {
#if defined(ARDUINO_M5STICK_C_PLUS) ||                                                                       \
    defined(ARDUINO_M5STICK_C_PLUS2) // check(EscPress) is the same of check(PrevPress) in these devices
    if (check(SelPress))
#else
    if (check(PrevPress) || check(UpPress))
#endif
    {
        sharkUp = true;
    }
    if (check(NextPress) || check(DownPress)) sharkDown = true;
}

void moveShark() {

    if (sharkDown) {
        sharkY += STEP; // Move para baixo
        sharkDown = false;
    }

    if (sharkUp) {
        sharkY -= STEP; // Move para cima
        sharkUp = false;
    }
    if (sharkY < 0) { sharkY = 0; }
    if (sharkY > tftHeight - sharkSize) { sharkY = tftHeight - sharkSize; }
}

// Função para mover peixes
void moveFish(Fish &f) {
    f.x -= 2; // Move o peixe para a esquerda
    if (f.x < -10) {
        tft.fillRect(f.x, f.y, 22, 11, bruceConfig.bgColor);
        f.x = tftWidth + random(20, 100);
        f.y = random(10, tftHeight - 20);
    }
}

// Função para verificar colisões entre o tubarão e os peixes
void checkCollisions() {
    for (int i = 0; i < 5; i++) {
        if ((sharkX < fish[i].x + fish[i].size) && (sharkX + sharkSize > fish[i].x) &&
            (sharkY < fish[i].y + fish[i].size) && (sharkY + sharkSize > fish[i].y)) {
            // Colidiu com um peixe
            tft.fillRect(fish[i].x, fish[i].y, 18, 8, bruceConfig.bgColor);
            fish[i].x = tftWidth + random(20, 100);
            fish[i].y = random(10, tftHeight - 20);
            score++;
        }
    }
}

// Função para exibir a pontuação
void displayScore() {
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setCursor(0, 0);
    tft.printf("Score: %d", score);
}

void shark_setup() {
    // Inicializa a posição dos peixes
    for (int i = 0; i < 5; i++) {
        fish[i].x = tftWidth + random(20, 100);
        fish[i].y = random(10, tftHeight - 20);
        fish[i].size = 8;
    }
    // desenha peixes e inicia o display
    initSprites();
    if (tft.getLogging()) tft.log_drawString("Not Supported", DRAWCENTRESTRING, tftWidth / 2, tftHeight / 2);
    // inicia o jogo
    shark_loop();
}

void shark_loop() {
    int downTime = 50;
    unsigned long time = 0;
    for (;;) {
        // Mostra a tela

        // Começa a desenhar o Sprite
        displayScore();
        detectInputs();
        if (millis() - time > downTime) {
            // Move o tubarão
            moveShark();
            // Desenha o tubarão
            drawShark();
            // Move e desenha os peixes
            for (int i = 0; i < 5; i++) {
                moveFish(fish[i]);
                drawFish(fish[i]);
            }
            // Verifica colisões
            checkCollisions();
            time = millis();
        }

        // Pequeno atraso para controlar a velocidade do loop
        if (score < 10) downTime = 50;
        else if (score >= 10 && score < 20) downTime = 40;
        else if (score >= 20 && score < 30) downTime = 35;
        else if (score >= 30 && score < 40) downTime = 30;
        else if (score >= 40 && score < 50) downTime = 25;
        else if (score >= 50 && score < 100) downTime = 15;
        else if (score < 100) downTime = 7;

        if (score == 99) {
            displaySuccess("So...");
            while (!check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
            while (check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); } // debounce
            displaySuccess("you just found");
            while (!check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
            while (check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); } // debounce
            displayInfo("hidden credits!");
            while (!check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
            while (check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); } // debounce
            displayInfo("main devs:");
            while (!check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
            while (check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); } // debounce
            options = {
                {"Pirata",       [=]() { displayError("github.com/bmorcelli", true); }   },
                {"pr3y",         [=]() { displaySuccess("github.com/pr3y", true); }      },
                {"IncursioHack", [=]() { displayInfo("github.com/IncursioHack", true); } },
                {"r3ck",         [=]() { displayInfo("github.com/rennancockles", true); }},
            };

            loopOptions(options);

            initSprites();
            score++;
        }

        if (check(EscPress)) {
            returnToMenu = true;
            goto Exit;
        }
    }

Exit:
    delay(150);
    Serial.println();
}
