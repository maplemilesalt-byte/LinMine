#ifndef LINMINE_LINUX_GFX_H
#define LINMINE_LINUX_GFX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Small SDL2-backed graphics layer used by the NT Minesweeper port. */
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

typedef enum LMSpriteSheet {
    LM_SHEET_BLOCKS = 0,
    LM_SHEET_LED = 1,
    LM_SHEET_BUTTON = 2
} LMSpriteSheet;

int lm_graphics_init(int width, int height, const char *title);
void lm_graphics_shutdown(void);

/* dir must contain the color and monochrome bitmap sets from winmine/bmp. */
int lm_graphics_load_assets(const char *dir, int color);
void lm_graphics_free_assets(void);

void lm_graphics_begin(void);
void lm_graphics_end(void);
void lm_graphics_present(void);

void lm_graphics_clear(unsigned int pixel);
void lm_graphics_fill_rect(LMRect rect, unsigned int pixel);
void lm_graphics_draw_rect(LMRect rect, unsigned int pixel);
void lm_graphics_set_pixel(int x, int y, unsigned int pixel);

/* Sprite sheets are arranged horizontally, matching the original DIBs. */
void lm_graphics_draw_sprite(LMSpriteSheet sheet, int index, int x, int y,
                             int width, int height);

/* Returns non-zero while an event was available. */
int lm_graphics_poll_event(LMInputButton *button, LMPoint *position, int *pressed);
int lm_graphics_should_close(void);

#ifdef __cplusplus
}
#endif

#endif
