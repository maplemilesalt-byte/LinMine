#include "gfx.h"

#include <SDL2/SDL.h>
#include <stdio.h>

#define LM_SHEET_COUNT 3

static SDL_Window *lm_window;
static SDL_Renderer *lm_renderer;
static SDL_Texture *lm_sheets[LM_SHEET_COUNT];
static int lm_closed;

static const char *lm_path(char *dst, size_t size, const char *dir, const char *name)
{
    int n;
    if (!dir || !name || size == 0) return NULL;
    n = snprintf(dst, size, "%s/%s", dir, name);
    if (n < 0 || (size_t)n >= size) return NULL;
    return dst;
}

static SDL_Texture *lm_load_bmp(const char *dir, const char *name)
{
    char path[1024];
    SDL_Surface *surface;
    SDL_Texture *texture;

    if (!lm_path(path, sizeof(path), dir, name)) return NULL;
    surface = SDL_LoadBMP(path);
    if (!surface) {
        fprintf(stderr, "LinMine: cannot load %s: %s\n", path, SDL_GetError());
        return NULL;
    }
    texture = SDL_CreateTextureFromSurface(lm_renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture)
        fprintf(stderr, "LinMine: cannot create texture: %s\n", SDL_GetError());
    return texture;
}

int lm_graphics_init(int width, int height, const char *title)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 0;
    lm_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 width, height, SDL_WINDOW_SHOWN);
    if (!lm_window) { SDL_Quit(); return 0; }
    lm_renderer = SDL_CreateRenderer(lm_window, -1,
                                     SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!lm_renderer)
        lm_renderer = SDL_CreateRenderer(lm_window, -1, SDL_RENDERER_SOFTWARE);
    if (!lm_renderer) {
        SDL_DestroyWindow(lm_window); lm_window = NULL; SDL_Quit(); return 0;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    lm_closed = 0;
    return 1;
}

void lm_graphics_shutdown(void)
{
    lm_graphics_free_assets();
    if (lm_renderer) SDL_DestroyRenderer(lm_renderer);
    if (lm_window) SDL_DestroyWindow(lm_window);
    lm_renderer = NULL;
    lm_window = NULL;
    SDL_Quit();
}

int lm_graphics_load_assets(const char *dir, int color)
{
    lm_graphics_free_assets();
    lm_sheets[LM_SHEET_BLOCKS] = lm_load_bmp(dir, color ? "blocks.bmp" : "blocksbw.bmp");
    lm_sheets[LM_SHEET_LED] = lm_load_bmp(dir, color ? "led.bmp" : "ledbw.bmp");
    lm_sheets[LM_SHEET_BUTTON] = lm_load_bmp(dir, color ? "button.bmp" : "buttonbw.bmp");
    if (!lm_sheets[0] || !lm_sheets[1] || !lm_sheets[2]) {
        lm_graphics_free_assets();
        return 0;
    }
    return 1;
}

void lm_graphics_free_assets(void)
{
    int i;
    for (i = 0; i < LM_SHEET_COUNT; ++i) {
        if (lm_sheets[i]) SDL_DestroyTexture(lm_sheets[i]);
        lm_sheets[i] = NULL;
    }
}

void lm_graphics_begin(void) {}
void lm_graphics_end(void) {}
void lm_graphics_present(void) { if (lm_renderer) SDL_RenderPresent(lm_renderer); }

static void lm_color(unsigned int pixel)
{
    SDL_SetRenderDrawColor(lm_renderer,
        (Uint8)(pixel >> 16), (Uint8)(pixel >> 8), (Uint8)pixel,
        (Uint8)((pixel >> 24) ? (pixel >> 24) : 255));
}

void lm_graphics_clear(unsigned int pixel)
{
    if (!lm_renderer) return;
    lm_color(pixel); SDL_RenderClear(lm_renderer);
}

void lm_graphics_fill_rect(LMRect rect, unsigned int pixel)
{
    SDL_Rect r = { rect.x, rect.y, rect.width, rect.height };
    if (!lm_renderer) return;
    lm_color(pixel); SDL_RenderFillRect(lm_renderer, &r);
}

void lm_graphics_draw_rect(LMRect rect, unsigned int pixel)
{
    SDL_Rect r = { rect.x, rect.y, rect.width, rect.height };
    if (!lm_renderer) return;
    lm_color(pixel); SDL_RenderDrawRect(lm_renderer, &r);
}

void lm_graphics_set_pixel(int x, int y, unsigned int pixel)
{
    if (!lm_renderer) return;
    lm_color(pixel); SDL_RenderDrawPoint(lm_renderer, x, y);
}

void lm_graphics_draw_sprite(LMSpriteSheet sheet, int index, int x, int y,
                             int width, int height)
{
    SDL_Rect src, dst;
    int count = sheet == LM_SHEET_BLOCKS ? 16 : (sheet == LM_SHEET_LED ? 12 : 5);
    if (!lm_renderer || sheet < 0 || sheet >= LM_SHEET_COUNT ||
        !lm_sheets[sheet] || index < 0 || index >= count) return;
    src.x = index * width; src.y = 0; src.w = width; src.h = height;
    dst = src; dst.x = x; dst.y = y;
    SDL_RenderCopy(lm_renderer, lm_sheets[sheet], &src, &dst);
}

int lm_graphics_poll_event(LMInputButton *button, LMPoint *position, int *pressed)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) { lm_closed = 1; return 1; }
        if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) *button = LM_BUTTON_LEFT;
            else if (event.button.button == SDL_BUTTON_MIDDLE) *button = LM_BUTTON_MIDDLE;
            else if (event.button.button == SDL_BUTTON_RIGHT) *button = LM_BUTTON_RIGHT;
            else continue;
            if (position) { position->x = event.button.x; position->y = event.button.y; }
            if (pressed) *pressed = event.type == SDL_MOUSEBUTTONDOWN;
            return 1;
        }
    }
    return 0;
}

int lm_graphics_should_close(void) { return lm_closed; }
