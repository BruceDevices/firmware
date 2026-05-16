#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AnimatedGIF.h>
AnimatedGIF gif;

//#define PIXEL_TYPE LITTLE_ENDIAN_PIXELS
//#define BITS_PER_PIXEL 16

#define PIXEL_TYPE GIF_PALETTE_RGB8888
#define BITS_PER_PIXEL 32

int main(int argc, char *argv[])
{
    SDL_Window *win;
    SDL_Surface *canvas, *winSurface;
    void *pOld;
    int rc, w, h;
    uint8_t *pFrameBuf; // GIF 8-bit and 16-bit combined frame buffer
    
    if (argc != 2) {
        printf("sdl2 gif player\nUsage: sdl2_gif <filename>\n");
        return -1;
    }
    gif.begin(PIXEL_TYPE);
    rc = gif.open(argv[1], NULL);
    if (!rc) {
    	printf("Error opening %s = %d\n", argv[1], gif.getLastError());
    	return -1;
    }
    w = gif.getCanvasWidth(); h = gif.getCanvasHeight();
    
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    pFrameBuf = (uint8_t *)malloc(w * h * ((BITS_PER_PIXEL/8)+1)); // 8-bit frame + 16-bit or 32-bit frame
    gif.setFrameBuf(pFrameBuf);
    gif.setDrawType(GIF_DRAW_COOKED);

    win = SDL_CreateWindow("GIF Player", 100, 100, w, h, SDL_WINDOW_SHOWN);
    if (win == nullptr) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    // Create a surface to hold the GIF canvas
#if BITS_PER_PIXEL == 16
    canvas = SDL_CreateRGBSurfaceWithFormat(0, w, h, 16, SDL_PIXELFORMAT_RGB565);
#else
    canvas = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
#endif
    if (canvas == nullptr) {
        printf("SDL_CreateSurface error %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
	SDL_Quit();
	return EXIT_FAILURE;
    }
    pOld = canvas->pixels; // keep old pixel pointer; we will substitute our own
    canvas->pixels = &pFrameBuf[w * h]; // point to the pixels the library will generate
    winSurface = SDL_GetWindowSurface(win);
    
    bool bQuit = false;
    int iLoopCount = gif.getLoopCount();
    if (iLoopCount == 0) iLoopCount = 5; // infinite->5
    for (int i=0; i<iLoopCount && !bQuit; i++) {
        while (!bQuit && gif.playFrame(false, &rc, NULL)) {
            SDL_Rect rect;
            SDL_Event e;
            while (SDL_PollEvent(&e)) { // take care of queued events
                if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN) {
                    bQuit = true;
                }
            }
            rect.x = gif.getFrameXOff(); rect.y = gif.getFrameYOff();
            rect.w = gif.getFrameWidth(); rect.h = gif.getFrameHeight();
            SDL_BlitSurface(canvas, &rect, winSurface, &rect);
            SDL_UpdateWindowSurface(win);
	    SDL_Delay(rc);
        }
        gif.reset();
    } // for i

    // Clean up
    canvas->pixels = pOld; // restore original pointer
    SDL_FreeSurface(canvas);
    SDL_FreeSurface(winSurface);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return EXIT_SUCCESS;
} /* main() */
