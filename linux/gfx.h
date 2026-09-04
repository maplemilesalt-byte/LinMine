#ifndef LINMINE_LINUX_GFX_H
#define LINMINE_LINUX_GFX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Linux graphics/input abstraction for LinMine.
 * The game logic should not depend on SDL/X11/Wayland directly.
 */

typedef struct LMPoint {
    int x;
    int y;
} LMPoint;

typedef struct LMRect {
    int x;
    int y;
    int width;
    int height;
} LMRect;

typedef enum LMInputButton {
    LM_BUTTON_LEFT = 1,
    LM_BUTTON_MIDDLE = 2,
    LM_BUTTON_RIGHT = 3
} LMInputButton;

int lm_graphics_init(int width, int height, const char *title);
void lm_graphics_shutdown(void);

void lm_graphics_begin(void);
void lm_graphics_end(void);
void lm_graphics_present(void);

void lm_graphics_clear(unsigned int pixel);
void lm_graphics_fill_rect(LMRect rect, unsigned int pixel);
void lm_graphics_draw_rect(LMRect rect, unsigned int pixel);

void lm_graphics_set_pixel(int x, int y, unsigned int pixel);

/* Returns non-zero while an event was available. */
int lm_graphics_poll_event(LMInputButton *button, LMPoint *position, int *pressed);

int lm_graphics_should_close(void);

#ifdef __cplusplus
}
#endif

#endif
