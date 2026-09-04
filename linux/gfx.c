#include "gfx.h"

/* Backend-neutral stub.
 * This file deliberately contains no Windows API.  A concrete backend
 * (currently intended to be SDL2) can implement these functions without
 * changing the game logic.
 */

static int lm_closed;

int lm_graphics_init(int width, int height, const char *title)
{
    (void)width;
    (void)height;
    (void)title;
    lm_closed = 0;
    return 0;
}

void lm_graphics_shutdown(void)
{
}

void lm_graphics_begin(void)
{
}

void lm_graphics_end(void)
{
}

void lm_graphics_present(void)
{
}

void lm_graphics_clear(unsigned int pixel)
{
    (void)pixel;
}

void lm_graphics_fill_rect(LMRect rect, unsigned int pixel)
{
    (void)rect;
    (void)pixel;
}

void lm_graphics_draw_rect(LMRect rect, unsigned int pixel)
{
    (void)rect;
    (void)pixel;
}

void lm_graphics_set_pixel(int x, int y, unsigned int pixel)
{
    (void)x;
    (void)y;
    (void)pixel;
}

int lm_graphics_poll_event(LMInputButton *button, LMPoint *position, int *pressed)
{
    (void)button;
    (void)position;
    (void)pressed;
    return 0;
}

int lm_graphics_should_close(void)
{
    return lm_closed;
}
